#include "StringImpl.h"

#include "../../../OrbisUtil/include/MessageHandler.h"

#include <algorithm>

namespace hdt
{
	void* StringManager::threadLoop(void* a_vthis)
	{
		auto a_this = reinterpret_cast<StringManager* const>(a_vthis);

		while (!a_this->m_gcExit)
		{
			for (auto& i : a_this->m_buckets)
			{
				if (a_this->m_gcExit)
				{
					break;
				}

				i.clean();

				if (a_this->m_gcExit)
				{
					break;
				}

				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}

			if (a_this->m_gcExit)
			{
				break;
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}

#if _DEBUG
		char threadname[32];
		scePthreadGetname(a_this->m_gcthreadPair.first, threadname);
		PRINT_FMT_N("Thread %s Exiting... normal?", threadname);
#endif

		scePthreadExit(0);
		return nullptr;
	};

	StringImpl::StringImpl(size_t hash, std::string&& str) : 
		m_hash(hash), 
		m_str(std::move(str))
	{
		m_refCount = 0;
	}


	void StringImpl::retain()
	{
		m_refCount.fetch_add(1);
	}

	void StringImpl::release()
	{
		if (m_refCount.fetch_sub(1) == 1)
		{
			delete this;
		}
	}

	StringManager* StringManager::instance()
	{
		static StringManager s;
		return &s;
	}

	StringImpl* StringManager::get(const char * begin, const char * end)
	{
		std::string str(begin, end);
		size_t hash = std::hash<std::string>()(str);
		auto& bucket = m_buckets[hash % BucketCount];
		return bucket.get(hash, std::move(str));
	}

	StringImpl* StringManager::Bucket::get(size_t hash, std::string && str)
	{
		std::lock_guard<decltype(m_lock)> l(m_lock);

		auto iter = std::find_if(m_list.begin(), m_list.end(), [&, hash](StringImpl* i) 
		{
			return i->hash() == hash && i->str() == str;
		});

		if (iter != m_list.end())
		{
			return *iter;
		}
		else
		{
			auto ret = new StringImpl(hash, std::move(str));
			m_list.push_back(ret);
			return ret;
		}
	}

	void StringManager::Bucket::clean()
	{
		std::lock_guard<decltype(m_lock)> l(m_lock);

		m_list.erase(std::remove_if(m_list.begin(), m_list.end(), [](StringImpl* i) { return i->refcount() == 1; }), m_list.end());
	}

	StringManager::StringManager() : 
		m_gcExit(false)
	{
#if KUSE_POSIX_TYPES
		if (!scePthreadAttrInit(&m_gcthreadPair.second))
		{
			auto pthreadret = scePthreadCreate(&m_gcthreadPair.first, nullptr, threadLoop, reinterpret_cast<void*>(this), "String Manager GC Thread");
		}
#else
		std::construct_at(&m_gcthread, "String Manager Garbage Collector Thread", threadLoop, this);
#endif

	}

	StringManager::~StringManager()
	{
		m_gcExit = true;

#if KUSE_POSIX_TYPES
		if (m_gcthreadPair.first != scePthreadSelf())
			scePthreadJoin(m_gcthreadPair.first, nullptr);
#else
		if (m_gcthread.joinable())
		{
			m_gcthread.join();
		}
#endif
	}
}

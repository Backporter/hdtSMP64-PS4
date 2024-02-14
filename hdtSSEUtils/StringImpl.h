#pragma once

#include "IString.h"
#include "../hdtSSEUtils/Ref.h"

#include <thread>
#include <vector>
#include <atomic>
#include <mutex>

namespace hdt
{
	class StringImpl : public IString
	{
	public:
		StringImpl(size_t hash, std::string&& str);
		virtual ~StringImpl() = default;

		virtual void retain();
		virtual void release();

		virtual const char* cstr() const { return m_str.c_str(); }
		virtual size_t size() const { return m_str.size(); }

		inline long refcount() const { return m_refCount; }
		inline size_t hash() const { return m_hash; }
		inline const std::string str() const { return m_str; }

	protected:
		std::atomic_long m_refCount;
		size_t			m_hash;
		std::string		m_str;
	};

	class StringManager final
	{
	private:
		static void* threadLoop(void*);
	public:
		static StringManager* instance();
		
		StringImpl* get(const char* begin, const char* end);
	private:
		StringManager();
		~StringManager();

		struct Bucket
		{
		public:
			StringImpl* get(size_t hash, std::string&& str);
			
			void clean();
		protected:
			std::vector<Ref<StringImpl>>	m_list;
			std::mutex						m_lock;
		};

		static const size_t BucketCount = 65536;

		Bucket				m_buckets[BucketCount];
		Ref<StringImpl>		m_empty;

#if KUSE_POSIX_TYPES
		std::pair<pthread_t, pthread_attr_t> m_gcthreadPair;
#else
		std::thread			m_gcthread;
#endif

		std::atomic_bool	m_gcExit;
	};
}

#pragma once

#include "IString.h"

#include "../../../OrbisUtil/include/type_traits.h"

#include <type_traits>
#include <functional>

namespace hdt
{
	class IString;
	class RefObject;

	namespace ref 
	{
		// IString
		extern void retain(IString*);
		extern void release(IString*);

		//
		extern void retain(ConsoleRE::NiRefObject* object);
		extern void release(ConsoleRE::NiRefObject* object);

		//
		extern void retain(RefObject* o);
		extern void release(RefObject* o);
	}

	template <typename T>
	class Ref
	{
	public:
		Ref() = default;

		Ref(T* r) : 
			m_ptr(r) 
		{ 
			if (m_ptr)
			{
				ref::retain(m_ptr);
			}
		}

		Ref(const Ref& r) :
			m_ptr(r.m_ptr) 
		{
			if (m_ptr)
			{
				ref::retain(m_ptr);
			}
		}

		Ref(Ref&& r) noexcept :
			m_ptr(r.m_ptr) 
		{ 
			r.m_ptr = nullptr; 
		}

		~Ref() 
		{
			if (m_ptr)
			{
				ref::release(m_ptr);
			}
		}

		inline Ref& operator =(const Ref& r)
		{
			if (m_ptr != r.m_ptr)
			{
				if (m_ptr)
				{
					ref::release(m_ptr);
				}

				m_ptr = r.m_ptr;

				if (m_ptr)
				{
					ref::retain(m_ptr);
				}
			}

			return *this;
		}

		inline Ref& operator =(T* r)
		{
			if (m_ptr != r)
			{
				if (m_ptr)
				{
					ref::release(m_ptr);
				}

				m_ptr = r;

				if (m_ptr)
				{
					ref::retain(m_ptr);
				}
			}

			return *this;
		}

		inline Ref& operator =(Ref&& r)
		{
			auto t = m_ptr;
			m_ptr = r.m_ptr;
			r.m_ptr = t;
			return *this;
		}

		inline operator T*() const { return m_ptr; }

		template <class T1, class = std::enable_if_t<std::is_base_of<T1, T>::value>> 
		inline operator Ref<T1>&() { return *(Ref<T1>*)this; }
		
		template <class T1, class = std::enable_if_t<std::is_base_of<T1, T>::value>> 
		inline operator const Ref<T1>&() const { return *(const Ref<T1>*)this; }

		inline T* operator ()() const { return m_ptr; }
		inline T* operator ->() const { return m_ptr; }
		inline T** addr() { return &m_ptr; }

		template <class T1> T1* cast() const { return dynamic_cast<T1*>(m_ptr); }
		template <class T1> T1* as() const { return static_cast<T1*>(m_ptr); }

		static inline Ref moveRef(T* ptr) { Ref ret; ret.m_ptr = ptr; return ret; }

	protected:
		T* m_ptr{ nullptr };
	};

	template <class T>
	inline bool operator ==(const Ref<T>& a, const Ref<T>& b) { return a() == b(); }

	template <class T>
	inline bool operator !=(const Ref<T>& a, const Ref<T>& b) { return a() != b(); }

	template <class T> inline Ref<T> makeRef(T* ptr) { return Ref<T>(ptr); }
	template <class T> inline Ref<T> moveRef(T* ptr) { return Ref<T>::moveRef(ptr); }
}

namespace std
{
	template <typename T>
	struct hash<hdt::Ref<T>> : public hash<T*> {};
}


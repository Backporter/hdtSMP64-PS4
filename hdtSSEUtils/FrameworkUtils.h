#pragma once

#include "StringImpl.h"

namespace hdt
{
	class IDStr : public Ref<IString>
	{
		typedef Ref<IString> MyBase;
	public:
		IDStr() = default;

		IDStr(const char* str) : 
			MyBase(!str ? nullptr : StringManager::instance()->get(str, str + strlen(str))) 
		{
		}

		IDStr(const std::string& str) : 
			MyBase(!str.c_str() ? nullptr : StringManager::instance()->get(str.c_str(), str.c_str() + str.length())) 
		{
		}

		IDStr(const IDStr& str) : 
			MyBase(str) 
		{}

		IDStr(IDStr&& str) noexcept : 
			MyBase(std::move(str)) 
		{}

		inline IDStr& operator =(const IDStr& rhs) 
		{
			MyBase::operator=(rhs); 

			return *this; 
		}

		inline IDStr& operator =(IDStr&& rhs) noexcept 
		{ 
			MyBase::operator=(std::move(rhs)); 
			return *this; 
		}
	};

	inline bool operator ==(const IDStr& a, const IDStr& b) { return a() == b(); }
	inline bool operator !=(const IDStr& a, const IDStr& b) { return a() != b(); }
}

namespace std
{
	template<>
	struct hash<hdt::IDStr> : public hash<hdt::IString*> 
	{
	};
}

#pragma once

//
#include "hdtSkyrimPhysicsWorld.h"
#include "Offsets.h"

//
#include <algorithm>
#include <random>

namespace hdt 
{
	float				 magnitude(ConsoleRE::NiPoint3 p);
	size_t				 randomGenerator(size_t min, size_t max);
	void				 WeatherCheck();
	ConsoleRE::NiPoint3* getWindDirection();
}

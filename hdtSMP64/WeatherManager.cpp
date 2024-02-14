#include "WeatherManager.h"

using namespace hdt;

ConsoleRE::NiPoint3 precipDirection { 0.f, 0.f, 0.f };

std::vector<uint32_t> notExteriorWorlds = 
{ 
	0x69857, 0x1EE62, 0x20DCB, 0x1FAE2, 0x34240, 0x50015, 0x2C965, 0x29AB7, 0x4F838, 0x3A9D6, 0x243DE, 0xC97EB, 0xC350D, 0x1CDD3, 0x1CDD9, 0x21EDB, 0x1E49D, 0x2B101, 0x2A9D8, 0x20BFE 
};

static inline size_t randomGeneratorLowMoreProbable(size_t lowermin, size_t lowermax, size_t highermin, size_t highermax, int probability) {

	std::mt19937 rng;
	rng.seed(std::random_device()());

	std::uniform_int_distribution<std::mt19937::result_type> dist(1, probability);

	if (dist(rng) == 1)
	{
		//higher
		rng.seed(std::random_device()());

		std::uniform_int_distribution<std::mt19937::result_type> distir(highermin, highermax);

		return distir(rng);
	}
	else
	{
		rng.seed(std::random_device()());

		std::uniform_int_distribution<std::mt19937::result_type> distir(lowermin, lowermax);

		return distir(rng);
	}
}

size_t hdt::randomGenerator(size_t min, size_t max) 
{
	std::mt19937 rng;
	rng.seed(std::random_device()());

	std::uniform_int_distribution<std::mt19937::result_type> dist(min, max);

	return dist(rng);
}

static inline ConsoleRE::NiPoint3 crossProduct(ConsoleRE::NiPoint3 A, ConsoleRE::NiPoint3 B)
{
	return ConsoleRE::NiPoint3(A.y * B.z - A.z * B.y, A.z * B.x - A.x * B.z, A.x * B.y - A.y * B.x);
}

// Calculates a dot product
static inline float dot(ConsoleRE::NiPoint3 a, ConsoleRE::NiPoint3 b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

// Calculates a cross product
static inline ConsoleRE::NiPoint3 cross(ConsoleRE::NiPoint3 a, ConsoleRE::NiPoint3 b)
{
	return ConsoleRE::NiPoint3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}

static inline ConsoleRE::NiPoint3 rotate(const ConsoleRE::NiPoint3& v, const ConsoleRE::NiPoint3& axis, float theta)
{
	const float cos_theta = cosf(theta);

	return (v * cos_theta) + (crossProduct(axis, v) * sinf(theta)) + (axis * dot(axis, v)) * (1 - cos_theta);
}

float hdt::magnitude(ConsoleRE::NiPoint3 p)
{
	return sqrtf(p.x * p.x + p.y * p.y + p.z * p.z);
}

void hdt::WeatherCheck()
{
	ConsoleRE::TESObjectCELL* cell = nullptr;
	ConsoleRE::Actor* player = nullptr;

	const auto world = SkyrimPhysicsWorld::get();
	while (true)
	{
		const auto skyPtr = ConsoleRE::Sky::GetSingleton();;
		if (skyPtr)
		{
			player = skyrim_cast<ConsoleRE::Actor*>(ConsoleRE::TESForm::GetFormFromID(0x14));
			if (!player || !player->loadedData)
			{
				ConsoleRE::NiPoint3 wind{ 0,0,0 };
				world->setWind(&wind, 0, 1);
				sleep(5000 / 1000);
				continue;
			}

			cell = player->parentCell;

			if (!cell)
			{
				ConsoleRE::NiPoint3 wind{ 0,0,0 };
				world->setWind(&wind, 0, 1); // remove wind immediately
				continue;
			}

			ConsoleRE::TESWorldSpace* worldSpace = cell->worldSpace;

			if (!worldSpace) // Interior cell
			{
				//LOG("In interior cell. Waiting for 5 seconds");
				ConsoleRE::NiPoint3 wind{ 0,0,0 };
				world->setWind(&wind, 0, 1); // remove wind immediately
				sleep(5000 / 1000);
				continue;
			}
			else
			{
				if (std::find(notExteriorWorlds.begin(), notExteriorWorlds.end(), worldSpace->formID) != notExteriorWorlds.end())
				{
					//LOG("In interior cell world. Waiting for 5 seconds");
					ConsoleRE::NiPoint3 wind{ 0,0,0 };
					world->setWind(&wind, 0, 1); // remove wind immediately
					sleep(5000 / 1000);
					continue;
				}
			}


			//Wind Detection
			const float range = (randomGeneratorLowMoreProbable(0, 5, 6, 50, 10) / 10.0f);
			precipDirection = ConsoleRE::NiPoint3{ 0.f, 1.f, 0.f };
			if (skyPtr->currentWeather)
			{
				xUtilty::Log::GetSingleton()->Write(xUtilty::Log::logLevel::kNone,"Wind Speed: %2.2g, Wind Direction: %2.2g, Weather Wind Speed: %2.2g WindDir:%2.2g WindDirRange:%2.2g", skyPtr->windSpeed, skyPtr->windAngle, skyPtr->currentWeather->windSpeed, skyPtr->currentWeather->windDirection * 180.0f / 256.0f, skyPtr->currentWeather->windDirectionRange * 360.0f / 256.0f);

				// use weather wind info
				//Wind Speed is the only thing that changes. Wind direction and range are same all the time as set in CK.
				const float theta = (((skyPtr->currentWeather->windDirection) * 180.0f) / 256.0f) - 90.f + randomGenerator(-range, range);
				precipDirection = rotate(precipDirection, ConsoleRE::NiPoint3(0, 0, 1.0f), theta / 57.295776f);
				world->setWind(&precipDirection, world->m_windStrength * scaleSkyrim * skyPtr->windSpeed);
			}
			else 
			{
				xUtilty::Log::GetSingleton()->Write(xUtilty::Log::logLevel::kNone,"Wind Speed: %2.2g, Wind Direction: %2.2g", skyPtr->windSpeed, skyPtr->windAngle);
				// use sky wind info
				const float theta = (((skyPtr->windAngle) * 180.0f) / 256.0f) - 90.f + randomGenerator(-range, range);
				precipDirection = rotate(precipDirection, ConsoleRE::NiPoint3(0, 0, 1.0f), theta / 57.295776f);
				world->setWind(&precipDirection, world->m_windStrength * scaleSkyrim * skyPtr->windSpeed);
			}
			sleep(500 / 1000);
		}
		else
		{
			ConsoleRE::NiPoint3 wind{ 0,0,0 };
			world->setWind(&wind, 0, 1); // remove wind immediately
			// LOG("Sky is null. waiting for 5 seconds.");
			sleep(5000 / 1000);
		}
	}
}

ConsoleRE::NiPoint3* hdt::getWindDirection()
{
	return &precipDirection;
}

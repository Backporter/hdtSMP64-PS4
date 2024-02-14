#include "config.h"
#include "XmlReader.h"

#include "hdtSkyrimPhysicsWorld.h"

#include <clocale>

namespace hdt
{
	static void solver(XMLReader& reader)
	{
		while (reader.Inspect())
		{
			switch (reader.GetInspected())
			{
			case XMLReader::Inspected::StartTag:
				if (reader.GetLocalName() == "numIterations")
				{
					SkyrimPhysicsWorld::get()->getSolverInfo().m_numIterations = btClamped(reader.readInt(), 4, 128);
				}
				else if (reader.GetLocalName() == "groupIterations")
				{
					ConstraintGroup::MaxIterations = btClamped(reader.readInt(), 0, 4096);
				}
				else if (reader.GetLocalName() == "groupEnableMLCP")
				{
					ConstraintGroup::EnableMLCP = reader.readBool();
				}
				else if (reader.GetLocalName() == "erp")
				{
					SkyrimPhysicsWorld::get()->getSolverInfo().m_erp = btClamped(reader.readFloat(), 0.01f, 1.0f);
				}
				else if (reader.GetLocalName() == "min-fps") 
				{
					SkyrimPhysicsWorld::get()->min_fps = (btClamped(reader.readInt(), 1, 300));
					SkyrimPhysicsWorld::get()->m_timeTick = 1.0f / SkyrimPhysicsWorld::get()->min_fps;
				}
				else if (reader.GetLocalName() == "maxSubSteps")
				{
					SkyrimPhysicsWorld::get()->m_maxSubSteps = btClamped(reader.readInt(), 1, 60);
				}
				else
				{
					xUtilty::Log::GetSingleton()->Write(xUtilty::Log::logLevel::kNone,"Unknown config : %s", reader.GetLocalName().c_str());
					reader.skipCurrentElement();
				}
				break;
			case XMLReader::Inspected::EndTag:
				return;
			}
		}
	}

	static void wind(XMLReader& reader)
	{
		while (reader.Inspect())
		{
			switch (reader.GetInspected())
			{
			case XMLReader::Inspected::StartTag:
				if (reader.GetLocalName() == "windStrength")
				{
					SkyrimPhysicsWorld::get()->m_windStrength = btClamped(reader.readFloat(), 0.f, 1000.f);
				}
				else if (reader.GetLocalName() == "enabled")
				{
					SkyrimPhysicsWorld::get()->m_enableWind = reader.readBool();
				}
				else if (reader.GetLocalName() == "distanceForNoWind")
				{
					SkyrimPhysicsWorld::get()->m_distanceForNoWind = btClamped(reader.readFloat(), 0.f, 10000.f);
				}
				else if (reader.GetLocalName() == "distanceForMaxWind")
				{
					SkyrimPhysicsWorld::get()->m_distanceForMaxWind = btClamped(reader.readFloat(), 0.f, 10000.f);
				}
				else
				{
					xUtilty::Log::GetSingleton()->Write(xUtilty::Log::logLevel::kNone,"Unknown config : ", reader.GetLocalName().c_str());
					reader.skipCurrentElement();
				}
				break;
			case XMLReader::Inspected::EndTag:
				return;
			}
		}
	}

	void SetLogLevel(int level)
	{
	}

	static void smp(XMLReader& reader)
	{
		while (reader.Inspect())
		{
			switch (reader.GetInspected())
			{
			case XMLReader::Inspected::StartTag:
				if (reader.GetLocalName() == "logLevel")
				{
					SetLogLevel(reader.readInt());
				}
				else if (reader.GetLocalName() == "enableNPCFaceParts")
				{
					ActorManager::instance()->m_skinNPCFaceParts = reader.readBool();
				}
				else if (reader.GetLocalName() == "clampRotations")
				{
					SkyrimPhysicsWorld::get()->m_clampRotations = reader.readBool();
				}
				else if (reader.GetLocalName() == "rotationSpeedLimit")
				{
					SkyrimPhysicsWorld::get()->m_rotationSpeedLimit = reader.readFloat();
				}
				else if (reader.GetLocalName() == "unclampedResets")
				{
					SkyrimPhysicsWorld::get()->m_unclampedResets = reader.readBool();
				}
				else if (reader.GetLocalName() == "unclampedResetAngle")
				{
					SkyrimPhysicsWorld::get()->m_unclampedResetAngle = reader.readFloat();
				}
				else if (reader.GetLocalName() == "percentageOfFrameTime")
				{
					SkyrimPhysicsWorld::get()->m_percentageOfFrameTime = std::clamp(reader.readInt() * 10, 1, 1000);
				}
				else if (reader.GetLocalName() == "enableCuda")
				{
					if (reader.readBool())
					{
						xUtilty::Log::GetSingleton()->Write(xUtilty::Log::logLevel::kNone,"CUDA isn't built into this version.");
					}
				}
				else if (reader.GetLocalName() == "cudaDevice") 
				{
				}
				else if (reader.GetLocalName() == "maximumActiveSkeletons")
				{
					ActorManager::instance()->m_maxActiveSkeletons = reader.readInt();
				}
				else if (reader.GetLocalName() == "autoAdjustMaxSkeletons")
				{
					ActorManager::instance()->m_autoAdjustMaxSkeletons = reader.readBool();
				}
				else if (reader.GetLocalName() == "sampleSize")
				{
					ActorManager::instance()->m_sampleSize = std::max(reader.readInt(), 1);
				}
				else if (reader.GetLocalName() == "disable1stPersonViewPhysics")
				{
					ActorManager::instance()->m_disable1stPersonViewPhysics = reader.readBool();
				}
				else
				{
					xUtilty::Log::GetSingleton()->Write(xUtilty::Log::logLevel::kNone,"Unknown config : %s", reader.GetLocalName().c_str());
					reader.skipCurrentElement();
				}
				break;
			case XMLReader::Inspected::EndTag:
				return;
			}
		}
	}

	static void config(XMLReader& reader)
	{
		while (reader.Inspect())
		{
			switch (reader.GetInspected())
			{
			case XMLReader::Inspected::StartTag:
				if (reader.GetLocalName() == "solver")
				{
					solver(reader);
				}
				else if (reader.GetLocalName() == "wind")
				{
					wind(reader);
				}
				else if (reader.GetLocalName() == "smp")
				{
					smp(reader);
				}
				else
				{
					xUtilty::Log::GetSingleton()->Write(xUtilty::Log::logLevel::kNone,"Unknown config : %s", reader.GetLocalName().c_str());
					reader.skipCurrentElement();
				}
				break;
			case XMLReader::Inspected::EndTag:
				return;
			}
		}
	}

	void loadConfig()
	{
		auto bytes = readAllFile2("/app0/data/skse/plugins/hdtSkinnedMeshConfigs/configs.xml");
		if (bytes.empty()) 
		{
			return;
		}
		
		// Store original locale
		char saved_locale[32];
		strcpy_s(saved_locale, std::setlocale(LC_NUMERIC, nullptr));

		// Set locale to en_US
		std::setlocale(LC_NUMERIC, "en_US");

		XMLReader reader((uint8_t*)bytes.data(), bytes.size());

		while (reader.Inspect())
		{
			if (reader.GetInspected() == XMLReader::Inspected::StartTag)
			{
				if (reader.GetLocalName() == "configs")
				{
					config(reader);
				}
				else
				{
					xUtilty::Log::GetSingleton()->Write(xUtilty::Log::logLevel::kNone,"Unknown config : %s", reader.GetLocalName().c_str());
					reader.skipCurrentElement();
				}
			}
		}

		// Restore original locale
		std::setlocale(LC_NUMERIC, saved_locale);
	}
}

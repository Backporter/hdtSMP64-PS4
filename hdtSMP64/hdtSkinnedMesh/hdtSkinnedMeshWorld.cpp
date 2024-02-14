#include <random>
#include "hdtSkinnedMeshWorld.h"
#include "hdtSkinnedMeshAlgorithm.h"
#include "hdtDispatcher.h"
#include "hdtSimulationIslandManager.h"
#include "hdtBoneScaleConstraint.h"
#include "hdtSkyrimSystem.h"
#include "hdtSkyrimPhysicsWorld.h"

namespace hdt
{
	SkinnedMeshWorld::SkinnedMeshWorld()
		: btDiscreteDynamicsWorldMt(nullptr, nullptr, m_solverPool, &m_constraintSolver, nullptr)
	{
		btSetTaskScheduler(btGetPPLTaskScheduler());

		m_windSpeed = _mm_setzero_ps();

		auto collisionConfiguration = new btDefaultCollisionConfiguration;
		auto collisionDispatcher = new CollisionDispatcher(collisionConfiguration);
		SkinnedMeshAlgorithm::registerAlgorithm(collisionDispatcher);
		m_dispatcher1 = collisionDispatcher;

		auto broadphase = new btDbvtBroadphase();
		m_broadphasePairCache = broadphase;
		m_solverPool = new btConstraintSolverPoolMt(BT_MAX_THREAD_COUNT);

		//m_islandManager->~btSimulationIslandManager();
		//new (m_islandManager) SimulationIslandManager();
	}

	SkinnedMeshWorld::~SkinnedMeshWorld()
	{
		for (auto system : m_systems)
		{
			for (int i = 0; i < system->m_meshes.size(); ++i)
			{
				removeCollisionObject(system->m_meshes[i]);
			}
			
			for (int i = 0; i < system->m_constraints.size(); ++i)
			{
				removeConstraint(system->m_constraints[i]->m_constraint);
			}			
			
			for (int i = 0; i < system->m_bones.size(); ++i)
			{
				removeRigidBody(&system->m_bones[i]->m_rig);
			}

			for (auto i : system->m_constraintGroups)
			{
				for (auto j : i->m_constraints)
				{
					removeConstraint(j->m_constraint);
				}
			}
		}

		m_systems.clear();
	}

	void SkinnedMeshWorld::addSkinnedMeshSystem(SkinnedMeshSystem* system)
	{
		if (std::find(m_systems.begin(), m_systems.end(), system) != m_systems.end())
		{
			return;
		}

		m_systems.push_back(system);

		for (int i = 0; i < system->m_meshes.size(); ++i)
		{
			addCollisionObject(system->m_meshes[i], 1, 1);
		}
		for (int i = 0; i < system->m_bones.size(); ++i)
		{
			system->m_bones[i]->m_rig.setActivationState(DISABLE_DEACTIVATION);
			addRigidBody(&system->m_bones[i]->m_rig, 0, 0);
		}

		for (auto i : system->m_constraintGroups)
		{
			for (auto j : i->m_constraints)
			{
				addConstraint(j->m_constraint, true);
			}
		}

		for (int i = 0; i < system->m_constraints.size(); ++i)
		{
			addConstraint(system->m_constraints[i]->m_constraint, true);
		}

		// -10 allows RESET_PHYSICS down the calls. But equality with a float?...
		system->readTransform(RESET_PHYSICS);

		system->m_world = this;
	}

	void SkinnedMeshWorld::removeSkinnedMeshSystem(SkinnedMeshSystem* system)
	{
		auto idx = std::find(m_systems.begin(), m_systems.end(), system);
		if (idx == m_systems.end())
		{
			return;
		}

		for (auto i : system->m_constraintGroups)
		{
			for (auto j : i->m_constraints)
			{
				removeConstraint(j->m_constraint);
			}
		}

		for (int i = 0; i < system->m_meshes.size(); ++i)
		{
			removeCollisionObject(system->m_meshes[i]);
		}

		for (int i = 0; i < system->m_constraints.size(); ++i)
		{
			removeConstraint(system->m_constraints[i]->m_constraint);
		}

		for (int i = 0; i < system->m_bones.size(); ++i)
		{
			removeRigidBody(&system->m_bones[i]->m_rig);
		}

		std::swap(*idx, m_systems.back());
		m_systems.pop_back();

		system->m_world = nullptr;
	}

	int SkinnedMeshWorld::stepSimulation(btScalar remainingTimeStep, int maxSubSteps, btScalar fixedTimeStep)
	{
		applyGravity();
		if (hdt::SkyrimPhysicsWorld::get()->m_enableWind)
		{
			applyWind();
		}

		while (remainingTimeStep > fixedTimeStep)
		{
			internalSingleStepSimulation(fixedTimeStep);
			remainingTimeStep -= fixedTimeStep;
		}
		// For the sake of the bullet library, we don't manage a step that would be lower than a 300Hz frame.
		// Review this when (screens / Skyrim) will allow 300Hz+.
		constexpr auto minPossiblePeriod = 1.0f / 300.0f;
		if (remainingTimeStep > minPossiblePeriod)
		{
			internalSingleStepSimulation(remainingTimeStep);
		}

		clearForces();

		_bodies.clear();
		_shapes.clear();

		return 0;
	}

	void SkinnedMeshWorld::performDiscreteCollisionDetection()
	{
		for (int i = 0; i < m_systems.size(); ++i)
		{
			m_systems[i]->internalUpdate();
		}

		btDiscreteDynamicsWorldMt::performDiscreteCollisionDetection();
	}

	void SkinnedMeshWorld::applyGravity()
	{
		for (auto& i : m_systems)
		{
			for (auto& j : i->m_bones)
			{
				auto body = &j->m_rig;
				if (!body->isStaticOrKinematicObject() && !(body->getFlags() & BT_DISABLE_WORLD_GRAVITY))
				{
					body->setGravity(m_gravity * j->m_gravityFactor);
				}
			}
		}

		btDiscreteDynamicsWorldMt::applyGravity();
	}

	void SkinnedMeshWorld::applyWind()
	{
		for (auto& i : m_systems)
		{
			auto system = static_cast<SkyrimSystem*>(i());
			if (btFuzzyZero(system->m_windFactor)) // skip any systems that aren't affected by wind
			{
				continue;
			}
			for (auto& j : i->m_bones)
			{
				auto body = &j->m_rig;
				if (!body->isStaticOrKinematicObject() && (rand() % 5)) // apply randomly 80% of the time to desync wind across npcs
				{
					body->applyCentralForce(m_windSpeed *j->m_windFactor * system->m_windFactor);
				}
			}
		}
	}

	void SkinnedMeshWorld::predictUnconstraintMotion(btScalar timeStep)
	{
		for (int i = 0; i < m_nonStaticRigidBodies.size(); i++)
		{
			btRigidBody* body = m_nonStaticRigidBodies[i];
			if (!body->isStaticOrKinematicObject())
			{
				// not realistic, just an approximate
				body->applyDamping(timeStep);
				body->predictIntegratedTransform(timeStep, body->getInterpolationWorldTransform());
			}
			else
			{
				body->predictIntegratedTransform(timeStep, body->getInterpolationWorldTransform());
			}
		}
	}

	void SkinnedMeshWorld::integrateTransforms(btScalar timeStep)
	{
		for (int i = 0; i < m_collisionObjects.size(); ++i)
		{
			auto body = m_collisionObjects[i];
			if (body->isKinematicObject())
			{
				btTransformUtil::integrateTransform(
					body->getWorldTransform(),
					body->getInterpolationLinearVelocity(),
					body->getInterpolationAngularVelocity(),
					timeStep,
					body->getInterpolationWorldTransform());
				body->setWorldTransform(body->getInterpolationWorldTransform());
			}
		}

		btVector3 limitMin(-1e+9f, -1e+9f, -1e+9f);
		btVector3 limitMax(1e+9f, 1e+9f, 1e+9f);
		for (int i = 0; i < m_nonStaticRigidBodies.size(); i++)
		{
			btRigidBody* body = m_nonStaticRigidBodies[i];
			auto lv = body->getLinearVelocity();
			lv.setMax(limitMin);
			lv.setMin(limitMax);
			body->setLinearVelocity(lv);

			auto av = body->getAngularVelocity();
			av.setMax(limitMin);
			av.setMin(limitMax);
			body->setAngularVelocity(av);
		}

		btDiscreteDynamicsWorldMt::integrateTransforms(timeStep);
	}

	void SkinnedMeshWorld::solveConstraints(btContactSolverInfo& solverInfo)
	{
		BT_PROFILE("solveConstraints");
		if (!m_collisionObjects.size())
		{
			return;
		}

		m_solverPool->prepareSolve(getCollisionWorld()->getNumCollisionObjects(), getCollisionWorld()->getDispatcher()->getNumManifolds());

		m_constraintSolver.m_groups.clear();
		for (auto& i : m_systems)
		{
			for (auto& j : i->m_constraintGroups)
			{
				m_constraintSolver.m_groups.push_back(j);
			}
		}

		btPersistentManifold** manifold = m_dispatcher1->getInternalManifoldPointer();
		int maxNumManifolds = m_dispatcher1->getNumManifolds();
		m_solverPool->solveGroup(&m_collisionObjects[0], m_collisionObjects.size(), manifold, maxNumManifolds, &m_constraints[0], m_constraints.size(), solverInfo, m_debugDrawer, m_dispatcher1);

		m_solverPool->allSolved(solverInfo, m_debugDrawer);
		static_cast<CollisionDispatcher*>(m_dispatcher1)->clearAllManifold();
		m_constraintSolver.m_groups.clear();
	}
}

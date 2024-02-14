#include "hdtDispatcher.h"
#include "hdtSkinnedMeshBody.h"
#include "hdtSkinnedMeshAlgorithm.h"
#include <LinearMath/btPoolAllocator.h>

namespace hdt
{
	void CollisionDispatcher::clearAllManifold()
	{
		std::lock_guard<decltype(m_lock)> l(m_lock);
		for (int i = 0; i < m_manifoldsPtr.size(); ++i)
		{
			auto manifold = m_manifoldsPtr[i];
			manifold->~btPersistentManifold();
			if (m_persistentManifoldPoolAllocator->validPtr(manifold))
			{
				m_persistentManifoldPoolAllocator->freeMemory(manifold);
			}
			else
			{
				btAlignedFree(manifold);
			}
		}

		m_manifoldsPtr.clear();
	}

	bool needsCollision(const SkinnedMeshBody* shape0, const SkinnedMeshBody* shape1)
	{		
		if (!shape0 || !shape1 || shape0 == shape1)
		{
			return false;
		}

		if (shape0->m_isKinematic && shape1->m_isKinematic)
		{
			return false;
		}
		
		return shape0->canCollideWith(shape1) && shape1->canCollideWith(shape0);
	}

	bool CollisionDispatcher::needsCollision(const btCollisionObject* body0, const btCollisionObject* body1)
	{
		auto shape0 = dynamic_cast<const SkinnedMeshBody*>(body0);
		auto shape1 = dynamic_cast<const SkinnedMeshBody*>(body1);

		if (shape0 || shape1)
		{
			return hdt::needsCollision(shape0, shape1);
		}
		if (body0->isStaticOrKinematicObject() && body1->isStaticOrKinematicObject())
		{
			return false;
		}
		if (body0->checkCollideWith(body1) || body1->checkCollideWith(body0))
		{
			auto rb0 = static_cast<SkinnedMeshBone*>(body0->getUserPointer());
			auto rb1 = static_cast<SkinnedMeshBone*>(body0->getUserPointer());

			return rb0->canCollideWith(rb1) && rb1->canCollideWith(rb0);
		}
		else 
		{
			return false;
		}
	}

	void CollisionDispatcher::dispatchAllCollisionPairs(btOverlappingPairCache* pairCache, const btDispatcherInfo& dispatchInfo, btDispatcher* dispatcher)
	{
		auto size = pairCache->getNumOverlappingPairs();
		if (!size) 
		{
			return;
		}

		m_pairs.reserve(size);
		auto pairs = pairCache->getOverlappingPairArrayPtr();

		SpinLock lock;
		std::unordered_set<SkinnedMeshBody*> bodies;
		std::unordered_set<PerVertexShape*> vertex_shapes;
		std::unordered_set<PerTriangleShape*> triangle_shapes;

		/* concurrency::parallel_for(0, size, [&](int i) */
		auto lamda = [&](int i)
		{
			auto& pair = pairs[i];

			auto shape0 = dynamic_cast<SkinnedMeshBody*>(static_cast<btCollisionObject*>(pair.m_pProxy0->m_clientObject));
			auto shape1 = dynamic_cast<SkinnedMeshBody*>(static_cast<btCollisionObject*>(pair.m_pProxy1->m_clientObject));

			if (shape0 || shape1)
			{
				if (hdt::needsCollision(shape0, shape1) && shape0->isBoundingSphereCollided(shape1))
				{
					// HDT_LOCK_GUARD(l, lock);
					bodies.insert(shape0);
					bodies.insert(shape1);
					m_pairs.push_back(std::make_pair(shape0, shape1));

					auto a = shape0->m_shape->asPerTriangleShape();
					auto b = shape1->m_shape->asPerTriangleShape();

					if (a)
					{
						triangle_shapes.insert(a);
					}
					else
					{
						vertex_shapes.insert(shape0->m_shape->asPerVertexShape());
					}
					
					if (b)
					{
						triangle_shapes.insert(b);
					}
					else	
					{
						vertex_shapes.insert(shape1->m_shape->asPerVertexShape());
					}
					
					if (a && b)
					{
						vertex_shapes.insert(a->m_verticesCollision);
						vertex_shapes.insert(b->m_verticesCollision);
					}
				}
			}
			else 
			{
				getNearCallback()(pair, *this, dispatchInfo);
			}
		};

		for (int i = 0; i < size; i++)
		{
			lamda(i);
		}
		
		PRINT_FMT("doing caculations.. (%s)", "LOL");
		std::for_each(bodies.begin(), bodies.end(), [](SkinnedMeshBody* shape) /* concurrency::parallel_for_each */
		{
			shape->internalUpdate();
		});

		std::for_each(vertex_shapes.begin(), vertex_shapes.end(), [](PerVertexShape* shape) /* concurrency::parallel_for_each */
		{
			shape->internalUpdate();
		});

		std::for_each(triangle_shapes.begin(), triangle_shapes.end(), [](PerTriangleShape* shape) /* concurrency::parallel_for_each */
		{
			shape->internalUpdate();
		});

		for (auto body : bodies)
		{
			body->m_bulletShape.m_aabb = body->m_shape->m_tree.aabbAll;
		}

		PRINT_FMT("doing caculations.. (%s)", "LOL");
		std::for_each(m_pairs.begin(), m_pairs.end(), [&, this](const std::pair<SkinnedMeshBody*, SkinnedMeshBody*>& i) /* concurrency::parallel_for_each */
		{
			if (i.first->m_shape->m_tree.collapseCollideL(&i.second->m_shape->m_tree))
			{
				SkinnedMeshAlgorithm::processCollision(i.first, i.second, this);
			}
		});

		m_pairs.clear();
	}

	int CollisionDispatcher::getNumManifolds() const
	{
		return m_manifoldsPtr.size();
	}

	btPersistentManifold* CollisionDispatcher::getManifoldByIndexInternal(int index)
	{
		return m_manifoldsPtr[index];
	}

	btPersistentManifold** CollisionDispatcher::getInternalManifoldPointer()
	{
		return btCollisionDispatcherMt::getInternalManifoldPointer();
	}
}

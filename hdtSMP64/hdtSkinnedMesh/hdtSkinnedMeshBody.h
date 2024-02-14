#pragma once

#include "hdtBulletHelper.h"
#include "hdtSkinnedMeshBone.h"
#include "hdtVertex.h"
#include "hdtAABB.h"

#include <BulletCollision/Gimpact/btBoxCollision.h>

namespace hdt
{
	class SkinnedMeshShape;

	class SkinnedMeshBody
		: public btCollisionObject
		  , public RefObject
	{
	public:

		SkinnedMeshBody();
		virtual ~SkinnedMeshBody();

		struct CollisionShape : public btCollisionShape // a shape only used for markout
		{
			CollisionShape() : m_aabb(_mm_setzero_ps(), _mm_setzero_ps()) { m_shapeType = CUSTOM_CONCAVE_SHAPE_TYPE; }

			Aabb m_aabb;

			void getAabb(const btTransform& t, btVector3& aabbMin, btVector3& aabbMax) const override
			{
				aabbMin = m_aabb.m_min;
				aabbMax = m_aabb.m_max;
			}

			void setLocalScaling(const btVector3& scaling) override
			{
			}

			const btVector3& getLocalScaling() const override
			{
				static const btVector3 ret(1, 1, 1);
				return ret;
			}

			void calculateLocalInertia(btScalar mass, btVector3& inertia) const override
			{
			}

			const char* getName() const override { return "btSkinnedMeshBody"; }
			btScalar getMargin() const override { return 0; }

			void setMargin(btScalar m) override
			{
			}
		} m_bulletShape;

		struct SkinnedBone
		{
			btMatrix4x3T vertexToBone;
			BoundingSphere localBoundingSphere;
			BoundingSphere worldBoundingSphere;
			SkinnedMeshBone* ptr = nullptr;
			float weightThreshold;
			bool isKinematic;
		};

		IDStr m_name;

		//		int m_priority;
		bool m_isKinematic;
		bool m_useBoundingSphere;
		Ref<SkinnedMeshShape> m_shape;

		int addBone(SkinnedMeshBone* bone, const btQsTransform& verticesToBone, const BoundingSphere& boundingSphere);

		void finishBuild();

		virtual void internalUpdate();

		std::vector<SkinnedBone> m_skinnedBones;

		std::vector<Bone> m_bones;


		std::vector<Vertex> m_vertices;

		std::vector<VertexPos> m_vpos;


		std::vector<IDStr> m_tags;
		std::unordered_set<IDStr> m_canCollideWithTags;
		std::unordered_set<IDStr> m_noCollideWithTags;
		std::vector<SkinnedMeshBone*> m_canCollideWithBones;
		std::vector<SkinnedMeshBone*> m_noCollideWithBones;



		float flexible(const Vertex& v);

		bool canCollideWith(const SkinnedMeshBone* bone) const
		{
			if (m_canCollideWithBones.size())
			{
				return std::find(m_canCollideWithBones.begin(), m_canCollideWithBones.end(), bone) != m_canCollideWithBones.end();
			}
			return std::find(m_noCollideWithBones.begin(), m_noCollideWithBones.end(), bone) == m_noCollideWithBones.end();
		}

		virtual bool canCollideWith(const SkinnedMeshBody* body) const;

		void updateBoundingSphereAabb();
		bool isBoundingSphereCollided(SkinnedMeshBody* rhs);
	};
}

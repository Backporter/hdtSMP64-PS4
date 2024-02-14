#include "hdtConstraintGroup.h"
#include "hdtGroupConstraintSolver.h"
#include <LinearMath/btCpuFeatureUtility.h>

namespace hdt
{
	int ConstraintGroup::MaxIterations = 2;
	bool ConstraintGroup::EnableMLCP = true;

	ConstraintGroup::ConstraintGroup()
		: btMLCPSolver(&m_mlcpSolver)
	{
		int cpuFeatures = btCpuFeatureUtility::getCpuFeatures();
		if ((cpuFeatures & btCpuFeatureUtility::CPU_FEATURE_FMA3) && (cpuFeatures & btCpuFeatureUtility::
			CPU_FEATURE_SSE4_1))
		{
			m_resolveSingleConstraintRowGeneric = GroupConstraintSolver::getResolveSingleConstraintRowGenericAVX();
			m_resolveSingleConstraintRowLowerLimit = GroupConstraintSolver::
				getResolveSingleConstraintRowLowerLimitAVX();
		}
	}

	int ConstraintGroup::getOrInitSolverBody(btCollisionObject& body, btScalar timeStep)
	{
		int solverBodyIdA = -1;

		if (body.getCompanionId() >= 0)
		{
			//body has already been converted
			solverBodyIdA = body.getCompanionId();
			assert(solverBodyIdA < m_solverBodies->size());
		}
		else
		{
			assert(false);
			btRigidBody* rb = btRigidBody::upcast(&body);
			//convert both active and kinematic objects (for their velocity)
			if (rb && (rb->getInvMass() || rb->isKinematicObject()))
			{
				solverBodyIdA = m_solverBodies->size();
				btSolverBody& solverBody = m_solverBodies->expand();
				initSolverBody(&solverBody, &body, timeStep);
				body.setCompanionId(solverBodyIdA);
			}
			else
			{
				if (m_fixedBodyId < 0)
				{
					m_fixedBodyId = m_solverBodies->size();
					btSolverBody& fixedBody = m_solverBodies->expand();
					initSolverBody(&fixedBody, nullptr, timeStep);
				}
				return m_fixedBodyId;
				//			return 0;//assume first one is a fixed solver body
			}
		}

		return solverBodyIdA;
	}

	void ConstraintGroup::setup(btAlignedObjectArray<btSolverBody>* bodies, const btContactSolverInfo& infoGlobal)
	{
		m_solverBodies = bodies;
		m_maxOverrideNumSolverIterations = 0;

		m_btConstraints.resize(m_constraints.size());
		for (int i = 0; i < m_constraints.size(); i++)
		{
			m_btConstraints[i] = m_constraints[i]->getConstraint();
			btTypedConstraint* constraint = m_btConstraints[i];
			constraint->buildJacobian();
			constraint->internalSetAppliedImpulse(0.0f);
		}

		{
			int totalNumRows = 0;
			int i;

			m_tmpConstraintSizesPool.resizeNoInitialize(m_btConstraints.size());
			//calculate the total number of contraint rows
			for (i = 0; i < m_btConstraints.size(); i++)
			{
				btTypedConstraint::btConstraintInfo1& info1 = m_tmpConstraintSizesPool[i];
				btJointFeedback* fb = m_btConstraints[i]->getJointFeedback();
				if (fb)
				{
					fb->m_appliedForceBodyA.setZero();
					fb->m_appliedTorqueBodyA.setZero();
					fb->m_appliedForceBodyB.setZero();
					fb->m_appliedTorqueBodyB.setZero();
				}

				if (m_btConstraints[i]->isEnabled())
				{
				}
				if (m_btConstraints[i]->isEnabled())
				{
					m_btConstraints[i]->getInfo1(&info1);
				}
				else
				{
					info1.m_numConstraintRows = 0;
					info1.nub = 0;
				}
				totalNumRows += info1.m_numConstraintRows;
			}
			m_tmpSolverNonContactConstraintPool.resizeNoInitialize(totalNumRows);


			///setup the btSolverConstraints
			int currentRow = 0;

			for (i = 0; i < m_btConstraints.size(); i++)
			{
				const btTypedConstraint::btConstraintInfo1& info1 = m_tmpConstraintSizesPool[i];

				if (info1.m_numConstraintRows)
				{
					assert(currentRow<totalNumRows);

					btSolverConstraint* currentConstraintRow = &m_tmpSolverNonContactConstraintPool[currentRow];
					btTypedConstraint* constraint = m_btConstraints[i];
					btRigidBody& rbA = constraint->getRigidBodyA();
					btRigidBody& rbB = constraint->getRigidBodyB();

					int solverBodyIdA = getOrInitSolverBody(rbA, infoGlobal.m_timeStep);
					int solverBodyIdB = getOrInitSolverBody(rbB, infoGlobal.m_timeStep);

					btSolverBody* bodyAPtr = &(*m_solverBodies)[solverBodyIdA];
					btSolverBody* bodyBPtr = &(*m_solverBodies)[solverBodyIdB];

					int overrideNumSolverIterations = constraint->getOverrideNumSolverIterations() > 0
						                                  ? constraint->getOverrideNumSolverIterations()
						                                  : infoGlobal.m_numIterations;
					if (overrideNumSolverIterations > m_maxOverrideNumSolverIterations)
						m_maxOverrideNumSolverIterations = overrideNumSolverIterations;

					int j;
					for (j = 0; j < info1.m_numConstraintRows; j++)
					{
						memset(&currentConstraintRow[j], 0, sizeof(btSolverConstraint));
						currentConstraintRow[j].m_lowerLimit = -SIMD_INFINITY;
						currentConstraintRow[j].m_upperLimit = SIMD_INFINITY;
						currentConstraintRow[j].m_appliedImpulse = 0.f;
						currentConstraintRow[j].m_appliedPushImpulse = 0.f;
						currentConstraintRow[j].m_solverBodyIdA = solverBodyIdA;
						currentConstraintRow[j].m_solverBodyIdB = solverBodyIdB;
						currentConstraintRow[j].m_overrideNumSolverIterations = overrideNumSolverIterations;
					}

					bodyAPtr->internalGetDeltaLinearVelocity().setValue(0.f, 0.f, 0.f);
					bodyAPtr->internalGetDeltaAngularVelocity().setValue(0.f, 0.f, 0.f);
					bodyAPtr->internalGetPushVelocity().setValue(0.f, 0.f, 0.f);
					bodyAPtr->internalGetTurnVelocity().setValue(0.f, 0.f, 0.f);
					bodyBPtr->internalGetDeltaLinearVelocity().setValue(0.f, 0.f, 0.f);
					bodyBPtr->internalGetDeltaAngularVelocity().setValue(0.f, 0.f, 0.f);
					bodyBPtr->internalGetPushVelocity().setValue(0.f, 0.f, 0.f);
					bodyBPtr->internalGetTurnVelocity().setValue(0.f, 0.f, 0.f);


					btTypedConstraint::btConstraintInfo2 info2;
					info2.fps = 1.f / infoGlobal.m_timeStep;
					info2.erp = infoGlobal.m_erp;
					info2.m_J1linearAxis = currentConstraintRow->m_contactNormal1;
					info2.m_J1angularAxis = currentConstraintRow->m_relpos1CrossNormal;
					info2.m_J2linearAxis = currentConstraintRow->m_contactNormal2;
					info2.m_J2angularAxis = currentConstraintRow->m_relpos2CrossNormal;
					info2.rowskip = sizeof(btSolverConstraint) / sizeof(btScalar); //check this
					///the size of btSolverConstraint needs be a multiple of btScalar
					assert(info2.rowskip*sizeof(btScalar) == sizeof(btSolverConstraint));
					info2.m_constraintError = &currentConstraintRow->m_rhs;
					currentConstraintRow->m_cfm = infoGlobal.m_globalCfm;
					info2.m_damping = infoGlobal.m_damping;
					info2.cfm = &currentConstraintRow->m_cfm;
					info2.m_lowerLimit = &currentConstraintRow->m_lowerLimit;
					info2.m_upperLimit = &currentConstraintRow->m_upperLimit;
					info2.m_numIterations = infoGlobal.m_numIterations;
					m_btConstraints[i]->getInfo2(&info2);

					///finalize the constraint setup
					for (j = 0; j < info1.m_numConstraintRows; j++)
					{
						btSolverConstraint& solverConstraint = currentConstraintRow[j];

						if (solverConstraint.m_upperLimit >= m_btConstraints[i]->getBreakingImpulseThreshold())
							solverConstraint.m_upperLimit = m_btConstraints[i]->getBreakingImpulseThreshold();

						if (solverConstraint.m_lowerLimit <= -m_btConstraints[i]->getBreakingImpulseThreshold())
							solverConstraint.m_lowerLimit = -m_btConstraints[i]->getBreakingImpulseThreshold();

						solverConstraint.m_originalContactPoint = constraint;

						{
							const btVector3& ftorqueAxis1 = solverConstraint.m_relpos1CrossNormal;
							solverConstraint.m_angularComponentA = constraint
							                                       ->getRigidBodyA().getInvInertiaTensorWorld() *
								ftorqueAxis1 * constraint->getRigidBodyA().getAngularFactor();
						}
						{
							const btVector3& ftorqueAxis2 = solverConstraint.m_relpos2CrossNormal;
							solverConstraint.m_angularComponentB = constraint
							                                       ->getRigidBodyB().getInvInertiaTensorWorld() *
								ftorqueAxis2 * constraint->getRigidBodyB().getAngularFactor();
						}

						{
							btVector3 iMJlA = solverConstraint.m_contactNormal1 * rbA.getInvMass();
							btVector3 iMJaA = rbA.getInvInertiaTensorWorld() * solverConstraint.m_relpos1CrossNormal;
							btVector3 iMJlB = solverConstraint.m_contactNormal2 * rbB.getInvMass(); //sign of normal?
							btVector3 iMJaB = rbB.getInvInertiaTensorWorld() * solverConstraint.m_relpos2CrossNormal;

							btScalar sum = iMJlA.dot(solverConstraint.m_contactNormal1);
							sum += iMJaA.dot(solverConstraint.m_relpos1CrossNormal);
							sum += iMJlB.dot(solverConstraint.m_contactNormal2);
							sum += iMJaB.dot(solverConstraint.m_relpos2CrossNormal);
							btScalar fsum = btFabs(sum);
							assert(fsum > SIMD_EPSILON);
							solverConstraint.m_jacDiagABInv = fsum > SIMD_EPSILON ? btScalar(1.) / sum : 0.f;
						}

						{
							btScalar rel_vel;
							btVector3 externalForceImpulseA = bodyAPtr->m_originalBody
								                                  ? bodyAPtr->m_externalForceImpulse
								                                  : btVector3(0, 0, 0);
							btVector3 externalTorqueImpulseA = bodyAPtr->m_originalBody
								                                   ? bodyAPtr->m_externalTorqueImpulse
								                                   : btVector3(0, 0, 0);

							btVector3 externalForceImpulseB = bodyBPtr->m_originalBody
								                                  ? bodyBPtr->m_externalForceImpulse
								                                  : btVector3(0, 0, 0);
							btVector3 externalTorqueImpulseB = bodyBPtr->m_originalBody
								                                   ? bodyBPtr->m_externalTorqueImpulse
								                                   : btVector3(0, 0, 0);

							btScalar vel1Dotn = solverConstraint.m_contactNormal1.dot(
									rbA.getLinearVelocity() + externalForceImpulseA)
								+ solverConstraint.m_relpos1CrossNormal.dot(
									rbA.getAngularVelocity() + externalTorqueImpulseA);

							btScalar vel2Dotn = solverConstraint.m_contactNormal2.dot(
									rbB.getLinearVelocity() + externalForceImpulseB)
								+ solverConstraint.m_relpos2CrossNormal.dot(
									rbB.getAngularVelocity() + externalTorqueImpulseB);

							rel_vel = vel1Dotn + vel2Dotn;
							btScalar restitution = 0.f;
							btScalar positionalError = solverConstraint.m_rhs; //already filled in by getConstraintInfo2
							btScalar velocityError = restitution - rel_vel * info2.m_damping;
							btScalar penetrationImpulse = positionalError * solverConstraint.m_jacDiagABInv;
							btScalar velocityImpulse = velocityError * solverConstraint.m_jacDiagABInv;
							solverConstraint.m_rhs = penetrationImpulse + velocityImpulse;
							solverConstraint.m_appliedImpulse = 0.f;
						}
					}
				}
				currentRow += m_tmpConstraintSizesPool[i].m_numConstraintRows;
			}
		}

		if (!EnableMLCP)
			return;

		{
			BT_PROFILE("gather constraint data");

			int numFrictionPerContact = m_tmpSolverContactConstraintPool.size() ==
			                            m_tmpSolverContactFrictionConstraintPool.size()
				                            ? 1
				                            : 2;

			m_allConstraintPtrArray.resize(0);
			m_limitDependencies.resize(m_tmpSolverNonContactConstraintPool.size());
			assert(m_limitDependencies.size() == m_tmpSolverNonContactConstraintPool.size());

			int dindex = 0;
			for (int i = 0; i < m_tmpSolverNonContactConstraintPool.size(); i++)
			{
				m_allConstraintPtrArray.push_back(&m_tmpSolverNonContactConstraintPool[i]);
				m_limitDependencies[dindex++] = -1;
			}

			if (!m_allConstraintPtrArray.size())
			{
				m_A.resize(0, 0);
				m_b.resize(0);
				m_x.resize(0);
				m_lo.resize(0);
				m_hi.resize(0);
				return;
			}
		}

		createMLCPFast(infoGlobal);
		if (solveMLCP(infoGlobal))
		{
			BT_PROFILE("process MLCP results");
			for (int i = 0; i < m_allConstraintPtrArray.size(); i++)
			{
				btSolverConstraint& c = *m_allConstraintPtrArray[i];
				int sbA = c.m_solverBodyIdA;
				int sbB = c.m_solverBodyIdB;
				//btRigidBody* orgBodyA = m_tmpSolverBodyPool[sbA].m_originalBody;
				//	btRigidBody* orgBodyB = m_tmpSolverBodyPool[sbB].m_originalBody;

				btSolverBody& solverBodyA = (*m_solverBodies)[sbA];
				btSolverBody& solverBodyB = (*m_solverBodies)[sbB];

				{
					btScalar deltaImpulse = m_x[i] - c.m_appliedImpulse;
					c.m_appliedImpulse = m_x[i];
					solverBodyA.internalApplyImpulse(c.m_contactNormal1 * solverBodyA.internalGetInvMass(),
					                                 c.m_angularComponentA, deltaImpulse);
					solverBodyB.internalApplyImpulse(c.m_contactNormal2 * solverBodyB.internalGetInvMass(),
					                                 c.m_angularComponentB, deltaImpulse);
				}

				{
					btScalar deltaImpulse = m_xSplit[i] - c.m_appliedPushImpulse;
					solverBodyA.internalApplyPushImpulse(c.m_contactNormal1 * solverBodyA.internalGetInvMass(),
					                                     c.m_angularComponentA, deltaImpulse);
					solverBodyB.internalApplyPushImpulse(c.m_contactNormal2 * solverBodyB.internalGetInvMass(),
					                                     c.m_angularComponentB, deltaImpulse);
					c.m_appliedPushImpulse = m_xSplit[i];
				}
			}
		}
	}

	void ConstraintGroup::createMLCPFast(const btContactSolverInfo& infoGlobal)
	{
		struct btJointNode
		{
			int jointIndex; // pointer to enclosing dxJoint object
			int otherBodyIndex; // *other* body this joint is connected to
			int nextJointNodeIndex; //-1 for null
			int constraintRowIndex;
		};

		int numConstraintRows = m_allConstraintPtrArray.size();

		int n = numConstraintRows;
		{
			m_b.resize(numConstraintRows);
			m_bSplit.resize(numConstraintRows);
			m_b.setZero();
			m_bSplit.setZero();
			for (int i = 0; i < numConstraintRows; i++)
			{
				btScalar jacDiag = m_allConstraintPtrArray[i]->m_jacDiagABInv;
				if (!btFuzzyZero(jacDiag))
				{
					btScalar rhs = m_allConstraintPtrArray[i]->m_rhs;
					btScalar rhsPenetration = m_allConstraintPtrArray[i]->m_rhsPenetration;
					m_b[i] = rhs / jacDiag;
					m_bSplit[i] = rhsPenetration / jacDiag;
				}
			}
		}

		m_lo.resize(numConstraintRows);
		m_hi.resize(numConstraintRows);

		{
			for (int i = 0; i < numConstraintRows; i++)
			{
				if (false) //m_limitDependencies[i]>=0)
				{
					m_lo[i] = -BT_INFINITY;
					m_hi[i] = BT_INFINITY;
				}
				m_lo[i] = m_allConstraintPtrArray[i]->m_lowerLimit;
				m_hi[i] = m_allConstraintPtrArray[i]->m_upperLimit;
				if (m_lo[i] == -SIMD_INFINITY) m_lo[i] = -BT_INFINITY;
				if (m_hi[i] == SIMD_INFINITY) m_hi[i] = BT_INFINITY;
			}
		}

		//
		int m = m_allConstraintPtrArray.size();

		int numBodies = m_solverBodies->size();
		btAlignedObjectArray<int> bodyJointNodeArray;
		bodyJointNodeArray.resize(numBodies, -1);
		btAlignedObjectArray<btJointNode> jointNodeArray;
		jointNodeArray.reserve(2 * m_allConstraintPtrArray.size());

		btMatrixXu J3;
		J3.resize(2 * m, 8);
		btMatrixXu JinvM3;

		JinvM3.resize(2 * m, 8);
		JinvM3.setZero();
		J3.setZero();

		int cur = 0;
		int rowOffset = 0;
		btAlignedObjectArray<int> ofs;
		ofs.resizeNoInitialize(m_allConstraintPtrArray.size());

		{
			int c = 0;
			int numRows = 0;

			for (int i = 0; i < m_allConstraintPtrArray.size(); i += numRows, c++)
			{
				ofs[c] = rowOffset;
				int sbA = m_allConstraintPtrArray[i]->m_solverBodyIdA;
				int sbB = m_allConstraintPtrArray[i]->m_solverBodyIdB;
				btRigidBody* orgBodyA = (*m_solverBodies)[sbA].m_originalBody;
				btRigidBody* orgBodyB = (*m_solverBodies)[sbB].m_originalBody;

				numRows = m_tmpConstraintSizesPool[c].m_numConstraintRows;
				if (orgBodyA)
				{
					{
						int slotA = -1;
						//find free jointNode slot for sbA
						slotA = jointNodeArray.size();
						jointNodeArray.expand(); //NonInitializing();
						int prevSlot = bodyJointNodeArray[sbA];
						bodyJointNodeArray[sbA] = slotA;
						jointNodeArray[slotA].nextJointNodeIndex = prevSlot;
						jointNodeArray[slotA].jointIndex = c;
						jointNodeArray[slotA].constraintRowIndex = i;
						jointNodeArray[slotA].otherBodyIndex = orgBodyB ? sbB : -1;
					}
					for (int row = 0; row < numRows; row++, cur++)
					{
						btVector3 normalInvMass = m_allConstraintPtrArray[i + row]->m_contactNormal1 * orgBodyA->
							getInvMass();
						btVector3 relPosCrossNormalInvInertia = m_allConstraintPtrArray[i + row]->m_relpos1CrossNormal *
							orgBodyA->getInvInertiaTensorWorld();

						for (int r = 0; r < 3; r++)
						{
							J3.setElem(cur, r, m_allConstraintPtrArray[i + row]->m_contactNormal1[r]);
							J3.setElem(cur, r + 4, m_allConstraintPtrArray[i + row]->m_relpos1CrossNormal[r]);
							JinvM3.setElem(cur, r, normalInvMass[r]);
							JinvM3.setElem(cur, r + 4, relPosCrossNormalInvInertia[r]);
						}
						J3.setElem(cur, 3, 0);
						JinvM3.setElem(cur, 3, 0);
						J3.setElem(cur, 7, 0);
						JinvM3.setElem(cur, 7, 0);
					}
				}
				else
				{
					cur += numRows;
				}
				if (orgBodyB)
				{
					{
						int slotB = -1;
						//find free jointNode slot for sbA
						slotB = jointNodeArray.size();
						jointNodeArray.expand(); //NonInitializing();
						int prevSlot = bodyJointNodeArray[sbB];
						bodyJointNodeArray[sbB] = slotB;
						jointNodeArray[slotB].nextJointNodeIndex = prevSlot;
						jointNodeArray[slotB].jointIndex = c;
						jointNodeArray[slotB].otherBodyIndex = orgBodyA ? sbA : -1;
						jointNodeArray[slotB].constraintRowIndex = i;
					}

					for (int row = 0; row < numRows; row++, cur++)
					{
						btVector3 normalInvMassB = m_allConstraintPtrArray[i + row]->m_contactNormal2 * orgBodyB->
							getInvMass();
						btVector3 relPosInvInertiaB = m_allConstraintPtrArray[i + row]->m_relpos2CrossNormal * orgBodyB
							->getInvInertiaTensorWorld();

						for (int r = 0; r < 3; r++)
						{
							J3.setElem(cur, r, m_allConstraintPtrArray[i + row]->m_contactNormal2[r]);
							J3.setElem(cur, r + 4, m_allConstraintPtrArray[i + row]->m_relpos2CrossNormal[r]);
							JinvM3.setElem(cur, r, normalInvMassB[r]);
							JinvM3.setElem(cur, r + 4, relPosInvInertiaB[r]);
						}
						J3.setElem(cur, 3, 0);
						JinvM3.setElem(cur, 3, 0);
						J3.setElem(cur, 7, 0);
						JinvM3.setElem(cur, 7, 0);
					}
				}
				else
				{
					cur += numRows;
				}
				rowOffset += numRows;
			}
		}


		//compute JinvM = J*invM.
		const btScalar* JinvM = JinvM3.getBufferPointer();
		const btScalar* Jptr = J3.getBufferPointer();
		m_A.resize(n, n);
		m_A.setZero();
		int c = 0;

		{
			int numRows = 0;
			for (int i = 0; i < m_allConstraintPtrArray.size(); i += numRows, c++)
			{
				int row__ = ofs[c];
				int sbA = m_allConstraintPtrArray[i]->m_solverBodyIdA;
				int sbB = m_allConstraintPtrArray[i]->m_solverBodyIdB;

				numRows = m_tmpConstraintSizesPool[c].m_numConstraintRows;

				const btScalar* JinvMrow = JinvM + 2 * 8 * static_cast<size_t>(row__);

				int startJointNodeA = bodyJointNodeArray[sbA];
				while (startJointNodeA >= 0)
				{
					int j0 = jointNodeArray[startJointNodeA].jointIndex;
					int cr0 = jointNodeArray[startJointNodeA].constraintRowIndex;
					if (j0 < c)
					{
						int numRowsOther = m_tmpConstraintSizesPool[j0].m_numConstraintRows;
						size_t ofsother = (m_allConstraintPtrArray[cr0]->m_solverBodyIdB == sbA) ? 8 * numRowsOther : 0;
						//printf("%d joint i %d and j0: %d: ",count++,i,j0);
						m_A.multiplyAdd2_p8r(JinvMrow,
						                     Jptr + 2 * 8 * static_cast<size_t>(ofs[j0]) + ofsother, numRows,
						                     numRowsOther, row__, ofs[j0]);
					}
					startJointNodeA = jointNodeArray[startJointNodeA].nextJointNodeIndex;
				}

				int startJointNodeB = bodyJointNodeArray[sbB];
				while (startJointNodeB >= 0)
				{
					int j1 = jointNodeArray[startJointNodeB].jointIndex;
					int cj1 = jointNodeArray[startJointNodeB].constraintRowIndex;

					if (j1 < c)
					{
						int numRowsOther = m_tmpConstraintSizesPool[j1].m_numConstraintRows;
						size_t ofsother = (m_allConstraintPtrArray[cj1]->m_solverBodyIdB == sbB) ? 8 * numRowsOther : 0;
						m_A.multiplyAdd2_p8r(JinvMrow + 8 * static_cast<size_t>(numRows),
						                     Jptr + 2 * 8 * static_cast<size_t>(ofs[j1]) + ofsother, numRows,
						                     numRowsOther, row__, ofs[j1]);
					}
					startJointNodeB = jointNodeArray[startJointNodeB].nextJointNodeIndex;
				}
			}

			// compute diagonal blocks of m_A
			int row__ = 0;
			int numJointRows = m_allConstraintPtrArray.size();

			int jj = 0;
			for (; row__ < numJointRows;)
			{
				//int sbA = m_allConstraintPtrArray[row__]->m_solverBodyIdA;
				int sbB = m_allConstraintPtrArray[row__]->m_solverBodyIdB;
				btRigidBody* orgBodyB = (*m_solverBodies)[sbB].m_originalBody;


				const unsigned int infom = m_tmpConstraintSizesPool[jj].m_numConstraintRows;

				const btScalar* JinvMrow = JinvM + 2 * 8 * static_cast<size_t>(row__);
				const btScalar* Jrow = Jptr + 2 * 8 * static_cast<size_t>(row__);
				m_A.multiply2_p8r(JinvMrow, Jrow, infom, infom, row__, row__);
				if (orgBodyB)
				{
					m_A.multiplyAdd2_p8r(JinvMrow + 8 * static_cast<size_t>(infom),
					                     Jrow + 8 * static_cast<size_t>(infom), infom, infom, row__, row__);
				}
				row__ += infom;
				jj++;
			}
		}

		// add cfm to the diagonal of m_A
		for (int i = 0; i < m_A.rows(); ++i)
		{
			m_A.setElem(i, i, m_A(i, i) + infoGlobal.m_globalCfm / infoGlobal.m_timeStep);
		}

		///fill the upper triangle of the matrix, to make it symmetric
		m_A.copyLowerToUpperTriangle();

		m_x.resize(numConstraintRows);
		m_xSplit.resize(numConstraintRows);

		if (infoGlobal.m_solverMode & SOLVER_USE_WARMSTARTING)
		{
			for (int i = 0; i < m_allConstraintPtrArray.size(); i++)
			{
				const btSolverConstraint& c = *m_allConstraintPtrArray[i];
				m_x[i] = c.m_appliedImpulse;
				m_xSplit[i] = c.m_appliedPushImpulse;
			}
		}
		else
		{
			m_x.setZero();
			m_xSplit.setZero();
		}
	}

	btScalar ConstraintGroup::solveSingleIteration(int iteration, btCollisionObject** /*bodies */, int /*numBodies*/,
	                                               btPersistentManifold** /*manifoldPtr*/, int /*numManifolds*/,
	                                               btTypedConstraint** constraints, int numConstraints,
	                                               const btContactSolverInfo& infoGlobal, btIDebugDraw* /*debugDrawer*/)
	{
		float maxImpulse = 0.f;

		///solve all joint constraints, using SIMD, if available
		for (int j = 0; j < m_tmpSolverNonContactConstraintPool.size(); j++)
		{
			btSolverConstraint& constraint = m_tmpSolverNonContactConstraintPool[j];
			float deltaImpulse = resolveSingleConstraintRowGenericSIMD((*m_solverBodies)[constraint.m_solverBodyIdA],
			                                                           (*m_solverBodies)[constraint.m_solverBodyIdB],
			                                                           constraint);
			maxImpulse = std::max(maxImpulse, deltaImpulse);
		}

		return maxImpulse;
	}

	void ConstraintGroup::iteration(btCollisionObject** bodies, size_t numBodies, const btContactSolverInfo& info)
	{
		solveGroupCacheFriendlyIterations(bodies, numBodies, nullptr, 0, m_btConstraints.data(), m_btConstraints.size(),
		                                  info, nullptr);
	}

	btScalar ConstraintGroup::solveGroupCacheFriendlyIterations(btCollisionObject** bodies, int numBodies,
	                                                            btPersistentManifold** manifoldPtr, int numManifolds,
	                                                            btTypedConstraint** constraints, int numConstraints,
	                                                            const btContactSolverInfo& infoGlobal,
	                                                            btIDebugDraw* debugDrawer)
	{
		for (int iteration = 0; iteration < MaxIterations; iteration++)
			if (solveSingleIteration(iteration, bodies, numBodies, manifoldPtr, numManifolds, constraints,
			                         numConstraints, infoGlobal, debugDrawer) < FLT_EPSILON)
				break;
		return 0.f;
	}
}

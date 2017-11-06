#include <gcore/sxgcore.h>

#include "BaseTrigger.h"


BEGIN_PROPTABLE(CBaseTrigger)
	DEFINE_OUTPUT(m_onTouchStart, "OnTouchStart", "On touch start")
	DEFINE_OUTPUT(m_onTouchEnd, "OnTouchEnd", "On touch end")
	DEFINE_OUTPUT(m_onTouchEndAll, "OnTouchEndAll", "On touch end all")

	DEFINE_INPUT(inEnable, "enable", "Enable", PDF_NONE)
	DEFINE_INPUT(inDisable, "disable", "Disable", PDF_NONE)
	DEFINE_INPUT(inToggle, "toggle", "Toggle", PDF_NONE)
END_PROPTABLE()

REGISTER_ENTITY(CBaseTrigger, trigger);

CBaseTrigger::CBaseTrigger(EntityManager * pMgr):
	BaseClass(pMgr),
	m_bEnabled(true),
	m_pGhostObject(NULL),
	m_idUpdateInterval(-1)
{
	m_idUpdateInterval = SET_INTERVAL(update, 0);
}

CBaseTrigger::~CBaseTrigger()
{
	CLEAR_INTERVAL(m_idUpdateInterval);
}

void CBaseTrigger::OnPostLoad()
{
	BaseClass::OnPostLoad();

	if(m_pAnimPlayer)
	{
		m_pAnimPlayer->enable(false);
		//m_pAnimPlayer->setOverrideMaterial("dev_trigger.dds");
	}
}

void CBaseTrigger::enable()
{
	if(!m_bEnabled)
	{
		m_bEnabled = true;
		m_idUpdateInterval = SET_INTERVAL(update, 0);
		if(m_pGhostObject)
		{
			SXPhysics_GetDynWorld()->addCollisionObject(m_pGhostObject, btBroadphaseProxy::SensorTrigger, btBroadphaseProxy::CharacterFilter);
		}
	}
}
void CBaseTrigger::disable()
{
	if(m_bEnabled)
	{
		m_bEnabled = false;
		CLEAR_INTERVAL(m_idUpdateInterval);
		if(m_pGhostObject)
		{
			SXPhysics_GetDynWorld()->removeCollisionObject(m_pGhostObject);
		}
	}
}
void CBaseTrigger::toggle()
{
	if(m_bEnabled)
	{
		disable();
	}
	else
	{
		enable();
	}
}

void CBaseTrigger::inEnable(inputdata_t * pInputdata)
{
	enable();
}
void CBaseTrigger::inDisable(inputdata_t * pInputdata)
{
	disable();
}
void CBaseTrigger::inToggle(inputdata_t * pInputdata)
{
	toggle();
}

void CBaseTrigger::CreatePhysBody()
{
	if(m_pCollideShape)
	{
		m_pGhostObject = new btPairCachingGhostObject();
		m_pGhostObject->setWorldTransform(btTransform(Q4_BTQUAT(m_vOrientation), F3_BTVEC(m_vPosition)));
		m_pGhostObject->setUserPointer(this);
		m_pGhostObject->setCollisionShape(m_pCollideShape);
		m_pGhostObject->setCollisionFlags(m_pGhostObject->getCollisionFlags() ^ btCollisionObject::CF_NO_CONTACT_RESPONSE);

		SXPhysics_GetDynWorld()->addCollisionObject(m_pGhostObject, btBroadphaseProxy::SensorTrigger, btBroadphaseProxy::CharacterFilter);
	}
}

void CBaseTrigger::RemovePhysBody()
{
	if(m_pGhostObject)
	{
		SXPhysics_GetDynWorld()->removeCollisionObject(m_pGhostObject);
		mem_delete(m_pGhostObject);
	}
}

void CBaseTrigger::onTouchStart(SXbaseEntity *pActivator)
{
	FIRE_OUTPUT(m_onTouchStart, pActivator);
}
void CBaseTrigger::onTouchEnd(SXbaseEntity *pActivator)
{
	FIRE_OUTPUT(m_onTouchEnd, pActivator);
}
void CBaseTrigger::onTouchEndAll(SXbaseEntity *pActivator)
{
	FIRE_OUTPUT(m_onTouchEndAll, pActivator);
}

void CBaseTrigger::OnSync()
{
	BaseClass::OnSync();

	if(!m_pGhostObject || !m_bEnabled)
	{
		return;
	}
	m_aNewTouches.clearFast();

	btManifoldArray manifoldArray;
	btBroadphasePairArray &pairArray = m_pGhostObject->getOverlappingPairCache()->getOverlappingPairArray();
	int iTouches = pairArray.size();
	for(int i = 0; i < iTouches; ++i)
	{
		const btBroadphasePair &pair = pairArray[i];
		btBroadphasePair *pCollisionPair = SXPhysics_GetDynWorld()->getPairCache()->findPair(pair.m_pProxy0, pair.m_pProxy1);
		if(!pCollisionPair)
		{
			continue;
		}
		else
		{
			if(pCollisionPair->m_algorithm)
			{
				pCollisionPair->m_algorithm->getAllContactManifolds(manifoldArray);
				if(!manifoldArray.size())
				{
					continue;
				}
				else
				{
					for(int j = 0, jl = manifoldArray.size(); j < jl; ++j)
					{
						if(manifoldArray[j]->getNumContacts() > 0)
						{
							const btCollisionObject * pObject = (manifoldArray[0]->getBody0() == m_pGhostObject)
								? manifoldArray[0]->getBody1()
								: manifoldArray[0]->getBody0();

							SXbaseEntity * pEnt = (SXbaseEntity*)pObject->getUserPointer();
							if(pEnt)
							{
								m_aNewTouches.push_back(pEnt);
								//printf("touched %s\n", pEnt->GetClassName());
							}
						}
					}
				}
				//printf("m=%d ", manifoldArray.size());
			}
		}
	}
	//m_pGhostObject->getOverlappingObject(0);
	//printf("%d\n", m_iTouches);
}

void CBaseTrigger::update(float dt)
{
	if(!m_bEnabled)
	{
		return;
	}

	
	bool bFound;
	for(int i = 0, l = m_aNewTouches.size(); i < l; ++i)
	{
		bFound = false;
		for(int j = 0, jl = m_aTouches.size(); j < jl; ++j)
		{
			if(m_aNewTouches[i] == m_aTouches[j])
			{
				m_aTouches[j] = NULL;
				bFound = true;
				break;
			}
		}
		if(!bFound)
		{
			onTouchStart(m_aNewTouches[i]);
		}
	}
	SXbaseEntity * pLastTouch = NULL;
	for(int j = 0, jl = m_aTouches.size(); j < jl; ++j)
	{
		if(m_aTouches[j])
		{
			onTouchEnd(m_aTouches[j]);
			pLastTouch = m_aTouches[j];
		}
	}
	if(pLastTouch && !m_aNewTouches.size())
	{
		onTouchEndAll(m_aTouches[0]);
	}
	m_aTouches.swap(m_aNewTouches);
}
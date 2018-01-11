#include "BaseAnimating.h"
#include "gcore/sxgcore.h"

/*! \skydocent base_animating
Базовый класс для объектов, имеющих объем в игровом мире
*/

BEGIN_PROPTABLE(CBaseAnimating)
	//! Файл модели. Поддерживаются статические и анимированные модели
	DEFINE_FIELD_STRING(m_szModelFile, 0, "model", "Model file", EDITOR_FILEFIELD)
		FILE_OPTION("Model file (*.dse)", "*.dse")
	EDITOR_FILE_END()

	//! Масштаб модели
	DEFINE_FIELD_FLOAT(m_fBaseScale, 0, "scale", "Scale", EDITOR_TEXTFIELD)
END_PROPTABLE()

REGISTER_ENTITY_NOLISTING(CBaseAnimating, base_animating);

CBaseAnimating::CBaseAnimating(CEntityManager * pMgr):
	BaseClass(pMgr),
	m_pAnimPlayer(NULL),
	m_fBaseScale(1.0f),
	m_pCollideShape(NULL),
	m_pRigidBody(NULL)
{
}

CBaseAnimating::~CBaseAnimating()
{
	releasePhysics();
	mem_release(m_pAnimPlayer);
}

void CBaseAnimating::getMinMax(float3 * min, float3 * max)
{
	if (m_pAnimPlayer)
	{
		const ISXBound * bound = m_pAnimPlayer->getBound();
		bound->GetMinMax(min, max);
	}
}

void CBaseAnimating::getSphere(float3 * center, float * radius)
{
	if(m_pAnimPlayer)
	{
		const ISXBound * bound = m_pAnimPlayer->getBound();
		bound->GetSphere(center, radius);
	}
}

bool CBaseAnimating::setKV(const char * name, const char * value)
{
	if(!BaseClass::setKV(name, value))
	{
		return(false);
	}
	if(!strcmp(name, "model"))
	{
		setModel(value);
	}
	else if(!strcmp(name, "scale"))
	{
		releasePhysics();
		m_pAnimPlayer->setScale(m_fBaseScale);
		initPhysics();
	}
	return(true);
}

void CBaseAnimating::setModel(const char * mdl)
{
	_setStrVal(&m_szModelFile, mdl);
	releasePhysics();
	if(!mdl[0] && m_pAnimPlayer)
	{
		mem_release(m_pAnimPlayer);
		return;
	}
	if(!m_pAnimPlayer)
	{
		m_pAnimPlayer = SXAnim_CreatePlayer(mdl);
	}
	else
	{
		m_pAnimPlayer->setModel(mdl);
	}
	initPhysics();
}

float3 CBaseAnimating::getAttachmentPos(int id)
{
	float3 pos;
	if(!m_pAnimPlayer && id >= 0)
	{
		pos = m_pAnimPlayer->getBoneTransformPos(id);
	}

	return(getOrient() * pos + getPos());
}

SMQuaternion CBaseAnimating::getAttachmentRot(int id)
{
	SMQuaternion rot;
	if(!m_pAnimPlayer && id >= 0)
	{
		rot = m_pAnimPlayer->getBoneTransformRot(id);
	}

	return(getOrient() * rot);
}

void CBaseAnimating::onSync()
{
	BaseClass::onSync();
	if(!m_pParent && m_pRigidBody)
	{
		setPos(BTVEC_F3(m_pRigidBody->getWorldTransform().getOrigin()));
		setOrient(BTQUAT_Q4(m_pRigidBody->getWorldTransform().getRotation()));
	}
	else if(m_pRigidBody)
	{
	//	m_pRigidBody->getWorldTransform().setOrigin(F3_BTVEC(getPos()));
	//	m_pRigidBody->getWorldTransform().setRotation(Q4_BTQUAT(getOrient()));
	}
	if(m_pAnimPlayer)
	{
		m_pAnimPlayer->setScale(m_fBaseScale);
		m_pAnimPlayer->setPos(getPos());
		m_pAnimPlayer->setOrient(getOrient());
	}
}

void CBaseAnimating::playAnimation(const char * name, UINT iFadeTime, UINT slot)
{
	if(m_pAnimPlayer)
	{
		m_pAnimPlayer->play(name, iFadeTime, slot);
	}
}

bool CBaseAnimating::playingAnimations(const char* name)
{
	if(m_pAnimPlayer)
	{
		return(m_pAnimPlayer->playingAnimations(name));
	}
	return(false);
}

void CBaseAnimating::playActivity(const char * name, UINT iFadeTime, UINT slot)
{
	if(m_pAnimPlayer)
	{
		m_pAnimPlayer->startActivity(name, iFadeTime, slot);
	}
}

void CBaseAnimating::initPhysics()
{
	if(!m_pAnimPlayer)
	{
		return;
	}
	int32_t iShapeCount;
	HITBOX_TYPE * phTypes;
	float3_t ** ppfData;
	int32_t * pfDataLen;

	//m_pAnimPlayer->setScale(m_fBaseScale);

	m_pAnimPlayer->getPhysData(&iShapeCount, &phTypes, &ppfData, &pfDataLen);

	for(int i = 0; i < iShapeCount; ++i)
	{
		if(phTypes[i] == HT_CONVEX)
		{
			m_pCollideShape = new btConvexHullShape((float*)ppfData[i], pfDataLen[i], sizeof(ppfData[0][0]));
		}
		break;
	}

	m_pAnimPlayer->freePhysData(iShapeCount, phTypes, ppfData, pfDataLen);


	createPhysBody();
}

void CBaseAnimating::createPhysBody()
{
	if(m_pCollideShape)
	{

		btVector3 vInertia;
		const float fMass = 1.0f;
		m_pCollideShape->calculateLocalInertia(fMass, vInertia);

		btDefaultMotionState * motionState = new btDefaultMotionState(btTransform(Q4_BTQUAT(m_vOrientation), F3_BTVEC(m_vPosition)));
		btRigidBody::btRigidBodyConstructionInfo rigidBodyCI(
			fMass,                  // mass
			motionState,        // initial position
			m_pCollideShape,    // collision shape of body
			vInertia  // local inertia
			);
		m_pRigidBody = new btRigidBody(rigidBodyCI);

		//m_pRigidBody->setFriction(100.0f);
		m_pRigidBody->setUserPointer(this);
		SXPhysics_AddShape(m_pRigidBody);
	}
}

void CBaseAnimating::removePhysBody()
{
	SXPhysics_RemoveShape(m_pRigidBody);
	mem_delete(m_pRigidBody);
}

void CBaseAnimating::releasePhysics()
{
	removePhysBody();
	mem_delete(m_pCollideShape);
}

void CBaseAnimating::setPos(const float3 & pos)
{
	BaseClass::setPos(pos);
	if(m_pRigidBody)
	{
		m_pRigidBody->getWorldTransform().setOrigin(F3_BTVEC(pos));
	}
}

void CBaseAnimating::setOrient(const SMQuaternion & q)
{
	BaseClass::setOrient(q);
	if(m_pRigidBody)
	{
		m_pRigidBody->getWorldTransform().setRotation(Q4_BTQUAT(q));
	}
}
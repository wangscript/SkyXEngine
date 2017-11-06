#include "BaseWeaponAddon.h"

BEGIN_PROPTABLE(CBaseWeaponAddon)
	//empty
END_PROPTABLE()

REGISTER_ENTITY_NOLISTING(CBaseWeaponAddon, base_wpn_addon);

CBaseWeaponAddon::CBaseWeaponAddon(EntityManager * pMgr):
	BaseClass(pMgr),
	m_addonType(WPN_ADDON_NONE)
{
}

WPN_ADDON CBaseWeaponAddon::getType() const
{
	return(m_addonType);
}
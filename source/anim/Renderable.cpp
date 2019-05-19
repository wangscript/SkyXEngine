#include "Renderable.h"
#include "animated.h"
#include "RenderableVisibility.h"

extern AnimationManager * g_mgr;

X_RENDER_STAGE CRenderable::getStages()
{
	return(XRS_GBUFFER | XRS_SHADOWS);
}

UINT CRenderable::getPriorityForStage(X_RENDER_STAGE stage)
{
	return(20);
}

void CRenderable::renderStage(X_RENDER_STAGE stage, IXRenderableVisibility *pVisibility)
{
	ID idVisCalcObj = 0;
	if(pVisibility)
	{
		assert(pVisibility->getPluginId() == 0);

		idVisCalcObj = ((CRenderableVisibility*)pVisibility)->getVisCalcObjId();
	}

	switch(stage)
	{
	case XRS_PREPARE:
		break;
	case XRS_GBUFFER:
		g_mgr->render(idVisCalcObj);
		break;
	case XRS_SHADOWS:
		g_mgr->render(idVisCalcObj);
		break;
	case XRS_GI:
		break;
	case XRS_POSTPROCESS_MAIN:
		break;
	case XRS_TRANSPARENT:
		break;
	case XRS_POSTPROCESS_FINAL:
		break;
	case XRS_EDITOR_2D:
		break;
	}
}

void CRenderable::startup(IGXContext *pDevice, IXMaterialSystem *pMaterialSystem)
{
	m_pDevice = pDevice;
	m_pMaterialSystem = pMaterialSystem;
}
void CRenderable::shutdown()
{
}

void CRenderable::newVisData(IXRenderableVisibility **ppVisibility)
{
	*ppVisibility = new CRenderableVisibility(0);
}

IXMaterialSystem *CRenderable::getMaterialSystem()
{
	return(m_pMaterialSystem);
}
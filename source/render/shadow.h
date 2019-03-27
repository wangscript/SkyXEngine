
/***********************************************************
Copyright © Vitaliy Buturlin, Evgeny Danilovich, 2017, 2018
See the license in LICENSE
***********************************************************/

#ifndef __SHADOW_H
#define __SHADOW_H

#include <gdefines.h>

#include <common/Math.h>
#include <common/Array.h>
#include <graphix/graphix.h>
#include <gcore/sxgcore.h>
#include <light/IXLight.h>

class IBaseShadowMap
{
public:
	virtual void genShadow(IGXTexture2D *pShadowMap) = 0;
};

class CShadowMap: public IBaseShadowMap
{
public:
	CShadowMap();
	~CShadowMap();

	SX_ALIGNED_OP_MEM2();

	static UINT GetMapMemory(UINT uSize);

	void init(IGXContext *pContext, UINT uSize);
	
	void setLight(IXLight *pLight);
	void process(IXRenderPipeline *pRenderPipeline);
	void genShadow(IGXTexture2D *shadowmap);
	
private:
	IGXContext *m_pDevice = NULL;

	static IGXDepthStencilSurface *ms_pDepthStencilSurface;
	static UINT ms_uDepthStencilSurfaceRefCount;

	static void InitDepthStencilSurface(IGXContext *pContext, UINT uSize);
	static void ReleaseDepthStencilSurface();

	IGXTexture2D *m_pDepthMap = NULL;
	IGXTexture2D *m_pNormalMap = NULL;
	IGXTexture2D *m_pFluxMap = NULL;

	float4x4 m_mScaleBiasMat;
	float m_fBias = 0.0001f;
	float m_fBlurPixel = 0.5f;

	float m_fSize;

	IXLight *m_pLight = NULL;

	float4x4 m_mView;
	float4x4 m_mProj;
};

//##########################################################################


#endif

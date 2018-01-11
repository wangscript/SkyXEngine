#ifndef _crosshair_H_
#define _crosshair_H_

//#include <gdefines.h>
#include "sxgame.h"
//#include <common/SXmath.h>

#define CROSSHAIR_MINSEGS 12
#define CROSSHAIR_MAXSEGS 12

static_assert(CROSSHAIR_MINSEGS <= CROSSHAIR_MAXSEGS, "Maxsegs cannot be lesser than minsegs!");

class CCrosshair
{
public:
	enum STYLE
	{
		SPLIT_MOVE,
		SCALED
	};

	CCrosshair();
	~CCrosshair();

	void update();
	void render();
	void onSync();

	void enable(bool en = true);

	void setNumSegmens(int num);
	void setAngle(float a);
	void setFixedRadius(float r);
	void setStyle(STYLE style);
	void setMaxSize(float size);
	void setTexture(ID id);
	
	void setSize(float size);

	void setTexInfo(const float2_t & offs, const float2_t & size);

protected:

	typedef struct
	{
		float3_t pos;
		float2_t tex;
	} Vertex;

	bool m_bDirty;
	bool m_bBuildBuff;
	float m_fSize;
	ID m_idTexture;
	bool m_bHidden;
	float m_fMaxSize;
	STYLE m_style;
	float m_fFixedRadius;
	float m_fAngle;
	int m_iNumSegs;

	byte * m_pMemoryBlob;

	byte m_u8ActiveBuffer;
	Vertex * m_pVertices[2];
	UINT * m_pIndices[2];
	int m_iVertexCount[2];
	int m_iIndexCount[2];

	IDirect3DVertexBuffer9 * m_pVertexBuffer;
	IDirect3DIndexBuffer9 * m_pIndexBuffer;

	IDirect3DDevice9 * m_pDev;
	IDirect3DTexture9 * m_pTexture;

	float2_t m_f2TexOffs;
	float2_t m_f2TexSize;
};

#endif

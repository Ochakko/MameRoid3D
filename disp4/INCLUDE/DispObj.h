#ifndef DISPOBJH
#define DISPOBJH

#include <D3DX9.h>

class CMQOObject;
class CPolyMesh3;
class CExtLine;

class CDispObj
{
public:
	CDispObj();
	~CDispObj();

	int CreateDispObj( LPDIRECT3DDEVICE9 pdev, CPolyMesh3* pm3, int hasbone );
	int CreateDispObj( LPDIRECT3DDEVICE9 pdev, CExtLine* extline );
	int RenderNormal( int lightflag, D3DXVECTOR4 diffusemult, CMQOObject* morphbaseobj = 0 );
	int RenderLine( D3DXVECTOR4 diffusemult );

	int CopyDispV( CPolyMesh3* pm3 );

private:
	int InitParams();
	int DestroyObjs();

	int CreateDecl();
	int CreateVBandIB();
	int CreateVBandIBLine();

public:
	int m_hasbone;

	LPDIRECT3DDEVICE9 m_pdev;
	CPolyMesh3* m_pm3;//äOïîÉÅÉÇÉä
	CExtLine* m_extline;//äOïîÉÅÉÇÉä

	IDirect3DVertexDeclaration9* m_declbone;
	IDirect3DVertexDeclaration9* m_declnobone;
	IDirect3DVertexDeclaration9* m_declline;

    LPDIRECT3DVERTEXBUFFER9 m_VB;
	LPDIRECT3DVERTEXBUFFER9 m_InfB;
	LPDIRECT3DINDEXBUFFER9 m_IB;
};



#endif
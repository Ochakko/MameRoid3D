#ifndef XFILEH
#define XFILEH

#include <D3DX9.h>

#include <coef.h>

class CModel;
class CMQOObject;
class CBone;

class CXFile
{
public:
	CXFile();
	~CXFile();

	int WriteXFile( WCHAR* srcpath, CModel* srcmodel, float mult );

private:

	int Write2File( int spnum, char* lpFormat, ... );
	int CloseFile();

	int WriteDispObject( CMQOObject* srcobj );
	void WriteBoneObjectReq( int spnum, CBone* srcbone );

	int WriteTransformationMatrix( int spnum, CBone* srcbone );
	int WritePolyMesh3( int spnum, CMQOObject* srcobj );

	int WritePosition( int spnum, CMQOObject* srcobj );
	int WriteFaceIndex( int spnum, CMQOObject* srcobj );
	int WriteNormal( int spnum, CMQOObject* srcobj );
	int WriteUV( int spnum, CMQOObject* srcobj );
	int WriteMaterialList( int spnum, CMQOObject* srcobj );
	int WriteDuplicationIndices( int spnum, CMQOObject* srcobj );
	int WriteSkinMeshHeader( int spnum, CMQOObject* srcobj, int infbonenum );
	int WriteSkinWeights( int spnum, BONEINFLUENCE* boneinf, int infbonenum );

	int WriteAnimationSet( MOTINFO* srcmi );

	int CheckMultiBytesChar( char* srcname );

private:
	CModel* m_model;
	float m_mult;

	HANDLE m_hfile;
};



#endif
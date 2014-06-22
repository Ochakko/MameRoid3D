#ifndef QUBFILEH
#define QUBFILEH

#include <d3dx9.h>
#include <coef.h>
#include <XMLIO.h>

#include <map>

class CModel;
class CMQOObject;
class CBone;
class CMotionPoint;
class CQuaternion;
class CPmCipherDll;
class CMorphKey;

class CQubFile : public CXMLIO
{
public:
	CQubFile();
	virtual ~CQubFile();

	int WriteQubFile( WCHAR* strpath, MOTINFO* srcmotinfo, CBone* srctopbone, int srcbonenum, std::map<int,CMQOObject*>& srcbaseobj, char* mmotname );
	int LoadQubFile( WCHAR* strpath, CModel* srcmodel, int* newid );
	int LoadQubFileFromPnd( CPmCipherDll* cipher, int qubindex, CModel* srcmodel, int* newid );

private:
	virtual int InitParams();
	virtual int DestroyObjs();

	int WriteFileInfo( char* mmotname );
	void WriteBoneReq( CBone* srcbone, int broflag );
	int WriteMorphBase( CMQOObject* srcbase );

	int CheckFileVersion( XMLIOBUF* xmliobuf );
	int ReadMotionInfo( WCHAR* wfilename, XMLIOBUF* xmliobuf );
	int ReadBone( XMLIOBUF* xmliobuf );
	int ReadMotionPoint( XMLIOBUF* xmliobuf, CBone* srcbone );

	int ReadMorphBase( XMLIOBUF* xmliobuf );
	int ReadMorphKey( XMLIOBUF* xmliobuf, CMQOObject* srcbase );
	int ReadBlendWeight( XMLIOBUF* xmliobuf, CMQOObject* setbase, CMorphKey* setmk );

public:
	int m_fileversion;
	CModel* m_model;
	CBone* m_topbone;
	int m_bonenum;
	int m_basenum;
	int m_motid;
	MOTINFO* m_motinfo;
};

#endif
#ifndef KbsFileH
#define KbsFileH

#include <d3dx9.h>
#include <coef.h>
#include <XMLIO.h>

class CModel;

class CKbsFile : public CXMLIO
{
public:
	CKbsFile();
	virtual ~CKbsFile();

	int SaveKbsFile( WCHAR* filename );
	int LoadKbsFile( CModel* srcmodel, WCHAR* filename );

private:
	virtual int InitParams();
	virtual int DestroyObjs();

	int WriteFileInfo();
	int WriteJointInfo( int skelno );

	int CheckFileVersion( XMLIOBUF* xmliobuf );
	int ReadJointInfo( XMLIOBUF* xmliobuf, int skelno );

public:
	TSELEM m_elem[ SKEL_MAX ];
private:
	CModel* m_model;

};

#endif
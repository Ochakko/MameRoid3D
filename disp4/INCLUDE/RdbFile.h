#ifndef RDBFILEH
#define RDBFILEH

#include <d3dx9.h>
#include <coef.h>
#include <XMLIO.h>

#include <vector>
#include <string>
#include <map>

class CModel;
class CPmCipherDll;

class CRdbFile : public CXMLIO
{
public:
	CRdbFile();
	virtual ~CRdbFile();

	int WriteRdbFile( WCHAR* projdir, WCHAR* projname, std::vector<MODELELEM>& srcmodelindex );
	int LoadRdbFile( WCHAR* strpath, CModel* (*srcmqofunc)(), int (*srcqubfunc)(), int (*srcdelmfunc)( int delindex ) );
	int LoadRdbFileFromPnd( CPmCipherDll* cipher, int rdbindex, 
		CModel* (*srcmqofunc)( CPmCipherDll* cipher, char* modelfolder, char* mqopath ), int (*srcqubfunc)( CPmCipherDll* cipher, char* motpath ), int (*srcdelmfunc)( int delindex ) );
private:
	virtual int InitParams();
	virtual int DestroyObjs();

	int WriteFileInfo();
	int WriteChara( MODELELEM* srcme );
	int WriteMotion( WCHAR* motfolder, CModel* srcmodel, MOTINFO* miptr, int motcnt, std::map<std::string, MOTINFO*>& mname2minfo );

	int CheckFileVersion( XMLIOBUF* xmliobuf );
	int ReadProjectInfo( XMLIOBUF* xmliobuf, int* charanumptr );
	int ReadChara( XMLIOBUF* xmliobuf );
	int ReadMotion( XMLIOBUF* xmliobuf, WCHAR* modelfolder, CModel* modelptr );

public:
	CPmCipherDll* m_cipher;
	std::vector<MODELELEM> m_modelindex;
	WCHAR m_newdirname[MAX_PATH];

	std::map<int, MOTINFO*> m_motinfo;
	WCHAR m_wloaddir[MAX_PATH];
	char m_mloaddir[MAX_PATH];

	CModel* (*m_MqoFunc)();
	CModel* (*m_MqoPndFunc)( CPmCipherDll* cipher, char* modelfolder, char* mqopath );
	int (*m_QubFunc)();
	int (*m_QubPndFunc)( CPmCipherDll* cipher, char* motpath );
	int (*m_DelMotFunc)( int delindex );
};

#endif
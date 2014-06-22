#include <stdio.h>
#include <stdarg.h>
#include <math.h>
#include <string.h>
#include <wchar.h>
#include <ctype.h>
#include <malloc.h>
#include <memory.h>

#include <windows.h>
#include <crtdbg.h>

#include <RdbFile.h>

#include <Model.h>
#include <MQOMaterial.h>

#define DBGH
#include <dbg.h>

#include <PmCipherDll.h>

using namespace std;


extern float g_tmpmqomult;
extern WCHAR g_tmpmqopath[MULTIPATH];


CRdbFile::CRdbFile()
{
	InitParams();
}

CRdbFile::~CRdbFile()
{
	DestroyObjs();
}

int CRdbFile::InitParams()
{
	CXMLIO::InitParams();
	m_modelindex.erase( m_modelindex.begin(), m_modelindex.end() );
	ZeroMemory( m_newdirname, sizeof( WCHAR ) * MAX_PATH );

	m_cipher = 0;
	ZeroMemory( m_wloaddir, sizeof( WCHAR ) * MAX_PATH );
	ZeroMemory( m_mloaddir, sizeof( char ) * MAX_PATH );


	return 0;
}

int CRdbFile::DestroyObjs()
{
	CXMLIO::DestroyObjs();

	InitParams();

	return 0;
}


int CRdbFile::WriteRdbFile( WCHAR* projdir, WCHAR* projname, std::vector<MODELELEM>& srcmodelindex )
{
	m_modelindex = srcmodelindex;
	m_mode = XMLIO_WRITE;


	WCHAR strpath[MAX_PATH];
	swprintf_s( strpath, MAX_PATH, L"%s\\%s\\%s.rdb", projdir, projname, projname );


	swprintf_s( m_newdirname, MAX_PATH, L"%s\\%s", projdir, projname );
	DWORD fattr;
	fattr = GetFileAttributes( m_newdirname );
	if( (fattr == -1) || ((fattr & FILE_ATTRIBUTE_DIRECTORY) == 0) ){
		int bret;
		bret = CreateDirectory( m_newdirname, NULL );
		if( bret == 0 ){
			::MessageBox( NULL, L"ディレクトリの作成に失敗しました。\nプロジェクトの保存に失敗しました。", L"エラー", MB_OK );
			_ASSERT( 0 );
			return 1;
		}
	}


	m_hfile = CreateFile( strpath, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS,
		FILE_FLAG_SEQUENTIAL_SCAN, NULL );
	if( m_hfile == INVALID_HANDLE_VALUE ){
		DbgOut( L"RdbFile : WriteRdbFile : file open error !!!\n" );
		_ASSERT( 0 );
		return 1;
	}


	CallF( Write2File( "<?xml version=\"1.0\" encoding=\"Shift_JIS\"?>\r\n<RDB>\r\n" ), return 1 );  
	CallF( WriteFileInfo(), return 1 );


	int modelnum = m_modelindex.size();
	int modelcnt;
	for( modelcnt = 0; modelcnt < modelnum; modelcnt++ ){
		MODELELEM curme = m_modelindex[ modelcnt ];
		CallF( WriteChara( &curme ), return 1 );
	}

	CallF( Write2File( "</RDB>\r\n" ), return 1 );

	return 0;
}

int CRdbFile::WriteFileInfo()
{

	CallF( Write2File( "  <FileInfo>\r\n    <kind>OpenRDBProjectFile</kind>\r\n    <version>1001</version>\r\n    <type>0</type>\r\n  </FileInfo>\r\n" ), return 1 );

	CallF( Write2File( "  <ProjectInfo>\r\n" ), return 1 );
	CallF( Write2File( "    <CharaNum>%d</CharaNum>\r\n", m_modelindex.size() ), return 1 );
	CallF( Write2File( "  </ProjectInfo>\r\n" ), return 1 );

	return 0;
}

int CRdbFile::WriteChara( MODELELEM* srcme )
{
	char filename[MAX_PATH] = {0};
	char modelfolder[MAX_PATH] = {0};

	CModel* curmodel = srcme->modelptr;

	WideCharToMultiByte( CP_ACP, 0, curmodel->m_filename, -1, filename, MAX_PATH, NULL, NULL );
	WideCharToMultiByte( CP_ACP, 0, curmodel->m_modelfolder, -1, modelfolder, MAX_PATH, NULL, NULL );

	CallF( Write2File( "  <Chara>\r\n" ), return 1 );
	CallF( Write2File( "    <ModelFolder>%s</ModelFolder>\r\n", modelfolder ), return 1 );
	CallF( Write2File( "    <ModelFile>%s</ModelFile>\r\n", filename ), return 1 );
	CallF( Write2File( "    <ModelMult>%f</ModelMult>\r\n", curmodel->m_loadmult ), return 1 );
	CallF( Write2File( "    <MotionNum>%d</MotionNum>\r\n", curmodel->m_motinfo.size() ), return 1 );

	WCHAR charafolder[MAX_PATH] = {0L};
	swprintf_s( charafolder, MAX_PATH, L"%s\\%s", m_newdirname, curmodel->m_modelfolder );
	DWORD fattr;
	fattr = GetFileAttributes( charafolder );
	if( (fattr == -1) || ((fattr & FILE_ATTRIBUTE_DIRECTORY) == 0) ){
		int bret;
		bret = CreateDirectory( charafolder, NULL );
		if( bret == 0 ){
			::MessageBox( NULL, L"ディレクトリの作成に失敗しました。\nプロジェクトの保存に失敗しました。", L"エラー", MB_OK );
			_ASSERT( 0 );
			return 1;
		}
	}

	BOOL bret;
	BOOL bcancel;
	WCHAR srcpath[MAX_PATH] = {0L};
	WCHAR dstpath[MAX_PATH] = {0L};
	swprintf_s( srcpath, MAX_PATH, L"%s\\%s", curmodel->m_dirname, curmodel->m_filename );
	swprintf_s( dstpath, MAX_PATH, L"%s\\%s", charafolder, curmodel->m_filename );

	int chksame = wcscmp( curmodel->m_dirname, charafolder );
	if( chksame != 0 ){
		bcancel = FALSE;
		bret = CopyFileEx( srcpath, dstpath, NULL, NULL, &bcancel, 0 );
		if( bret == 0 ){
			_ASSERT( 0 );
			return 1;
		}
	}
	map<int, CMQOMaterial*>::iterator itrmat;
	for( itrmat = curmodel->m_material.begin(); itrmat != curmodel->m_material.end(); itrmat++ ){
		CMQOMaterial* curmqomat = itrmat->second;
		if( curmqomat && curmqomat->tex[0] && (curmqomat->m_texid >= 0) ){
			WCHAR wtex[256] = {0L};
			MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, curmqomat->tex, 256, wtex, 256 );
			swprintf_s( srcpath, MAX_PATH, L"%s\\%s", curmodel->m_dirname, wtex );
			swprintf_s( dstpath, MAX_PATH, L"%s\\%s", charafolder, wtex );

			int chksame = wcscmp( curmodel->m_dirname, charafolder );
			if( chksame != 0 ){
				bcancel = FALSE;
				bret = CopyFileEx( srcpath, dstpath, NULL, NULL, &bcancel, 0 );
				if( bret == 0 ){
					_ASSERT( 0 );
					return 1;
				}
			}
		}
	}

	WCHAR motfolder[MAX_PATH] = {0L};
	swprintf_s( motfolder, MAX_PATH, L"%s\\Motion", charafolder );
	DWORD fattrm;
	fattrm = GetFileAttributes( motfolder );
	if( (fattrm == -1) || ((fattrm & FILE_ATTRIBUTE_DIRECTORY) == 0) ){
		int bret;
		bret = CreateDirectory( motfolder, NULL );
		if( bret == 0 ){
			::MessageBox( NULL, L"ディレクトリの作成に失敗しました。\nプロジェクトの保存に失敗しました。", L"エラー", MB_OK );
			_ASSERT( 0 );
			return 1;
		}
	}

	map<string, MOTINFO*> mname2minfo;

	int motcnt = 0;
	map<int, MOTINFO*>::iterator itrmi;
	for( itrmi = curmodel->m_motinfo.begin(); itrmi != curmodel->m_motinfo.end(); itrmi++ ){
		MOTINFO* miptr = itrmi->second;
		_ASSERT( miptr );

		CallF( WriteMotion( motfolder, curmodel, miptr, motcnt, mname2minfo ), return 1 );
		motcnt++;
	}

	mname2minfo.clear();

	CallF( Write2File( "  </Chara>\r\n" ), return 1 );


//  <Chara>
//    <ModelFolder>rgn_0</ModelFolder>
//    <ModelFile>rgn.mqo</ModelFile>
//    <ModelMult>1.0</ModelMult>
//    <MotionNum>2</MotionNum>
//    <Motion>
//      <MotionFile>test1.qub</MotionFile>
//      <MotionName>test1</MotionName>
//    </Motion>
//    <Motion>
//      <MotionFile>test2.qub</MotionFile>
//      <MotionName>test2</MotionName>
//    </Motion>
//  </Chara>

  return 0;
}
int CRdbFile::WriteMotion( WCHAR* motfolder, CModel* srcmodel, MOTINFO* miptr, int motcnt, map<string, MOTINFO*>& mname2minfo )
{
//    <Motion>
//      <MotionFile>test1.qub</MotionFile>
//      <MotionName>test1</MotionName>
//    </Motion>
	CallF( Write2File( "    <Motion>\r\n" ), return 1 );

	char mmotname[256]={0};
	char mfilename[MAX_PATH] = {0};

	strcpy_s( mmotname, 256, miptr->motname );

	MOTINFO* chkmi = mname2minfo[ miptr->motname ];
	while( chkmi ){
		strcat_s( mmotname, 256, "_2nd" );
		chkmi = mname2minfo[ mmotname ];
	}
	
	sprintf_s( mfilename, MAX_PATH, "%s.qub", mmotname );
	mname2minfo[ mmotname ] = miptr;


	CallF( Write2File( "      <MotionFile>%s</MotionFile>\r\n", mfilename ), return 1 );
	CallF( Write2File( "      <MotionName>%s</MotionName>\r\n", mmotname ), return 1 );

	CallF( Write2File( "    </Motion>\r\n" ), return 1 );

	WCHAR wfilename[MAX_PATH] = {0L};
	MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, mfilename, MAX_PATH, wfilename, MAX_PATH );

	WCHAR motpath[MAX_PATH] = {0L};
	swprintf_s( motpath, MAX_PATH, L"%s\\%s", motfolder, wfilename );
	CallF( srcmodel->SaveQubFile( motpath, miptr->motid, mmotname ), return 1 );

	return 0;
}


int CRdbFile::LoadRdbFileFromPnd( CPmCipherDll* cipher, int rdbindex, 
		CModel* (*srcmqofunc)( CPmCipherDll* cipher, char* modelfolder, char* mqopath ), int (*srcqubfunc)( CPmCipherDll* cipher, char* motpath ), int (*srcdelmfunc)( int delindex ) )
{
	m_mode = XMLIO_LOADFROMPND;
	m_MqoFunc = 0;
	m_MqoPndFunc = srcmqofunc;
	m_QubFunc = 0;
	m_QubPndFunc = srcqubfunc;
	m_DelMotFunc = srcdelmfunc;
	m_cipher = cipher;

	PNDPROP prop;
	ZeroMemory( &prop, sizeof( PNDPROP ) );
	CallF( cipher->GetProperty( rdbindex, &prop ), return 1 );

	strcpy_s( m_mloaddir, MAX_PATH, prop.path );
	char* lasten;
	lasten = strrchr( m_mloaddir, '\\' );
	if( !lasten ){
		_ASSERT( 0 );
		return 1;
	}
	*lasten = 0;

	CallF( SetBuffer( cipher, rdbindex ), return 1 );

	int posstep = 0;
	XMLIOBUF fileinfobuf;
	ZeroMemory( &fileinfobuf, sizeof( XMLIOBUF ) );
	CallF( SetXmlIOBuf( &m_xmliobuf, "<FileInfo>", "</FileInfo>", &fileinfobuf ), return 1 );
	CallF( CheckFileVersion( &fileinfobuf ), return 1 );

	XMLIOBUF projinfobuf;
	ZeroMemory( &projinfobuf, sizeof( XMLIOBUF ) );
	CallF( SetXmlIOBuf( &m_xmliobuf, "<ProjectInfo>", "</ProjectInfo>", &projinfobuf ), return 1 );
	int charanum = 0;
	CallF( ReadProjectInfo( &projinfobuf, &charanum ), return 1 );

	int characnt;
	for( characnt = 0; characnt < charanum; characnt++ ){
		XMLIOBUF charabuf;
		ZeroMemory( &charabuf, sizeof( XMLIOBUF ) );
		CallF( SetXmlIOBuf( &m_xmliobuf, "<Chara>", "</Chara>", &charabuf ), return 1 );
		CallF( ReadChara( &charabuf ), return 1 );
	}

	return 0;
}


int CRdbFile::LoadRdbFile( WCHAR* strpath, CModel* (*srcmqofunc)(), int (*srcqubfunc)(), int (*srcdelmfunc)( int delindex ) )
{
	m_mode = XMLIO_LOAD;
	m_MqoFunc = srcmqofunc;
	m_QubFunc = srcqubfunc;
	m_DelMotFunc = srcdelmfunc;

	wcscpy_s( m_wloaddir, MAX_PATH, strpath );
	WCHAR* lasten;
	lasten = wcsrchr( m_wloaddir, TEXT('\\') );
	if( !lasten ){
		_ASSERT( 0 );
		return 1;
	}
	*lasten = 0L;

	m_hfile = CreateFile( strpath, GENERIC_READ, 0, NULL, OPEN_EXISTING,
		FILE_FLAG_SEQUENTIAL_SCAN, NULL );
	if( m_hfile == INVALID_HANDLE_VALUE ){
		_ASSERT( 0 );
		return 1;
	}	

	CallF( SetBuffer(), return 1 );

	int posstep = 0;
	XMLIOBUF fileinfobuf;
	ZeroMemory( &fileinfobuf, sizeof( XMLIOBUF ) );
	CallF( SetXmlIOBuf( &m_xmliobuf, "<FileInfo>", "</FileInfo>", &fileinfobuf ), return 1 );
	CallF( CheckFileVersion( &fileinfobuf ), return 1 );

	XMLIOBUF projinfobuf;
	ZeroMemory( &projinfobuf, sizeof( XMLIOBUF ) );
	CallF( SetXmlIOBuf( &m_xmliobuf, "<ProjectInfo>", "</ProjectInfo>", &projinfobuf ), return 1 );
	int charanum = 0;
	CallF( ReadProjectInfo( &projinfobuf, &charanum ), return 1 );

	int characnt;
	for( characnt = 0; characnt < charanum; characnt++ ){
		XMLIOBUF charabuf;
		ZeroMemory( &charabuf, sizeof( XMLIOBUF ) );
		CallF( SetXmlIOBuf( &m_xmliobuf, "<Chara>", "</Chara>", &charabuf ), return 1 );
		CallF( ReadChara( &charabuf ), return 1 );
	}

	return 0;
}


int CRdbFile::CheckFileVersion( XMLIOBUF* xmlbuf )
{
	char kind[256];
	char version[256];
	char type[256];
	ZeroMemory( kind, sizeof( char ) * 256 );
	ZeroMemory( version, sizeof( char ) * 256 );
	ZeroMemory( type, sizeof( char ) * 256 );

	CallF( Read_Str( xmlbuf, "<kind>", "</kind>", kind, 256 ), return 1 );
	CallF( Read_Str( xmlbuf, "<version>", "</version>", version, 256 ), return 1 );
	CallF( Read_Str( xmlbuf, "<type>", "</type>", type, 256 ), return 1 );

	int cmpkind, cmpversion, cmptype;
	cmpkind = strcmp( kind, "OpenRDBProjectFile" );
	cmpversion = strcmp( version, "1001" );
	cmptype = strcmp( type, "0" );

	if( (cmpkind == 0) && (cmpversion == 0) && (cmptype == 0) ){
		return 0;
	}else{
		_ASSERT( 0 );
		return 1;
	}

	return 0;
}
int CRdbFile::ReadProjectInfo( XMLIOBUF* xmlbuf, int* charanumptr )
{
	CallF( Read_Int( xmlbuf, "<CharaNum>", "</CharaNum>", charanumptr ), return 1 );

	return 0;
}
int CRdbFile::ReadChara( XMLIOBUF* xmlbuf )
{
//    <ModelFolder>rgn_0</ModelFolder>
//    <ModelFile>rgn.mqo</ModelFile>
//    <ModelMult>1.0</ModelMult>
//    <MotionNum>2</MotionNum>

	char modelfolder[MAX_PATH] = {0};
	char filename[MAX_PATH] = {0};
	float modelmult = 1.0f;
	int motionnum = 0;

	CallF( Read_Str( xmlbuf, "<ModelFolder>", "</ModelFolder>", modelfolder, MAX_PATH ), return 1 );
	CallF( Read_Str( xmlbuf, "<ModelFile>", "</ModelFile>", filename, MAX_PATH ), return 1 );
	CallF( Read_Float( xmlbuf, "<ModelMult>", "</ModelMult>", &modelmult ), return 1 );
	CallF( Read_Int( xmlbuf, "<MotionNum>", "</MotionNum>", &motionnum ), return 1 );

	char mmqopath[MAX_PATH] = {0};
	WCHAR wmodelfolder[MAX_PATH] = {0L};
	WCHAR wfilename[MAX_PATH] = {0L};

	MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, modelfolder, MAX_PATH, wmodelfolder, MAX_PATH );
	MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, filename, MAX_PATH, wfilename, MAX_PATH );
	swprintf_s( g_tmpmqopath, MULTIPATH, L"%s\\%s\\%s", m_wloaddir, wmodelfolder, wfilename );
	if( m_mode == XMLIO_LOADFROMPND ){
		sprintf_s( mmqopath, MAX_PATH, "%s\\%s\\%s", m_mloaddir, modelfolder, filename );
	}
	g_tmpmqomult = modelmult;


	CModel* newmodel = 0;

	if( m_mode == XMLIO_LOADFROMPND ){
		newmodel = (*m_MqoPndFunc)( m_cipher, modelfolder, mmqopath );
	}else{
		newmodel = (*m_MqoFunc)();
	}

	_ASSERT( newmodel );
	if( motionnum > 0 ){
		int motioncnt;

		for( motioncnt = 0; motioncnt < motionnum; motioncnt++ ){
			XMLIOBUF motionbuf;
			ZeroMemory( &motionbuf, sizeof( XMLIOBUF ) );
			CallF( SetXmlIOBuf( xmlbuf, "<Motion>", "</Motion>", &motionbuf ), return 1 );
			CallF( ReadMotion( &motionbuf, wmodelfolder, newmodel ), return 1 );
		}
		if( newmodel ){
			//defaultの空モーションを削除
			CallF( (*m_DelMotFunc)( 0 ), return 1 );
		}
	}

	return 0;
}
int CRdbFile::ReadMotion( XMLIOBUF* xmlbuf, WCHAR* modelfolder, CModel* modelptr )
{
	char motname[256] = {0};
	char motfile[MAX_PATH] = {0};

	CallF( Read_Str( xmlbuf, "<MotionFile>", "</MotionFile>", motfile, MAX_PATH ), return 1 );
	CallF( Read_Str( xmlbuf, "<MotionName>", "</MotionName>", motname, 256 ), return 1 );

	WCHAR wmotfile[MAX_PATH] = {0L};
	MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, motfile, MAX_PATH, wmotfile, MAX_PATH );

	if( modelptr ){
		if( m_mode == XMLIO_LOADFROMPND ){
			char mmodelfolder[MAX_PATH] = {0};
			WideCharToMultiByte( CP_ACP, 0, modelfolder, -1, mmodelfolder, MAX_PATH, NULL, NULL );

			char motpath[MAX_PATH];
			sprintf_s( motpath, MAX_PATH, "%s\\%s\\Motion\\%s", m_mloaddir, mmodelfolder, motfile );

			CallF( (*m_QubPndFunc)( m_cipher, motpath ), return 1 );
		}else{

			swprintf_s( g_tmpmqopath, MULTIPATH, L"%s\\%s\\Motion\\%s", m_wloaddir, modelfolder, wmotfile );
			CallF( (*m_QubFunc)(), return 1 );
		}
	}
	return 0;
}


#include <stdlib.h>
#include <math.h>
#include <stdio.h>

#include <string.h>
#include <ctype.h>
#include <malloc.h>
#include <memory.h>

#include <windows.h>

#include <crtdbg.h>


//#include <coef.h>
#include "KbsFile.h"

#include <Model.h>
#include <Bone.h>

#define DBGH
#include <dbg.h>

using namespace std;

/***
static int s_deftwist[SKEL_MAX] = {
	1, 1,

	1, 0, 1,
	1, 0, 1,

	1, 1,

	1, 0, 0,
	1, 0, 0
};
***/
static int s_deftwist[SKEL_MAX] = {
	1, 1,

	1, 1, 1,
	1, 1, 1,

	1, 1,

	1, 1, 1,
	1, 1, 1
};

CKbsFile::CKbsFile()
{
	InitParams();
}
CKbsFile::~CKbsFile()
{
	DestroyObjs();
}

int CKbsFile::InitParams()
{
	CXMLIO::InitParams();

	int skelno;
	for( skelno = 0; skelno < SKEL_MAX; skelno++ ){
		TSELEM* curelem;
		curelem = m_elem + skelno;
		curelem->skelno = skelno;
		ZeroMemory( curelem->jointname, sizeof( char ) * 256 );
		curelem->jointno = -1;
		curelem->twistflag = s_deftwist[skelno];
	}

	m_model = 0;

	return 0;
}
int CKbsFile::DestroyObjs()
{
	CXMLIO::DestroyObjs();

	return 0;
}

int CKbsFile::SaveKbsFile( WCHAR* filename )
{

	CXMLIO::DestroyObjs();
	CXMLIO::InitParams();
	m_mode = XMLIO_WRITE;

	m_hfile = CreateFile( filename, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS,
		FILE_FLAG_SEQUENTIAL_SCAN, NULL );
	if( m_hfile == INVALID_HANDLE_VALUE ){
		DbgOut( L"KbsFile : WriteKbsFile : file open error !!!\n" );
		_ASSERT( 0 );
		return 1;
	}


	CallF( Write2File( "<?xml version=\"1.0\" encoding=\"Shift_JIS\"?>\r\n<KBS>\r\n" ), return 1 );  
	CallF( WriteFileInfo(), return 1 );

	int skelno;
	for( skelno = 0; skelno < SKEL_MAX; skelno++ ){
		CallF( WriteJointInfo( skelno ), return 1 );
	}

	CallF( Write2File( "</KBS>\r\n" ), return 1 );

	return 0;
}

int CKbsFile::WriteFileInfo()
{

	CallF( Write2File( "  <FileInfo>\r\n    <kind>KinectBoneSettingFile</kind>\r\n    <version>1001</version>\r\n    <type>0</type>\r\n  </FileInfo>\r\n" ), return 1 );

	return 0;
}

int CKbsFile::WriteJointInfo( int skelno )
{
	if( (skelno < 0) || (skelno >= SKEL_MAX) ){
		_ASSERT( 0 );
		return 1;
	}

	TSELEM* curtse = m_elem + skelno;

	CallF( Write2File( "  <%s>\r\n    <RDBBone>%s</RDBBone>\r\n    <twist>%d</twist>\r\n  </%s>\r\n", strconvskel[skelno], curtse->jointname, curtse->twistflag, strconvskel[skelno] ), return 1 );

	return 0;
}

int CKbsFile::LoadKbsFile( CModel* srcmodel, WCHAR* filename )
{
	DestroyObjs();
	InitParams();
	m_model = srcmodel;
	m_mode = XMLIO_LOAD;

	if( !m_model->m_topbone ){
		_ASSERT( 0 );
		return 1;
	}


	m_hfile = CreateFile( filename, GENERIC_READ, 0, NULL, OPEN_EXISTING,
		FILE_FLAG_SEQUENTIAL_SCAN, NULL );
	if( m_hfile == INVALID_HANDLE_VALUE ){
		_ASSERT( 0 );
		return 1;
	}	

	CallF( SetBuffer(), return 1 );

	XMLIOBUF fileinfobuf;
	ZeroMemory( &fileinfobuf, sizeof( XMLIOBUF ) );
	CallF( SetXmlIOBuf( &m_xmliobuf, "<FileInfo>", "</FileInfo>", &fileinfobuf ), return 1 );
	CallF( CheckFileVersion( &fileinfobuf ), return 1 );

	int skelno;
	for( skelno = 0; skelno < SKEL_MAX; skelno++ ){
		XMLIOBUF skelbuf;
		ZeroMemory( &skelbuf, sizeof( XMLIOBUF ) );
		char startpat[256] = {0};
		char endpat[256] = {0};
		sprintf_s( startpat, 256, "<%s>", strconvskel[skelno] );
		sprintf_s( endpat, 256, "</%s>", strconvskel[skelno] );

		m_xmliobuf.pos = 0;//!!!!!! skelの記述順序を自由にするため先頭から探す。
		CallF( SetXmlIOBuf( &m_xmliobuf, startpat, endpat, &skelbuf ), return 1 );
		CallF( ReadJointInfo( &skelbuf, skelno ), return 1 );
	}

	return 0;
}

int CKbsFile::CheckFileVersion( XMLIOBUF* xmlbuf )
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
	cmpkind = strcmp( kind, "KinectBoneSettingFile" );
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

int CKbsFile::ReadJointInfo( XMLIOBUF* xmliobuf, int skelno )
{
	char bonename[256] = {0};
	int twistflag = 0;

	CallF( Read_Str( xmliobuf, "<RDBBone>", "</RDBBone>", bonename, 256 ), return 1 );
	CallF( Read_Int( xmliobuf, "<twist>", "</twist>", &twistflag ), return 1 );

	TSELEM* curtse = m_elem + skelno;
	
	curtse->twistflag = twistflag;

	CBone* findbone = 0;
	findbone = m_model->m_bonename[ bonename ];
	if( findbone ){
		strcpy_s( curtse->jointname, 256, bonename );
		curtse->jointno = findbone->m_boneno;
	}else{
		strcpy_s( curtse->jointname, 256, m_model->m_topbone->m_bonename );
		curtse->jointno = m_model->m_topbone->m_boneno;
	}

	return 0;
}


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

#include <QubFile.h>

#include <Model.h>
#include <mqoobject.h>
#include <Bone.h>
#include <MotionPoint.h>
#include <quaternion.h>
#include <MorphKey.h>


#define DBGH
#include <dbg.h>

#include <string>

#include <PmCipherDll.h>

using namespace std;

CQubFile::CQubFile()
{
	InitParams();
}

CQubFile::~CQubFile()
{
	DestroyObjs();
}

int CQubFile::InitParams()
{
	CXMLIO::InitParams();

	m_topbone = 0;
	m_bonenum = 0;
	m_motid = -1;
	m_motinfo = 0;

	m_fileversion = 0;
	m_basenum = 0;
	return 0;
}

int CQubFile::DestroyObjs()
{
	CXMLIO::DestroyObjs();

	InitParams();

	return 0;
}


int CQubFile::WriteQubFile( WCHAR* strpath, MOTINFO* srcmotinfo, CBone* srctopbone, int srcbonenum, map<int,CMQOObject*>& srcbaseobj, char* mmotname )
{
	m_topbone = srctopbone;
	m_motinfo = srcmotinfo;
	if( srcmotinfo ){
		m_motid = srcmotinfo->motid;
	}else{
		m_motid = -1;
		_ASSERT( 0 );
		return 1;
	}
	m_bonenum = srcbonenum;
	m_basenum = srcbaseobj.size();
	m_mode = XMLIO_WRITE;


	m_hfile = CreateFile( strpath, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS,
		FILE_FLAG_SEQUENTIAL_SCAN, NULL );
	if( m_hfile == INVALID_HANDLE_VALUE ){
		DbgOut( L"QubFile : WriteQubFile : file open error !!!\n" );
		_ASSERT( 0 );
		return 1;
	}


	CallF( Write2File( "<?xml version=\"1.0\" encoding=\"Shift_JIS\"?>\r\n<QUB>\r\n" ), return 1 );  


	CallF( WriteFileInfo( mmotname ), return 1 );

	WriteBoneReq( m_topbone, 0 );
	
	map<int,CMQOObject*>::iterator itrbase;
	for( itrbase = srcbaseobj.begin(); itrbase != srcbaseobj.end(); itrbase++ ){
		CMQOObject* baseobj = itrbase->second;
		if( baseobj ){
			WriteMorphBase( baseobj );
		}
	}

	CallF( Write2File( "</QUB>\r\n" ), return 1 );

	return 0;
}

int CQubFile::WriteFileInfo( char* mmotname )
{

//	CallF( Write2File( "  <FileInfo>\r\n    <kind>OpenRDBMotionFile</kind>\r\n    <version>1001</version>\r\n    <type>0</type>\r\n  </FileInfo>\r\n" ), return 1 );
	CallF( Write2File( "  <FileInfo>\r\n    <kind>OpenRDBMotionFile</kind>\r\n    <version>1002</version>\r\n    <type>0</type>\r\n  </FileInfo>\r\n" ), return 1 );

	CallF( Write2File( "  <MOTIONINFO>\r\n" ), return 1 );
	CallF( Write2File( "    <Name>%s</Name>\r\n", mmotname ), return 1 );
	CallF( Write2File( "    <FrameLeng>%f</FrameLeng>\r\n", m_motinfo->frameleng ), return 1 );
	CallF( Write2File( "    <Loop>%d</Loop>\r\n", m_motinfo->loopflag ), return 1 );
	CallF( Write2File( "    <BoneNum>%d</BoneNum>\r\n", m_bonenum ), return 1 );
	CallF( Write2File( "    <MorphBaseNum>%d</MorphBaseNum>\r\n", m_basenum ), return 1 );

	CallF( Write2File( "  </MOTIONINFO>\r\n" ), return 1 );

	return 0;
}

void CQubFile::WriteBoneReq( CBone* srcbone, int broflag )
{
	CallF( Write2File( "  <Bone>\r\n" ), return );

	int mpnum = 0;
	int mpcnt = 0;
	CMotionPoint* curmp;	
	curmp = srcbone->m_motionkey[ m_motid ];
	while( curmp ){
		mpcnt++;
		curmp = curmp->m_next;
	}
	mpnum = mpcnt;

	CallF( Write2File( "    <Name>%s</Name>\r\n", srcbone->m_bonename ), return );
	CallF( Write2File( "    <MPNum>%d</MPNum>\r\n", mpnum ), return );


	mpcnt = 0;
	curmp;	
	curmp = srcbone->m_motionkey[ m_motid ];
	while( curmp ){
		mpcnt++;

		CallF( Write2File( "      <MotionPoint>\r\n" ), return );
		CallF( Write2File( "        <Frame>%.7f</Frame>\r\n", curmp->m_frame ), return );
		CallF( Write2File( "        <Q>%f, %f, %f, %f</Q>\r\n", curmp->m_q.x, curmp->m_q.y, curmp->m_q.z, curmp->m_q.w ), return );
		CallF( Write2File( "        <Eul>%f, %f, %f</Eul>\r\n", curmp->m_eul.x, curmp->m_eul.y, curmp->m_eul.z ), return );
		CallF( Write2File( "        <Tra>%f, %f, %f</Tra>\r\n", curmp->m_tra.x, curmp->m_tra.y, curmp->m_tra.z ), return );
		CallF( Write2File( "      </MotionPoint>\r\n" ), return );

		curmp = curmp->m_next;
	}

	if( mpcnt != mpnum ){
		_ASSERT( 0 );
		return;
	}

	CallF( Write2File( "  </Bone>\r\n" ), return );

	if( srcbone->m_child ){
		WriteBoneReq( srcbone->m_child, 1 );
	}

	if( broflag && srcbone->m_brother ){
		WriteBoneReq( srcbone->m_brother, 1 );
	}
}

int CQubFile::WriteMorphBase( CMQOObject* baseobj )
{
	CallF( Write2File( "  <MorphBase>\r\n" ), return 1 );

	CallF( Write2File( "    <BaseName>%s</BaseName>\r\n", baseobj->m_name ), return 1 );

	int keynum = 0;
	CMorphKey* curmk = baseobj->m_morphkey[ m_motid ];
	while( curmk ){
		keynum++;
		curmk = curmk->m_next;
	}
	CallF( Write2File( "    <MKNum>%d</MKNum>\r\n", keynum ), return 1 );

	int writeno = 0;
	if( keynum > 0 ){
		curmk = baseobj->m_morphkey[ m_motid ];
		while( curmk ){
			CallF( Write2File( "    <MorphKey>\r\n" ), return 1 );
			CallF( Write2File( "      <Frame>%f</Frame>\r\n", curmk->m_frame ), return 1 );
			int targetnum = curmk->m_blendweight.size();
			CallF( Write2File( "      <TargetNum>%d</TargetNum>\r\n", targetnum ), return 1 );

			if( targetnum > 0 ){
				map<CMQOObject*, float>::iterator itrbw;
				for( itrbw = curmk->m_blendweight.begin(); itrbw != curmk->m_blendweight.end(); itrbw++ ){
					
					CallF( Write2File( "      <BlendWeight>\r\n" ), return 1 );

					CallF( Write2File( "        <TargetName>%s</TargetName>\r\n", itrbw->first->m_name ), return 1 );
					CallF( Write2File( "        <Rate>%f</Rate>\r\n", itrbw->second ), return 1 );

					CallF( Write2File( "      </BlendWeight>\r\n" ), return 1 );
				}
			}

			CallF( Write2File( "    </MorphKey>\r\n" ), return 1 );

			writeno++;
			curmk = curmk->m_next;
		}

	}
	_ASSERT( writeno == keynum );

	CallF( Write2File( "  </MorphBase>\r\n" ), return 1 );

	return 0;
}

int CQubFile::LoadQubFileFromPnd( CPmCipherDll* cipher, int qubindex, CModel* srcmodel, int* newid )
{
	m_model = srcmodel;
	m_motid = -1;
	m_bonenum = 0;
	m_mode = XMLIO_LOADFROMPND;

	CallF( SetBuffer( cipher, qubindex ), return 1 );

	WCHAR wfilename[MAX_PATH] = {0};
	PNDPROP prop;
	ZeroMemory( &prop, sizeof( PNDPROP ) );
	CallF( cipher->GetProperty( qubindex, &prop ), return 1 );
	MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, prop.filename, -1, wfilename, MAX_PATH );


	int posstep = 0;
	XMLIOBUF fileinfobuf;
	ZeroMemory( &fileinfobuf, sizeof( XMLIOBUF ) );
	CallF( SetXmlIOBuf( &m_xmliobuf, "<FileInfo>", "</FileInfo>", &fileinfobuf ), return 1 );
	m_fileversion = CheckFileVersion( &fileinfobuf );
	if( m_fileversion <= 0 ){
		_ASSERT( 0 );
		return 1;
	}

	XMLIOBUF motinfobuf;
	ZeroMemory( &motinfobuf, sizeof( XMLIOBUF ) );
	CallF( SetXmlIOBuf( &m_xmliobuf, "<MOTIONINFO>", "</MOTIONINFO>", &motinfobuf ), return 1 );
	CallF( ReadMotionInfo( wfilename, &motinfobuf ), return 1 );

	int bonecnt;
	for( bonecnt = 0; bonecnt < m_bonenum; bonecnt++ ){
		XMLIOBUF bonebuf;
		ZeroMemory( &bonebuf, sizeof( XMLIOBUF ) );
		CallF( SetXmlIOBuf( &m_xmliobuf, "<Bone>", "</Bone>", &bonebuf ), return 1 );
		CallF( ReadBone( &bonebuf ), return 1 );
	}

	if( m_fileversion >= 2 ){
		int basecnt;
		for( basecnt = 0; basecnt < m_basenum; basecnt++ ){
			XMLIOBUF basebuf;
			ZeroMemory( &basebuf, sizeof( XMLIOBUF ) );
			CallF( SetXmlIOBuf( &m_xmliobuf, "<MorphBase>", "</MorphBase>", &basebuf ), return 1 );
			CallF( ReadMorphBase( &basebuf ), return 1 );
		}
	}

	*newid = m_motid;

	return 0;
}


int CQubFile::LoadQubFile( WCHAR* strpath, CModel* srcmodel, int* newid )
{
	m_model = srcmodel;
	m_motid = -1;
	m_bonenum = 0;
	m_mode = XMLIO_LOAD;

	WCHAR wfilename[MAX_PATH] = {0L};
	WCHAR* lasten;
	lasten = wcsrchr( strpath, TEXT('\\') );
	if( !lasten ){
		_ASSERT( 0 );
		return 1;
	}
	wcscpy_s( wfilename, MAX_PATH, lasten + 1 );

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
	m_fileversion = CheckFileVersion( &fileinfobuf );
	if( m_fileversion <= 0 ){
		_ASSERT( 0 );
		return 1;
	}

	XMLIOBUF motinfobuf;
	ZeroMemory( &motinfobuf, sizeof( XMLIOBUF ) );
	CallF( SetXmlIOBuf( &m_xmliobuf, "<MOTIONINFO>", "</MOTIONINFO>", &motinfobuf ), return 1 );
	CallF( ReadMotionInfo( wfilename, &motinfobuf ), return 1 );

	int bonecnt;
	for( bonecnt = 0; bonecnt < m_bonenum; bonecnt++ ){
		XMLIOBUF bonebuf;
		ZeroMemory( &bonebuf, sizeof( XMLIOBUF ) );
		CallF( SetXmlIOBuf( &m_xmliobuf, "<Bone>", "</Bone>", &bonebuf ), return 1 );
		CallF( ReadBone( &bonebuf ), return 1 );
	}

	if( m_fileversion >= 2 ){
		int basecnt;
		for( basecnt = 0; basecnt < m_basenum; basecnt++ ){
			XMLIOBUF basebuf;
			ZeroMemory( &basebuf, sizeof( XMLIOBUF ) );
			CallF( SetXmlIOBuf( &m_xmliobuf, "<MorphBase>", "</MorphBase>", &basebuf ), return 1 );
			CallF( ReadMorphBase( &basebuf ), return 1 );
		}
	}
	*newid = m_motid;

	return 0;
}


int CQubFile::CheckFileVersion( XMLIOBUF* xmlbuf )
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

	int cmpkind, cmpversion1, cmpversion2, cmptype;
	cmpkind = strcmp( kind, "OpenRDBMotionFile" );
	cmpversion1 = strcmp( version, "1001" );
	cmpversion2 = strcmp( version, "1002" );
	cmptype = strcmp( type, "0" );

	if( (cmpkind == 0) && (cmptype == 0) ){
		if( cmpversion1 == 0 ){
			return 1;
		}else if( cmpversion2 == 0 ){
			return 2;
		}else{
			return 0;
		}

		return 0;
	}else{
		_ASSERT( 0 );
		return 0;
	}

	return 0;
}
int CQubFile::ReadMotionInfo( WCHAR* wfilename, XMLIOBUF* xmlbuf )
{
	char motionname[256] = {0};
	float fmotionleng = 0.0f;
	int loopflag = 0;

	CallF( Read_Str( xmlbuf, "<Name>", "</Name>", motionname, 256 ), return 1 );
	CallF( Read_Float( xmlbuf, "<FrameLeng>", "</FrameLeng>", &fmotionleng ), return 1 );
	CallF( Read_Int( xmlbuf, "<Loop>", "</Loop>", &loopflag ), return 1 );
	CallF( Read_Int( xmlbuf, "<BoneNum>", "</BoneNum>", &m_bonenum ), return 1 );
	if( m_fileversion >= 2 ){
		CallF( Read_Int( xmlbuf, "<MorphBaseNum>", "</MorphBaseNum>", &m_basenum ), return 1 );
	}

	CallF( m_model->AddMotion( motionname, wfilename, (double)fmotionleng, &m_motid ), return 1 );
	m_motinfo = m_model->m_motinfo[ m_motid ];
	if( !m_motinfo ){
		_ASSERT( 0 );
		return 1;
	}
	m_motinfo->loopflag = loopflag;

	return 0;
}
int CQubFile::ReadBone( XMLIOBUF* xmlbuf )
{
//    <Name>bonename1</Name>
//    <MPNum>2</MPNum>

	char bonename[256] = {0};
	int mpnum = 0;

	CallF( Read_Str( xmlbuf, "<Name>", "</Name>", bonename, 256 ), return 1 );
	CallF( Read_Int( xmlbuf, "<MPNum>", "</MPNum>", &mpnum ), return 1 );

	CBone* curbone = m_model->m_bonename[ bonename ];

	if( mpnum > 0 ){
		int mpcnt;

		for( mpcnt = 0; mpcnt < mpnum; mpcnt++ ){
			XMLIOBUF mpbuf;
			ZeroMemory( &mpbuf, sizeof( XMLIOBUF ) );
			CallF( SetXmlIOBuf( xmlbuf, "<MotionPoint>", "</MotionPoint>", &mpbuf ), return 1 );
			if( curbone ){
				CallF( ReadMotionPoint( &mpbuf, curbone ), return 1 );
			}
		}
	}

	return 0;
}
int CQubFile::ReadMotionPoint( XMLIOBUF* xmlbuf, CBone* srcbone )
{
	float fframe = 0.0f;
	CQuaternion q;
	D3DXVECTOR3 eul, tra;

	CallF( Read_Float( xmlbuf, "<Frame>", "</Frame>", &fframe ), return 1 );
	CallF( Read_Q( xmlbuf, "<Q>", "</Q>", &q ), return 1 );
	CallF( Read_Vec3( xmlbuf, "<Eul>", "</Eul>", &eul ), return 1 );
	CallF( Read_Vec3( xmlbuf, "<Tra>", "</Tra>", &tra ), return 1 );

	CMotionPoint* newmp = 0;
	int existflag = 0;
	newmp = srcbone->AddMotionPoint( m_motid, (double)fframe, 0, &existflag );
	if( !newmp ){
		_ASSERT( 0 );
		return 1;
	}

//DbgOut( L"readmp : bone %s, frame %f, q(%f, %f, %f, %f)\r\n",
//	srcbone->m_wbonename, fframe, q.x, q.y, q.z, q.w );

	newmp->m_q = q;
	newmp->m_eul = eul;
	newmp->m_tra = tra;

/***
    <MotionPoint>
      <Frame>0.0</Frame>
      <Q>0.0, 0.0, 0.0, 1.0</Q>
      <Eul>0.0, 0.0, 0.0</Eul>
      <Tra>0.0, 0.0, 0.0</Tra>
    </MotionPoint>
***/
	return 0;
}

int CQubFile::ReadMorphBase( XMLIOBUF* xmliobuf )
{
	char basename[256] = {0};
	int mknum = 0;
	CallF( Read_Str( xmliobuf, "<BaseName>", "</BaseName>", basename, 256 ), return 1 );
	CallF( Read_Int( xmliobuf, "<MKNum>", "</MKNum>", &mknum ), return 1 );


	CMQOObject* setbase = 0;
	map<int, CMQOObject*>::iterator itrbase;
	for( itrbase = m_model->m_mbaseobject.begin(); itrbase != m_model->m_mbaseobject.end(); itrbase++ ){
		CMQOObject* curbase = itrbase->second;
		if( curbase ){
			int cmp;
			cmp = strcmp( basename, curbase->m_name );
			if( cmp == 0 ){
				setbase = curbase;
				break;
			}
		}
	}

	if( mknum > 0 ){
		int mkno;
		for( mkno = 0; mkno < mknum; mkno++ ){
			XMLIOBUF mkbuf;
			ZeroMemory( &mkbuf, sizeof( XMLIOBUF ) );
			CallF( SetXmlIOBuf( xmliobuf, "<MorphKey>", "</MorphKey>", &mkbuf ), return 1 );
			CallF( ReadMorphKey( &mkbuf, setbase ), return 1 );
		}
	}

	return 0;
}
int CQubFile::ReadMorphKey( XMLIOBUF* xmliobuf, CMQOObject* srcbase )
{
	float frame = 0.0f;
	int targetnum = 0;
	CallF( Read_Float( xmliobuf, "<Frame>", "</Frame>", &frame ), return 1 );
	CallF( Read_Int( xmliobuf, "<TargetNum>", "</TargetNum>", &targetnum ), return 1 );

	if( targetnum > 0 ){
		CMorphKey* newmk = 0;
		if( srcbase ){
			int existflag = 0;
			newmk = srcbase->AddMorphKey( m_motid, frame, &existflag );
			if( !newmk ){
				_ASSERT( 0 );
				return 1;
			}
		}

		int bwcnt;
		for( bwcnt = 0; bwcnt < targetnum; bwcnt++ ){
			XMLIOBUF bwbuf;
			ZeroMemory( &bwbuf, sizeof( XMLIOBUF ) );
			CallF( SetXmlIOBuf( xmliobuf, "<BlendWeight>", "</BlendWeight>", &bwbuf ), return 1 );
			CallF( ReadBlendWeight( &bwbuf, srcbase, newmk ), return 1 );
		}
	}

	return 0;
}
int CQubFile::ReadBlendWeight( XMLIOBUF* xmliobuf, CMQOObject* setbase, CMorphKey* setmk )
{
	char targetname[256] = {0};
	float rate = 0.0f;

	CallF( Read_Str( xmliobuf, "<TargetName>", "</TargetName>", targetname, 256 ), return 1 );
	CallF( Read_Float( xmliobuf, "<Rate>", "</Rate>", &rate ), return 1 );

	if( setbase && setmk ){
		CMQOObject* settarget = 0;
		map<int, CMQOObject*>::iterator itrtarget;
		for( itrtarget = setbase->m_morphobj.begin(); itrtarget != setbase->m_morphobj.end(); itrtarget++ ){
			CMQOObject* curtarget = itrtarget->second;
			if( curtarget ){
				int cmp;
				cmp = strcmp( targetname, curtarget->m_name );
				if( cmp == 0 ){
					settarget = curtarget;
					break;
				}
			}
		}

		if( settarget ){
			CallF( setmk->SetBlendWeight( settarget, rate ), return 1 );
		}
	}

	return 0;
}

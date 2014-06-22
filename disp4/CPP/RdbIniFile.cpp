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

#include <RdbIniFile.h>

#define DBGH
#include <dbg.h>


extern WCHAR g_mediadir[ MAX_PATH ];


static char s_wpstart[ WINPOS_MAX ][ 30 ] = {
	"<MainWindow>",
	"<MainTimeLine>",
	"<MorphTimeLine>",
	"<LongTimeLine>",
	"<ToolWindow>",
	"<ObjWindow>",
	"<ModelWindow>"
};
static char s_wpend[ WINPOS_MAX ][ 30 ] = {
	"</MainWindow>",
	"</MainTimeLine>",
	"</MorphTimeLine>",
	"</LongTimeLine>",
	"</ToolWindow>",
	"</ObjWindow>",
	"</ModelWindow>"
};

CRdbIniFile::CRdbIniFile()
{
	InitParams();
}

CRdbIniFile::~CRdbIniFile()
{
	DestroyObjs();
}

int CRdbIniFile::InitParams()
{
	CXMLIO::InitParams();

	ZeroMemory( &m_winpos, sizeof( WINPOS ) * WINPOS_MAX );

	m_defaultwp[WINPOS_3D].posx = 410;
	m_defaultwp[WINPOS_3D].posy = 0;
	m_defaultwp[WINPOS_3D].width = 620;
	m_defaultwp[WINPOS_3D].height = 620;
	m_defaultwp[WINPOS_3D].minwidth = 100;
	m_defaultwp[WINPOS_3D].minheight = 100;
	m_defaultwp[WINPOS_3D].dispflag = 1;

	m_defaultwp[WINPOS_TL].posx = 0;
	m_defaultwp[WINPOS_TL].posy = 0;
	m_defaultwp[WINPOS_TL].width = 410;
	m_defaultwp[WINPOS_TL].height = 527;
	m_defaultwp[WINPOS_TL].minwidth = 100;
	m_defaultwp[WINPOS_TL].minheight = 100;
	m_defaultwp[WINPOS_TL].dispflag = 1;

	m_defaultwp[WINPOS_MTL].posx = 0;
	m_defaultwp[WINPOS_MTL].posy = 527;
	m_defaultwp[WINPOS_MTL].width = 410;
	m_defaultwp[WINPOS_MTL].height = 125;
	m_defaultwp[WINPOS_MTL].minwidth = 100;
	m_defaultwp[WINPOS_MTL].minheight = 100;
	m_defaultwp[WINPOS_MTL].dispflag = 1;

	m_defaultwp[WINPOS_LTL].posx = 0;
	m_defaultwp[WINPOS_LTL].posy = 652;
	m_defaultwp[WINPOS_LTL].width = 1030;
	m_defaultwp[WINPOS_LTL].height = 90;
	m_defaultwp[WINPOS_LTL].minwidth = 100;
	m_defaultwp[WINPOS_LTL].minheight = 90;
	m_defaultwp[WINPOS_LTL].dispflag = 1;

	m_defaultwp[WINPOS_TOOL].posx = 410;
	m_defaultwp[WINPOS_TOOL].posy = 70;//95
	m_defaultwp[WINPOS_TOOL].width = 150;
	m_defaultwp[WINPOS_TOOL].height = 10;
	m_defaultwp[WINPOS_TOOL].minwidth = 100;
	m_defaultwp[WINPOS_TOOL].minheight = 10;
	m_defaultwp[WINPOS_TOOL].dispflag = 1;

	m_defaultwp[WINPOS_OBJ].posx = 410;
	m_defaultwp[WINPOS_OBJ].posy = 340;
	m_defaultwp[WINPOS_OBJ].width = 150;
	m_defaultwp[WINPOS_OBJ].height = 200;
	m_defaultwp[WINPOS_OBJ].minwidth = 100;
	m_defaultwp[WINPOS_OBJ].minheight = 100;
	m_defaultwp[WINPOS_OBJ].dispflag = 1;

	m_defaultwp[WINPOS_MODEL].posx = 410;
	m_defaultwp[WINPOS_MODEL].posy = 540;
	m_defaultwp[WINPOS_MODEL].width = 150;
	m_defaultwp[WINPOS_MODEL].height = 110;
	m_defaultwp[WINPOS_MODEL].minwidth = 100;
	m_defaultwp[WINPOS_MODEL].minheight = 100;
	m_defaultwp[WINPOS_MODEL].dispflag = 1;


	return 0;
}

int CRdbIniFile::DestroyObjs()
{
	CXMLIO::DestroyObjs();

	InitParams();

	return 0;
}


int CRdbIniFile::WriteRdbIniFile( WINPOS* srcwinpos )
{
	m_mode = XMLIO_WRITE;

	MoveMemory( m_winpos, srcwinpos, sizeof( WINPOS ) * WINPOS_MAX );

	WCHAR strpath[MAX_PATH];
	swprintf_s( strpath, MAX_PATH, L"%sOpenRDB.ini", g_mediadir );

	m_hfile = CreateFile( strpath, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS,
		FILE_FLAG_SEQUENTIAL_SCAN, NULL );
	if( m_hfile == INVALID_HANDLE_VALUE ){
		DbgOut( L"RdbIniFile : WriteRdbIniFile : file open error !!!\n" );
		_ASSERT( 0 );
		return 1;
	}

	CallF( Write2File( "<?xml version=\"1.0\" encoding=\"Shift_JIS\"?>\r\n<RDBINI>\r\n" ), return 1 );  

	int kind;
	for( kind = WINPOS_3D; kind < WINPOS_MAX; kind++ ){
		CallF( Write2File( "  %s\r\n", &(s_wpstart[kind][0]) ), return 1 );

		WINPOS* curwp = m_winpos + kind;
		WINPOS* defwp = m_defaultwp + kind;

		char wline[ 256 ];

		int tmpval;
		if( (curwp->posx >= 0) && (curwp->posx <= 5000) ){
			tmpval = curwp->posx;
		}else{
			tmpval = defwp->posx;
		}
		sprintf_s( wline, 256, "    <posx>%d</posx>\r\n", tmpval );
		CallF( Write2File( wline ), return 1 );

		if( (curwp->posy >= 0) && (curwp->posy <= 5000) ){
			tmpval = curwp->posy;
		}else{
			tmpval = defwp->posy;
		}
		sprintf_s( wline, 256, "    <posy>%d</posy>\r\n", tmpval );
		CallF( Write2File( wline ), return 1 );

		if( (curwp->width >= 100) && (curwp->width <= 2500) ){
			tmpval = curwp->width;
		}else{
			tmpval = defwp->width;
		}
		sprintf_s( wline, 256, "    <width>%d</width>\r\n", tmpval );
		CallF( Write2File( wline ), return 1 );

		if( (curwp->height >= 50) && (curwp->height <= 2500) ){
			tmpval = curwp->height;
		}else{
			tmpval = defwp->height;
		}
		sprintf_s( wline, 256, "    <height>%d</height>\r\n", tmpval );
		CallF( Write2File( wline ), return 1 );

		if( kind != WINPOS_3D ){
			if( (curwp->minwidth >= 90) && (curwp->width <= 1000) ){
				tmpval = curwp->minwidth;
			}else{
				tmpval = defwp->minwidth;
			}
			sprintf_s( wline, 256, "    <minwidth>%d</minwidth>\r\n", tmpval );
			CallF( Write2File( wline ), return 1 );

			if( (curwp->height >= 50) && (curwp->height <= 1000) ){
				tmpval = curwp->minheight;
			}else{
				tmpval = defwp->minheight;
			}
			sprintf_s( wline, 256, "    <minheight>%d</minheight>\r\n", tmpval );
			CallF( Write2File( wline ), return 1 );


			if( (curwp->dispflag == 0) || (curwp->dispflag == 1) ){
				tmpval = curwp->dispflag;
			}else{
				tmpval = 1;
			}
			sprintf_s( wline, 256, "    <dispflag>%d</dispflag>\r\n", tmpval );
			CallF( Write2File( wline ), return 1 );
		}

		CallF( Write2File( "  %s\r\n", &(s_wpend[kind][0]) ), return 1 );
	}

	CallF( Write2File( "</RDBINI>\r\n" ), return 1 );

	return 0;
}


int CRdbIniFile::LoadRdbIniFile()
{
	m_mode = XMLIO_LOAD;
	MoveMemory( m_winpos, m_defaultwp, sizeof( WINPOS ) * WINPOS_MAX );

	WCHAR strpath[MAX_PATH];
	swprintf_s( strpath, MAX_PATH, L"%sOpenRDB.ini", g_mediadir );

	m_hfile = CreateFile( strpath, GENERIC_READ, 0, NULL, OPEN_EXISTING,
		FILE_FLAG_SEQUENTIAL_SCAN, NULL );
	if( m_hfile == INVALID_HANDLE_VALUE ){
		return 0;//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	}	

	CallF( SetBuffer(), return 1 );

	int kind;
	for( kind = WINPOS_3D; kind < WINPOS_MAX; kind++ ){
		XMLIOBUF wpbuf;
		ZeroMemory( &wpbuf, sizeof( XMLIOBUF ) );
		CallF( SetXmlIOBuf( &m_xmliobuf, &(s_wpstart[kind][0]), &(s_wpend[kind][0]), &wpbuf ), return 1 );
		CallF( ReadWinPos( &wpbuf, kind, m_winpos + kind ), return 1 );
	}

	return 0;
}

int CRdbIniFile::ReadWinPos( XMLIOBUF* wpbuf, int kind, WINPOS* dstwinpos )
{
	int ret;
	int tmpval;

	ret = Read_Int( wpbuf, "<posx>", "</posx>", &tmpval );
	if( !ret && (tmpval >= 0) && (tmpval <= 5000)){
		dstwinpos->posx = tmpval;
	}else{
		dstwinpos->posx = m_defaultwp[kind].posx;
	}

	ret = Read_Int( wpbuf, "<posy>", "</posy>", &tmpval );
	if( !ret && (tmpval >= 0) && (tmpval <= 5000)){
		dstwinpos->posy = tmpval;
	}else{
		dstwinpos->posy = m_defaultwp[kind].posy;
	}

	ret = Read_Int( wpbuf, "<width>", "</width>", &tmpval );
	if( !ret && (tmpval >= 100) && (tmpval <= 2500)){
		dstwinpos->width = tmpval;
	}else{
		dstwinpos->width = m_defaultwp[kind].width;
	}

	ret = Read_Int( wpbuf, "<height>", "</height>", &tmpval );
	if( !ret && (tmpval >= 50) && (tmpval <= 2500)){
		dstwinpos->height = tmpval;
	}else{
		dstwinpos->height = m_defaultwp[kind].height;
	}

	if( kind != WINPOS_3D ){
		ret = Read_Int( wpbuf, "<minwidth>", "</minwidth>", &tmpval );
		if( !ret && (tmpval >= 90) && (tmpval <= 1000)){
			dstwinpos->minwidth = tmpval;
		}else{
			dstwinpos->minwidth = m_defaultwp[kind].minwidth;
		}

		ret = Read_Int( wpbuf, "<minheight>", "</minheight>", &tmpval );
		if( !ret && (tmpval >= 50) && (tmpval <= 1000)){
			dstwinpos->minheight = tmpval;
		}else{
			dstwinpos->minheight = m_defaultwp[kind].minheight;
		}

		ret = Read_Int( wpbuf, "<dispflag>", "</dispflag>", &tmpval );
		if( !ret && (tmpval >= 0) && (tmpval <= 1)){
			dstwinpos->dispflag = tmpval;
		}else{
			dstwinpos->dispflag = m_defaultwp[kind].dispflag;
		}
	}

	return 0;
}

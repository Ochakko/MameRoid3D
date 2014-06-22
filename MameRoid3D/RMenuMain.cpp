
#include "RMenuMain.h"

#include <mqoobject.h>
#include <MorphKey.h>

#define DBGH
#include <dbg.h>

#include <crtdbg.h>
#include "resource.h"

using namespace std;

CRMenuMain::CRMenuMain()
{
	InitParams();
}
CRMenuMain::~CRMenuMain()
{

}

int CRMenuMain::InitParams()
{
	m_rmenu = 0;
	m_rsubmenu = 0;
	return 0;
}

int CRMenuMain::Create( HWND srcappwnd, CMorphKey* srcmk, map<int,CMQOObject*>& srctargetlist )
{
	m_rmenu = LoadMenu( (HINSTANCE)GetWindowLong( srcappwnd, GWL_HINSTANCE ), MAKEINTRESOURCE( IDR_MENU2 ) );
	m_rsubmenu = GetSubMenu( m_rmenu, 0 );

	int menuno, menunum;
	menunum = GetMenuItemCount( m_rsubmenu );
	for( menuno = 0; menuno < menunum; menuno++ ){
		RemoveMenu( m_rsubmenu, 0, MF_BYPOSITION);
	}

	int setno = 0;
	map<int,CMQOObject*>::iterator itrtar;
	for( itrtar = srctargetlist.begin(); itrtar != srctargetlist.end(); itrtar++ ){
		CMQOObject* tarptr = itrtar->second;
		if( tarptr ){
			WCHAR wname[256];
			MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, tarptr->m_name, 256, wname, 256 );

			WCHAR mes[270];
			int existflag = ExistTargetInKey( tarptr, srcmk );
			if( existflag ){
				swprintf_s( mes, 270, L"%s-%.3f", wname, srcmk->m_blendweight[ tarptr ] );
			}else{
				swprintf_s( mes, 270, L"%s-none", wname );
			}
			AppendMenu( m_rsubmenu, MF_STRING, 62000 + setno, mes );

			setno++;
		}
	}

	return 0;
}

int CRMenuMain::ExistTargetInKey( CMQOObject* srctarget, CMorphKey* srckey )
{
	int flag = 0;

	map<CMQOObject*,float>::iterator itrfind;
	itrfind = srckey->m_blendweight.find( srctarget );

	if( itrfind != srckey->m_blendweight.end() ){
		flag = 1;
	}else{
		flag = 0;
	}

	return flag;
}


int CRMenuMain::Destroy()
{
	DestroyMenu( m_rmenu );
	m_rmenu = 0;
	m_rsubmenu = 0;

	return 0;
}

CMQOObject* CRMenuMain::TrackPopupMenu( HWND srchwnd, POINT pt, std::map<int,CMQOObject*>& srctargetlist )
{
	Params2Dlg();


///////
	int tarindex;
	tarindex = ::TrackPopupMenu( m_rsubmenu, TPM_LEFTALIGN | TPM_RETURNCMD, pt.x, pt.y, 0, srchwnd, NULL);

	CMQOObject* retobj = 0;
	if( (tarindex >= 62000) && (tarindex < (62000 + MAXTARGETNUM) ) ){
		int targetppnum = tarindex - 62000;

		map<int,CMQOObject*>::iterator itrobj = srctargetlist.begin();
		int ppno;
		for( ppno = 0; ppno < targetppnum; ppno++ ){
			itrobj++;
			if( itrobj == srctargetlist.end() ){
				_ASSERT( 0 );
				return 0;
			}
		}
		retobj = itrobj->second;
	}

	return retobj;
}

int CRMenuMain::Params2Dlg()
{

	return 0;
}



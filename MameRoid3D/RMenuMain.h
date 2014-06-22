#ifndef RMENU1H
#define RMENU1H

#include <Windows.h>

#include <coef.h>
#include <map>

class CMorphKey;
class CMQOObject;

class CRMenuMain
{
public:
	CRMenuMain();
	~CRMenuMain();

	int Create( HWND srcappwnd, CMorphKey* srcmk, std::map<int,CMQOObject*>& srctargetlist );
	int Destroy();

	CMQOObject* TrackPopupMenu( HWND srchwnd, POINT pt, std::map<int,CMQOObject*>& srctargetlist );

private:
	int InitParams();
	int Params2Dlg();

	int ExistTargetInKey( CMQOObject* srctarget, CMorphKey* srckey );

private:
	HMENU m_rmenu;
	HMENU m_rsubmenu;
};

#endif
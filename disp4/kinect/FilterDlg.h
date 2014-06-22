// FilterDlg.h : CFilterDlg ‚ÌéŒ¾

#ifndef __FilterDlg_H_
#define __FilterDlg_H_

#include "../../MameRoid3D/resource.h"       // ƒƒCƒ“ ƒVƒ“ƒ{ƒ‹
#include <coef.h>

/////////////////////////////////////////////////////////////////////////////
// CFilterDlg
class CFilterDlg
{
public:
	CFilterDlg();
	~CFilterDlg();

//	enum { IDD = IDD_FILTERDLG };

	int DoModal( HWND parwnd );

	int OnInitDialog(HWND hwnd);
	int OnOK(HWND hwnd);
	int OnCancel(HWND hwnd);

private:
	int SetWnd( HWND hwnd );
	int SetCombo( HWND hwnd );
	int ParamsToDlg( HWND hwnd );

public:
	int m_filtertype;
	int m_filtersize;

private:
	HWND m_filtertype_wnd;
	HWND m_filtersize_wnd;
};

#endif //__FilterDlg_H_

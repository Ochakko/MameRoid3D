// FilterDlg.cpp : CFilterDlg のインプリメンテーション
#include <FilterDlg.h>

#include <Commdlg.h>
#include "FilterType.h"

#define DBGH
#include <dbg.h>

static LRESULT CALLBACK DlgProc(HWND hDlgWnd, UINT msg, WPARAM wp, LPARAM lp);
static CFilterDlg* s_dlg = 0;

/////////////////////////////////////////////////////////////////////////////
// CFilterDlg

CFilterDlg::CFilterDlg()
{
	m_filtertype = AVGF_NONE;
	m_filtersize = avgfsize[0];
}

CFilterDlg::~CFilterDlg()
{
}

int CFilterDlg::DoModal( HWND parwnd )
{

	s_dlg = this;//!!!!!!!!

	int dlgret = 0;
	dlgret = (int)DialogBoxW( (HINSTANCE)GetModuleHandle(NULL), MAKEINTRESOURCE( IDD_FILTERDLG ), 
		parwnd, (DLGPROC)DlgProc );
	return dlgret;
}

LRESULT CALLBACK DlgProc(HWND hDlgWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg) {
        case WM_INITDIALOG:
			s_dlg->OnInitDialog( hDlgWnd );
			return FALSE;
        case WM_COMMAND:
            switch (LOWORD(wp)) {
                case IDOK:
					s_dlg->OnOK( hDlgWnd );
                    break;
                case IDCANCEL:
					s_dlg->OnCancel( hDlgWnd );
                    break;
				default:
                    return FALSE;
            }
        default:
            return FALSE;
    }
    return TRUE;
}


int CFilterDlg::OnInitDialog( HWND hwnd )
{
	SetWnd( hwnd );
	SetCombo( hwnd );
	ParamsToDlg( hwnd );

	return 1;  // システムにフォーカスを設定させます
}

int CFilterDlg::OnOK( HWND hwnd )
{
///
	int combono;
	combono = SendMessage( m_filtertype_wnd, CB_GETCURSEL, 0, 0);
	if( combono < 0 ){
		return 0;
	}
	m_filtertype = combono;

	combono = SendMessage( m_filtersize_wnd, CB_GETCURSEL, 0, 0);
	if( combono < 0 ){
		return 0;
	}
	m_filtersize = avgfsize[combono];

	EndDialog(hwnd, IDOK);

	return 0;
}

int CFilterDlg::OnCancel( HWND hwnd )
{
	EndDialog( hwnd, IDCANCEL );
	return 0;
}

int CFilterDlg::SetWnd( HWND hwnd )
{
	m_filtertype_wnd = GetDlgItem( hwnd, IDC_FILTERTYPE );
	m_filtersize_wnd = GetDlgItem( hwnd, IDC_FILTERSIZE );
	return 0;
}

int CFilterDlg::ParamsToDlg( HWND hwnd )
{	
	SendMessage( m_filtertype_wnd, CB_SETCURSEL, m_filtertype, 0);
	SendMessage( m_filtersize_wnd, CB_SETCURSEL, m_filtersize, 0);

	return 0;
}

int CFilterDlg::SetCombo( HWND hwnd )
{
	int fno;
	for( fno = 0; fno < AVGF_MAX; fno++ ){
		SendMessage( m_filtertype_wnd, CB_ADDSTRING, 0, (LPARAM)&stravgf[fno][0]);
	}
	SendMessage( m_filtertype_wnd, CB_SETCURSEL, 0, 0);

	for( int i=0; i<10; i++ ){
		SendMessage( m_filtersize_wnd, CB_ADDSTRING, 0, (LPARAM)&stravgfsize[i][0]);
	}
	SendMessage( m_filtersize_wnd, CB_SETCURSEL, 0, 0);

	return 0;
}

// ConvSetDlg.h : CConvSetDlg の宣言

#ifndef __ConvSetDlg_H_
#define __ConvSetDlg_H_

#include "../../MameRoid3D/resource.h"       // メイン シンボル
#include <coef.h>

#include <vector>

class CKbsFile;
class CModel;

/////////////////////////////////////////////////////////////////////////////
// CConvSetDlg
class CConvSetDlg
{
public:
	CConvSetDlg();
	~CConvSetDlg();

//	enum { IDD = IDD_CONVSETDLG };

	int DoModal( HWND parwnd, CModel* srcmodel, CKbsFile* kbsptr );

// ハンドラのプロトタイプ:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

	int OnInitDialog( HWND hwnd );
	int OnOK( HWND hwnd );
	int OnCancel( HWND hwnd );
	int OnRefKbs( HWND hwnd );
	int OnRefKbsIn( HWND hwnd );

private:
	int SetWnd( HWND hwnd );
	int SetCombo();
	int ParamsToDlg( HWND hwnd );

private:
	CModel* m_model;

	HWND m_skel_wnd[ SKEL_MAX ];
	std::vector<int> m_combo2boneno;

	CKbsFile* m_kbsfile;

	WCHAR m_kbsname[MAX_PATH];
	WCHAR m_kbsin[MAX_PATH];
};

#endif //__ConvSetDlg_H_

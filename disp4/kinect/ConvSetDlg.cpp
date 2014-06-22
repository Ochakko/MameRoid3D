// ConvSetDlg.cpp : CConvSetDlg のインプリメンテーション
#include <ConvSetDlg.h>

#include <Commdlg.h>
//#include <Afxdlgs.h>

#include <KbsFile.h>
#include <Model.h>
#include <Bone.h>

#define DBGH
#include <dbg.h>

static LRESULT CALLBACK DlgProc(HWND hDlgWnd, UINT msg, WPARAM wp, LPARAM lp);
static CConvSetDlg* s_dlg = 0;

/////////////////////////////////////////////////////////////////////////////
// CConvSetDlg

static int s_chkid[SKEL_MAX] = 
{
	IDC_CHKTOPOFJOINT,
	IDC_CHKTORSO,
	IDC_CHKLEFTHIP,
	IDC_CHKLEFTKNEE,
	IDC_CHKLEFTFOOT,
	IDC_CHKRIGHTHIP,
	IDC_CHKRIGHTKNEE,
	IDC_CHKRIGHTFOOT,
	IDC_CHKNECK,
	IDC_CHKHEAD,
	IDC_CHKLEFTSHOULDER,
	IDC_CHKLEFTELBOW,
	IDC_CHKLEFTHAND,
	IDC_CHKRIGHTSHOULDER,
	IDC_CHKRIGHTELBOW,
	IDC_CHKRIGHTHAND
};
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

CConvSetDlg::CConvSetDlg() : m_combo2boneno()
{
	m_model = 0;
	m_kbsfile = 0;
	ZeroMemory( m_skel_wnd, sizeof( HWND ) * SKEL_MAX );

	ZeroMemory( m_kbsname, sizeof( WCHAR ) * MAX_PATH );
	wcscpy_s( m_kbsname, MAX_PATH, L"default.kst" );
	ZeroMemory( m_kbsin, sizeof( WCHAR ) * MAX_PATH );

}

CConvSetDlg::~CConvSetDlg()
{
}

int CConvSetDlg::DoModal( HWND parwnd, CModel* srcmodel, CKbsFile* kbsptr )
{
	ZeroMemory( m_kbsname, sizeof( WCHAR ) * MAX_PATH );
	wcscpy_s( m_kbsname, MAX_PATH, L"default.kbs" );
	ZeroMemory( m_kbsin, sizeof( WCHAR ) * MAX_PATH );

	s_dlg = this;//!!!!!!!!
	m_model = srcmodel;
	m_kbsfile = kbsptr;

	int dlgret = 0;
	dlgret = (int)DialogBoxW( (HINSTANCE)GetModuleHandle(NULL), MAKEINTRESOURCE( IDD_CONVSETDLG ), 
		parwnd, (DLGPROC)DlgProc );
	return dlgret;
}

LRESULT CALLBACK DlgProc(HWND hDlgWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg) {
        case WM_INITDIALOG:
			if( s_dlg ){
				s_dlg->OnInitDialog( hDlgWnd );
			}
			return FALSE;
        case WM_COMMAND:
            switch (LOWORD(wp)) {
                case IDOK:
					s_dlg->OnOK( hDlgWnd );
                    break;
                case IDCANCEL:
					s_dlg->OnCancel( hDlgWnd );
                    break;
				case IDC_REFKSTIN:
					s_dlg->OnRefKbsIn( hDlgWnd );
					break;
				case IDC_REFKST:
					s_dlg->OnRefKbs( hDlgWnd );
					break;
				default:
                    return FALSE;
            }
        default:
            return FALSE;
    }
    return TRUE;
}


int CConvSetDlg::OnInitDialog( HWND hwnd )
{
	SetWnd( hwnd );
	SetCombo();
	ParamsToDlg( hwnd );

	return 1;  // システムにフォーカスを設定させます
}

int CConvSetDlg::OnRefKbs( HWND hwnd )
{
	WCHAR tmpname[MAX_PATH];
	if( m_kbsname[0] != 0 ){
		wcscpy_s( tmpname, MAX_PATH, m_kbsname );
	}else{
		tmpname[0] = 0L;
	}

	OPENFILENAME ofn;
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hwnd;
	ofn.hInstance = 0;
	ofn.lpstrFilter = L"KinectBoneSettingFile (*.kbs)\0*.kbs\0";
	ofn.lpstrCustomFilter = NULL;
	ofn.nMaxCustFilter = 0;
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = tmpname;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.lpstrTitle = NULL;
	ofn.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
	ofn.nFileOffset = 0;
	ofn.nFileExtension = 0;
	ofn.lpstrDefExt = L"kbs";
	ofn.lCustData = NULL;
	ofn.lpfnHook = NULL;
	ofn.lpTemplateName = NULL;
	if( !GetSaveFileName(&ofn) ){
		return S_OK;
	}

	wcscpy_s( m_kbsname, MAX_PATH, tmpname );
	SetDlgItemText( hwnd, IDC_KSTNAME, m_kbsname );

	return 0;
}

int CConvSetDlg::OnOK( HWND hwnd )
{

	int cno;
	for( cno = 0; cno < SKEL_MAX; cno++ ){
		TSELEM* curts;
		curts = m_kbsfile->m_elem + cno;

		int tmpcombo;
		tmpcombo = SendMessage( m_skel_wnd[cno], CB_GETCURSEL, 0, 0 );
		if( tmpcombo != CB_ERR ){
			UINT ischecked;
			int tmptwist;
			ischecked = IsDlgButtonChecked( hwnd, s_chkid[ cno ] );
			if( ischecked == BST_CHECKED ){
				tmptwist = 0;
			}else{
				tmptwist = 1;
			}
			
			_ASSERT( tmpcombo < (int)m_model->m_bonelist.size() );
			int boneno = m_combo2boneno[ tmpcombo ];
			CBone* curbone = m_model->m_bonelist[ boneno ];
			_ASSERT( curbone );

			strcpy_s( curts->jointname, 256, curbone->m_bonename );
//::MessageBoxA( NULL, curts->jointname, curbone->m_bonename, MB_OK );
			curts->jointno = curbone->m_boneno;
			curts->skelno = cno;
			curts->twistflag = tmptwist;
		}else{
			_ASSERT( 0 );
			strcpy_s( curts->jointname, 256, "存在しないジョイント" );
			curts->jointno = -1;
			curts->skelno = cno;
			curts->twistflag = 1;
		}
	}

	CallF( m_kbsfile->SaveKbsFile( m_kbsname ), return 1 );
    EndDialog(hwnd, IDOK);

//	EndDialog(wID);
	return 0;
}

int CConvSetDlg::OnCancel( HWND hwnd )
{
    EndDialog(hwnd, IDCANCEL);
	return 0;
}

int CConvSetDlg::SetWnd( HWND hwnd )
{
	m_skel_wnd[SKEL_TOPOFJOINT] = GetDlgItem( hwnd, IDC_TOPOFJOINT );
	m_skel_wnd[SKEL_TORSO] = GetDlgItem( hwnd, IDC_TORSO );
	
	m_skel_wnd[SKEL_LEFT_HIP] = GetDlgItem( hwnd, IDC_LEFTHIP );
	m_skel_wnd[SKEL_LEFT_KNEE] = GetDlgItem( hwnd, IDC_LEFTKNEE );
	m_skel_wnd[SKEL_LEFT_FOOT] = GetDlgItem( hwnd, IDC_LEFTFOOT );

	m_skel_wnd[SKEL_RIGHT_HIP] = GetDlgItem( hwnd, IDC_RIGHTHIP );
	m_skel_wnd[SKEL_RIGHT_KNEE] = GetDlgItem( hwnd, IDC_RIGHTKNEE );
	m_skel_wnd[SKEL_RIGHT_FOOT] = GetDlgItem( hwnd, IDC_RIGHTFOOT );

	m_skel_wnd[SKEL_NECK] = GetDlgItem( hwnd, IDC_NECK );
	m_skel_wnd[SKEL_HEAD] = GetDlgItem( hwnd, IDC_HEAD );

	m_skel_wnd[SKEL_LEFT_SHOULDER] = GetDlgItem( hwnd, IDC_LEFTSHOULDER );
	m_skel_wnd[SKEL_LEFT_ELBOW] = GetDlgItem( hwnd, IDC_LEFTELBOW );
	m_skel_wnd[SKEL_LEFT_HAND] = GetDlgItem( hwnd, IDC_LEFTHAND );

	m_skel_wnd[SKEL_RIGHT_SHOULDER] = GetDlgItem( hwnd, IDC_RIGHTSHOULDER );
	m_skel_wnd[SKEL_RIGHT_ELBOW] = GetDlgItem( hwnd, IDC_RIGHTELBOW );
	m_skel_wnd[SKEL_RIGHT_HAND] = GetDlgItem( hwnd, IDC_RIGHTHAND );

	int skelno;
	for( skelno = 0; skelno < SKEL_MAX; skelno++ ){
		_ASSERT( m_skel_wnd[skelno] );
	}

	//m_kbsname_wnd = GetDlgItem( IDC_KSTNAME );
	//m_kbsin_wnd = GetDlgItem( IDC_KSTIN );

	return 0;
}

int CConvSetDlg::ParamsToDlg( HWND hwnd )
{	
	int cno;
	for( cno = 0; cno < SKEL_MAX; cno++ ){
		SendMessage( m_skel_wnd[cno], CB_SETCURSEL, (WPARAM)0, 0 );
	}

	SetDlgItemText( hwnd, IDC_KSTNAME, m_kbsname );
	SetDlgItemText( hwnd, IDC_KSTIN, m_kbsin );

	int skno;
	for( skno = 0; skno < SKEL_MAX; skno++ ){
		if( (m_kbsfile->m_elem + skno)->twistflag == 0 ){
			CheckDlgButton( hwnd, s_chkid[skno], BST_CHECKED );
		}else{
			CheckDlgButton( hwnd, s_chkid[skno], BST_UNCHECKED );
		}
	}

	return 0;
}

int CConvSetDlg::SetCombo()
{

	int cno;
	for( cno = 0; cno < SKEL_MAX; cno++ ){
		SendMessage( m_skel_wnd[cno], CB_RESETCONTENT, 0, 0 );
	}

	if( !m_model->m_topbone ){
		return 0;
	}
	
	int skelno;
	for( skelno = 0; skelno < SKEL_MAX; skelno++ ){
		map<int, CBone*>::iterator itrbone;
		for( itrbone = m_model->m_bonelist.begin(); itrbone != m_model->m_bonelist.end(); itrbone++ ){
			CBone* curbone = itrbone->second;
			_ASSERT( curbone );
			SendMessage( m_skel_wnd[skelno], CB_ADDSTRING, 0, (LPARAM)curbone->m_wbonename );
		}

		TSELEM* curtse = m_kbsfile->m_elem + skelno;
		strcpy_s( curtse->jointname, 256, m_model->m_topbone->m_bonename );
		curtse->jointno = m_model->m_topbone->m_boneno;
	}


	m_combo2boneno.clear();

	int combono = 0;
	map<int, CBone*>::iterator itrbone2;
	for( itrbone2 = m_model->m_bonelist.begin(); itrbone2 != m_model->m_bonelist.end(); itrbone2++ ){
		CBone* curbone = itrbone2->second;
		_ASSERT( curbone );
		m_combo2boneno.push_back( curbone->m_boneno );
		combono++;
	}

	_ASSERT( combono == m_model->m_bonelist.size() );

	return 0;
}

int CConvSetDlg::OnRefKbsIn( HWND hwnd )
{

	OPENFILENAME ofn;
	WCHAR buf[MAX_PATH];
	buf[0] = 0L;
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hwnd;
	ofn.hInstance = 0;
	ofn.lpstrFilter = L"KinectBoneSetting FILE (*.kbs)\0*.kbs\0";
	ofn.lpstrCustomFilter = NULL;
	ofn.nMaxCustFilter = 0;
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = buf;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.lpstrTitle = NULL;
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	ofn.nFileOffset = 0;
	ofn.nFileExtension = 0;
	ofn.lpstrDefExt =NULL;
	ofn.lCustData = NULL;
	ofn.lpfnHook = NULL;
	ofn.lpTemplateName = NULL;
	if( GetOpenFileName(&ofn) == 0 )
		return 0;

	ZeroMemory( m_kbsin, sizeof( WCHAR ) * MAX_PATH );
	wcscpy_s( m_kbsin, MAX_PATH, buf );
	SetDlgItemText( hwnd, IDC_KSTIN, m_kbsin );

	CallF( m_kbsfile->LoadKbsFile( m_model, m_kbsin ), return 1 );

	int skelno;
	for( skelno = 0; skelno < SKEL_MAX; skelno++ ){
		TSELEM* curtse = m_kbsfile->m_elem + skelno;
		if( curtse->jointno >= 0 ){
			int jcnt;
			int combono = -1;
			for( jcnt = 0; jcnt < (int)m_model->m_bonelist.size(); jcnt++ ){
				if( m_combo2boneno[jcnt] == curtse->jointno ){
					combono = jcnt;
					break;
				}
			}
			if( combono >= 0 ){
				SendMessage( m_skel_wnd[skelno], CB_SETCURSEL, (WPARAM)combono, 0 );
			}else{
				SendMessage( m_skel_wnd[skelno], CB_SETCURSEL, (WPARAM)0, 0 );
			}

			if( curtse->twistflag == 0 ){
				CheckDlgButton( hwnd, s_chkid[skelno], BST_CHECKED );
			}else{
				CheckDlgButton( hwnd, s_chkid[skelno], BST_UNCHECKED );
			}

		}else{
			SendMessage( m_skel_wnd[skelno], CB_SETCURSEL, (WPARAM)0, 0 );
		}
	}

	return 0;
}

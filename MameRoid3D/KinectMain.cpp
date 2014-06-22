#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "KinectMain.h"

#define DBGH
#include <dbg.h>

#include <crtdbg.h>

#include <KbsFile.h>
#include <rps.h>

#include <Model.h>
#include <Bone.h>
#include <MotionPoint.h>
#include <quaternion.h>

#include <ConvSetDlg.h>
#include <FilterType.h>
#include <FilterDlg.h>

#include <MySprite.h>

//#include "MultiCapDlg.h"

CKinectMain::CKinectMain( LPDIRECT3DDEVICE9 pdev )
{
	m_pdev = pdev;
	m_model = 0;

	ZeroMemory( m_filepath, sizeof( WCHAR ) * _MAX_PATH );
	m_pluginversion = 0.0f;
	ZeroMemory( m_filename, sizeof( WCHAR ) * _MAX_PATH );
	wcscpy_s( m_filename, _MAX_PATH, L"KinecoDX.dll" );

	m_validflag = 0;//!!!!!!!!!!!!!!!!!!!!!!
	m_hModule = NULL;

	OpenNIInit = NULL;
	OpenNIClean = NULL;
	OpenNIDrawDepthMap = NULL;
	OpenNIDepthTexture = NULL;
	OpenNIGetSkeltonJointPosition = NULL;
	OpenNIIsTracking = NULL;
	OpenNIGetVersion = NULL;

	m_ptex = NULL;
	m_hwnd = NULL;

	m_texwidth = 256;
	m_texheight = 256;

	m_rendercnt = 0;

	m_capmode = 0;
	m_capmotid = 0;
	m_capstartframe = 0;
	m_capendframe = 0;
	m_capframe = 0;
	m_capstep = 20;

	m_timermax = 9 * 30;
	m_curtimer = m_timermax;

	m_dlg = 0;
}
void CKinectMain::DestroyRps()
{
	if( m_model && m_model->m_rps ){
		delete m_model->m_rps;
		m_model->m_rps = 0;
	}

}
void CKinectMain::DestroyKbs()
{
	if( m_model && m_model->m_kbs ){
		delete m_model->m_kbs;
		m_model->m_kbs = 0;
	}
}


CKinectMain::~CKinectMain()
{
	UnloadPlugin();
//	DestroyRps();
//	DestroyKbs();

	if( m_dlg ){
		delete m_dlg;
		m_dlg = 0;
	}		
}

int CKinectMain::SetFilePath( WCHAR* pluginpath )
{
	wcscpy_s( m_filepath, _MAX_PATH, pluginpath );

	return 0;
}


#define GPA(proc) \
	*(FARPROC *)&proc = GetProcAddress(m_hModule, #proc);


int CKinectMain::LoadPlugin( HWND srchwnd )
{
	m_hwnd = srchwnd;
	m_validflag = 0;//!!!!!!!!!!!!

	WCHAR dllname[_MAX_PATH];
	swprintf_s( dllname, _MAX_PATH, L"%s\\%s", m_filepath, m_filename );

	m_hModule = LoadLibrary( dllname );
	if(m_hModule == NULL){
		DbgOut( L"KinectMain : LoadPlugin : LoadLibrary error %s!!!\r\n", dllname );
		_ASSERT( 0 );
		return 1;
	}

	GPA(OpenNIInit);
	GPA(OpenNIClean);
	GPA(OpenNIDrawDepthMap);
	GPA(OpenNIDepthTexture);
	GPA(OpenNIGetSkeltonJointPosition);
	GPA(OpenNIIsTracking);
	GPA(OpenNIGetVersion);



	if( !OpenNIInit || !OpenNIClean || !OpenNIDrawDepthMap || !OpenNIDepthTexture || !OpenNIGetSkeltonJointPosition || 
		!OpenNIIsTracking || !OpenNIGetVersion ){
		DbgOut( L"KinectMain : LoadPlugin : this dll is not for OpenRDB %s\r\n", m_filename );
		return 1;//!!!!!!!!!!!!!!!!!
	}

////////////
	if( OpenNIGetVersion ){
		OpenNIGetVersion( &m_pluginversion );
	}

	if( m_dlg ){
		delete m_dlg;
		m_dlg = 0;
	}
	m_dlg = new CConvSetDlg();
	if( !m_dlg ){
		_ASSERT( 0 );
		return 1;
	}

	m_validflag = 1;//!!!!!!!!!!!!!!!!!!!!!!!!!	

	return 0;
}
int CKinectMain::UnloadPlugin()
{
	m_validflag = 0;//!!!!!!!!!!!!!!!!

	if( m_hModule ){
		FreeLibrary( m_hModule );
		m_hModule = NULL;
	}
	
	return 0;
}


int CKinectMain::StartCapture( CModel* srcmodel, int capmode )
{
	m_model = srcmodel;

	bool bret;
	if( m_validflag == 0 ){
		::MessageBox( m_hwnd, L"Kinectライブラリをロードしてから再試行してください。", L"エラー", MB_OK );
		return 1;
	}
	if( !m_model->m_kbs ){
		::MessageBox( m_hwnd, L"kbsファイルが読み込まれていないのでキャプチャ出来ません。\nkbsファイルを読み込んでから再試行してください。", L"エラー", MB_OK );
		return 1;
	}
	m_capmode = capmode;

	::MessageBox( m_hwnd, L"kinect capture start", L"kinect", MB_OK );

	m_rendercnt = 0;

	DestroyRps();

	OpenNIClean();



	int ret;

	m_model->m_rps = new CRps( m_model );
	if( !m_model->m_rps ){
		_ASSERT( 0 );
		m_validflag = 0;
		return 1;
	}
	ret = m_model->m_rps->CreateParams();
	if( ret ){
		_ASSERT( 0 );
		m_validflag = 0;
		return 1;
	}



	m_capmotid = m_model->m_curmotinfo->motid;
	if( m_capmotid < 0 ){
		_ASSERT( 0 );
		return 0;
	}

	m_capstartframe = 0.0;
	m_capendframe = m_model->m_curmotinfo->frameleng - 1.0;
	m_capframe = 0.0;
	m_capstep = 2.0;

	ret = CreateMotionPoints( m_capmotid, m_capstartframe, m_capendframe, 1 );
	if( ret ){
		_ASSERT( 0 );
		return 1;
	}


	char strpath[MAX_PATH] = {0};
	WideCharToMultiByte( CP_ACP, 0, m_filepath, -1, strpath, MAX_PATH, NULL, NULL );
	bret = OpenNIInit( 0, m_hwnd, 0, m_pdev, strpath, 0 );
	if( bret == false ){
		_ASSERT( 0 );
		m_validflag = 0;
		return 1;
	}

	OpenNIDepthTexture( &m_ptex );
	if( !m_ptex ){
		_ASSERT( 0 );
		return 1;
	}


	HRESULT hr;
	D3DSURFACE_DESC	sdesc;
	hr = m_ptex->GetLevelDesc( 0, &sdesc );
	if( hr != D3D_OK ){
		DbgOut( L"KinectMain : GetLevelDesc error !!!\n" );
		_ASSERT( 0 );
		m_validflag = 0;
		return 1;
	}
	m_texwidth = sdesc.Width;
	m_texheight = sdesc.Height;

	return 0;
}
int CKinectMain::EndCapture()
{
	if( m_validflag == 0 ){
		return 0;
	}

	m_ptex = NULL;
	OpenNIClean();

	::MessageBox( m_hwnd, L"kinect capture end", L"kinect", MB_OK );

	return 0;
}

int CKinectMain::Update( CModel* srcmodel, double* dstframe )
{
	if( (m_validflag == 0) || !m_model->m_kbs || !m_model->m_rps ){
		_ASSERT( 0 );
		*dstframe = 0.0;
		return 0;
	}
	if( m_rendercnt % 2 ){
		m_rendercnt++;
		return 0;
	}


	OpenNIDrawDepthMap( 0 );

	bool trflag = false;
	OpenNIIsTracking( &trflag );

	D3DXVECTOR3 tmppos[ SKEL_MAX ];
	int ret;


	if( (m_rendercnt == 0) && trflag ){
		m_capframe = m_capstartframe;
		//g_motdlg->m_motparamdlg->m_sl_mp_wnd.SendMessage( TBM_SETPOS, (WPARAM)TRUE, (LPARAM)m_capframe );

		ret = m_model->m_rps->InitArMp( m_model->m_kbs->m_elem, m_capmotid, m_capframe );
		_ASSERT( !ret );
		
		OpenNIGetSkeltonJointPosition( SKEL_MAX, tmppos );
		int skno;
		for( skno = 0; skno < SKEL_MAX; skno++ ){
			tmppos[skno].x *= -1.0f;
		}
		ret = m_model->m_rps->SetRpsElem( 0, tmppos );
		_ASSERT( !ret );

		ret = m_model->m_rps->CalcTraQ( m_model->m_kbs->m_elem, 1 );
		_ASSERT( !ret );

		MODELBOUND mb;
		m_model->GetModelBound( &mb );
		ret = m_model->m_rps->SetMotion( mb, m_model->m_kbs->m_elem, m_capmotid, m_capframe );
		_ASSERT( !ret );

		m_rendercnt++;
	}else if( trflag ){

		int dosetflag = 0;

		if( m_capmode == 0 ){
			m_capframe = (double)m_rendercnt / 2.0 * m_capstep;
			if( m_capframe > m_capendframe ){
				m_rendercnt = 0;
				*dstframe = m_capendframe;
				EndCapture();
				return 2;//!!!!!!!!!!!!!!!!!終わりの印
			}
			dosetflag = 1;
		}

		OpenNIGetSkeltonJointPosition( SKEL_MAX, tmppos );
		int skno;
		for( skno = 0; skno < SKEL_MAX; skno++ ){
			tmppos[skno].x *= -1.0f;
		}
		ret = m_model->m_rps->SetRpsElem( 2, tmppos );
		_ASSERT( !ret );

		ret = m_model->m_rps->CalcTraQ( m_model->m_kbs->m_elem, 1 );
		_ASSERT( !ret );

		if( dosetflag == 1 ){
			MODELBOUND mb;
			m_model->GetModelBound( &mb );
			ret = m_model->m_rps->SetMotion( mb, m_model->m_kbs->m_elem, m_capmotid, m_capframe );
			_ASSERT( !ret );
		}
		m_rendercnt++;
	}else{
		m_rendercnt = 0;
		*dstframe = 0.0;
		return 0;//!!!!!!!!!!!!!!!!!!!!!!!
	}

	*dstframe = m_capframe;

	return 0;
}

int CKinectMain::LoadKbs( CModel* srcmodel )
{
	if( m_validflag == 0 ){
		::MessageBox( m_hwnd, L"Kinectライブラリをロードしてから再試行してください。", L"エラー", MB_OK );
		return 1;
	}

	m_model = srcmodel;

	DestroyKbs();

	OPENFILENAME ofn;
	WCHAR buf[_MAX_PATH];
	buf[0] = 0;
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = m_hwnd;
	ofn.hInstance = 0;
	ofn.lpstrFilter = L"KinectBoneSettingFile(*.kbs)\0*.kbs\0";
	ofn.lpstrCustomFilter = NULL;
	ofn.nMaxCustFilter = 0;
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = buf;
	ofn.nMaxFile = _MAX_PATH;
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
	if( GetOpenFileName(&ofn) == 0 ){
		return 0;
	}

	int ret;
	m_model->m_kbs = new CKbsFile();
	if( !m_model->m_kbs ){
		_ASSERT( 0 );
		m_validflag = 0;
		return 1;
	}
	ret = m_model->m_kbs->LoadKbsFile( m_model, buf );
	if( ret ){
		_ASSERT( 0 );
		m_validflag = 0;
		DestroyKbs();
		return 1;
	}

/***
	D3DXVECTOR3 xpdir( 1.0f, 0.0f, 0.0f );
	D3DXVECTOR3 xmdir( -1.0f, 0.0f, 0.0f );
	D3DXVECTOR3 zmdir( 0.0f, 0.0f, -1.0f );
	D3DXVECTOR3 zpdir( 0.0f, 0.0f, 1.0f );

	ret = m_model->SetMiko3rdVec( (m_model->m_kbs->m_elem + SKEL_RIGHT_KNEE)->jointno, xmdir );
	_ASSERT( !ret );
	ret = m_model->SetMiko3rdVec( (m_model->m_kbs->m_elem + SKEL_RIGHT_FOOT)->jointno, xmdir );
	_ASSERT( !ret );
	ret = m_model->SetMiko3rdVec( (m_model->m_kbs->m_elem + SKEL_LEFT_KNEE)->jointno, xpdir );
	_ASSERT( !ret );
	ret = m_model->SetMiko3rdVec( (m_model->m_kbs->m_elem + SKEL_LEFT_FOOT)->jointno, xpdir );
	_ASSERT( !ret );

	ret = m_model->SetMiko3rdVec( (m_model->m_kbs->m_elem + SKEL_RIGHT_ELBOW)->jointno, zmdir );
	_ASSERT( !ret );
	ret = m_model->SetMiko3rdVec( (m_model->m_kbs->m_elem + SKEL_RIGHT_HAND)->jointno, xpdir );
	_ASSERT( !ret );
	ret = m_model->SetMiko3rdVec( (m_model->m_kbs->m_elem + SKEL_LEFT_ELBOW)->jointno, zpdir );
	_ASSERT( !ret );
	ret = m_model->SetMiko3rdVec( (m_model->m_kbs->m_elem + SKEL_LEFT_HAND)->jointno, xpdir );
	_ASSERT( !ret );
***/


	return 0;
}

int CKinectMain::EditKbs( CModel* srcmodel )
{
	m_model = srcmodel;

	if( m_validflag == 0 ){
		::MessageBox( m_hwnd, L"Kinectライブラリをロードしてから再試行してください。", L"エラー", MB_OK );
		return 1;
	}

	DestroyKbs();

	m_model->m_kbs = new CKbsFile();
	if( !m_model->m_kbs ){
		_ASSERT( 0 );
		m_validflag = 0;
		return 1;
	}
	int ret;
	ret = (int)m_dlg->DoModal( m_hwnd, m_model, m_model->m_kbs );
/***
	if( ret == IDOK ){

		D3DXVECTOR3 xpdir( 1.0f, 0.0f, 0.0f );
		D3DXVECTOR3 xmdir( -1.0f, 0.0f, 0.0f );
		D3DXVECTOR3 zmdir( 0.0f, 0.0f, -1.0f );
		D3DXVECTOR3 zpdir( 0.0f, 0.0f, 1.0f );

		ret = m_model->SetMiko3rdVec( (m_model->m_kbs->m_elem + SKEL_RIGHT_KNEE)->jointno, xmdir );
		_ASSERT( !ret );
		ret = m_model->SetMiko3rdVec( (m_model->m_kbs->m_elem + SKEL_RIGHT_FOOT)->jointno, xmdir );
		_ASSERT( !ret );
		ret = m_model->SetMiko3rdVec( (m_model->m_kbs->m_elem + SKEL_LEFT_KNEE)->jointno, xpdir );
		_ASSERT( !ret );
		ret = m_model->SetMiko3rdVec( (m_model->m_kbs->m_elem + SKEL_LEFT_FOOT)->jointno, xpdir );
		_ASSERT( !ret );

		ret = m_model->SetMiko3rdVec( (m_model->m_kbs->m_elem + SKEL_RIGHT_ELBOW)->jointno, zmdir );
		_ASSERT( !ret );
		ret = m_model->SetMiko3rdVec( (m_model->m_kbs->m_elem + SKEL_RIGHT_HAND)->jointno, xpdir );
		_ASSERT( !ret );
		ret = m_model->SetMiko3rdVec( (m_model->m_kbs->m_elem + SKEL_LEFT_ELBOW)->jointno, zpdir );
		_ASSERT( !ret );
		ret = m_model->SetMiko3rdVec( (m_model->m_kbs->m_elem + SKEL_LEFT_HAND)->jointno, xpdir );
		_ASSERT( !ret );
	}
***/

	return 0;
}


int CKinectMain::CreateMotionPoints( int cookie, double startframe, double endframe, int forceflag )
{
	//forceflag == 1のときは初期状態のモーションポイントを作る。

	if( !m_model->m_curmotinfo ){
		_ASSERT( 0 );
		return 1;//!!!!!!!!
	}

	CQuaternion initq;
	initq.SetParams( 1.0f, 0.0f, 0.0f, 0.0f );

	int framecnt = 0;
	double frameno;
	for( frameno = startframe; frameno <= endframe; frameno += 2.0 ){
		double setframe = startframe + (double)framecnt * 2.0;
		map<int, CBone*>::iterator itrbone;
		for( itrbone = m_model->m_bonelist.begin(); itrbone != m_model->m_bonelist.end(); itrbone++ ){
			CBone* curbone = itrbone->second;
			_ASSERT( curbone );

			CMotionPoint* newmp = 0;
			int existflag = 0;
			int calcflag = !forceflag;
			newmp = curbone->AddMotionPoint( cookie, setframe, calcflag, &existflag );
			if( !newmp ){
				_ASSERT( 0 );
				return 1;
			}
		}
		framecnt++;
	}
		
	return 0;
}


int CKinectMain::ApplyFilter( CModel* srcmodel )
{
	if( m_validflag == 0 ){
		::MessageBoxA( m_hwnd, "Kinect用のプラグインが正常に読み込まれなかったので\n平滑化出来ません。", "エラー", MB_OK );
		return 1;
	}
	if( !srcmodel->m_kbs ){
		::MessageBoxA( m_hwnd, "kbsファイルが読み込まれていないので平滑化出来ません。\nkbsファイルを読み込んでから再試行してください。", "エラー", MB_OK );
		return 1;
	}
	m_model = srcmodel;

	int ret;
/***
	D3DXVECTOR3 xpdir( 1.0f, 0.0f, 0.0f );
	D3DXVECTOR3 xmdir( -1.0f, 0.0f, 0.0f );
	D3DXVECTOR3 zmdir( 0.0f, 0.0f, -1.0f );
	D3DXVECTOR3 zpdir( 0.0f, 0.0f, 1.0f );

	ret = m_model->SetMiko3rdVec( (m_model->m_kbs->m_elem + SKEL_RIGHT_KNEE)->jointno, xmdir );
	_ASSERT( !ret );
	ret = m_model->SetMiko3rdVec( (m_model->m_kbs->m_elem + SKEL_RIGHT_FOOT)->jointno, xmdir );
	_ASSERT( !ret );
	ret = m_model->SetMiko3rdVec( (m_model->m_kbs->m_elem + SKEL_LEFT_KNEE)->jointno, xpdir );
	_ASSERT( !ret );
	ret = m_model->SetMiko3rdVec( (m_model->m_kbs->m_elem + SKEL_LEFT_FOOT)->jointno, xpdir );
	_ASSERT( !ret );

	ret = m_model->SetMiko3rdVec( (m_model->m_kbs->m_elem + SKEL_RIGHT_ELBOW)->jointno, zmdir );
	_ASSERT( !ret );
	ret = m_model->SetMiko3rdVec( (m_model->m_kbs->m_elem + SKEL_RIGHT_HAND)->jointno, xpdir );
	_ASSERT( !ret );
	ret = m_model->SetMiko3rdVec( (m_model->m_kbs->m_elem + SKEL_LEFT_ELBOW)->jointno, zpdir );
	_ASSERT( !ret );
	ret = m_model->SetMiko3rdVec( (m_model->m_kbs->m_elem + SKEL_LEFT_HAND)->jointno, xpdir );
	_ASSERT( !ret );
***/



	CFilterDlg dlg;
	ret = (int)dlg.DoModal( m_hwnd );
	if( ret != IDOK ){
		return 0;
	}

	int curmotid;
	curmotid = m_model->m_curmotinfo->motid;
	if( curmotid < 0 ){
		return 0;
	}

	double frameleng = m_model->m_curmotinfo->frameleng;


	ret = CreateMotionPoints( curmotid, 0.0, frameleng - 1.0, 0 );
	if( ret ){
		_ASSERT( 0 );
		return 1;
	}

	ret = AvrgMotion( curmotid, frameleng, dlg.m_filtertype, dlg.m_filtersize );
	if( ret ){
		_ASSERT( 0 );
		return 1;
	}

	return 0;
}
/***********************************************************

　　フィルタサイズが大きいほど、
  　たくさんのフレーム間で平滑がなされる。

　　・移動平均：
　　　　通常の線形による平滑化。

　　・加重移動平均：
　　　　徐々に重みを小さくしていく平滑化。
　　　　移動平均よりも、少しピークが残りやすい。

　　・ガウシアン：
　　　　ガウシアンフィルタによる平滑化。
	　　元のモーションの再現率が大きい。（といいなあ）
　
***********************************************************/
int CKinectMain::AvrgMotion( int motid, double frameleng, int type, int filter_size )
{
	int iframeleng = (int)( frameleng + 0.1 );

	int half_filtersize = filter_size / 2;

	//errno_t err;

	D3DXVECTOR3 tmp_vec3;
	D3DXVECTOR3 tmp_pos3;

	//オイラー角変換用
	D3DXVECTOR3 dirx( 1.0f, 0.0f, 0.0f );
	D3DXVECTOR3 diry( 0.0f, 1.0f, 0.0f );
	D3DXVECTOR3 dirz( 0.0f, 0.0f, 1.0f );
	CQuaternion qx, qy, qz;

	//オイラー角取得用
	CQuaternion curq;
	CQuaternion axisq;
	CQuaternion invaxisq;

	//オイラー角格納用
	D3DXVECTOR3 *smooth_eul;
	D3DXVECTOR3 *eul;
	D3DXVECTOR3 befeul;
	smooth_eul	= new D3DXVECTOR3[ iframeleng ];
	if( !smooth_eul ){
		_ASSERT( 0 );
		return 1;
	}
	eul	= new D3DXVECTOR3[ iframeleng ];
	if( !eul ){
		_ASSERT( 0 );
		return 1;
	}

	D3DXVECTOR3* tra;
	tra = new D3DXVECTOR3[ iframeleng ];
	if( !tra ){
		_ASSERT( 0 );
		return 1;
	}
	D3DXVECTOR3* smooth_tra;
	smooth_tra = new D3DXVECTOR3[ iframeleng ];
	if( !smooth_tra ){
		_ASSERT( 0 );
		return 1;
	}



	int skno, seri;
	for( skno = 0; skno < SKEL_MAX; skno++ ){
		seri = m_model->m_kbs->m_elem[skno].jointno;

		if( (seri < 0) || (seri >= (int)m_model->m_bonelist.size()) ){
			_ASSERT( 0 );
			continue;
		}

		CBone* curbone = m_model->m_bonelist[ seri ];
		if( curbone ){
			CMotionPoint* firstmp = curbone->m_motionkey[ motid ];
			if( !firstmp ){
				_ASSERT( 0 );
				return 1;
			}
			CMotionPoint* curmp = firstmp;

			//オイラー角初期化
			befeul = D3DXVECTOR3( 0.0f, 0.0f, 0.0f );
			ZeroMemory( eul, sizeof( D3DXVECTOR3 ) * iframeleng );
			ZeroMemory( smooth_eul, sizeof( D3DXVECTOR3 ) * iframeleng );
			ZeroMemory( tra, sizeof( D3DXVECTOR3 ) * iframeleng );
			ZeroMemory( smooth_tra, sizeof( D3DXVECTOR3 ) * iframeleng );
			


			//全クォータニオンに対するオイラー角の取得
			for( int frameno = 0; frameno < iframeleng; frameno+=2 ){
				double dframe = (double)frameno;
				if( !curmp ){
					_ASSERT( 0 );
					return 1;
				}
				eul[frameno] = curmp->m_eul;
				*(tra + frameno) = curmp->m_tra;
				befeul = eul[frameno];

				*(tra + frameno + 1) = *(tra + frameno);
				eul[frameno + 1] = eul[frameno];

				curmp = curmp->m_next;
			}


			//平滑化処理
			switch (type) {

			case AVGF_NONE:							//平滑処理なし
				break;

			case AVGF_MOVING:					//移動平均
				//平滑化処理
				for( int frameno = 0; frameno < iframeleng; frameno++ ){
					tmp_vec3.x = tmp_vec3.y = tmp_vec3.z = 0.0f;
					tmp_pos3.x = tmp_pos3.y = tmp_pos3.z = 0.0f;
					for( int k=-half_filtersize; k<= half_filtersize; k++ ){
						int index = frameno + k;
						if( ( index < 0 )||( index >= frameleng ) ){
							tmp_vec3 += eul[frameno];
							tmp_pos3 += tra[frameno];
						}
						else{
							tmp_vec3 += eul[index];
							tmp_pos3 += tra[index];
						}
					}
					smooth_eul[frameno] = tmp_vec3 / (float)filter_size;
					smooth_tra[frameno] = tmp_pos3 / (float)filter_size;
				}
				break;

			case AVGF_WEIGHTED_MOVING:			//加重移動平均
			{
				double denomVal = 0.0;
				float weightVal = 0.0;

				int sumd  = 0;
				int coef = 0;
				for( int i=1; i<=filter_size; i++ ){
					sumd += i;
				}
				denomVal = 1.0 / (double)sumd;

				for( int frameno = 0; frameno < iframeleng; frameno++ ){
					tmp_vec3.x = tmp_vec3.y = tmp_vec3.z = 0.0f;
					tmp_pos3.x = tmp_pos3.y = tmp_pos3.z = 0.0f;
					for( int k=-filter_size; k<= 0; k++ ){
						int index = frameno + k;
						coef = k + filter_size;
						weightVal = (float)( denomVal * (double)coef );
						if( ( index < 0 )||( index >= frameleng ) ){
							tmp_vec3 = tmp_vec3 + eul[frameno] * weightVal;
							tmp_pos3 = tmp_pos3 + tra[frameno] * weightVal;
						}
						else{
							tmp_vec3 = tmp_vec3 + eul[index] * weightVal;
							tmp_pos3 = tmp_pos3 + tra[index] * weightVal;
						}
					}
					smooth_eul[frameno] = tmp_vec3;
					smooth_tra[frameno] = tmp_pos3;
				}
				break;
			}
								
			case AVGF_GAUSSIAN:					//ガウシアン 

				double normalizeVal = 0.0;
				float normalDistVal = 0.0;

				int N = filter_size - 1;
				int sum = 0;
				int r = 0;
				for( int i=0; i<=N; i++ ){
					sum +=  combi( N, i );
				}
				normalizeVal = 1.0 / (double)sum;

				for( int frameno = 0; frameno < iframeleng; frameno++ ){
					tmp_vec3.x = tmp_vec3.y = tmp_vec3.z = 0.0f;
					tmp_pos3.x = tmp_pos3.y = tmp_pos3.z = 0.0f;
					for( int k=-half_filtersize; k<= half_filtersize; k++ ){
						int index = frameno + k;
						r = k + half_filtersize;
						normalDistVal = (float)( normalizeVal * (double) combi( N, r ) );
						if( ( index < 0 )||( index >= frameleng ) ){
							tmp_vec3 = tmp_vec3 + eul[frameno] * normalDistVal;
							tmp_pos3 = tmp_pos3 + tra[frameno] * normalDistVal;
						}
						else{
							tmp_vec3 = tmp_vec3 + eul[index] * normalDistVal;
							tmp_pos3 = tmp_pos3 + tra[index] * normalDistVal;
						}
					}
					smooth_eul[frameno] = tmp_vec3;
					smooth_tra[frameno] = tmp_pos3;
				}
				
				break;
			}


			//モーションポイントに計算値をセット
			//全クォータニオンに対するオイラー角の取得
			axisq = curbone->m_axisq;
			axisq.inv( &invaxisq );
			curmp = firstmp;
			for( int frameno = 0; frameno < iframeleng; frameno+=2 ){
				if( !curmp ){
					_ASSERT( 0 );
					return 1;
				}
				curmp->SetEul( &axisq, smooth_eul[frameno] );
				curmp->m_tra = smooth_tra[frameno];
				curmp = curmp->m_next;
			}
		}
	}

	delete [] smooth_eul;
	delete [] eul;
	delete [] tra;
	delete [] smooth_tra;

	return 0;
}

//ガウシアンフィルタ用
int CKinectMain::combi( int N, int rp ) {
	int n, r;

	//２次元配列の確保
	int **combi = new int*[N+1];
	combi[0] = new int[(N+1) * (N+1)];
	for(int i = 1; i < N+1; i++){
		combi[i] = combi[0] + i * (N+1);
	}

	//結果を残す方法
	//・nCr = n-1Cr-1 + n-1Cr 
	//・nC0 = 1, nCn = 1 　　　を用いて計算する
	for(n = 0; n <= N; n ++){
		combi[n][0] = combi[n][n] = 1;
		for(r = 1; r < n; r ++){
			combi[n][r] = combi[n - 1][r - 1] + combi[n - 1][r];
		}
	}

	int ret = combi[N][rp];

	delete[] combi[0];
	delete[] combi;

	return ret;
}


int CKinectMain::QtoEul( CModel* srcmodel, CQuaternion srcq, D3DXVECTOR3 befeul, int boneno, D3DXVECTOR3* eulptr, CQuaternion* axisqptr )
{
	CBone* curbone;
	curbone = srcmodel->m_bonelist[boneno];
	if( !curbone ){
		_ASSERT( 0 );
		return 1;
	}

	*axisqptr = curbone->m_axisq;

	D3DXVECTOR3 cureul( 0.0f, 0.0f, 0.0f );
	srcq.Q2Eul( axisqptr, befeul, &cureul );

	*eulptr = cureul;

	return 0;
}

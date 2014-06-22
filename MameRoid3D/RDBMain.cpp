//--------------------------------------------------------------------------------------
// File: BasicHLSL.cpp
//
// This sample shows a simple example of the Microsoft Direct3D's High-Level 
// Shader Language (HLSL) using the Effect interface. 
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include <DXUT.h>
#include <DXUTcamera.h>
#include <DXUTgui.h>
#include <DXUTsettingsdlg.h>
#include <SDKmisc.h>
#include "resource.h"

#include <Coef.h>
#include <Model.h>
#include <TexBank.h>
#include <Bone.h>
#include <MySprite.h>
#include <mqoobject.h>

#include <OrgWindow.h>
#include <MorphKey.h>

#define DBGH
#include <dbg.h>

#include <shlobj.h> //shell
#include <Commdlg.h>
#include <RdbFile.h>
#include <XFile.h>
#include <ColladaExporter.h>
#include <VecMath.h>

#include "KinectMain.h"
#include "RMenuMain.h"

#include <mqomaterial.h>

#include <RdbIniFile.h>

//#include <imagehlp.h>
//#include <string>
#include <PmCipherDll.h>
#include <FBXFile.h>

#include "RegistDlg.h"

static const int s_appindex = 1;

static const char s_appserial[2][37] = { 
	"A7A057B6-21AD-421c-AE36-1B63F96E9861",
	"A8D2D8E1-F922-4ffa-84EE-62C1BAC90126"
};

static const char s_appkey[2][37] = {
	"62808465-564C-4c93-9882-6A873C1735FF",
	"CDF8A126-1CCD-4c2d-87DD-7DD57E577DCD"
};


static int s_registflag = 0;
static HKEY s_hkey;
static int RegistKey();
static int IsRegist();

int RegistKey()
{
	CRegistDlg dlg;
	dlg.DoModal();

	
	//appseri "2D385197-19CD-4313-A649-7DFC80F433E4" index 1

	if( strcmp( s_appkey[ s_appindex ], dlg.m_regkey ) == 0 ){
		if( s_registflag == 0 ){
			LONG lret;
			DWORD dwret;
			lret = RegCreateKeyExA( HKEY_CURRENT_USER, "Software\\OchakkoLab\\Mameroid3D_ver0017", 0, "",
				REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &s_hkey, &dwret );
			if( dwret == REG_CREATED_NEW_KEY ){
				lret = RegSetValueExA( s_hkey, "registkey", 0, REG_SZ, (LPBYTE)(dlg.m_regkey), sizeof(char) * 36 );
				if( lret != ERROR_SUCCESS ){
					::MessageBoxA( NULL, "エラー　：　レジストに失敗しました。", "レジストエラー", MB_OK );
				}
				RegCloseKey( s_hkey );
				::MessageBoxA( NULL, "成功　：　レジストに成功しました。", "レジスト成功", MB_OK );
			}else{
				::MessageBoxA( NULL, "エラー　：　レジストに失敗しました。", "レジストエラー", MB_OK );
			}
		}
	}else{
		::MessageBoxA( NULL, "エラー　：　不正なレジストキーです。", "レジストエラー", MB_OK );
	}

	return 0;
}

int IsRegist()
{
	s_registflag = 0;

	LONG lret;

	lret = RegOpenKeyExA( HKEY_CURRENT_USER, "Software\\OchakkoLab\\Mameroid3D_ver0017", 0, KEY_ALL_ACCESS, &s_hkey );
	if( lret == ERROR_SUCCESS ){
		DWORD dwtype;
		char strkey[37] = {0};
		DWORD dwsize = 37;
		lret = RegQueryValueExA( s_hkey, "registkey", NULL, &dwtype, (LPBYTE)strkey, &dwsize );
		if( lret == ERROR_SUCCESS ){
			if( dwtype == REG_SZ ){
				if( strncmp( strkey, s_appkey[ s_appindex ], 36 ) == 0 ){
					s_registflag = 1;
				}
			}
		}
		RegCloseKey( s_hkey );
	}

	return 0;

}




float g_initcamz = 10.0f;
static float s_fAspectRatio = 1.0f;
static float s_projnear = 0.001f;


using namespace OrgWinGUI;

typedef struct tag_selmorph
{
	CMQOObject* target;
	CMorphKey* mk;
}SELMORPH;

typedef struct tag_spaxis
{
	CMySprite* sprite;
	POINT dispcenter;
}SPAXIS;

static int s_fbxbunki = 1;
static float s_cammvstep = 100.0f;
static WINPOS s_winpos[WINPOS_MAX];
static HBITMAP s_tlprop_bmp = NULL;
static HBITMAP s_tlscale_bmp = NULL;

static int s_editmotionflag = 0;

static int s_mainwidth = 620;
static int s_mainheight = 620;

static bool s_oneditmorph = false;
static SELMORPH s_selmorph = {0,0};

static int m_frompnd = 0;
static CKinectMain* s_kinect = 0;

static WCHAR s_rdbpath[MAX_PATH] = {0};
static WCHAR s_pndpath[MAX_PATH] = {0};
static char s_pndkey[PNDKEYLENG] = {0};
static WCHAR s_rdbsavename[64] = {0};
static WCHAR s_rdbsavedir[MAX_PATH] = {0};

static double s_time = 0.0;
static double s_difftime = 0.0;
static int s_ikkind = 0;
static int s_iklevel = 0;
static int s_splevel = 1;

static PICKINFO s_pickinfo;
static vector<TLELEM> s_tlarray;
static int s_curmotmenuindex = -1;

static HWND		s_mainwnd = 0;
static HMENU	s_mainmenu = 0;
static HMENU	s_animmenu = 0;
static HMENU	s_modelmenu = 0;
static int s_filterindex = 1;

static LPDIRECT3DDEVICE9 s_pdev = 0;

static CModel* s_model = NULL;
static CModel* s_select = NULL;
static CModel* s_bmark = NULL;
static CModel* s_ground = NULL;
static CModel* s_dummytri = NULL;
static CMySprite* s_bcircle = 0;
static CMySprite* s_kinsprite = 0;
static SPAXIS s_spaxis[3];


static double s_timescale = 8.0;

static int s_curboneno = -1;
static int s_curbaseno = -1;

static OrgWindow* s_timelineWnd = 0;
static OWP_Timeline* s_owpTimeline = 0;
static OWP_PlayerButton* s_owpPlayerButton = 0;

static OrgWindow* s_MtimelineWnd = 0;
static OWP_Timeline* s_owpMTimeline = 0;
static OWP_PlayerButton* s_owpMPlayerButton = 0;

static OrgWindow* s_LtimelineWnd = 0;
static OWP_Timeline* s_owpLTimeline = 0;
static OWP_PlayerButton* s_owpLPlayerButton = 0;

static OrgWindow* s_toolWnd = 0;		
static OWP_Button* s_toolCopyB = 0;
static OWP_Button* s_toolSymCopyB = 0;
static OWP_Button* s_toolSymCopy2B = 0;
static OWP_Button* s_toolCutB = 0;
static OWP_Button* s_toolPasteB = 0;
static OWP_Button* s_toolDeleteB = 0;
static OWP_Button* s_toolNewKeyB = 0;
static OWP_Button* s_toolNewKeyAllB = 0;
static OWP_Button* s_toolInitKeyB = 0;

static OWP_Button* s_toolMNewKeyB = 0;
static OWP_Button* s_toolMNewKeyAllB = 0;

static OrgWindow* s_morphWnd = 0;
static OWP_Slider* s_morphSlider = 0;
static OWP_Button* s_morphOneB = 0;
static OWP_Button* s_morphOneExB = 0;
static OWP_Button* s_morphZeroB = 0;
static OWP_Button* s_morphCalcB = 0;
static OWP_Button* s_morphDelB = 0;
static OWP_Button* s_morphCloseB = 0;
static bool s_closemorphFlag = false;

static OrgWindow* s_layerWnd = 0;
static OWP_LayerTable* s_owpLayerTable = 0;

static bool s_closeFlag = false;			// 終了フラグ
static bool s_closetoolFlag = false;
static bool s_closeobjFlag = false;
static bool s_closemodelFlag = false;
static bool s_copyFlag = false;			// コピーフラグ
static int s_symcopyFlag = 0;
static bool s_cutFlag = false;			// カットフラグ
static bool s_pasteFlag = false;			// ペーストフラグ
static bool s_cursorFlag = false;			// カーソル移動フラグ
static bool s_selectFlag = false;			// キー選択フラグ
static bool s_keyShiftFlag = false;		// キー移動フラグ
static bool s_deleteFlag = false;		// キー削除フラグ
static bool s_newkeyFlag = false;
static bool s_newkeyallFlag = false;
static bool s_initkeyFlag = false;
static bool s_motpropFlag = false;
static bool s_underpropFlag = false;

static int s_previewFlag = 0;			// プレビューフラグ
static bool s_befkeyFlag = false;
static bool s_nextkeyFlag = false;

static bool s_McloseFlag = false;
static bool s_MnextkeyFlag = false;
static bool s_MbefkeyFlag = false;
//static bool s_copyFlag = false;			// コピーフラグ
//static bool s_cutFlag = false;			// カットフラグ
//static bool s_pasteFlag = false;			// ペーストフラグ
static bool s_McursorFlag = false;			// カーソル移動フラグ
static bool s_MselectFlag = false;			// キー選択フラグ
static bool s_MkeyShiftFlag = false;		// キー移動フラグ
//static bool s_MdeleteFlag = false;		// キー削除フラグ
static bool s_MnewkeyFlag = false;
static bool s_MnewkeyallFlag = false;
//static bool s_initkeyFlag = false;

static bool s_LcloseFlag = false;
static bool s_LnextkeyFlag = false;
static bool s_LbefkeyFlag = false;
static bool s_LcursorFlag = false;			// カーソル移動フラグ



static bool s_dispmw = true;
static bool s_disptool = true;
static bool s_dispobj = true;
static bool s_dispmodel = true;
static bool s_dispground = true;
static bool s_dispselect = true;
static bool s_displightarrow = true;
static bool s_Mdispmw = true;
static bool s_Ldispmw = true;
static bool s_dispjoint = true;

static double s_keyShiftTime = 0.0;			// キー移動量
static list<OWP_Timeline::KeyInfo> s_copyKeyInfoList;	// コピーされたキー情報リスト
static list<OWP_Timeline::KeyInfo> s_deletedKeyInfoList;	// 削除されたキー情報リスト
static list<OWP_Timeline::KeyInfo> s_selectKeyInfoList;	// コピーされたキー情報リスト

static double s_MkeyShiftTime = 0.0;			// キー移動量
static list<OWP_Timeline::KeyInfo> s_McopyKeyInfoList;	// コピーされたキー情報リスト
static list<OWP_Timeline::KeyInfo> s_MdeletedKeyInfoList;	// 削除されたキー情報リスト
static list<OWP_Timeline::KeyInfo> s_MselectKeyInfoList;	// コピーされたキー情報リスト


typedef struct tag_modelpanel
{
	OrgWindow* panel;
	OWP_RadioButton* radiobutton;
	OWP_Separator* separator;
	vector<OWP_CheckBox*> checkvec;
	int modelindex;
}MODELPANEL;

static MODELPANEL s_modelpanel;

static map<int, int> s_lineno2boneno;
static map<int, int> s_boneno2lineno;

static vector<MODELELEM> s_modelindex;
static MODELBOUND	s_totalmb;
static int s_curmodelmenuindex = -1;

static WCHAR s_tmpmotname[256] = {0L};
static double s_tmpmotframeleng = 100.0f;
static int s_tmpmotloop = 0;

static WCHAR s_projectname[64] = {0L};
static WCHAR s_projectdir[MAX_PATH + 1] = {0L};

static int s_camtargetflag = 0;
CDXUTCheckBox* s_CamTargetCheckBox = 0;
CDXUTCheckBox* s_LightCheckBox = 0;
D3DXVECTOR3 s_camtargetpos = D3DXVECTOR3( 0.0f, 0.0f, 0.0f );


//#define DEBUG_VS   // Uncomment this line to debug vertex shaders 
//#define DEBUG_PS   // Uncomment this line to debug pixel shaders 

D3DXVECTOR3		g_vCenter;
CTexBank*	g_texbank = 0;

float g_tmpmqomult = 1.0f;
WCHAR g_tmpmqopath[MULTIPATH] = {0L};


//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
ID3DXFont*                  g_pFont = NULL;         // Font for drawing text
ID3DXSprite*                g_pSprite = NULL;       // Sprite for batching draw text calls
bool                        g_bShowHelp = true;     // If true, it renders the UI control text
CModelViewerCamera          g_Camera;               // A model viewing camera
ID3DXEffect*                g_pEffect = NULL;       // D3DX effect interface
//ID3DXMesh*                  g_pMesh = NULL;         // Mesh object
//IDirect3DTexture9*          g_pMeshTexture = NULL;  // Mesh texture



CDXUTDialogResourceManager  g_DialogResourceManager; // manager for shared resources of dialogs
CD3DSettingsDlg             g_SettingsDlg;          // Device settings dialog
CDXUTDialog                 g_HUD;                  // manages the 3D UI
CDXUTDialog                 g_SampleUI;             // dialog for sample specific controls
bool                        g_bEnablePreshader;     // if TRUE, then D3DXSHADER_NO_PRESHADER is used when compiling the shader
D3DXMATRIXA16               g_mCenterWorld;

#define MAX_LIGHTS 3
CDXUTDirectionWidget g_LightControl[MAX_LIGHTS];
float                       g_fLightScale;
int                         g_nNumActiveLights;
int                         g_nActiveLight;

double						g_dspeed = 1.0;

D3DXHANDLE g_hRenderBoneL0 = 0;
D3DXHANDLE g_hRenderBoneL1 = 0;
D3DXHANDLE g_hRenderBoneL2 = 0;
D3DXHANDLE g_hRenderBoneL3 = 0;
D3DXHANDLE g_hRenderNoBoneL0 = 0;
D3DXHANDLE g_hRenderNoBoneL1 = 0;
D3DXHANDLE g_hRenderNoBoneL2 = 0;
D3DXHANDLE g_hRenderNoBoneL3 = 0;
D3DXHANDLE g_hRenderLine = 0;
D3DXHANDLE g_hRenderSprite = 0;

D3DXHANDLE g_hmBoneQ = 0;
D3DXHANDLE g_hmBoneTra = 0;
D3DXHANDLE g_hmWorld = 0;
D3DXHANDLE g_hmVP = 0;
D3DXHANDLE g_hEyePos = 0;
D3DXHANDLE g_hnNumLight = 0;
D3DXHANDLE g_hLightDir = 0;
D3DXHANDLE g_hLightDiffuse = 0;
D3DXHANDLE g_hLightAmbient = 0;

D3DXHANDLE g_hdiffuse = 0;
D3DXHANDLE g_hambient = 0;
D3DXHANDLE g_hspecular = 0;
D3DXHANDLE g_hpower = 0;
D3DXHANDLE g_hemissive = 0;
D3DXHANDLE g_hMeshTexture = 0;

BYTE g_keybuf[256];
BYTE g_befkeybuf[256];

WCHAR g_basedir[ MAX_PATH ] = {0L};
WCHAR g_mediadir[ MAX_PATH ] = {0L};

bool g_controlkey = false;

//--------------------------------------------------------------------------------------
// UI control IDs
//--------------------------------------------------------------------------------------
#define IDC_TOGGLEFULLSCREEN    1
#define IDC_TOGGLEREF           3
#define IDC_CHANGEDEVICE        4
#define IDC_ENABLE_PRESHADER    5
#define IDC_NUM_LIGHTS          6
#define IDC_NUM_LIGHTS_STATIC   7
#define IDC_ACTIVE_LIGHT        8
#define IDC_LIGHT_SCALE         9
#define IDC_LIGHT_SCALE_STATIC  10

#define IDC_COMBO_BONE			11
#define IDC_FK_XP				12
#define IDC_FK_XM				13
#define IDC_FK_YP				14
#define IDC_FK_YM				15
#define IDC_FK_ZP				16
#define IDC_FK_ZM				17

#define IDC_T_XP				18
#define IDC_T_XM				19
#define IDC_T_YP				20
#define IDC_T_YM				21
#define IDC_T_ZP				22
#define IDC_T_ZM				23

#define IDC_SPEED_STATIC		24
#define IDC_SPEED				25

#define IDC_CAMTARGET			26

#define IDC_IKKIND_GROUP		27
#define IDC_IK_ROT				28
#define IDC_IK_MV				29
#define IDC_IK_LIGHT			30

#define IDC_STATIC_IKLEVEL		31
#define IDC_COMBO_IKLEVEL		32
#define IDC_STATIC_SPLEVEL		33
#define IDC_COMBO_SPLEVEL		34
#define IDC_LIGHT_DISP			35

//--------------------------------------------------------------------------------------
// Forward declarations 
//--------------------------------------------------------------------------------------
bool CALLBACK IsDeviceAcceptable( D3DCAPS9* pCaps, D3DFORMAT AdapterFormat, D3DFORMAT BackBufferFormat, bool bWindowed,
                                  void* pUserContext );
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext );
HRESULT CALLBACK OnCreateDevice( IDirect3DDevice9* pd3dDevice, const D3DSURFACE_DESC* pBackBufferSurfaceDesc,
                                 void* pUserContext );
HRESULT CALLBACK OnResetDevice( IDirect3DDevice9* pd3dDevice, const D3DSURFACE_DESC* pBackBufferSurfaceDesc,
                                void* pUserContext );
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext );
void CALLBACK OnFrameRender( IDirect3DDevice9* pd3dDevice, double fTime, float fElapsedTime, void* pUserContext );
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing,
                          void* pUserContext );
void CALLBACK KeyboardProc( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext );
void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext );
void CALLBACK OnLostDevice( void* pUserContext );
void CALLBACK OnDestroyDevice( void* pUserContext );


LRESULT CALLBACK OpenMqoDlgProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK MotPropDlgProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK SaveRdbDlgProc(HWND hDlgWnd, UINT msg, WPARAM wp, LPARAM lp);
LRESULT CALLBACK ExportXDlgProc(HWND hDlgWnd, UINT msg, WPARAM wp, LPARAM lp);
LRESULT CALLBACK ExportFBXDlgProc(HWND hDlgWnd, UINT msg, WPARAM wp, LPARAM lp);
LRESULT CALLBACK PndConvDlgProc(HWND hDlgWnd, UINT msg, WPARAM wp, LPARAM lp);
LRESULT CALLBACK PndLoadDlgProc(HWND hDlgWnd, UINT msg, WPARAM wp, LPARAM lp);


void InitApp();
HRESULT LoadMesh( IDirect3DDevice9* pd3dDevice, WCHAR* strFileName, ID3DXMesh** ppMesh );
void RenderText( double fTime );

CModel* OpenMQOFile();
CModel* OpenMQOFileFromPnd( CPmCipherDll* cipher, char* modelfolder, char* mqopath );
int OpenQubFile();
int OpenQubFileFromPnd( CPmCipherDll* cipher, char* motpath );
int OnDelMotion( int delindex );


static int GetShaderHandle();
static int SetBaseDir();

static int OpenFile();
static int OpenRdbFile();
static int SaveProject();
static int PndConv();
static int PndLoad();
static int ExportXFile();
static int ExportColladaFile();
static int ExportFBXFile();

static void refreshTimeline(OWP_Timeline& timeline, OWP_Timeline& Mtimeline, int rbundoflag = 0 );
static int AddBoneEul( float addx, float addy, float addz );
static int AddBoneTra( float addx, float addy, float addz );
static int DispMotionWindow();
static int DispMorphMotionWindow();
static int DispLongMotionWindow();
static int DispToolWindow();
static int DispObjPanel();
static int DispModelPanel();
static int EraseKeyList();
static int DestroyTimeLine( int dellist );
static int AddTimeLine( int newmotid );
static int AddMotion( WCHAR* wfilename );
static int OnAnimMenu( int selindex, int saveundoflag = 1 );
static int AddModelBound( MODELBOUND* mb, MODELBOUND* addmb );

static int OnModelMenu( int selindex, int callbymenu );
static int OnDelModel( int delindex );
static int OnDelAllModel();
static int refreshModelPanel();
static int RenderSelectMark();
static int SetSelectState();

static int CreateModelPanel();
static int DestroyModelPanel();

static int CalcTargetPos( D3DXVECTOR3* dstpos );
static int CalcPickRay( D3DXVECTOR3* start3d, D3DXVECTOR3* end3d );
static void ActivatePanel( int state, int fromtlflag = 0 );
static int SetCamera6Angle();

static int LineNo2BaseNo( int lineno, int* basenoptr );
static int SetSpAxisParams();
static int PickSpAxis( POINT srcpos );

static int OnPreviewStop();
static int ChangeTimeScale();
static int ChangeTimeLineScale();
static int ChangeMTimeLineScale();
static int ChangeLTimeLineScale();
//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
INT WINAPI wWinMain( HINSTANCE, HINSTANCE, LPWSTR, int )
{
    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

	OpenDbgFile();
//_CrtSetBreakAlloc(2806);
//_CrtSetBreakAlloc(758);
//_CrtSetBreakAlloc(826);

	SetBaseDir();
	
	int ret;
	CRdbIniFile inifile;
	ret = inifile.LoadRdbIniFile();
	if( !ret ){
		MoveMemory( s_winpos, inifile.m_winpos, sizeof( WINPOS ) * WINPOS_MAX );
	}else{
		MoveMemory( s_winpos, inifile.m_defaultwp, sizeof( WINPOS ) * WINPOS_MAX );
	}
	s_mainwidth = s_winpos[WINPOS_3D].width;
	s_mainheight = s_winpos[WINPOS_3D].height;

	
    // Set the callback functions. These functions allow DXUT to notify
    // the application about device changes, user input, and windows messages.  The 
    // callbacks are optional so you need only set callbacks for events you're interested 
    // in. However, if you don't handle the device reset/lost callbacks then the sample 
    // framework won't be able to reset your device since the application must first 
    // release all device resources before resetting.  Likewise, if you don't handle the 
    // device created/destroyed callbacks then DXUT won't be able to 
    // recreate your device resources.
    DXUTSetCallbackD3D9DeviceAcceptable( IsDeviceAcceptable );
    DXUTSetCallbackD3D9DeviceCreated( OnCreateDevice );
    DXUTSetCallbackD3D9DeviceReset( OnResetDevice );
    DXUTSetCallbackD3D9FrameRender( OnFrameRender );
    DXUTSetCallbackD3D9DeviceLost( OnLostDevice );
    DXUTSetCallbackD3D9DeviceDestroyed( OnDestroyDevice );
    DXUTSetCallbackMsgProc( MsgProc );
    DXUTSetCallbackKeyboard( KeyboardProc );
    DXUTSetCallbackFrameMove( OnFrameMove );
    DXUTSetCallbackDeviceChanging( ModifyDeviceSettings );

    // Show the cursor and clip it when in full screen
    DXUTSetCursorSettings( true, true );

    InitApp();

	IsRegist();

	s_mainmenu = LoadMenuW( (HINSTANCE)GetModuleHandle(NULL), MAKEINTRESOURCE( IDR_MENU1 ) );
	if( s_mainmenu == NULL ){
		_ASSERT( 0 );
		return 1;
	}

    // Initialize DXUT and create the desired Win32 window and Direct3D 
    // device for the application. Calling each of these functions is optional, but they
    // allow you to set several options which control the behavior of the framework.
    DXUTInit( true, true ); // Parse the command line and show msgboxes
    DXUTSetHotkeyHandling( true, true, true );
    DXUTCreateWindow( L"Mameroid3D", 0, 0, s_mainmenu );
	s_mainwnd = DXUTGetHWND();
	DXUTCreateDevice( true, s_mainwidth, s_mainheight );
	_ASSERT( s_mainwnd );
	
	SetWindowPos( s_mainwnd, HWND_TOP, s_winpos[WINPOS_3D].posx, s_winpos[WINPOS_3D].posy, s_mainwidth, s_mainheight, SWP_NOSIZE ); 


	//animmenu
	HMENU motmenu;
	motmenu = GetSubMenu( s_mainmenu, 2 );
	s_animmenu = GetSubMenu( motmenu, 3 );
	_ASSERT( s_animmenu );

	HMENU mdlmenu = GetSubMenu( s_mainmenu, 3 );
	s_modelmenu = GetSubMenu( mdlmenu, 3 );

	CallF( InitializeSdkObjects(), return 1 );


    // Pass control to DXUT for handling the message pump and 
    // dispatching render calls. DXUT will call your FrameMove 
    // and FrameRender callback when there is idle time between handling window messages.
    DXUTMainLoop();

    // Perform any application-level cleanup here. Direct3D device resources are released within the
    // appropriate callback functions and therefore don't require any cleanup code here


    return DXUTGetExitCode();
}


//--------------------------------------------------------------------------------------
// Initialize the app 
//--------------------------------------------------------------------------------------
void InitApp()
{
	WCHAR tlpropname[MAX_PATH] = {0L};
	WCHAR tlscalename[MAX_PATH] = {0L};

	swprintf_s( tlpropname, MAX_PATH, L"%sTL_Prop.bmp", g_mediadir );
	swprintf_s( tlscalename, MAX_PATH, L"%sTL_Scale.bmp", g_mediadir );

	s_tlprop_bmp = (HBITMAP)LoadImage( (HINSTANCE)GetModuleHandle(NULL), tlpropname, IMAGE_BITMAP, 20, 20, LR_LOADFROMFILE );
	if( s_tlprop_bmp == NULL ){
		_ASSERT( 0 );
	}
	s_tlscale_bmp = (HBITMAP)LoadImage( (HINSTANCE)GetModuleHandle(NULL), tlscalename, IMAGE_BITMAP, 40, 20, LR_LOADFROMFILE );
	if( s_tlprop_bmp == NULL ){
		_ASSERT( 0 );
	}


    g_bEnablePreshader = true;

	ZeroMemory( s_spaxis, sizeof( SPAXIS ) * 3 );

    for( int i = 0; i < MAX_LIGHTS; i++ )
        g_LightControl[i].SetLightDirection( D3DXVECTOR3( sinf( D3DX_PI * 2 * i / MAX_LIGHTS - D3DX_PI / 6 ),
                                                          0, -cosf( D3DX_PI * 2 * i / MAX_LIGHTS - D3DX_PI / 6 ) ) );

    g_nActiveLight = 0;
    g_nNumActiveLights = 1;
    g_fLightScale = 0.5f;

    // Initialize dialogs
    g_SettingsDlg.Init( &g_DialogResourceManager );
    g_SampleUI.Init( &g_DialogResourceManager );

	int iY;
    g_SampleUI.SetCallback( OnGUIEvent ); iY = 0;

    WCHAR sz[100];
    swprintf_s( sz, 100, L"# Lights: %d", g_nNumActiveLights );
    g_SampleUI.AddStatic( IDC_NUM_LIGHTS_STATIC, sz, 35, 0, 125, 22 );
    g_SampleUI.AddSlider( IDC_NUM_LIGHTS, 50, iY += 24, 100, 22, 1, MAX_LIGHTS, g_nNumActiveLights );

    //iY += 24;
    swprintf_s( sz, 100, L"Light scale: %0.2f", g_fLightScale );
    g_SampleUI.AddStatic( IDC_LIGHT_SCALE_STATIC, sz, 35, iY += 24, 125, 22 );
    g_SampleUI.AddSlider( IDC_LIGHT_SCALE, 50, iY += 24, 100, 22, 0, 20, ( int )( g_fLightScale * 10.0f ) );

    //iY += 24;
    g_SampleUI.AddButton( IDC_ACTIVE_LIGHT, L"Change active light (K)", 35, iY += 24, 125, 22, 'K' );

	g_SampleUI.AddCheckBox( IDC_LIGHT_DISP, L"ライト矢印を表示する", 25, iY += 24, 450, 16, true, 0U, false, &s_LightCheckBox );

	//iY += 24;
	g_SampleUI.AddComboBox( IDC_COMBO_BONE, 35, iY += 24, 125, 22 );
	g_SampleUI.AddButton( IDC_FK_XP, L"Rot X+", 35, iY += 24, 60, 22 );
	g_SampleUI.AddButton( IDC_FK_XM, L"Rot X-", 100, iY, 60, 22 );
	g_SampleUI.AddButton( IDC_FK_YP, L"Rot Y+", 35, iY += 24, 60, 22 );
	g_SampleUI.AddButton( IDC_FK_YM, L"Rot Y-", 100, iY, 60, 22 );
	g_SampleUI.AddButton( IDC_FK_ZP, L"Rot Z+", 35, iY += 24, 60, 22 );
	g_SampleUI.AddButton( IDC_FK_ZM, L"Rot Z-",100, iY, 60, 22 );

	//iY += 24;

	g_SampleUI.AddButton( IDC_T_XP, L"Tra X+", 35, iY += 24, 60, 22 );
	g_SampleUI.AddButton( IDC_T_XM, L"Tra X-", 100, iY, 60, 22 );
	g_SampleUI.AddButton( IDC_T_YP, L"Tra Y+", 35, iY += 24, 60, 22 );
	g_SampleUI.AddButton( IDC_T_YM, L"Tra Y-", 100, iY, 60, 22 );
	g_SampleUI.AddButton( IDC_T_ZP, L"Tra Z+", 35, iY += 24, 60, 22 );
	g_SampleUI.AddButton( IDC_T_ZM, L"Tra Z-",100, iY, 60, 22 );

	//iY += 24;

    swprintf_s( sz, 100, L"Motion Speed: %0.2f", g_dspeed );
    g_SampleUI.AddStatic( IDC_SPEED_STATIC, sz, 35, iY += 24, 125, 22 );
    g_SampleUI.AddSlider( IDC_SPEED, 50, iY += 24, 100, 22, 3, 30, ( int )( g_dspeed * 10.0f ) );

	g_SampleUI.AddCheckBox( IDC_CAMTARGET, L"選択部を注視点にする", 25, iY += 24, 450, 16, false, 0U, false, &s_CamTargetCheckBox );


	//iY += 24;
    g_SampleUI.AddRadioButton( IDC_IK_ROT, IDC_IKKIND_GROUP, L"回転IK", 50, iY += 24, 300, 16 );
    g_SampleUI.AddRadioButton( IDC_IK_MV, IDC_IKKIND_GROUP, L"移動IK", 50, iY += 24, 300, 16 );
    g_SampleUI.AddRadioButton( IDC_IK_LIGHT, IDC_IKKIND_GROUP, L"ライト回転", 50, iY += 24, 300, 16 );
    CDXUTRadioButton* pRadioButtonRot = g_SampleUI.GetRadioButton( IDC_IK_ROT );
    pRadioButtonRot->SetChecked( 1 );
    CDXUTRadioButton* pRadioButtonMv = g_SampleUI.GetRadioButton( IDC_IK_MV );
    pRadioButtonMv->SetChecked( 0 );
    CDXUTRadioButton* pRadioButtonL = g_SampleUI.GetRadioButton( IDC_IK_LIGHT );
    pRadioButtonL->SetChecked( 0 );


    wcscpy_s( sz, 100, L"Ring IK Level  ↓" );
    g_SampleUI.AddStatic( IDC_STATIC_IKLEVEL, sz, 35, iY += 24, 125, 22 );
	g_SampleUI.AddComboBox( IDC_COMBO_IKLEVEL, 35, iY += 24, 125, 22 );
    CDXUTComboBox* pComboBox0 = g_SampleUI.GetComboBox( IDC_COMBO_IKLEVEL );
	pComboBox0->RemoveAllItems();
	int level;
	for( level = 0; level < 15; level++ ){
		ULONG levelval = (ULONG)level;
		WCHAR strlevel[256];
		swprintf_s( strlevel, 256, L"%02d", level );
		pComboBox0->AddItem( strlevel, ULongToPtr( levelval ) );
	}
	pComboBox0->SetSelectedByData( ULongToPtr( 0 ) );

	//iY += 24;
    wcscpy_s( sz, 100, L"Sprite IK Level  ↓" );
    g_SampleUI.AddStatic( IDC_STATIC_IKLEVEL, sz, 35, iY += 24, 125, 22 );
	g_SampleUI.AddComboBox( IDC_COMBO_SPLEVEL, 35, iY += 24, 125, 22 );
    CDXUTComboBox* pComboBox1 = g_SampleUI.GetComboBox( IDC_COMBO_SPLEVEL );
	pComboBox1->RemoveAllItems();
	int splevel;
	for( splevel = 0; splevel < 15; splevel++ ){
		ULONG levelval = (ULONG)splevel;
		WCHAR strlevel[256];
		swprintf_s( strlevel, 256, L"%02d", splevel );
		pComboBox1->AddItem( strlevel, ULongToPtr( levelval ) );
	}
	pComboBox1->SetSelectedByData( ULongToPtr( 1 ) );


	s_timelineWnd = new OrgWindow(
							0,
							L"TimeLine",				//ウィンドウクラス名
							GetModuleHandle(NULL),	//インスタンスハンドル
							WindowPos(s_winpos[WINPOS_TL].posx, s_winpos[WINPOS_TL].posy),		//位置
							WindowSize(s_winpos[WINPOS_TL].width,s_winpos[WINPOS_TL].height),	//サイズ
							L"TimeLine",				//タイトル
							s_mainwnd,					//親ウィンドウハンドル
							true,					//表示・非表示状態
							70,50,70 );				//カラー

	s_timelineWnd->setSizeMin( OrgWinGUI::WindowSize( s_winpos[WINPOS_TL].minwidth, s_winpos[WINPOS_TL].minheight ) );


	// ウィンドウの閉じるボタンのイベントリスナーに
	// 終了フラグcloseFlagをオンにするラムダ関数を登録する
	s_timelineWnd->setCloseListener( [](){ s_closeFlag=true; } );

	// ウィンドウのキーボードイベントリスナーに
	// コピー/カット/ペーストフラグcopyFlag/cutFlag/pasteFlagをオンにするラムダ関数を登録する
	// コピー等のキーボードを使用する処理はキーボードイベントリスナーを使用しなくても
	// メインループ内でマイフレームキー状態を監視することで作成可能である。
	s_timelineWnd->setKeyboardEventListener( [](const KeyboardEvent &e){
		if( e.ctrlKey && !e.repeat && e.onDown ){
			switch( e.keyCode ){
			case 'C': 
				s_copyFlag=true; 
				break;
			case 'B':
				s_symcopyFlag=1;
				break;
			case 'X': 
				s_cutFlag=true; 
				break;
			case 'V': 
				s_pasteFlag=true; 
				break;
			case 'P': 
				s_previewFlag=1;
				break;
			case 'S': 
				s_previewFlag=0; 
				break;
			case 'D':
				s_deleteFlag=true;
				break;
			default:
				break;
			}
		}
	} );

	s_owpPlayerButton = new OWP_PlayerButton( s_tlprop_bmp, s_tlscale_bmp );
	s_owpPlayerButton->setButtonSize(20);
	s_timelineWnd->addParts(*s_owpPlayerButton);

	s_owpPlayerButton->setFrontPlayButtonListener( [](){ s_previewFlag = 1; } );
	s_owpPlayerButton->setBackPlayButtonListener( [](){  s_previewFlag = -1; } );
	s_owpPlayerButton->setFrontStepButtonListener( [](){ s_nextkeyFlag = true; } );
	s_owpPlayerButton->setBackStepButtonListener( [](){  s_befkeyFlag = true; } );
	s_owpPlayerButton->setStopButtonListener( [](){  s_previewFlag = 0; } );
	s_owpPlayerButton->setResetButtonListener( [](){ if( s_owpTimeline ){ OnPreviewStop(); } } );
	s_owpPlayerButton->setpropsetButtonListener( [](){ if( s_owpTimeline ){ s_motpropFlag = true; } } );
	s_owpPlayerButton->setscalesetButtonListener( [](){ if( s_owpTimeline ){ ChangeTimeLineScale(); } } );


////////// morph time line
	s_MtimelineWnd = new OrgWindow(
							0,
							L"MorphTimeLine",				//ウィンドウクラス名
							GetModuleHandle(NULL),	//インスタンスハンドル
							WindowPos(s_winpos[WINPOS_MTL].posx, s_winpos[WINPOS_MTL].posy),		//位置
							WindowSize(s_winpos[WINPOS_MTL].width, s_winpos[WINPOS_MTL].height),	//サイズ
							L"MorphTimeLine",				//タイトル
							s_mainwnd,					//親ウィンドウハンドル
							true,					//表示・非表示状態
							70,50,70 );				//カラー

	s_MtimelineWnd->setSizeMin( OrgWinGUI::WindowSize( s_winpos[WINPOS_MTL].minwidth, s_winpos[WINPOS_MTL].minheight ) );

	s_MtimelineWnd->setCloseListener( [](){ s_McloseFlag=true; } );

	s_owpMPlayerButton = new OWP_PlayerButton( s_tlprop_bmp, s_tlscale_bmp );
	s_owpMPlayerButton->setButtonSize(20);
	s_MtimelineWnd->addParts(*s_owpMPlayerButton);

	s_owpMPlayerButton->setFrontPlayButtonListener( [](){ s_previewFlag = 1; } );
	s_owpMPlayerButton->setBackPlayButtonListener( [](){  s_previewFlag = -1; } );
	s_owpMPlayerButton->setFrontStepButtonListener( [](){ s_MnextkeyFlag = true; } );
	s_owpMPlayerButton->setBackStepButtonListener( [](){  s_MbefkeyFlag = true; } );
	s_owpMPlayerButton->setStopButtonListener( [](){  s_previewFlag = 0; } );
	s_owpMPlayerButton->setResetButtonListener( [](){ if( s_owpMTimeline ){ OnPreviewStop(); } } );
	s_owpMPlayerButton->setpropsetButtonListener( [](){ if( s_owpTimeline ){ s_motpropFlag = true; } } );
	s_owpMPlayerButton->setscalesetButtonListener( [](){ if( s_owpMTimeline ){ ChangeMTimeLineScale(); } } );


///////// Long Timeline
	s_LtimelineWnd = new OrgWindow(
							0,
							L"LongTimeLine",				//ウィンドウクラス名
							GetModuleHandle(NULL),	//インスタンスハンドル
							WindowPos(s_winpos[WINPOS_LTL].posx, s_winpos[WINPOS_LTL].posy),		//位置
							WindowSize(s_winpos[WINPOS_LTL].width, s_winpos[WINPOS_LTL].height),	//サイズ
							L"Long TimeLine",				//タイトル
							s_mainwnd,					//親ウィンドウハンドル
							true,					//表示・非表示状態
							70,50,70 );				//カラー


	s_LtimelineWnd->setSizeMin( OrgWinGUI::WindowSize( s_winpos[WINPOS_LTL].minwidth, s_winpos[WINPOS_LTL].minheight ) );


	// ウィンドウの閉じるボタンのイベントリスナーに
	// 終了フラグcloseFlagをオンにするラムダ関数を登録する
	s_LtimelineWnd->setCloseListener( [](){ s_LcloseFlag=true; } );

	s_owpLPlayerButton = new OWP_PlayerButton( s_tlprop_bmp, s_tlscale_bmp );
	s_owpLPlayerButton->setButtonSize(20);
	s_LtimelineWnd->addParts(*s_owpLPlayerButton);

	s_owpLPlayerButton->setFrontPlayButtonListener( [](){ s_previewFlag = 1; } );
	s_owpLPlayerButton->setBackPlayButtonListener( [](){  s_previewFlag = -1; } );
	s_owpLPlayerButton->setFrontStepButtonListener( [](){ s_LnextkeyFlag = true; } );
	s_owpLPlayerButton->setBackStepButtonListener( [](){  s_LbefkeyFlag = true; } );
	s_owpLPlayerButton->setStopButtonListener( [](){  s_previewFlag = 0; } );
	s_owpLPlayerButton->setResetButtonListener( [](){ if( s_owpLTimeline ){ OnPreviewStop(); } } );
	s_owpLPlayerButton->setpropsetButtonListener( [](){ if( s_owpTimeline ){ s_motpropFlag = true; } } );
	s_owpLPlayerButton->setscalesetButtonListener( [](){ if( s_owpLTimeline ){ ChangeLTimeLineScale(); } } );

/////////
	// ツールウィンドウを作成してボタン類を追加
	s_toolWnd= new OrgWindow(
							1,
							_T("ToolWindow"),		//ウィンドウクラス名
							GetModuleHandle(NULL),	//インスタンスハンドル
							WindowPos(s_winpos[WINPOS_TOOL].posx, s_winpos[WINPOS_TOOL].posy),		//位置
							WindowSize(s_winpos[WINPOS_TOOL].width, s_winpos[WINPOS_TOOL].height),		//サイズ
							_T("ツールウィンドウ"),	//タイトル
							s_timelineWnd->getHWnd(),	//親ウィンドウハンドル
							true,					//表示・非表示状態
							70,50,70,				//カラー
							true,					//閉じられるか否か
							false);					//サイズ変更の可否

	s_toolWnd->setSizeMin( OrgWinGUI::WindowSize( s_winpos[WINPOS_TOOL].minwidth, s_winpos[WINPOS_TOOL].minheight ) );


	s_toolCopyB = new OWP_Button(_T("コピー(範囲)"));
	s_toolSymCopyB = new OWP_Button(_T("対称Copy(範囲-他)"));
	s_toolSymCopy2B = new OWP_Button(_T("対称Copy(範囲-自)"));
	s_toolCutB = new OWP_Button(_T("カット(範囲)"));
	s_toolPasteB = new OWP_Button(_T("ペースト"));
	s_toolDeleteB = new OWP_Button(_T("削除(範囲)"));
	s_toolNewKeyB = new OWP_Button(_T("姿勢キー新規(１個)"));
	s_toolNewKeyAllB = new OWP_Button(_T("姿勢キー新規(全)"));
	s_toolMNewKeyB = new OWP_Button(_T("モーフキー新規(１個)"));
	s_toolMNewKeyAllB = new OWP_Button(_T("モーフキー新規(全)"));
	s_toolInitKeyB = new OWP_Button(_T("キー初期化(範囲)"));

	s_toolWnd->addParts(*s_toolCopyB);
	s_toolWnd->addParts(*s_toolSymCopyB);
	s_toolWnd->addParts(*s_toolSymCopy2B);
	s_toolWnd->addParts(*s_toolCutB);
	s_toolWnd->addParts(*s_toolPasteB);
	s_toolWnd->addParts(*s_toolDeleteB);
	s_toolWnd->addParts(*s_toolNewKeyB);
	s_toolWnd->addParts(*s_toolNewKeyAllB);
	s_toolWnd->addParts(*s_toolMNewKeyB);
	s_toolWnd->addParts(*s_toolMNewKeyAllB);
	s_toolWnd->addParts(*s_toolInitKeyB);

	s_toolWnd->setCloseListener( [](){ s_closetoolFlag=true; } );
	s_toolCopyB->setButtonListener( [](){ s_copyFlag = true; } );
	s_toolSymCopyB->setButtonListener( [](){ s_symcopyFlag = 1; } );
	s_toolSymCopy2B->setButtonListener( [](){ s_symcopyFlag = 2; } );
	s_toolCutB->setButtonListener( [](){ s_cutFlag = true; } );
	s_toolPasteB->setButtonListener( [](){ s_pasteFlag = true; } );
	s_toolDeleteB->setButtonListener( [](){ s_deleteFlag = true; } );
	s_toolNewKeyB->setButtonListener( [](){ s_newkeyFlag = true; } );
	s_toolNewKeyAllB->setButtonListener( [](){ s_newkeyallFlag = true; } );
	s_toolMNewKeyB->setButtonListener( [](){ s_MnewkeyFlag = true; } );
	s_toolMNewKeyAllB->setButtonListener( [](){ s_MnewkeyallFlag = true; } );
	s_toolInitKeyB->setButtonListener( [](){ s_initkeyFlag = true; } );

////
	s_morphWnd= new OrgWindow(
							1,
							_T("MorphWindow"),		//ウィンドウクラス名
							GetModuleHandle(NULL),	//インスタンスハンドル
							WindowPos(100, 400),		//位置
							WindowSize(150,10),		//サイズ
							_T("モーフウィンドウ"),	//タイトル
							s_MtimelineWnd->getHWnd(),	//親ウィンドウハンドル
							false,					//表示・非表示状態
							70,50,70,				//カラー
							true,					//閉じられるか否か
							false);					//サイズ変更の可否

	s_morphSlider= new OWP_Slider(0.0,1.0,0.0);
	s_morphOneB = new OWP_Button(_T("ブレンド1.0"));
	s_morphOneExB = new OWP_Button(_T("ブレンド排他的1.0"));
	s_morphZeroB = new OWP_Button(_T("ブレンド0.0"));
	s_morphCalcB = new OWP_Button(_T("ブレンド計算"));
	s_morphDelB = new OWP_Button(_T("ブレンド削除"));
	s_morphCloseB = new OWP_Button(_T("編集終了"));

	s_morphWnd->addParts(*s_morphSlider);
	s_morphWnd->addParts(*s_morphOneB);
	s_morphWnd->addParts(*s_morphOneExB);
	s_morphWnd->addParts(*s_morphZeroB);
	s_morphWnd->addParts(*s_morphCalcB);
	s_morphWnd->addParts(*s_morphDelB);
	s_morphWnd->addParts(*s_morphCloseB);

	s_morphWnd->setCloseListener( [](){  
		s_morphWnd->setVisible( 0 );
		if( s_owpMTimeline ){
			s_model->SaveUndoMotion( s_curboneno, s_curbaseno );
		}
	} );

	s_morphSlider->setCursorListener( [](){
		if( s_oneditmorph && s_selmorph.mk && s_selmorph.target ){
			float val = (float)s_morphSlider->getValue();
			s_selmorph.mk->SetBlendWeight( s_selmorph.target, val );

			s_morphWnd->callRewrite();						//再描画
//			s_morphWnd->setRewriteOnChangeFlag(true);		//再描画要求を再開

		}
	} );
	s_morphOneB->setButtonListener( [](){
		if( s_oneditmorph && s_selmorph.mk && s_selmorph.target ){
			s_selmorph.mk->SetBlendWeight( s_selmorph.target, 1.0f );
			s_morphSlider->setValue( 1.0 );
		}		
	} );
	s_morphOneExB->setButtonListener( [](){
		if( s_oneditmorph && s_selmorph.mk && s_selmorph.target ){
			s_selmorph.mk->SetBlendOneExclude( s_selmorph.target );
			s_morphSlider->setValue( 1.0 );
		}		
	} );
	s_morphZeroB->setButtonListener( [](){
		if( s_oneditmorph && s_selmorph.mk && s_selmorph.target ){
			s_selmorph.mk->SetBlendWeight( s_selmorph.target, 0.0f );
			s_morphSlider->setValue( 0.0 );
		}		
	} );
	s_morphCalcB->setButtonListener( [](){
		if( s_oneditmorph && s_selmorph.mk && s_selmorph.target ){
			float calcval = 0.0f;
			CallF( s_selmorph.mk->m_baseobj->SetMorphBlendFU( s_model->m_curmotinfo->motid, s_selmorph.mk->m_frame, s_selmorph.mk, s_selmorph.target, &calcval ), return );		
			s_morphSlider->setValue( calcval );
DbgOut( L"morph calc : %f\r\n", calcval );
		}		
	} );
	s_morphDelB->setButtonListener( [](){
		if( s_oneditmorph && s_selmorph.mk && s_selmorph.target ){
			s_selmorph.mk->DeleteBlendWeight( s_selmorph.target );
//			s_morphWnd->setVisible( false );
//			s_oneditmorph = false;
		}		
	} );
	s_morphCloseB->setButtonListener( [](){
		s_morphWnd->setVisible( false );
		s_oneditmorph = false;
		if( s_owpMTimeline && s_model ){
			s_model->SaveUndoMotion( s_curboneno, s_curbaseno );
		}
	} );

////
	// ウィンドウを作成
	s_layerWnd = new OrgWindow(
							1,
							_T("LayerTool"),		//ウィンドウクラス名
							GetModuleHandle(NULL),	//インスタンスハンドル
							WindowPos(s_winpos[WINPOS_OBJ].posx, s_winpos[WINPOS_OBJ].posy),		//位置
							WindowSize(s_winpos[WINPOS_OBJ].width, s_winpos[WINPOS_OBJ].height),		//サイズ
							_T("オブジェクトパネル"),	//タイトル
							NULL,					//親ウィンドウハンドル
							true,					//表示・非表示状態
							70,50,70,				//カラー
							true,					//閉じられるか否か
							true);					//サイズ変更の可否

	s_layerWnd->setSizeMin( OrgWinGUI::WindowSize( s_winpos[WINPOS_OBJ].minwidth, s_winpos[WINPOS_OBJ].minheight ) );

	// レイヤーウィンドウパーツを作成
	s_owpLayerTable= new OWP_LayerTable(_T("レイヤーテーブル"));
	WCHAR label[256];
	wcscpy_s( label, 256, L"dummy name" );
	s_owpLayerTable->newLine( label, 0 );

	// ウィンドウにウィンドウパーツを登録
	s_layerWnd->addParts(*s_owpLayerTable);


	s_layerWnd->setCloseListener( [](){ s_closeobjFlag=true; } );

	//レイヤーのカーソルリスナー
	s_owpLayerTable->setCursorListener( [](){
		//_tprintf_s( _T("CurrentLayer: Index=%3d Name=%s\n"),
		//			owpLayerTable->getCurrentLine(),
		//			owpLayerTable->getCurrentLineName().c_str() );
	} );

	//レイヤーの移動リスナー
	s_owpLayerTable->setLineShiftListener( [](int from, int to){
		//_tprintf_s( _T("ShiftLayer: fromIndex=%3d toIndex=%3d\n"), from, to );
	} );

	//レイヤーの可視状態変更リスナー
	s_owpLayerTable->setChangeVisibleListener( [](int index){
		if( s_model ){
			CMQOObject* curobj = (CMQOObject*)( s_owpLayerTable->getObj(index) );
			if( curobj ){
				if( s_owpLayerTable->getVisible(index) ){
					curobj->m_dispflag = 1;				
				}else{
					curobj->m_dispflag = 0;
				}
			}
		}
	} );

	//レイヤーのロック状態変更リスナー
	s_owpLayerTable->setChangeLockListener( [](int index){
		//if( owpLayerTable->getLock(index) ){
		//	_tprintf_s( _T("ChangeLock: Index=%3d Lock='True'  Name=%s\n"),
		//				index,
		//				owpLayerTable->getName(index).c_str() );
		//}else{
		//	_tprintf_s( _T("ChangeLock: Index=%3d Lock='False' Name=%s\n"),
		//				index,
		//				owpLayerTable->getName(index).c_str() );
		//}
	} );

	//レイヤーのプロパティコールリスナー
	s_owpLayerTable->setCallPropertyListener( [](int index){
		//_tprintf_s( _T("CallProperty: Index=%3d Name=%s\n"),
		//			index,
		//			owpLayerTable->getName(index).c_str() );
	} );

//////////
	ZeroMemory( &s_pickinfo, sizeof( PICKINFO ) );

	s_modelpanel.panel = 0;
	s_modelpanel.radiobutton = 0;
	s_modelpanel.separator = 0;
	s_modelpanel.checkvec.clear();
	s_modelpanel.modelindex = -1;

}


//--------------------------------------------------------------------------------------
// Called during device initialization, this code checks the device for some 
// minimum set of capabilities, and rejects those that don't pass by returning E_FAIL.
//------------------------------------------------
bool CALLBACK IsDeviceAcceptable( D3DCAPS9* pCaps, D3DFORMAT AdapterFormat,
                                  D3DFORMAT BackBufferFormat, bool bWindowed, void* pUserContext )
{
    // No fallback defined by this app, so reject any device that 
    // doesn't support at least ps2.0
    if( pCaps->PixelShaderVersion < D3DPS_VERSION( 3, 0 ) )
        return false;

	if( pCaps->MaxVertexIndex <= 0x0000FFFF ){
		return false;
	}

    // Skip backbuffer formats that don't support alpha blending
    IDirect3D9* pD3D = DXUTGetD3D9Object();
    if( FAILED( pD3D->CheckDeviceFormat( pCaps->AdapterOrdinal, pCaps->DeviceType,
                                         AdapterFormat, D3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING,
                                         D3DRTYPE_TEXTURE, BackBufferFormat ) ) )
        return false;

    return true;
}


//--------------------------------------------------------------------------------------
// This callback function is called immediately before a device is created to allow the 
// application to modify the device settings. The supplied pDeviceSettings parameter 
// contains the settings that the framework has selected for the new device, and the 
// application can make any desired changes directly to this structure.  Note however that 
// DXUT will not correct invalid device settings so care must be taken 
// to return valid device settings, otherwise IDirect3D9::CreateDevice() will fail.  
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext )
{
    assert( DXUT_D3D9_DEVICE == pDeviceSettings->ver );

    HRESULT hr;
    IDirect3D9* pD3D = DXUTGetD3D9Object();
    D3DCAPS9 caps;

    V( pD3D->GetDeviceCaps( pDeviceSettings->d3d9.AdapterOrdinal,
                            pDeviceSettings->d3d9.DeviceType,
                            &caps ) );

    // If device doesn't support HW T&L or doesn't support 1.1 vertex shaders in HW 
    // then switch to SWVP.
    if( ( caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT ) == 0 ||
        caps.VertexShaderVersion < D3DVS_VERSION( 1, 1 ) )
    {
        pDeviceSettings->d3d9.BehaviorFlags = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
    }

    // Debugging vertex shaders requires either REF or software vertex processing 
    // and debugging pixel shaders requires REF.  
#ifdef DEBUG_VS
    if( pDeviceSettings->d3d9.DeviceType != D3DDEVTYPE_REF )
    {
        pDeviceSettings->d3d9.BehaviorFlags &= ~D3DCREATE_HARDWARE_VERTEXPROCESSING;
        pDeviceSettings->d3d9.BehaviorFlags &= ~D3DCREATE_PUREDEVICE;
        pDeviceSettings->d3d9.BehaviorFlags |= D3DCREATE_SOFTWARE_VERTEXPROCESSING;
    }
#endif
#ifdef DEBUG_PS
    pDeviceSettings->d3d9.DeviceType = D3DDEVTYPE_REF;
#endif
    // For the first device created if its a REF device, optionally display a warning dialog box
    static bool s_bFirstTime = true;
    if( s_bFirstTime )
    {
        s_bFirstTime = false;
        if( pDeviceSettings->d3d9.DeviceType == D3DDEVTYPE_REF )
            DXUTDisplaySwitchingToREFWarning( pDeviceSettings->ver );
    }

    return true;
}


//--------------------------------------------------------------------------------------
// This callback function will be called immediately after the Direct3D device has been 
// created, which will happen during application initialization and windowed/full screen 
// toggles. This is the best location to create D3DPOOL_MANAGED resources since these 
// resources need to be reloaded whenever the device is destroyed. Resources created  
// here should be released in the OnDestroyDevice callback. 
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnCreateDevice( IDirect3DDevice9* pd3dDevice, const D3DSURFACE_DESC* pBackBufferSurfaceDesc,
                                 void* pUserContext )
{
    HRESULT hr;

	s_pdev = pd3dDevice;

    V_RETURN( g_DialogResourceManager.OnD3D9CreateDevice( pd3dDevice ) );
    V_RETURN( g_SettingsDlg.OnD3D9CreateDevice( pd3dDevice ) );
    // Initialize the font
    V_RETURN( D3DXCreateFont( pd3dDevice, 15, 0, FW_BOLD, 1, FALSE, DEFAULT_CHARSET,
                              OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                              L"Arial", &g_pFont ) );

	s_totalmb.center = D3DXVECTOR3( 0.0f, 0.0f, 0.0f );
	s_totalmb.max = D3DXVECTOR3( 5.0f, 5.0f, 5.0f );
	s_totalmb.min = D3DXVECTOR3( -5.0f, -5.0f, -5.0f );
	s_totalmb.r = D3DXVec3Length( &s_totalmb.max );
	g_vCenter = s_totalmb.center;
	float fObjectRadius = s_totalmb.r;
    D3DXMatrixTranslation( &g_mCenterWorld, -g_vCenter.x, -g_vCenter.y, -g_vCenter.z );

    V_RETURN( CDXUTDirectionWidget::StaticOnD3D9CreateDevice( pd3dDevice ) );
    for( int i = 0; i < MAX_LIGHTS; i++ )
		g_LightControl[i].SetRadius( fObjectRadius );
 
    // Define DEBUG_VS and/or DEBUG_PS to debug vertex and/or pixel shaders with the 
    // shader debugger. Debugging vertex shaders requires either REF or software vertex 
    // processing, and debugging pixel shaders requires REF.  The 
    // D3DXSHADER_FORCE_*_SOFTWARE_NOOPT flag improves the debug experience in the 
    // shader debugger.  It enables source level debugging, prevents instruction 
    // reordering, prevents dead code elimination, and forces the compiler to compile 
    // against the next higher available software target, which ensures that the 
    // unoptimized shaders do not exceed the shader model limitations.  Setting these 
    // flags will cause slower rendering since the shaders will be unoptimized and 
    // forced into software.  See the DirectX documentation for more information about 
    // using the shader debugger.
    DWORD dwShaderFlags = D3DXFX_NOT_CLONEABLE;

#if defined( DEBUG ) || defined( _DEBUG )
    // Set the D3DXSHADER_DEBUG flag to embed debug information in the shaders.
    // Setting this flag improves the shader debugging experience, but still allows 
    // the shaders to be optimized and to run exactly the way they will run in 
    // the release configuration of this program.
    dwShaderFlags |= D3DXSHADER_DEBUG;
    #endif

#ifdef DEBUG_VS
        dwShaderFlags |= D3DXSHADER_FORCE_VS_SOFTWARE_NOOPT;
    #endif
#ifdef DEBUG_PS
        dwShaderFlags |= D3DXSHADER_FORCE_PS_SOFTWARE_NOOPT;
    #endif

    // Preshaders are parts of the shader that the effect system pulls out of the 
    // shader and runs on the host CPU. They should be used if you are GPU limited. 
    // The D3DXSHADER_NO_PRESHADER flag disables preshaders.
    if( !g_bEnablePreshader )
        dwShaderFlags |= D3DXSHADER_NO_PRESHADER;

    // Read the D3DX effect file
    WCHAR str[MAX_PATH];
    V_RETURN( DXUTFindDXSDKMediaFileCch( str, MAX_PATH, L"Shader\\Ochakko.fx" ) );

    // If this fails, there should be debug output as to 
    // why the .fx file failed to compile
    V_RETURN( D3DXCreateEffectFromFile( pd3dDevice, str, NULL, NULL, dwShaderFlags, NULL, &g_pEffect, NULL ) );

	CallF( GetShaderHandle(), return 1 );

    // Setup the camera's view parameters
    D3DXVECTOR3 vecEye( 0.0f, 0.0f, -g_initcamz );
    D3DXVECTOR3 vecAt ( 0.0f, 0.0f, -0.0f );
    g_Camera.SetViewParams( &vecEye, &vecAt );
    g_Camera.SetRadius( fObjectRadius * 3.0f, fObjectRadius * 0.5f, fObjectRadius * 10.0f );


	s_select = new CModel();
	if( !s_select ){
		_ASSERT( 0 );
		return 1;
	}
	CallF( s_select->LoadMQO( s_pdev, L"RDBMedia\\select_2.mqo", 0, 1.0f, 1 ), return 1 );
	CallF( s_select->CalcInf(), return 1 );
	CallF( s_select->MakeDispObj(), return 1 );
//	CallF( s_select->DbgDump(), return 1 );

	s_bmark = new CModel();
	if( !s_bmark ){
		_ASSERT( 0 );
		return 1;
	}
	CallF( s_bmark->LoadMQO( s_pdev, L"RDBMedia\\bonemark.mqo", 0, 1.0f, 1 ), return 1 );
	CallF( s_bmark->CalcInf(), return 1 );
	CallF( s_bmark->MakeDispObj(), return 1 );

	s_ground = new CModel();
	if( !s_ground ){
		_ASSERT( 0 );
		return 1;
	}
	CallF( s_ground->LoadMQO( s_pdev, L"RDBMedia\\ground2.mqo", 0, 1.0f, 1 ), return 1 );
	CallF( s_ground->CalcInf(), return 1 );
	CallF( s_ground->MakeDispObj(), return 1 );

	s_dummytri = new CModel();
	if( !s_dummytri ){
		_ASSERT( 0 );
		return 1;
	}
	CallF( s_dummytri->LoadMQO( s_pdev, L"RDBMedia\\dummytri.mqo", 0, 1.0f, 1 ), return 1 );
	CallF( s_dummytri->CalcInf(), return 1 );
	CallF( s_dummytri->MakeDispObj(), return 1 );


	s_bcircle = new CMySprite( s_pdev );
	if( !s_bcircle ){
		_ASSERT( 0 );
		return 1;
	}

	WCHAR mpath[MAX_PATH];
	wcscpy_s( mpath, MAX_PATH, g_basedir );
	WCHAR* lasten = 0;
	WCHAR* last2en = 0;
	lasten = wcsrchr( mpath, TEXT( '\\' ) );
	*lasten = 0L;
	last2en = wcsrchr( mpath, TEXT( '\\' ) );
	*last2en = 0L;
	wcscat_s( mpath, MAX_PATH, L"\\Media\\RDBMedia\\" );
	CallF( s_bcircle->Create( mpath, L"bonecircle.dds", 0, D3DPOOL_DEFAULT, 0 ), return 1 );

	s_spaxis[0].sprite = new CMySprite( s_pdev );
	_ASSERT( s_spaxis[0].sprite );
	CallF( s_spaxis[0].sprite->Create( mpath, L"X.png", 0, D3DPOOL_DEFAULT, 0 ), return 1 );

	s_spaxis[1].sprite = new CMySprite( s_pdev );
	_ASSERT( s_spaxis[1].sprite );
	CallF( s_spaxis[1].sprite->Create( mpath, L"Y.png", 0, D3DPOOL_DEFAULT, 0 ), return 1 );

	s_spaxis[2].sprite = new CMySprite( s_pdev );
	_ASSERT( s_spaxis[2].sprite );
	CallF( s_spaxis[2].sprite->Create( mpath, L"Z.png", 0, D3DPOOL_DEFAULT, 0 ), return 1 );


	SetSpAxisParams();

/***
	s_kinsprite = new CMySprite( s_pdev );
	if( !s_kinsprite ){
		_ASSERT( 0 );
		return 1;
	}
	CallF( s_kinsprite->CreateDecl(), return 1 );

	s_kinect = new CKinectMain( s_pdev );
	if( !s_kinect ){
		_ASSERT( 0 );
		return 1;
	}
	WCHAR kpath[MAX_PATH];
	wcscpy_s( kpath, MAX_PATH, g_basedir );
	WCHAR* klasten = 0;
	WCHAR* klast2en = 0;
	klasten = wcsrchr( kpath, TEXT( '\\' ) );
	*klasten = 0L;
	klast2en = wcsrchr( kpath, TEXT( '\\' ) );
	*klast2en = 0L;
	wcscat_s( kpath, MAX_PATH, L"\\KinecoDX\\Release" );
	s_kinect->SetFilePath( kpath );
***/

	s_pdev->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
	s_pdev->SetRenderState( D3DRS_BLENDOP, D3DBLENDOP_ADD );
	s_pdev->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
	s_pdev->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
	s_pdev->SetRenderState( D3DRS_ALPHATESTENABLE, TRUE );
	s_pdev->SetRenderState( D3DRS_ALPHAREF, 0x00 );
	s_pdev->SetRenderState( D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL );


    return S_OK;
}


//--------------------------------------------------------------------------------------
// This callback function will be called immediately after the Direct3D device has been 
// reset, which will happen after a lost device scenario. This is the best location to 
// create D3DPOOL_DEFAULT resources since these resources need to be reloaded whenever 
// the device is lost. Resources created here should be released in the OnLostDevice 
// callback. 
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnResetDevice( IDirect3DDevice9* pd3dDevice,
                                const D3DSURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext )
{
	int ret1;
	RECT clirect;
	ret1 = GetClientRect( s_mainwnd, &clirect );
	if( ret1 ){
		s_winpos[WINPOS_3D].width = clirect.right - clirect.left;
		s_winpos[WINPOS_3D].height = clirect.bottom - clirect.top + 22;
	}

    HRESULT hr;

	if( g_texbank ){
		CallF( g_texbank->Invalidate(), return 1 );
		CallF( g_texbank->Restore(), return 1 );
	}

    V_RETURN( g_DialogResourceManager.OnD3D9ResetDevice() );
    V_RETURN( g_SettingsDlg.OnD3D9ResetDevice() );

    if( g_pFont )
        V_RETURN( g_pFont->OnResetDevice() );
    if( g_pEffect )
        V_RETURN( g_pEffect->OnResetDevice() );

    // Create a sprite to help batch calls when drawing many lines of text
    V_RETURN( D3DXCreateSprite( pd3dDevice, &g_pSprite ) );

    for( int i = 0; i < MAX_LIGHTS; i++ )
        g_LightControl[i].OnD3D9ResetDevice( pBackBufferSurfaceDesc );

    // Setup the camera's projection parameters
    s_fAspectRatio = pBackBufferSurfaceDesc->Width / ( FLOAT )pBackBufferSurfaceDesc->Height;
	g_Camera.SetProjParams( D3DX_PI / 4, s_fAspectRatio, s_projnear, 3.0f * g_initcamz );

	g_Camera.SetWindow( pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height );
    g_Camera.SetButtonMasks( MOUSE_LEFT_BUTTON, MOUSE_WHEEL, MOUSE_MIDDLE_BUTTON );

	s_mainwidth = (int)pBackBufferSurfaceDesc->Width;
	s_mainheight = (int)pBackBufferSurfaceDesc->Height;

	SetSpAxisParams();

    //g_SampleUI.SetLocation( pBackBufferSurfaceDesc->Width - 170, pBackBufferSurfaceDesc->Height - 550 );
	g_SampleUI.SetLocation( pBackBufferSurfaceDesc->Width - 170, 0 );
    g_SampleUI.SetSize( 170, 750 );

    return S_OK;
}


//--------------------------------------------------------------------------------------
// This callback function will be called once at the beginning of every frame. This is the
// best location for your application to handle updates to the scene, but is not 
// intended to contain actual rendering calls, which should instead be placed in the 
// OnFrameRender callback.  
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext )
{
	static double savetime = 0.0;
	static int capcnt = 0;

	MoveMemory( g_befkeybuf, g_keybuf, sizeof( BYTE ) * 256 );

	GetKeyboardState( (PBYTE)g_keybuf );
	if( g_keybuf[ VK_SHIFT ] & 0x80 ){
		s_dispselect = false;
	}else{
		s_dispselect = true;
	}
	if( g_keybuf[ VK_CONTROL ] & 0x80 ){
		g_controlkey = true;
	}else{
		g_controlkey = false;
	}

	SetCamera6Angle();

	s_camtargetflag = (int)s_CamTargetCheckBox->GetChecked();
	s_displightarrow = s_LightCheckBox->GetChecked();

	if( !(g_befkeybuf['K'] & 0x80) && (g_keybuf['K'] & 0x80) ){
		if( s_owpTimeline && s_owpMTimeline ){
			refreshTimeline( *s_owpTimeline, *s_owpMTimeline );
		}		
	}
	if( !(g_befkeybuf['J'] & 0x80) && ( (g_keybuf[VK_CONTROL] & 0x80) && (g_keybuf['J'] & 0x80)) ){
		if( s_model ){
			s_dispjoint = !s_dispjoint;
		}		
	}


	int ret1;
	RECT winrect;
	ret1 = GetWindowRect( s_mainwnd, &winrect );
	if( ret1 ){
		s_winpos[WINPOS_3D].posx = winrect.left;
		s_winpos[WINPOS_3D].posy = winrect.top;
	}


	s_time = fTime;
 
	SetSelectState();


    // Update the camera's position based on user input 
    g_Camera.FrameMove( fElapsedTime );

	double difftime = fTime - savetime;


	double nextframe = 0.0;
	if( s_previewFlag ){
		if( s_model && s_model->m_curmotinfo ){
			if( s_previewFlag != 3 ){
				int endflag = 0;
				s_model->AdvanceTime( s_previewFlag, difftime, &nextframe, &endflag );
				if( endflag == 1 ){
					s_previewFlag = 0;
					nextframe = (double( (int)nextframe ) );//中途半端でとまると、以降のキーの生成時に不具合が出る。
				}

				vector<MODELELEM>::iterator itrmodel;
				for( itrmodel = s_modelindex.begin(); itrmodel != s_modelindex.end(); itrmodel++ ){
					CModel* curmodel = itrmodel->modelptr;
					if( curmodel && curmodel->m_curmotinfo ){
						curmodel->SetMotionFrame( nextframe );
					}
				}
				s_owpTimeline->setCurrentTime( nextframe, true );
				s_owpMTimeline->setCurrentTime( nextframe, true );
				s_owpLTimeline->setCurrentTime( nextframe, true );
			}else{
				/***
				int kret;
				kret = s_kinect->Update( s_model, &nextframe );
				if( kret ){
					_ASSERT( kret == 2 );
					s_kinect->EndCapture();
					s_previewFlag = 0;
					ActivatePanel( 1 );
					OnAnimMenu( s_curmotmenuindex );
				}
				***/
			}
		}else{
			s_previewFlag = 0;
		}
	}
	s_difftime = difftime;
	savetime = fTime;

	// 終了フラグを確認 //////////////////////////////////////////////////////////
	if( s_closeFlag ){
		s_closeFlag= false;
		s_dispmw = false;
		if( s_timelineWnd ){
			s_timelineWnd->setVisible( 0 );
		}

	}
	if( s_McloseFlag ){
		s_McloseFlag= false;
		s_Mdispmw = false;
		if( s_MtimelineWnd ){
			s_MtimelineWnd->setVisible( 0 );
		}

	}
	if( s_LcloseFlag ){
		s_LcloseFlag= false;
		s_Ldispmw = false;
		if( s_LtimelineWnd ){
			s_LtimelineWnd->setVisible( 0 );
		}

	}
	if( s_closetoolFlag ){
		s_closetoolFlag= false;
		s_disptool = false;
		if( s_toolWnd ){
			s_toolWnd->setVisible( 0 );
		}
	}
	if( s_closeobjFlag ){
		s_closeobjFlag= false;
		s_dispobj = false;
		if( s_layerWnd ){
			s_layerWnd->setVisible( 0 );
		}
	}
	if( s_closemodelFlag ){
		s_closemodelFlag = false;
		s_dispmodel = false;
		if( s_modelpanel.panel ){
			s_modelpanel.panel->setVisible( 0 );
		}
	}

	// カーソル移動フラグを確認 //////////////////////////////////////////////////

	if( s_McursorFlag ){
		s_McursorFlag = false;

		if( s_model && s_owpMTimeline ){
			double curframe = s_owpMTimeline->getCurrentTime();// 選択時刻;
			if( s_previewFlag == 0 ){
				//プレビュー時以外は中途半端な位置に止まらせない。変な位置にキーが出来ないようにするため。
				curframe = (double)( (int)curframe );
				s_owpMTimeline->setCurrentTime( curframe, true );
				s_owpTimeline->setCurrentTime( curframe, true );
				s_owpLTimeline->setCurrentTime( curframe, true );
			}
			if( curframe >= s_model->m_curmotinfo->frameleng ){
				curframe = s_model->m_curmotinfo->frameleng - 1.0;
				s_owpMTimeline->setCurrentTime( curframe, true );
				s_owpTimeline->setCurrentTime( curframe, true );
				s_owpLTimeline->setCurrentTime( curframe, true );
			}

			vector<MODELELEM>::iterator itrmodel;
			for( itrmodel = s_modelindex.begin(); itrmodel != s_modelindex.end(); itrmodel++ ){
				CModel* curmodel = itrmodel->modelptr;
				if( curmodel && curmodel->m_curmotinfo ){
					curmodel->SetMotionFrame( curframe );
				}
			}


			if( !s_model->m_mbaseobject.empty() ){
				int curlineno = s_owpMTimeline->getCurrentLine();// 選択行
				if( curlineno >= 0 ){
					if( curlineno >= (int)s_model->m_mbaseobject.size() ){
						return;
					}
					LineNo2BaseNo( curlineno, &s_curbaseno );

				}else{
					s_curbaseno = -1;
				}
			}else{
				s_curbaseno = -1;
			}
		}
	}


	if( s_cursorFlag ){
		s_cursorFlag = false;

		// カーソル位置をコンソールに出力
		if( s_owpTimeline && s_model ){
			double curframe = s_owpTimeline->getCurrentTime();// 選択時刻;
			if( s_previewFlag == 0 ){
				//プレビュー時以外は中途半端な位置に止まらせない。変な位置にキーが出来ないようにするため。
				curframe = (double)( (int)curframe );
				s_owpTimeline->setCurrentTime( curframe, true );
				s_owpMTimeline->setCurrentTime( curframe, true );
				s_owpLTimeline->setCurrentTime( curframe, true );
			}
			if( curframe >= s_model->m_curmotinfo->frameleng ){
				curframe = s_model->m_curmotinfo->frameleng - 1.0;
				s_owpTimeline->setCurrentTime( curframe, true );
				s_owpMTimeline->setCurrentTime( curframe, true );
				s_owpLTimeline->setCurrentTime( curframe, true );
			}
		

			if( s_model->m_curmotinfo ){
				vector<MODELELEM>::iterator itrmodel;
				for( itrmodel = s_modelindex.begin(); itrmodel != s_modelindex.end(); itrmodel++ ){
					CModel* curmodel = itrmodel->modelptr;
					if( curmodel && curmodel->m_curmotinfo ){
						curmodel->SetMotionFrame( curframe );
					}
				}

				int curlineno = s_owpTimeline->getCurrentLine();// 選択行
				if( curlineno >= 0 ){
					s_curboneno = s_lineno2boneno[ curlineno ];

					if( s_model ){
						CDXUTComboBox* pComboBox;
						pComboBox = g_SampleUI.GetComboBox( IDC_COMBO_BONE );
						CBone* pBone;
						pBone = s_model->m_bonelist[ s_curboneno ];
						if( pBone ){
							pComboBox->SetSelectedByData( ULongToPtr( s_curboneno ) );
						}
					}
				}else{
					s_curboneno = -1;
				}
			}else{
				s_curboneno = -1;
			}
		}
		//DbgOut( L"cursor : lineno %d, boneno %d, frame %f\r\n", curlineno, s_curboneno, s_curframe );
	}

	if( s_LcursorFlag ){
		s_LcursorFlag = false;

		// カーソル位置をコンソールに出力
		if( s_owpLTimeline && s_model ){
			double curframe = s_owpLTimeline->getCurrentTime();// 選択時刻;
			if( s_previewFlag == 0 ){
				//プレビュー時以外は中途半端な位置に止まらせない。変な位置にキーが出来ないようにするため。
				curframe = (double)( (int)curframe );
				s_owpLTimeline->setCurrentTime( curframe, true );
				s_owpTimeline->setCurrentTime( curframe, true );
				s_owpMTimeline->setCurrentTime( curframe, true );
			}
			if( curframe >= s_model->m_curmotinfo->frameleng ){
				curframe = s_model->m_curmotinfo->frameleng - 1.0;
				s_owpLTimeline->setCurrentTime( curframe, true );
				s_owpTimeline->setCurrentTime( curframe, true );
				s_owpMTimeline->setCurrentTime( curframe, true );
			}
		

			if( s_model->m_curmotinfo ){
				vector<MODELELEM>::iterator itrmodel;
				for( itrmodel = s_modelindex.begin(); itrmodel != s_modelindex.end(); itrmodel++ ){
					CModel* curmodel = itrmodel->modelptr;
					if( curmodel && curmodel->m_curmotinfo ){
						curmodel->SetMotionFrame( curframe );
					}
				}

				int curlineno = s_owpTimeline->getCurrentLine();// 選択行
				if( curlineno >= 0 ){
					s_curboneno = s_lineno2boneno[ curlineno ];

					if( s_model ){
						CDXUTComboBox* pComboBox;
						pComboBox = g_SampleUI.GetComboBox( IDC_COMBO_BONE );
						CBone* pBone;
						pBone = s_model->m_bonelist[ s_curboneno ];
						if( pBone ){
							pComboBox->SetSelectedByData( ULongToPtr( s_curboneno ) );
						}
					}
				}else{
					s_curboneno = -1;
				}
			}else{
				s_curboneno = -1;
			}
		}
		//DbgOut( L"cursor : lineno %d, boneno %d, frame %f\r\n", curlineno, s_curboneno, s_curframe );
	}


	// コピー/カット/ペーストフラグを確認 ////////////////////////////////////////
	if( s_copyFlag ){
		s_copyFlag= false;
		if( s_model && s_owpTimeline && s_model->m_curmotinfo){
			s_copyKeyInfoList.erase( s_copyKeyInfoList.begin(), s_copyKeyInfoList.end() );
			s_copyKeyInfoList = s_owpTimeline->getSelectedKey();

			list<OWP_Timeline::KeyInfo>::iterator itrcp;
			for( itrcp = s_copyKeyInfoList.begin(); itrcp != s_copyKeyInfoList.end(); itrcp++ ){
				CMotionPoint* mpptr = (CMotionPoint*)(itrcp->object);
				itrcp->eul = mpptr->m_eul;
				itrcp->tra = mpptr->m_tra;
			}
		}

		if( s_model && s_owpMTimeline && s_model->m_curmotinfo){
			s_McopyKeyInfoList.clear();
			s_McopyKeyInfoList = s_owpMTimeline->getSelectedKey();

			list<OWP_Timeline::KeyInfo>::iterator itrcp;
			for( itrcp = s_McopyKeyInfoList.begin(); itrcp != s_McopyKeyInfoList.end(); itrcp++ ){
				CMorphKey* mkptr = (CMorphKey*)(itrcp->object);
				itrcp->blendweight.clear();
				itrcp->mbase = mkptr->m_baseobj;
				itrcp->blendweight = mkptr->m_blendweight;
			}
		}
	}
	if( s_symcopyFlag ){

		if( s_model && s_owpTimeline && s_model->m_curmotinfo){
			s_copyKeyInfoList.erase( s_copyKeyInfoList.begin(), s_copyKeyInfoList.end() );
			s_copyKeyInfoList = s_owpTimeline->getSelectedKey();

			list<OWP_Timeline::KeyInfo>::iterator itrcp;
			for( itrcp = s_copyKeyInfoList.begin(); itrcp != s_copyKeyInfoList.end(); itrcp++ ){
				int srcboneno = s_lineno2boneno[ itrcp->lineIndex ];
				if( srcboneno >= 0 ){
					CBone* srcboneptr = s_model->m_bonelist[ srcboneno ];
					_ASSERT( srcboneptr );

					if( s_symcopyFlag == 1 ){
						int symboneno = -1;
						int existflag = 0;
						CallF( s_model->GetSymBoneNo( srcboneno, &symboneno, &existflag ), return );
						if( existflag ){
							CMotionPoint symmp;
							int existmp = 0;
							CallF( s_model->m_bonelist[ symboneno ]->CalcMotionPoint( s_model->m_curmotinfo->motid, itrcp->time, &symmp, &existmp ), return );
							CQuaternion symq;
							symq.SetParams( 1.0f, 0.0f, 0.0f, 0.0f );
							symmp.m_q.CalcSym( &symq );

							CMotionPoint* pbef = 0;
							CMotionPoint* pnext = 0;
							int existcur = 0;
							CallF( srcboneptr->GetBefNextMP( s_model->m_curmotinfo->motid, itrcp->time, &pbef, &pnext, &existcur ), return );
							D3DXVECTOR3 cpeul;
							D3DXVECTOR3 befeul;
							if( pbef ){
								befeul = pbef->m_eul;
							}else{
								befeul = D3DXVECTOR3( 0.0f, 0.0f, 0.0f );
							}
							CallF( symq.Q2Eul( &(srcboneptr->m_axisq), befeul, &cpeul ), return );
							itrcp->eul = cpeul;
							itrcp->tra.x = -symmp.m_tra.x;
							itrcp->tra.y = symmp.m_tra.y;
							itrcp->tra.z = symmp.m_tra.z;
						}else{
							//CMotionPoint* mpptr = (CMotionPoint*)(itrcp->object);
							//itrcp->eul = mpptr->m_eul;
							//itrcp->tra = mpptr->m_tra;

							int existmp = 0;
							CMotionPoint symmp;
							CallF( srcboneptr->CalcMotionPoint( s_model->m_curmotinfo->motid, itrcp->time, &symmp, &existmp ), return );

							CQuaternion symq;
							symq.SetParams( 1.0f, 0.0f, 0.0f, 0.0f );
							symmp.m_q.CalcSym( &symq );

							CMotionPoint* pbef = 0;
							CMotionPoint* pnext = 0;
							int existcur = 0;
							CallF( srcboneptr->GetBefNextMP( s_model->m_curmotinfo->motid, itrcp->time, &pbef, &pnext, &existcur ), return );
							D3DXVECTOR3 cpeul;
							D3DXVECTOR3 befeul;
							if( pbef ){
								befeul = pbef->m_eul;
							}else{
								befeul = D3DXVECTOR3( 0.0f, 0.0f, 0.0f );
							}
							CallF( symq.Q2Eul( &(srcboneptr->m_axisq), befeul, &cpeul ), return );
							itrcp->eul = cpeul;
							itrcp->tra.x = -symmp.m_tra.x;
							itrcp->tra.y = symmp.m_tra.y;
							itrcp->tra.z = symmp.m_tra.z;

						}
					}else if( s_symcopyFlag == 2 ){
						int existmp = 0;
						CMotionPoint symmp;
						CallF( srcboneptr->CalcMotionPoint( s_model->m_curmotinfo->motid, itrcp->time, &symmp, &existmp ), return );

						CQuaternion symq;
						symq.SetParams( 1.0f, 0.0f, 0.0f, 0.0f );
						symmp.m_q.CalcSym( &symq );

						CMotionPoint* pbef = 0;
						CMotionPoint* pnext = 0;
						int existcur = 0;
						CallF( srcboneptr->GetBefNextMP( s_model->m_curmotinfo->motid, itrcp->time, &pbef, &pnext, &existcur ), return );
						D3DXVECTOR3 cpeul;
						D3DXVECTOR3 befeul;
						if( pbef ){
							befeul = pbef->m_eul;
						}else{
							befeul = D3DXVECTOR3( 0.0f, 0.0f, 0.0f );
						}
						CallF( symq.Q2Eul( &(srcboneptr->m_axisq), befeul, &cpeul ), return );
						itrcp->eul = cpeul;
						itrcp->tra.x = -symmp.m_tra.x;
						itrcp->tra.y = symmp.m_tra.y;
						itrcp->tra.z = symmp.m_tra.z;
					}
				}
			}
		}
		s_symcopyFlag = 0;
	}


	if( s_cutFlag ){
		s_cutFlag= false;
		if( s_model && s_owpTimeline && s_model->m_curmotinfo){
			s_copyKeyInfoList.erase( s_copyKeyInfoList.begin(), s_copyKeyInfoList.end() );
			s_copyKeyInfoList = s_owpTimeline->getSelectedKey();

			list<OWP_Timeline::KeyInfo>::iterator itrcp;
			for( itrcp = s_copyKeyInfoList.begin(); itrcp != s_copyKeyInfoList.end(); itrcp++ ){
				CMotionPoint* mpptr = (CMotionPoint*)(itrcp->object);
				itrcp->eul = mpptr->m_eul;
				itrcp->tra = mpptr->m_tra;

				//motionpointのdeleteはdelete Listenerでする。
			}
			s_owpTimeline->deleteKey();
		}

		if( s_model && s_owpMTimeline && s_model->m_curmotinfo){
			s_McopyKeyInfoList.clear();
			s_McopyKeyInfoList = s_owpMTimeline->getSelectedKey();

			list<OWP_Timeline::KeyInfo>::iterator itrcp;
			for( itrcp = s_McopyKeyInfoList.begin(); itrcp != s_McopyKeyInfoList.end(); itrcp++ ){
				CMorphKey* mkptr = (CMorphKey*)(itrcp->object);
				itrcp->blendweight.clear();
				itrcp->mbase = mkptr->m_baseobj;
				itrcp->blendweight = mkptr->m_blendweight;

				//morphkeyのdeleteはdelete Listenerでする。
			}
			s_owpMTimeline->deleteKey();
		}

		if( s_model ){
			s_model->SaveUndoMotion( s_curboneno, s_curbaseno );
		}

	}
	if( s_deleteFlag ){
		s_deleteFlag = false;
		if( s_model && s_owpTimeline && s_model->m_curmotinfo){
			s_owpTimeline->deleteKey();
			//motionpointのdeleteはdelete Listenerでする。
		}
		if( s_model && s_owpMTimeline && s_model->m_curmotinfo){
			s_owpMTimeline->deleteKey();
			//morphkeyのdeleteはdelete Listenerでする。
		}
		if( s_model ){
			s_model->SaveUndoMotion( s_curboneno, s_curbaseno );
		}

	}
	if( s_newkeyFlag ){
		s_newkeyFlag = false;
		if( s_model && s_owpTimeline && s_model->m_curmotinfo && (s_curboneno >= 0) ){
			CMotionPoint* newmp = 0;
			int existflag = 0;
			newmp = s_model->AddMotionPoint( s_model->m_curmotinfo->motid, s_owpTimeline->getCurrentTime(),
				s_curboneno, 1, &existflag );
			if( existflag == 0 ){
				CBone* boneptr = s_model->m_bonelist[ s_curboneno ];
				if( boneptr ){
				s_owpTimeline->newKey(
					boneptr->m_wbonename,
					s_owpTimeline->getCurrentTime(),
					(void*)newmp);
				}
			}
			s_owpTimeline->callRewrite();						//再描画
			s_owpTimeline->setRewriteOnChangeFlag(true);		//再描画要求を再開
		}
		if( s_model ){
			s_model->SaveUndoMotion( s_curboneno, s_curbaseno );
		}

	}
	if( s_newkeyallFlag ){
		s_newkeyallFlag = false;
		if( s_model && s_owpTimeline && s_model->m_curmotinfo && (s_curboneno >= 0) ){
			map<int,CBone*>::iterator itrbone;
			for( itrbone = s_model->m_bonelist.begin(); itrbone != s_model->m_bonelist.end(); itrbone++ ){
				CBone* boneptr = itrbone->second;
				if( boneptr ){
					CMotionPoint* newmp = 0;
					int existflag = 0;
					newmp = boneptr->AddMotionPoint( s_model->m_curmotinfo->motid, s_owpTimeline->getCurrentTime(), 1, &existflag );
					if( existflag == 0 ){
						s_owpTimeline->newKey(
							boneptr->m_wbonename,
							s_owpTimeline->getCurrentTime(),
							(void*)newmp);
					}
				}
			}
			s_owpTimeline->callRewrite();						//再描画
			s_owpTimeline->setRewriteOnChangeFlag(true);		//再描画要求を再開
		}
		if( s_model ){
			s_model->SaveUndoMotion( s_curboneno, s_curbaseno );
		}

	}
	if( s_MnewkeyFlag ){
		s_MnewkeyFlag = false;
		if( s_model && s_owpMTimeline && s_model->m_curmotinfo && (s_curbaseno >= 0) ){
			CMorphKey* newmk = 0;
			int existflag = 0;
			newmk = s_model->AddMorphKey( s_model->m_curmotinfo->motid, s_owpMTimeline->getCurrentTime(),
				s_curbaseno, &existflag );
			if( existflag == 0 ){
				CMQOObject* objptr = s_model->m_mbaseobject[ s_curbaseno ];
				if( objptr ){
					WCHAR wname[256];
					MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, objptr->m_name, 256, wname, 256 );

					s_owpMTimeline->newKey(
						wname,
						s_owpMTimeline->getCurrentTime(),
						(void*)newmk);
				}
			}
			s_owpMTimeline->callRewrite();						//再描画
			s_owpMTimeline->setRewriteOnChangeFlag(true);		//再描画要求を再開
		}
		if( s_model ){
			s_model->SaveUndoMotion( s_curboneno, s_curbaseno );
		}

	}
	if( s_MnewkeyallFlag ){
		s_MnewkeyallFlag = false;
		if( s_model && s_owpTimeline && s_model->m_curmotinfo && (s_curbaseno >= 0) ){
			map<int,CMQOObject*>::iterator itrbase;
			for( itrbase = s_model->m_mbaseobject.begin(); itrbase != s_model->m_mbaseobject.end(); itrbase++ ){
				CMQOObject* baseptr = itrbase->second;
				if( baseptr ){
					CMorphKey* newmk = 0;
					int existflag = 0;
					newmk = s_model->AddMorphKey( s_model->m_curmotinfo->motid, s_owpMTimeline->getCurrentTime(),
						baseptr->m_objectno, &existflag );
					if( existflag == 0 ){
						WCHAR wname[256];
						MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, baseptr->m_name, 256, wname, 256 );

						s_owpMTimeline->newKey(
							wname,
							s_owpMTimeline->getCurrentTime(),
							(void*)newmk);
					}
				}
			}
			s_owpMTimeline->callRewrite();						//再描画
			s_owpMTimeline->setRewriteOnChangeFlag(true);		//再描画要求を再開
		}
		if( s_model ){
			s_model->SaveUndoMotion( s_curboneno, s_curbaseno );
		}

	}


	if( s_initkeyFlag ){
		s_initkeyFlag = false;
		if( s_model && s_owpTimeline && s_model->m_curmotinfo && (s_curboneno >= 0) ){
			list<OWP_Timeline::KeyInfo>::iterator itrsel;
			for( itrsel = s_selectKeyInfoList.begin(); itrsel != s_selectKeyInfoList.end(); itrsel++ ){
				CMotionPoint* curmp = (CMotionPoint*)itrsel->object;
				if( curmp ){
					curmp->InitMotion();
				}
			}
		}

		if( s_model && s_owpMTimeline && s_model->m_curmotinfo && (s_curbaseno >= 0) ){
			list<OWP_Timeline::KeyInfo>::iterator itrsel;
			for( itrsel = s_MselectKeyInfoList.begin(); itrsel != s_MselectKeyInfoList.end(); itrsel++ ){
				CMorphKey* curmk = (CMorphKey*)itrsel->object;
				if( curmk ){
					curmk->InitMotion();
				}
			}
		}
		if( s_model ){
			s_model->SaveUndoMotion( s_curboneno, s_curbaseno );
		}

	}


	if( s_pasteFlag ){
		s_pasteFlag= false;
		if( s_model && s_owpTimeline && s_model->m_curmotinfo){
			//コピーされたキーの先頭時刻を求める
			double copyStartTime= DBL_MAX;
			for(list<OWP_Timeline::KeyInfo>::iterator itr=s_copyKeyInfoList.begin();
				itr!=s_copyKeyInfoList.end(); itr++){
				if( itr->time<=copyStartTime ){
					copyStartTime= itr->time;
				}
			}

			//ペーストする
			s_owpTimeline->setRewriteOnChangeFlag(false);		//全てのキーをペーストし終えるまで再描画要求をストップ
			for(list<OWP_Timeline::KeyInfo>::iterator itr=s_copyKeyInfoList.begin(); itr!=s_copyKeyInfoList.end(); itr++){
				CMotionPoint* newmp = 0;
				int existflag = 0;
				_ASSERT( s_model->m_curmotinfo );
				newmp = s_model->AddMotionPoint( s_model->m_curmotinfo->motid, itr->time - copyStartTime + s_owpTimeline->getCurrentTime(),
					s_lineno2boneno[ itr->lineIndex ],
					1,
					&existflag );

				int boneno = s_lineno2boneno[ itr->lineIndex ];
				CBone* boneptr = s_model->m_bonelist[ boneno ];

				newmp->SetEul( &(boneptr->m_axisq), itr->eul );
				newmp->m_tra = itr->tra;

				if( existflag == 0 ){
					if( boneptr ){
						s_owpTimeline->newKey(
							//itr->label,//<--- 実態が削除されている可能性がある。
							boneptr->m_wbonename,
							itr->time - copyStartTime + s_owpTimeline->getCurrentTime(),
							(void*)newmp);
					}
				}
			}
			s_owpTimeline->callRewrite();						//再描画
			s_owpTimeline->setRewriteOnChangeFlag(true);		//再描画要求を再開
		}


		if( s_model && s_owpMTimeline && s_model->m_curmotinfo && !s_model->m_mbaseobject.empty()){
			//コピーされたキーの先頭時刻を求める
			double copyStartTime= DBL_MAX;
			for(list<OWP_Timeline::KeyInfo>::iterator itr=s_McopyKeyInfoList.begin();
				itr!=s_McopyKeyInfoList.end(); itr++){
				if( itr->time <= copyStartTime ){
					copyStartTime= itr->time;
				}
			}

			//ペーストする
			s_owpMTimeline->setRewriteOnChangeFlag(false);		//全てのキーをペーストし終えるまで再描画要求をストップ
			for(list<OWP_Timeline::KeyInfo>::iterator itr=s_McopyKeyInfoList.begin(); itr!=s_McopyKeyInfoList.end(); itr++){
				CMorphKey* newmk = 0;
				int existflag = 0;
				_ASSERT( s_model->m_curmotinfo );

				int baseno = -1;
				LineNo2BaseNo( itr->lineIndex, &baseno );
				if( baseno < 0 ){
					_ASSERT( 0 );
					return;
				}

				newmk = s_model->AddMorphKey( s_model->m_curmotinfo->motid, itr->time - copyStartTime + s_owpMTimeline->getCurrentTime(),
					baseno, &existflag );
				CMQOObject* baseobj = s_model->m_mbaseobject[ baseno ];
				if( !baseobj ){
					_ASSERT( 0 );
					return;
				}

				newmk->m_baseobj = itr->mbase;
				newmk->m_blendweight = itr->blendweight;

				if( existflag == 0 ){
					WCHAR wname[256];
					MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, baseobj->m_name, 256, wname, 256 );
					s_owpMTimeline->newKey(
						//itr->label,//<--- 実態が削除されている可能性がある。
						wname,
						itr->time - copyStartTime + s_owpMTimeline->getCurrentTime(),
						(void*)newmk);
				}
			}
			s_owpMTimeline->callRewrite();						//再描画
			s_owpMTimeline->setRewriteOnChangeFlag(true);		//再描画要求を再開
		}
		if( s_model ){
			s_model->SaveUndoMotion( s_curboneno, s_curbaseno );
		}

	}

	// キー選択フラグを確認 ///////////////////////////////////////////////////////
	if( s_selectFlag ){
		s_selectFlag= false;
		if( s_model && s_owpTimeline && s_model->m_curmotinfo){
			s_selectKeyInfoList.erase( s_selectKeyInfoList.begin(), s_selectKeyInfoList.end() );
			s_selectKeyInfoList = s_owpTimeline->getSelectedKey();
		}
	}
	if( s_MselectFlag ){
		s_MselectFlag= false;
		if( s_model && s_owpMTimeline && s_model->m_curmotinfo && !s_model->m_mbaseobject.empty()){
			s_MselectKeyInfoList.clear();
			s_MselectKeyInfoList = s_owpMTimeline->getSelectedKey();
		}
	}
	// キー移動フラグを確認 ///////////////////////////////////////////////////////////
	if( s_keyShiftFlag ){
		s_keyShiftFlag= false;

		if( (s_keyShiftTime != 0.0) && s_model && s_owpTimeline && s_model->m_curmotinfo){

			// 移動量をコンソールに出力
			//printf_s("ShiftTime:%5f\n",s_keyShiftTime);

			list<OWP_Timeline::KeyInfo>::iterator itrsel;
			for( itrsel = s_selectKeyInfoList.begin(); itrsel != s_selectKeyInfoList.end(); itrsel++ ){
				CMotionPoint* mpptr = (CMotionPoint*)(itrsel->object);
				CBone* boneptr = s_model->m_bonelist[ s_lineno2boneno[ itrsel->lineIndex ] ];
				mpptr->LeaveFromChain( s_model->m_curmotinfo->motid, boneptr );
			}
			for( itrsel = s_selectKeyInfoList.begin(); itrsel != s_selectKeyInfoList.end(); itrsel++ ){
				CMotionPoint* mpptr = (CMotionPoint*)(itrsel->object);
				CBone* boneptr = s_model->m_bonelist[ s_lineno2boneno[ itrsel->lineIndex ] ];
				boneptr->OrderMotionPoint( s_model->m_curmotinfo->motid, itrsel->time + s_keyShiftTime, mpptr, 0 );
			}

			// 選択しているキーを全て移動
			// この処理のみを行うラムダ関数がデフォルトで設定されている
			s_owpTimeline->shiftKeyTime( s_keyShiftTime );

		}
		s_selectKeyInfoList.erase( s_selectKeyInfoList.begin(), s_selectKeyInfoList.end() );
		if( s_owpTimeline ){
			s_selectKeyInfoList = s_owpTimeline->getSelectedKey();
		}

		if( s_model ){
			s_model->SaveUndoMotion( s_curboneno, s_curbaseno );
		}

	}
	if( s_MkeyShiftFlag ){
		s_MkeyShiftFlag= false;

		if( (s_MkeyShiftTime != 0.0) && s_model && s_owpMTimeline && s_model->m_curmotinfo){

			// 移動量をコンソールに出力
			//printf_s("ShiftTime:%5f\n",s_keyShiftTime);

			list<OWP_Timeline::KeyInfo>::iterator itrsel;
			for( itrsel = s_MselectKeyInfoList.begin(); itrsel != s_MselectKeyInfoList.end(); itrsel++ ){
				CMorphKey* mkptr = (CMorphKey*)(itrsel->object);

				int baseno = -1;
				LineNo2BaseNo( itrsel->lineIndex, &baseno );
				if( baseno < 0 ){
					_ASSERT( 0 );
					return;
				}
				CMQOObject* baseobj = s_model->m_mbaseobject[ baseno ];
				mkptr->LeaveFromChain( s_model->m_curmotinfo->motid, baseobj );
			}
			for( itrsel = s_MselectKeyInfoList.begin(); itrsel != s_MselectKeyInfoList.end(); itrsel++ ){
				CMorphKey* mkptr = (CMorphKey*)(itrsel->object);
				int baseno = -1;
				LineNo2BaseNo( itrsel->lineIndex, &baseno );
				if( baseno < 0 ){
					_ASSERT( 0 );
					return;
				}
				CMQOObject* baseobj = s_model->m_mbaseobject[ baseno ];
				baseobj->OrderMorphKey( s_model->m_curmotinfo->motid, itrsel->time + s_MkeyShiftTime, mkptr, 0 );
			}

			// 選択しているキーを全て移動
			// この処理のみを行うラムダ関数がデフォルトで設定されている
			s_owpMTimeline->shiftKeyTime( s_MkeyShiftTime );

		}
		s_MselectKeyInfoList.clear();
		if( s_owpMTimeline ){
			s_MselectKeyInfoList = s_owpMTimeline->getSelectedKey();
		}

		if( s_model ){
			s_model->SaveUndoMotion( s_curboneno, s_curbaseno );
		}

	}

	if( s_befkeyFlag ){
		s_befkeyFlag = false;

		if( s_model && s_model->m_curmotinfo ){
			double curframe = s_owpTimeline->getCurrentTime();
			int curline = s_owpTimeline->getCurrentLine();
			int curboneno = s_lineno2boneno[ curline ];
			CBone* curbone = s_model->m_bonelist[ curboneno ];
			double setframe = 0.0;
			if( curbone ){
				CMotionPoint* pbef = 0;
				CMotionPoint* pnext = 0;
				int existflag = 0;
				curbone->GetBefNextMP( s_model->m_curmotinfo->motid, curframe, &pbef, &pnext, &existflag );
				if( existflag ){
					pbef = pbef->m_prev;
				}
				if( pbef ){
					setframe = pbef->m_frame;
					s_owpTimeline->setCurrentTime( pbef->m_frame, true );
					s_owpMTimeline->setCurrentTime( pbef->m_frame, true );
					s_owpLTimeline->setCurrentTime( pbef->m_frame, true );
				}else{
					setframe = 0.0;
					s_owpTimeline->setCurrentTime( 0.0, true );
					s_owpMTimeline->setCurrentTime( 0.0, true );
					s_owpLTimeline->setCurrentTime( 0.0, true );
				}
			}
			vector<MODELELEM>::iterator itrmodel;
			for( itrmodel = s_modelindex.begin(); itrmodel != s_modelindex.end(); itrmodel++ ){
				CModel* curmodel = itrmodel->modelptr;
				if( curmodel && curmodel->m_curmotinfo ){
					curmodel->SetMotionFrame( setframe );
				}
			}
		}
	}
	if( s_nextkeyFlag ){
		s_nextkeyFlag = false;
		if( s_model && s_model->m_curmotinfo ){
			double curframe = s_owpTimeline->getCurrentTime();
			int curline = s_owpTimeline->getCurrentLine();
			int curboneno = s_lineno2boneno[ curline ];
			CBone* curbone = s_model->m_bonelist[ curboneno ];
			double setframe = 0.0;
			if( curbone ){
				CMotionPoint* pbef = 0;
				CMotionPoint* pnext = 0;
				int existflag = 0;
				curbone->GetBefNextMP( s_model->m_curmotinfo->motid, curframe, &pbef, &pnext, &existflag );
				if( pnext ){
					setframe = pnext->m_frame;
					s_owpTimeline->setCurrentTime( setframe, true );
					s_owpMTimeline->setCurrentTime( setframe, true );
					s_owpLTimeline->setCurrentTime( setframe, true );
				}else{
					setframe = s_model->m_curmotinfo->frameleng - 1.0;
					s_owpTimeline->setCurrentTime( setframe, true );
					s_owpMTimeline->setCurrentTime( setframe, true );
					s_owpLTimeline->setCurrentTime( setframe, true );
				}
				vector<MODELELEM>::iterator itrmodel;
				for( itrmodel = s_modelindex.begin(); itrmodel != s_modelindex.end(); itrmodel++ ){
					CModel* curmodel = itrmodel->modelptr;
					if( curmodel && curmodel->m_curmotinfo ){
						curmodel->SetMotionFrame( setframe );
					}
				}
			}
		}
	}

	if( s_MbefkeyFlag ){
		s_MbefkeyFlag = false;

		double setframe = 0.0;
		if( s_model && s_model->m_curmotinfo ){
			if( !s_model->m_mbaseobject.empty() ){
				double curframe = s_owpMTimeline->getCurrentTime();
				int curline = s_owpMTimeline->getCurrentLine();
				int curbaseno = -1;
				LineNo2BaseNo( curline, &curbaseno );
				_ASSERT( curbaseno >= 0 );
				CMQOObject* curbase = s_model->m_mbaseobject[ curbaseno ];
				if( curbase ){
					CMorphKey* pbef = 0;
					CMorphKey* pnext = 0;
					int existflag = 0;
					curbase->GetBefNextMK( s_model->m_curmotinfo->motid, curframe, 0, &pbef, &pnext, &existflag );
					if( existflag ){
						pbef = pbef->m_prev;
					}
					if( pbef ){
						setframe = pbef->m_frame;
						s_owpMTimeline->setCurrentTime( pbef->m_frame, true );
						s_owpTimeline->setCurrentTime( pbef->m_frame, true );
						s_owpLTimeline->setCurrentTime( pbef->m_frame, true );
					}else{
						setframe = 0.0;
						s_owpMTimeline->setCurrentTime( 0.0, true );
						s_owpTimeline->setCurrentTime( 0.0, true );
						s_owpLTimeline->setCurrentTime( 0.0, true );
					}
				}
			}else{
				setframe = 0.0;
				s_owpMTimeline->setCurrentTime( 0.0, true );
				s_owpTimeline->setCurrentTime( 0.0, true );
				s_owpLTimeline->setCurrentTime( 0.0, true );
			}
			vector<MODELELEM>::iterator itrmodel;
			for( itrmodel = s_modelindex.begin(); itrmodel != s_modelindex.end(); itrmodel++ ){
				CModel* curmodel = itrmodel->modelptr;
				if( curmodel && curmodel->m_curmotinfo ){
					curmodel->SetMotionFrame( setframe );
				}
			}
		}
	}
	if( s_MnextkeyFlag ){
		s_MnextkeyFlag = false;
		double setframe = 0.0;
		if( s_model && s_model->m_curmotinfo ){
			if( !s_model->m_mbaseobject.empty() ){
				double curframe = s_owpMTimeline->getCurrentTime();
				int curline = s_owpMTimeline->getCurrentLine();
				int curbaseno = -1;
				LineNo2BaseNo( curline, &curbaseno );
				_ASSERT( curbaseno >= 0 );
				CMQOObject* curbase = s_model->m_mbaseobject[ curbaseno ];
				if( curbase ){
					CMorphKey* pbef = 0;
					CMorphKey* pnext = 0;
					int existflag = 0;
					curbase->GetBefNextMK( s_model->m_curmotinfo->motid, curframe, 0, &pbef, &pnext, &existflag );
					if( pnext ){
						setframe = pnext->m_frame;
						s_owpMTimeline->setCurrentTime( pnext->m_frame, true );
						s_owpTimeline->setCurrentTime( pnext->m_frame, true );
						s_owpLTimeline->setCurrentTime( pnext->m_frame, true );
					}else{
						setframe = s_model->m_curmotinfo->frameleng - 1.0;
						s_owpMTimeline->setCurrentTime( setframe, true );
						s_owpTimeline->setCurrentTime( setframe, true );
						s_owpLTimeline->setCurrentTime( setframe, true );
					}
				}
			}else{
				setframe = s_model->m_curmotinfo->frameleng - 1.0;
				s_owpMTimeline->setCurrentTime( setframe, true );
				s_owpTimeline->setCurrentTime( setframe, true );
				s_owpLTimeline->setCurrentTime( setframe, true );
			}
			vector<MODELELEM>::iterator itrmodel;
			for( itrmodel = s_modelindex.begin(); itrmodel != s_modelindex.end(); itrmodel++ ){
				CModel* curmodel = itrmodel->modelptr;
				if( curmodel && curmodel->m_curmotinfo ){
					curmodel->SetMotionFrame( setframe );
				}
			}
		}
	}

	if( s_LbefkeyFlag ){
		s_LbefkeyFlag = false;

		double setframe = 0.0;
		if( s_model && s_model->m_curmotinfo ){
			double curframe = s_owpLTimeline->getCurrentTime();
			setframe = max( 0.0, (curframe - 50.0) );
			s_owpMTimeline->setCurrentTime( setframe, true );
			s_owpTimeline->setCurrentTime( setframe, true );
			s_owpLTimeline->setCurrentTime( setframe, true );

			vector<MODELELEM>::iterator itrmodel;
			for( itrmodel = s_modelindex.begin(); itrmodel != s_modelindex.end(); itrmodel++ ){
				CModel* curmodel = itrmodel->modelptr;
				if( curmodel && curmodel->m_curmotinfo ){
					curmodel->SetMotionFrame( setframe );
				}
			}
		}
	}
	if( s_LnextkeyFlag ){
		s_LnextkeyFlag = false;
		double setframe = 0.0;
		if( s_model && s_model->m_curmotinfo ){
			double curframe = s_owpLTimeline->getCurrentTime();
			setframe = min( (s_model->m_curmotinfo->frameleng - 1.0), (curframe + 50.0) );
			s_owpMTimeline->setCurrentTime( setframe, true );
			s_owpTimeline->setCurrentTime( setframe, true );
			s_owpLTimeline->setCurrentTime( setframe, true );

			vector<MODELELEM>::iterator itrmodel;
			for( itrmodel = s_modelindex.begin(); itrmodel != s_modelindex.end(); itrmodel++ ){
				CModel* curmodel = itrmodel->modelptr;
				if( curmodel && curmodel->m_curmotinfo ){
					curmodel->SetMotionFrame( setframe );
				}
			}
		}
	}



	if( s_motpropFlag ){

		if( s_model && s_model->m_curmotinfo && !s_underpropFlag ){
			s_underpropFlag = true;
			ActivatePanel( 0, 1 );
			int dlgret;
			dlgret = (int)DialogBoxW( (HINSTANCE)GetModuleHandle(NULL), MAKEINTRESOURCE( IDD_MOTPROPDLG ), 
				s_mainwnd, (DLGPROC)MotPropDlgProc );
			ActivatePanel( 1, 1 );
			if( (dlgret == IDOK) && s_tmpmotname[0] ){
				WideCharToMultiByte( CP_ACP, 0, s_tmpmotname, -1, s_model->m_curmotinfo->motname, 256, NULL, NULL );
				//s_model->m_curmotinfo->frameleng = s_tmpmotframeleng;
				s_model->m_curmotinfo->loopflag = s_tmpmotloop;

				s_owpTimeline->setMaxTime( s_tmpmotframeleng );
				s_owpMTimeline->setMaxTime( s_tmpmotframeleng );
				s_owpLTimeline->setMaxTime( s_tmpmotframeleng );
				s_model->ChangeMotFrameLeng( s_model->m_curmotinfo->motid, s_tmpmotframeleng );//はみ出たmpも削除


				//メニュー書き換え
				OnAnimMenu( s_curmotmenuindex );
			}
			s_underpropFlag = false;
		}
		s_motpropFlag = false;
	}

	// 削除されたキー情報のスタックを確認 ////////////////////////////////////////////
	for(; s_deletedKeyInfoList.begin()!=s_deletedKeyInfoList.end();
		s_deletedKeyInfoList.pop_front()){

		CMotionPoint* mpptr = (CMotionPoint*)( s_deletedKeyInfoList.begin()->object );
		CBone* boneptr = s_model->m_bonelist[ s_lineno2boneno[ s_deletedKeyInfoList.begin()->lineIndex ] ];
		mpptr->LeaveFromChain( s_model->m_curmotinfo->motid, boneptr );
		delete mpptr;
	}
	for(; s_MdeletedKeyInfoList.begin()!=s_MdeletedKeyInfoList.end();
		s_MdeletedKeyInfoList.pop_front()){

		CMorphKey* mkptr = (CMorphKey*)( s_MdeletedKeyInfoList.begin()->object );
		int baseno = -1;
		LineNo2BaseNo( s_MdeletedKeyInfoList.begin()->lineIndex, &baseno );
		_ASSERT( baseno >= 0 );
		CMQOObject* baseobj = s_model->m_mbaseobject[ baseno ];
		if( baseobj ){
			mkptr->LeaveFromChain( s_model->m_curmotinfo->motid, baseobj );
		}else{
			_ASSERT( 0 );
		}
		delete mkptr;
	}

///////////// undo
	if( s_model && g_controlkey && (g_keybuf[ 'Z' ] & 0x80) && !(g_befkeybuf[ 'Z' ] & 0x80) ){
		
		if( g_keybuf[ VK_SHIFT ] & 0x80 ){
			s_model->RollBackUndoMotion( 1, &s_curboneno, &s_curbaseno );//!!!!!!!!!!!
		}else{
			s_model->RollBackUndoMotion( 0, &s_curboneno, &s_curbaseno );//!!!!!!!!!!!
		}

		s_copyKeyInfoList.clear();
		s_deletedKeyInfoList.clear();
		s_selectKeyInfoList.clear();

		s_McopyKeyInfoList.clear();
		s_MdeletedKeyInfoList.clear();
		s_MselectKeyInfoList.clear();

		int chkcnt = 0;
		int findflag = 0;
		map<int, MOTINFO*>::iterator itrmi;
		for( itrmi = s_model->m_motinfo.begin(); itrmi != s_model->m_motinfo.end(); itrmi++ ){
			MOTINFO* curmi = itrmi->second;
			_ASSERT( curmi );
			if( curmi == s_model->m_curmotinfo ){
				findflag = 1;
				break;
			}
			chkcnt++;
		}

		if( findflag == 1 ){
			int selindex;
			selindex = chkcnt;
			OnAnimMenu( selindex, 0 );
		}

		int curlineno = s_boneno2lineno[ s_curboneno ];
		if( s_owpTimeline ){
			s_owpTimeline->setCurrentLine( curlineno, true );
		}

		if( s_morphWnd ){
			s_morphWnd->setVisible( 0 );
		}
	}

/////////////
	D3DXMATRIX mWorld;
    D3DXMATRIX mView;
    D3DXMATRIX mProj;
	mWorld = *g_Camera.GetWorldMatrix();
    mProj = *g_Camera.GetProjMatrix();
    mView = *g_Camera.GetViewMatrix();

	mWorld._41 = 0.0f;
	mWorld._42 = 0.0f;
	mWorld._43 = 0.0f;

	D3DXMATRIXA16 mW, mVP;
	mW = g_mCenterWorld * mWorld;
	mVP = mView * mProj;

	g_pEffect->SetMatrix( g_hmVP, &mVP );
	g_pEffect->SetMatrix( g_hmWorld, &mW );

	vector<MODELELEM>::iterator itrmodel;
	for( itrmodel = s_modelindex.begin(); itrmodel != s_modelindex.end(); itrmodel++ ){
		CModel* curmodel = itrmodel->modelptr;
		if( curmodel ){
			curmodel->UpdateMatrix( -1, &mW, &mVP );
		}
	}

	if( s_ground ){
		s_ground->UpdateMatrix( -1, &mWorld, &mVP );
	}
}


//--------------------------------------------------------------------------------------
// This callback function will be called at the end of every frame to perform all the 
// rendering calls for the scene, and it will also be called if the window needs to be 
// repainted. After this function has returned, DXUT will call 
// IDirect3DDevice9::Present to display the contents of the next buffer in the swap chain
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameRender( IDirect3DDevice9* pd3dDevice, double fTime, float fElapsedTime, void* pUserContext )
{
    HRESULT hr;

    // If the settings dialog is being shown, then
    // render it instead of rendering the app's scene
    if( g_SettingsDlg.IsActive() )
    {
        g_SettingsDlg.OnRender( fElapsedTime );
        return;
    }

    D3DXVECTOR3 vLightDir[MAX_LIGHTS];
    D3DXCOLOR vLightDiffuse[MAX_LIGHTS];
    D3DXMATRIXA16 mView;
    D3DXMATRIXA16 mProj;

    // Clear the render target and the zbuffer 
    V( pd3dDevice->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DXCOLOR( 0.0f, 0.25f, 0.25f, 0.55f ), 1.0f,
                          0 ) );

	s_pdev->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
	s_pdev->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
	s_pdev->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
	s_pdev->SetRenderState( D3DRS_ALPHATESTENABLE, TRUE );
	s_pdev->SetRenderState( D3DRS_ALPHAREF, 0x08 );
	s_pdev->SetRenderState( D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL );

	s_pdev->SetSamplerState( 0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP );
	s_pdev->SetSamplerState( 0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP );
//	s_pdev->SetSamplerState( 0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP );
//	s_pdev->SetSamplerState( 0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP );

    // Render the scene
    if( SUCCEEDED( pd3dDevice->BeginScene() ) )
    {
        // Get the projection & view matrix from the camera class
		D3DXMATRIX mWorld;
		D3DXMATRIX mView;
		D3DXMATRIX mProj;
		mWorld = *g_Camera.GetWorldMatrix();
		mProj = *g_Camera.GetProjMatrix();
		mView = *g_Camera.GetViewMatrix();
		mWorld._41 = 0.0f;
		mWorld._42 = 0.0f;
		mWorld._43 = 0.0f;

		D3DXMATRIX mWV = mWorld * mView;
        // Render the light arrow so the user can visually see the light dir
        for( int i = 0; i < g_nNumActiveLights; i++ )
        {
			if( s_displightarrow ){
				D3DXCOLOR arrowColor = ( i == g_nActiveLight ) ? D3DXCOLOR( 1, 1, 0, 1 ) : D3DXCOLOR( 1, 1, 1, 1 );
				V( g_LightControl[i].OnRender9( arrowColor, &mView, &mProj, g_Camera.GetEyePt() ) );
			}
            vLightDir[i] = g_LightControl[i].GetLightDirection();
            vLightDiffuse[i] = g_fLightScale * D3DXCOLOR( 1, 1, 1, 1 );
        }

		D3DXVECTOR3 lightamb( 1.0f, 1.0f, 1.0f );

        V( g_pEffect->SetValue( g_hLightDir, vLightDir, sizeof( D3DXVECTOR3 ) * MAX_LIGHTS ) );
        V( g_pEffect->SetValue( g_hLightDiffuse, vLightDiffuse, sizeof( D3DXVECTOR4 ) * MAX_LIGHTS ) );
		V( g_pEffect->SetValue( g_hLightAmbient, &lightamb, sizeof( D3DXVECTOR3 ) ) );
        V( g_pEffect->SetInt( g_hnNumLight, g_nNumActiveLights ) );
		D3DXVECTOR3 eyept = *g_Camera.GetEyePt();
		V( g_pEffect->SetValue( g_hEyePos, &eyept, sizeof( D3DXVECTOR3 ) ) );


		vector<MODELELEM>::iterator itrmodel;
		for( itrmodel = s_modelindex.begin(); itrmodel != s_modelindex.end(); itrmodel++ ){
			CModel* curmodel = itrmodel->modelptr;

			if( curmodel && curmodel->m_modeldisp ){
				curmodel->SetShaderConst();
				int lightflag = 1;
				D3DXVECTOR4 diffusemult = D3DXVECTOR4(1.0f, 1.0f, 1.0f, 1.0f);
				curmodel->OnRender( s_pdev, lightflag, diffusemult );
			}
		}
		if( s_ground && s_dispground ){
			D3DXVECTOR4 diffusemult = D3DXVECTOR4(1.0f, 1.0f, 1.0f, 1.0f);
			s_ground->OnRender( s_pdev, 0, diffusemult );
		}

		if( s_model && s_model->m_modeldisp && s_dispjoint ){
			s_model->RenderBoneMark( s_pdev, s_bmark, s_bcircle, s_curboneno );
		}

		if( s_select && (s_curboneno >= 0) && (s_previewFlag == 0) && (s_model && s_model->m_modeldisp) && s_dispjoint ){
			RenderSelectMark();
		}

		int spacnt;
		for( spacnt = 0; spacnt < 3; spacnt++ ){
			s_spaxis[spacnt].sprite->OnRender();
		}

		if( s_previewFlag != 3 ){
			//g_HUD.OnRender( fElapsedTime );
			g_SampleUI.OnRender( fElapsedTime );
		}else{
			/***
			D3DXVECTOR3 scpos = D3DXVECTOR3( 0.5f, 0.0f, 0.0f );
			s_kinsprite->SetPos( scpos );
			D3DXVECTOR2 bsize( 1.0f, 1.0f );
			s_kinsprite->SetSize( bsize );
			s_kinsprite->SetColor( D3DXVECTOR4( 1.0f, 1.0f, 1.0f, 0.7f ) );
			s_pdev->SetRenderState( D3DRS_ZFUNC, D3DCMP_ALWAYS );
			s_kinsprite->OnRender( s_kinect->m_ptex );
			s_pdev->SetRenderState( D3DRS_ZFUNC, D3DCMP_LESSEQUAL );
			***/
		}

		RenderText( fTime );

        V( pd3dDevice->EndScene() );
    }
}

//--------------------------------------------------------------------------------------
// Render the help and statistics text. This function uses the ID3DXFont interface for 
// efficient text rendering.
//--------------------------------------------------------------------------------------
void RenderText( double fTime )
{
	static double s_savetime = 0.0;

	double calcfps = 1.0 / (fTime - s_savetime);

    // The helper object simply helps keep track of text position, and color
    // and then it calls pFont->DrawText( m_pSprite, strMsg, -1, &rc, DT_NOCLIP, m_clr );
    // If NULL is passed in as the sprite object, then it will work fine however the 
    // pFont->DrawText() will not be batched together.  Batching calls will improves perf.
    CDXUTTextHelper txtHelper( g_pFont, g_pSprite, 15 );

    // Output statistics
    txtHelper.Begin();
    txtHelper.SetInsertionPos( 2, 0 );
    txtHelper.SetForegroundColor( D3DXCOLOR( 1.0f, 1.0f, 0.0f, 1.0f ) );
//    txtHelper.DrawTextLine( DXUTGetFrameStats( DXUTIsVsyncEnabled() ) );
 //   txtHelper.DrawTextLine( DXUTGetDeviceStats() );

    txtHelper.SetForegroundColor( D3DXCOLOR( 1.0f, 1.0f, 1.0f, 1.0f ) );
    txtHelper.DrawFormattedTextLine( L"fps : %0.2f fTime: %0.1f", calcfps, fTime );
/***
    // Draw help
    if( g_bShowHelp )
    {
        const D3DSURFACE_DESC* pd3dsdBackBuffer = DXUTGetD3D9BackBufferSurfaceDesc();
        txtHelper.SetInsertionPos( 2, pd3dsdBackBuffer->Height - 15 * 6 );
        txtHelper.SetForegroundColor( D3DXCOLOR( 1.0f, 0.75f, 0.0f, 1.0f ) );
        txtHelper.DrawTextLine( L"Controls:" );

        txtHelper.SetInsertionPos( 20, pd3dsdBackBuffer->Height - 15 * 5 );
        txtHelper.DrawTextLine( L"Rotate model: Left mouse button\n"
                                L"Rotate light: Right mouse button\n"
                                L"Rotate camera: Middle mouse button\n"
                                L"Zoom camera: Mouse wheel scroll\n" );

        txtHelper.SetInsertionPos( 250, pd3dsdBackBuffer->Height - 15 * 5 );
        txtHelper.DrawTextLine( L"Hide help: F1\n"
                                L"Quit: ESC\n" );
    }
    else
    {
        txtHelper.SetForegroundColor( D3DXCOLOR( 1.0f, 1.0f, 1.0f, 1.0f ) );
        txtHelper.DrawTextLine( L"Press F1 for help" );
    }
***/
    txtHelper.End();

	s_savetime = fTime;
}


//--------------------------------------------------------------------------------------
// Before handling window messages, DXUT passes incoming windows 
// messages to the application through this callback function. If the application sets 
// *pbNoFurtherProcessing to TRUE, then DXUT will not process this message.
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing,
                          void* pUserContext )
{
	int ret;

	if( uMsg == WM_COMMAND ){

		WORD menuid;
		menuid = LOWORD( wParam );

		if( (menuid >= 59900) && (menuid <= (59900 + MAXMOTIONNUM)) ){
			ActivatePanel( 0 );
			int selindex = menuid - 59900;
			OnAnimMenu( selindex );
			ActivatePanel( 1 );
			return 0;
		}else if( (menuid >= 61000) && (menuid <= (61000 + MAXMODELNUM)) ){
			ActivatePanel( 0 );
			int selindex = menuid - 61000;
			OnModelMenu( selindex, 1 );
			ActivatePanel( 1 );
			return 0;
		}else{
			switch( menuid ){
			case ID_40037:
				ActivatePanel( 0 );
				RegistKey();
				ActivatePanel( 1 );
				return 0;
				break;
			case ID_FILE_OPEN40001:
				ActivatePanel( 0 );
				OpenFile();
				ActivatePanel( 1 );
				return 0;
				break;
			case ID_FILESAVE:
				if( m_frompnd == 0 ){
					ActivatePanel( 0 );
					SaveProject();
					ActivatePanel( 1 );
				}else{
					MessageBoxW( s_mainwnd, L"暗号データ読み込み時には保存が出来ません。", L"エラー", MB_OK );
				}
				return 0;
				break;
			case ID_PNDCONV:
				if( s_registflag == 1 ){
					ActivatePanel( 0 );
					ret = PndConv();
					ActivatePanel( 1 );

					if( ret != 0 ){
						_ASSERT( 0 );
						::MessageBoxW( s_mainwnd, L"暗号化に失敗しました。\nダイアログの入力項目を確認して再試行してください。", L"エラー", MB_OK );
					}else{
						::MessageBoxW( s_mainwnd, L"暗号化ファイルは*.pndです。\n*.idx,*.idx2,*.tmpはデバッグファイルなので処理が成功した場合は消してください。", L"成功", MB_OK );
					}
				}
				return 0;
				break;
			case ID_PNDLOAD:
				if( s_registflag == 1 ){
					ActivatePanel( 0 );
					PndLoad();
					ActivatePanel( 1 );

					MessageBoxW( s_mainwnd, L"暗号データ読み込み時には保存が出来ないので注意してください。", L"注意", MB_OK );
				}
				return 0;
				break;
			case ID_FILE_EXPORTX:
				if( s_registflag == 1 ){
					if( m_frompnd == 0 ){
						ActivatePanel( 0 );
						ExportXFile();
						ActivatePanel( 1 );
					}else{
						MessageBoxW( s_mainwnd, L"暗号データ読み込み時には保存が出来ません。", L"エラー", MB_OK );
					}
				}
				return 0;
				break;
			case ID_FILE_EXPORTCOLLADAFILE:
				if( s_registflag == 1 ){
					if( m_frompnd == 0 ){
						ActivatePanel( 0 );
						ExportColladaFile();
						ActivatePanel( 1 );
					}else{
						MessageBoxW( s_mainwnd, L"暗号データ読み込み時には保存が出来ません。", L"エラー", MB_OK );
					}
				}
				return 0;
				break;
			case FILE_EXPORTFBX:
				if( s_registflag == 1 ){
					if( m_frompnd == 0 ){
						ActivatePanel( 0 );
						ExportFBXFile();
						ActivatePanel( 1 );
					}else{
						MessageBoxW( s_mainwnd, L"暗号データ読み込み時には保存が出来ません。", L"エラー", MB_OK );
					}
				}
				return 0;
				break;
			case ID_DISPMW40002:
				DispMotionWindow();
				return 0;
				break;
			case ID_DISPMORPH40031:
				DispMorphMotionWindow();
				return 0;
			case ID_DISPLONG:
				DispLongMotionWindow();
				break;
			case 4007:
				DispToolWindow();
				return 0;
				break;
			case 40012:
				DispObjPanel();
				return 0;
				break;
			case ID_DISPMODELPANEL:
				DispModelPanel();
				return 0;
			case ID_DISPGROUND:
				s_dispground = !s_dispground;
				return 0;
				break;
			case ID_NEWMOT:
				AddMotion( 0 );
				return 0;
				break;
			case ID_DELCURMOT:
				OnDelMotion( s_curmotmenuindex );
				return 0;
				break;
			case ID_DELMODEL:
				OnDelModel( s_curmodelmenuindex );
				return 0;
				break;
			case ID_DELALLMODEL:
				OnDelAllModel();
				return 0;
				break;
			case ID_LOADKLIB:
				//ret = s_kinect->LoadPlugin( s_mainwnd );
				//_ASSERT( !ret );
				break;
			case ID_LOADKBS:
				//ActivatePanel( 0 );
				//if( s_model ){
				//	s_kinect->LoadKbs( s_model );
				//}
				//ActivatePanel( 1 );
				break;
			case ID_EDITKBS:
				//ActivatePanel( 0 );
				//if( s_model ){
				//	s_kinect->EditKbs( s_model );
				//}
				//ActivatePanel( 1 );
				break;
			case ID_STARTCAP:
				//if( s_model ){
				//	ret = s_kinect->StartCapture( s_model, 0 );
				//	if( ret == 0 ){
				//		ActivatePanel( 0 );
				//		s_previewFlag = 3;
				//	}else{
				//		s_kinect->EndCapture();
				//		s_previewFlag = 0;
				//		ActivatePanel( 1 );
				//		OnAnimMenu( s_curmotmenuindex );
				//	}
				//}
				break;
			case ID_ENDCAP:
				//s_kinect->EndCapture();
				//s_previewFlag = 0;
				//ActivatePanel( 1 );
				//OnAnimMenu( s_curmotmenuindex );
				break;
			case ID_KINAVG:
				//if( s_model && s_model->m_curmotinfo ){
				//	s_kinect->ApplyFilter( s_model );
				//}
				break;
			case ID_KINFIRSTPOSE:
				//if( s_model && s_model->m_curmotinfo ){
				//	s_model->SetKinectFirstPose();
				//}
				break;
			default:
				break;
			}
		}
	}else if( (uMsg == WM_LBUTTONDOWN) || (uMsg == WM_LBUTTONDBLCLK) ){
		SetCapture( s_mainwnd );
		POINT ptCursor;
		GetCursorPos( &ptCursor );
		::ScreenToClient( s_mainwnd, &ptCursor );
		s_pickinfo.clickpos = ptCursor;
		s_pickinfo.mousepos = ptCursor;
		s_pickinfo.mousebefpos = ptCursor;
		s_pickinfo.diffmouse = D3DXVECTOR2( 0.0f, 0.0f );

		s_pickinfo.winx = (int)DXUTGetWindowWidth();
		s_pickinfo.winy = (int)DXUTGetWindowHeight();
		s_pickinfo.pickrange = 6;
		s_pickinfo.buttonflag = 0;

		s_pickinfo.pickobjno = -1;

		if( s_model ){
			int spakind = PickSpAxis( ptCursor );
			if( spakind != 0 ){
				s_pickinfo.buttonflag = spakind;
			}else{

				if( g_controlkey == false ){
					CallF( s_model->PickBone( &s_pickinfo ), return 1 );
				}

				if( s_pickinfo.pickobjno >= 0 ){
					s_curboneno = s_pickinfo.pickobjno;				

					if( s_owpTimeline ){
						s_owpTimeline->setCurrentLine( s_boneno2lineno[ s_curboneno ] );
					}

					CDXUTComboBox* pComboBox;
					pComboBox = g_SampleUI.GetComboBox( IDC_COMBO_BONE );
					CBone* pBone;
					pBone = s_model->m_bonelist[ s_curboneno ];
					if( pBone ){
						pComboBox->SetSelectedByData( ULongToPtr( s_curboneno ) );
					}

					s_pickinfo.buttonflag = PICK_CENTER;//!!!!!!!!!!!!!

				}else{
					if( s_dispselect ){
						int colliobjx, colliobjy, colliobjz, colliringx, colliringy, colliringz;
						colliobjx = 0;
						colliobjy = 0;
						colliobjz = 0;
						colliringx = 0;
						colliringy = 0;
						colliringz = 0;
						
						colliobjx = s_select->CollisionNoBoneObj_Mouse( &s_pickinfo, "objX" );
						colliobjy = s_select->CollisionNoBoneObj_Mouse( &s_pickinfo, "objY" );
						colliobjz = s_select->CollisionNoBoneObj_Mouse( &s_pickinfo, "objZ" );
						colliringx = s_select->CollisionNoBoneObj_Mouse( &s_pickinfo, "ringX" );
						colliringy = s_select->CollisionNoBoneObj_Mouse( &s_pickinfo, "ringY" );
						colliringz = s_select->CollisionNoBoneObj_Mouse( &s_pickinfo, "ringZ" );

						if( colliobjx || colliringx || colliobjy || colliringy || colliobjz || colliringz ){
							s_pickinfo.pickobjno = s_curboneno;
						}

						if( colliobjx || colliringx ){
							s_pickinfo.buttonflag = PICK_X;
						}else if( colliobjy || colliringy ){
							s_pickinfo.buttonflag = PICK_Y;
						}else if( colliobjz || colliringz ){
							s_pickinfo.buttonflag = PICK_Z;
						}else{
							ZeroMemory( &s_pickinfo, sizeof( PICKINFO ) );
						}
					}else{
						ZeroMemory( &s_pickinfo, sizeof( PICKINFO ) );
					}
				}
			}
		}else{
			ZeroMemory( &s_pickinfo, sizeof( PICKINFO ) );
		}

		if( s_model && (s_curboneno >= 0) && s_camtargetflag ){
			CBone* curbone = s_model->m_bonelist[ s_curboneno ];
			_ASSERT( curbone );
			s_camtargetpos = curbone->m_childworld;
		}
		D3DXVECTOR3 weye = *(g_Camera.GetEyePt());
		g_Camera.SetViewParams( &weye, &s_camtargetpos );

	}else if( uMsg == WM_MBUTTONDOWN ){
		SetCapture( s_mainwnd );
		POINT ptCursor;
		GetCursorPos( &ptCursor );
		::ScreenToClient( s_mainwnd, &ptCursor );
		s_pickinfo.clickpos = ptCursor;
		s_pickinfo.mousepos = ptCursor;
		s_pickinfo.mousebefpos = ptCursor;
		s_pickinfo.diffmouse = D3DXVECTOR2( 0.0f, 0.0f );

		s_pickinfo.winx = (int)DXUTGetWindowWidth();
		s_pickinfo.winy = (int)DXUTGetWindowHeight();
		s_pickinfo.pickrange = 6;

		s_pickinfo.pickobjno = -1;

		s_pickinfo.buttonflag = PICK_CAMMOVE;

	}else if( uMsg ==  WM_MOUSEMOVE ){
		if( s_pickinfo.buttonflag == PICK_CENTER ){
			s_pickinfo.mousebefpos = s_pickinfo.mousepos;
			POINT ptCursor;
			GetCursorPos( &ptCursor );
			::ScreenToClient( s_mainwnd, &ptCursor );
			s_pickinfo.mousepos = ptCursor;

			D3DXVECTOR3 tmpsc;
			s_model->TransformBone( s_pickinfo.winx, s_pickinfo.winy, s_curboneno, &s_pickinfo.objworld, &tmpsc, &s_pickinfo.objscreen );

			if( s_previewFlag == 0 ){
				D3DXVECTOR3 targetpos( 0.0f, 0.0f, 0.0f );
				CallF( CalcTargetPos( &targetpos ), return 1 );
				if( s_ikkind == 0 ){
					s_model->IKRotate( s_pickinfo.pickobjno, targetpos, s_iklevel );
				}else if( s_ikkind == 1 ){
					s_model->IKMove( s_pickinfo.pickobjno, targetpos );
				}
				s_editmotionflag = 1;
			}
		}else if( (s_pickinfo.buttonflag == PICK_X) || (s_pickinfo.buttonflag == PICK_Y) ){
			s_pickinfo.mousebefpos = s_pickinfo.mousepos;
			POINT ptCursor;
			GetCursorPos( &ptCursor );
			::ScreenToClient( s_mainwnd, &ptCursor );
			s_pickinfo.mousepos = ptCursor;

			if( s_previewFlag == 0 ){
				float deltax = (float)( s_pickinfo.mousepos.x - s_pickinfo.mousebefpos.x );
				if( s_ikkind == 0 ){
					s_model->IKRotateAxisDelta( s_pickinfo.buttonflag, s_pickinfo.pickobjno, deltax, s_iklevel );
				}else if( s_ikkind == 1 ){
					s_model->IKMoveAxisDelta( s_pickinfo.buttonflag, s_pickinfo.pickobjno, deltax );
				}
				s_editmotionflag = 1;
			}
		}else if( s_pickinfo.buttonflag == PICK_Z ){
			s_pickinfo.mousebefpos = s_pickinfo.mousepos;
			POINT ptCursor;
			GetCursorPos( &ptCursor );
			::ScreenToClient( s_mainwnd, &ptCursor );
			s_pickinfo.mousepos = ptCursor;
			if( s_previewFlag == 0 ){
				float deltax = (float)( s_pickinfo.mousepos.x - s_pickinfo.mousebefpos.x );
				if( s_ikkind == 0 ){
					AddBoneEul( 0.0f, 0.0f, deltax );
				}else if( s_ikkind == 1 ){
					s_model->IKMoveAxisDelta( s_pickinfo.buttonflag, s_pickinfo.pickobjno, deltax );
				}
				s_editmotionflag = 1;
			}
		}else if( s_pickinfo.buttonflag == PICK_CAMMOVE ){
			s_pickinfo.mousebefpos = s_pickinfo.mousepos;
			POINT ptCursor;
			GetCursorPos( &ptCursor );
			::ScreenToClient( s_mainwnd, &ptCursor );
			s_pickinfo.mousepos = ptCursor;

			D3DXVECTOR3 cammv;
			//cammv.x = ((float)s_pickinfo.mousepos.x - (float)s_pickinfo.mousebefpos.x) / (float)s_pickinfo.winx * -100.0f;
			//cammv.y = ((float)s_pickinfo.mousepos.y - (float)s_pickinfo.mousebefpos.y) / (float)s_pickinfo.winy * 100.0f;
			cammv.x = ((float)s_pickinfo.mousepos.x - (float)s_pickinfo.mousebefpos.x) / (float)s_pickinfo.winx * -s_cammvstep;
			cammv.y = ((float)s_pickinfo.mousepos.y - (float)s_pickinfo.mousebefpos.y) / (float)s_pickinfo.winy * s_cammvstep;
			cammv.z = 0.0f;

			D3DXMATRIX matview;
			D3DXVECTOR3 weye, wat;
			matview = *(g_Camera.GetViewMatrix());
			weye = *(g_Camera.GetEyePt());
			wat = s_camtargetpos;

			D3DXVECTOR3 cameye, camat;
			D3DXVec3TransformCoord( &cameye, &weye, &matview );
			D3DXVec3TransformCoord( &camat, &wat, &matview );

			D3DXVECTOR3 aftcameye, aftcamat;
			aftcameye = cameye + cammv;
			aftcamat = camat + cammv;

			D3DXMATRIX invmatview;
			D3DXMatrixInverse( &invmatview, NULL, &matview );

			D3DXVECTOR3 neweye, newat;
			D3DXVec3TransformCoord( &neweye, &aftcameye, &invmatview );
			D3DXVec3TransformCoord( &newat, &aftcamat, &invmatview );

			g_Camera.SetViewParams( &neweye, &newat );
			s_camtargetpos = newat;

		}else if( s_pickinfo.buttonflag == PICK_CAMROT ){
			s_pickinfo.mousebefpos = s_pickinfo.mousepos;
			POINT ptCursor;
			GetCursorPos( &ptCursor );
			::ScreenToClient( s_mainwnd, &ptCursor );
			s_pickinfo.mousepos = ptCursor;

			float roty, rotxz;
			rotxz = ((float)s_pickinfo.mousepos.x - (float)s_pickinfo.mousebefpos.x) / (float)s_pickinfo.winx * 250.0f;
			roty = ((float)s_pickinfo.mousepos.y - (float)s_pickinfo.mousebefpos.y) / (float)s_pickinfo.winy * 250.0f;

			D3DXMATRIX matview;
			D3DXVECTOR3 weye, wat;
			weye = *(g_Camera.GetEyePt());
			wat = s_camtargetpos;

			D3DXVECTOR3 viewvec, upvec, rotaxisy, rotaxisxz;
			viewvec = wat - weye;
			D3DXVec3Normalize( &viewvec, &viewvec );
			upvec = D3DXVECTOR3( 0.000001f, 1.0f, 0.0f );

			float chkdot;
			chkdot = DXVec3Dot( &viewvec, &upvec );
			if( fabs( chkdot ) < 0.99965f ){
				D3DXVec3Cross( &rotaxisxz, &upvec, &viewvec );
				D3DXVec3Normalize( &rotaxisxz, &rotaxisxz );

				D3DXVec3Cross( &rotaxisy, &viewvec, &rotaxisxz );
				D3DXVec3Normalize( &rotaxisy, &rotaxisy );
			}else if( chkdot >= 0.99965f ){
				rotaxisxz = upvec;
				D3DXVec3Cross( &rotaxisy, &viewvec, &rotaxisxz );
				D3DXVec3Normalize( &rotaxisy, &rotaxisy );
				if( roty < 0.0f ){
					roty = 0.0f;
				}else{
				}
			}else{
				rotaxisxz = upvec;
				D3DXVec3Cross( &rotaxisy, &viewvec, &rotaxisxz );
				D3DXVec3Normalize( &rotaxisy, &rotaxisy );
				if( roty > 0.0f ){
					roty = 0.0f;
				}else{
					//rotyだけ回す。
				}
			}


			if( s_model && (s_curboneno >= 0) && s_camtargetflag ){
				CBone* curbone = s_model->m_bonelist[ s_curboneno ];
				_ASSERT( curbone );
				s_camtargetpos = curbone->m_childworld;
			}


			D3DXMATRIX befrotmat, rotmaty, rotmatxz, aftrotmat;
			D3DXMatrixTranslation( &befrotmat, -s_camtargetpos.x, -s_camtargetpos.y, -s_camtargetpos.z );
			D3DXMatrixTranslation( &aftrotmat, s_camtargetpos.x, s_camtargetpos.y, s_camtargetpos.z );
			D3DXMatrixRotationAxis( &rotmaty, &rotaxisy, rotxz * (float)DEG2PAI );
			D3DXMatrixRotationAxis( &rotmatxz, &rotaxisxz, roty * (float)DEG2PAI );

			D3DXMATRIX mat;
			mat = befrotmat * rotmatxz * rotmaty * aftrotmat;
			D3DXVECTOR3 neweye;
			D3DXVec3TransformCoord( &neweye, &weye, &mat );

			float chkdot2;
			D3DXVECTOR3 newviewvec = weye - neweye;
			D3DXVec3Normalize( &newviewvec, &newviewvec );
			chkdot2 = DXVec3Dot( &newviewvec, &upvec );
			if( fabs( chkdot2 ) < 0.99965f ){
				D3DXVec3Cross( &rotaxisxz, &upvec, &viewvec );
				D3DXVec3Normalize( &rotaxisxz, &rotaxisxz );

				D3DXVec3Cross( &rotaxisy, &viewvec, &rotaxisxz );
				D3DXVec3Normalize( &rotaxisy, &rotaxisy );
			}else{
				roty = 0.0f;
				rotaxisxz = upvec;
				D3DXVec3Cross( &rotaxisy, &viewvec, &rotaxisxz );
				D3DXVec3Normalize( &rotaxisy, &rotaxisy );
			}
			D3DXMatrixRotationAxis( &rotmaty, &rotaxisy, rotxz * (float)DEG2PAI );
			D3DXMatrixRotationAxis( &rotmatxz, &rotaxisxz, roty * (float)DEG2PAI );
			mat = befrotmat * rotmatxz * rotmaty * aftrotmat;
			D3DXVec3TransformCoord( &neweye, &weye, &mat );

			g_Camera.SetViewParams( &neweye, &s_camtargetpos );
		}else if( s_pickinfo.buttonflag == PICK_SPA_X ){
			s_pickinfo.mousebefpos = s_pickinfo.mousepos;
			POINT ptCursor;
			GetCursorPos( &ptCursor );
			::ScreenToClient( s_mainwnd, &ptCursor );
			s_pickinfo.mousepos = ptCursor;
			if( s_previewFlag == 0 ){
				if( s_model && (s_curboneno >= 0) ){
					float delta;
					if( s_ikkind == 0 ){
						delta = (float)( (s_pickinfo.mousepos.x - s_pickinfo.mousebefpos.x) + (s_pickinfo.mousepos.y - s_pickinfo.mousebefpos.y) ) / s_mainwidth * 200.0f;
						s_model->IKRotateAxisDelta( PICK_X, s_curboneno, delta, s_splevel );
					}else if( s_ikkind == 1 ){
						delta = (float)( (s_pickinfo.mousepos.x - s_pickinfo.mousebefpos.x) + (s_pickinfo.mousepos.y - s_pickinfo.mousebefpos.y) ) / s_mainwidth * s_totalmb.r;
						s_model->IKMoveAxisDelta( PICK_X, s_curboneno, delta );
					}
					s_editmotionflag = 1;
				}
			}
		}else if( s_pickinfo.buttonflag == PICK_SPA_Y ){
			s_pickinfo.mousebefpos = s_pickinfo.mousepos;
			POINT ptCursor;
			GetCursorPos( &ptCursor );
			::ScreenToClient( s_mainwnd, &ptCursor );
			s_pickinfo.mousepos = ptCursor;

			if( s_previewFlag == 0 ){
				if( s_model && (s_curboneno >= 0) ){
					float delta;
					if( s_ikkind == 0 ){
						delta = (float)( (s_pickinfo.mousepos.x - s_pickinfo.mousebefpos.x) + (s_pickinfo.mousepos.y - s_pickinfo.mousebefpos.y) ) / s_mainwidth * 200.0f;
						s_model->IKRotateAxisDelta( PICK_Y, s_curboneno, delta, s_splevel );
					}else if( s_ikkind == 1 ){
						delta = (float)( (s_pickinfo.mousepos.x - s_pickinfo.mousebefpos.x) + (s_pickinfo.mousepos.y - s_pickinfo.mousebefpos.y) ) / s_mainwidth * s_totalmb.r;
						s_model->IKMoveAxisDelta( PICK_Y, s_curboneno, delta );
					}
					s_editmotionflag = 1;
				}
			}
		}else if( s_pickinfo.buttonflag == PICK_SPA_Z ){
			s_pickinfo.mousebefpos = s_pickinfo.mousepos;
			POINT ptCursor;
			GetCursorPos( &ptCursor );
			::ScreenToClient( s_mainwnd, &ptCursor );
			s_pickinfo.mousepos = ptCursor;
			if( s_previewFlag == 0 ){
				if( s_model && (s_curboneno >= 0) ){
					float delta;
					if( s_ikkind == 0 ){
						delta = (float)( (s_pickinfo.mousepos.x - s_pickinfo.mousebefpos.x) + (s_pickinfo.mousepos.y - s_pickinfo.mousebefpos.y) ) / s_mainwidth * 200.0f;
						AddBoneEul( 0.0f, 0.0f, delta );
					}else if( s_ikkind == 1 ){
						delta = (float)( (s_pickinfo.mousepos.x - s_pickinfo.mousebefpos.x) + (s_pickinfo.mousepos.y - s_pickinfo.mousebefpos.y) ) / s_mainwidth * s_totalmb.r;
						s_model->IKMoveAxisDelta( PICK_Z, s_curboneno, delta );
					}
					s_editmotionflag = 1;
				}
			}
		}

		if( s_model ){
			int listsize = s_model->m_newmplist.size();
			int elemno;
			for( elemno = 0; elemno < listsize; elemno++ ){
				NEWMPELEM nmpelem = s_model->m_newmplist[ elemno ];
				if( s_owpTimeline ){
					s_owpTimeline->newKey( nmpelem.boneptr->m_wbonename, nmpelem.mpptr->m_frame, (void*)nmpelem.mpptr );
				}
			}
			s_model->m_newmplist.erase( s_model->m_newmplist.begin(), s_model->m_newmplist.end() );
		}


	}else if( uMsg == WM_LBUTTONUP ){
		ReleaseCapture();
		s_pickinfo.buttonflag = 0;

		if( s_editmotionflag == 1 ){
			if( s_model ){
				s_model->SaveUndoMotion( s_curboneno, s_curbaseno );
			}
			s_editmotionflag = 0;
		}

	}else if( uMsg == WM_RBUTTONDOWN ){
		SetCapture( s_mainwnd );
		POINT ptCursor;
		GetCursorPos( &ptCursor );
		::ScreenToClient( s_mainwnd, &ptCursor );
		s_pickinfo.clickpos = ptCursor;
		s_pickinfo.mousepos = ptCursor;
		s_pickinfo.mousebefpos = ptCursor;
		s_pickinfo.diffmouse = D3DXVECTOR2( 0.0f, 0.0f );

		s_pickinfo.winx = (int)DXUTGetWindowWidth();
		s_pickinfo.winy = (int)DXUTGetWindowHeight();
		s_pickinfo.pickrange = 6;

		s_pickinfo.pickobjno = -1;

		s_pickinfo.buttonflag = PICK_CAMROT;

	}else if( uMsg == WM_RBUTTONUP ){
		ReleaseCapture();
		s_pickinfo.buttonflag = 0;
	}else if( uMsg == WM_MBUTTONUP ){
		ReleaseCapture();
		s_pickinfo.buttonflag = 0;
	}

    // Always allow dialog resource manager calls to handle global messages
    // so GUI state is updated correctly
    *pbNoFurtherProcessing = g_DialogResourceManager.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;

    if( g_SettingsDlg.IsActive() )
    {
        g_SettingsDlg.MsgProc( hWnd, uMsg, wParam, lParam );
        return 0;
    }

    // Give the dialogs a chance to handle the message first
    *pbNoFurtherProcessing = g_SampleUI.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;

    // Pass all remaining windows messages to camera so it can respond to user input

	if( uMsg == WM_LBUTTONDOWN ){
		g_Camera.HandleMessages( hWnd, WM_RBUTTONDOWN, wParam, lParam );
		if( s_ikkind == 2 ){
			g_LightControl[g_nActiveLight].HandleMessages( hWnd, WM_RBUTTONDOWN, wParam, lParam );
		}
	}else if( uMsg == WM_LBUTTONDBLCLK ){
	}else if( uMsg == WM_LBUTTONUP ){
		g_Camera.HandleMessages( hWnd, WM_RBUTTONUP, wParam, lParam );
		if( s_ikkind == 2 ){
			g_LightControl[g_nActiveLight].HandleMessages( hWnd, WM_RBUTTONUP, wParam, lParam );
		}
	}else if( uMsg == WM_RBUTTONDOWN ){
	}else if( uMsg == WM_RBUTTONUP ){
	}else if( uMsg == WM_RBUTTONDBLCLK ){
	}else if( uMsg == WM_MBUTTONDOWN ){
	}else if( uMsg == WM_MBUTTONUP ){
	}else{
		g_Camera.HandleMessages( hWnd, uMsg, wParam, lParam );
		if( s_ikkind == 2 ){
			g_LightControl[g_nActiveLight].HandleMessages( hWnd, uMsg, wParam, lParam );
		}
	}

	if( uMsg == WM_ACTIVATE ){
		if( wParam == 1 ){
			DbgOut( L"%f, activate wparam 1\r\n", s_time );
			ActivatePanel( 1 );
		}
	}

	if( uMsg == WM_SYSCOMMAND ){
		switch( wParam ){
		case SC_CLOSE:
			DbgOut( L"%f, syscommand close\r\n", s_time );
			break;
		case SC_MINIMIZE:
			DbgOut( L"%f, syscommand minimize\r\n", s_time );
			ActivatePanel( 0 );
			break;
		case SC_MAXIMIZE:
			DbgOut( L"%f, syscommand maximize\r\n", s_time );
			DefWindowProc( s_mainwnd, uMsg, wParam, lParam );
			ActivatePanel( 1 );
			return 1;//!!!!!!!!!!!!!
			break;
		}
	}


    return 0;
}


//--------------------------------------------------------------------------------------
// As a convenience, DXUT inspects the incoming windows messages for
// keystroke messages and decodes the message parameters to pass relevant keyboard
// messages to the application.  The framework does not remove the underlying keystroke 
// messages, which are still passed to the application's MsgProc callback.
//--------------------------------------------------------------------------------------
void CALLBACK KeyboardProc( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext )
{
    if( bKeyDown )
    {
        switch( nChar )
        {
            case VK_F1:
                g_bShowHelp = !g_bShowHelp; break;
        }
    }
}


//--------------------------------------------------------------------------------------
// Handles the GUI events
//--------------------------------------------------------------------------------------
void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext )
{
    CDXUTComboBox* pComboBox;
	D3DXVECTOR3 cureul, neweul;
	int tmpboneno;
	CBone* tmpbone;
	WCHAR sz[100];
	D3DXVECTOR3 weye;
	float trastep = 10.0f;
	float radstep = (10.0f * (float)DEG2PAI) * 10.0f * 12.0f / (float)PAI;

    switch( nControlID )
    {
        case IDC_ACTIVE_LIGHT:
            if( !g_LightControl[g_nActiveLight].IsBeingDragged() )
            {
                g_nActiveLight++;
                g_nActiveLight %= g_nNumActiveLights;
            }
            break;

        case IDC_NUM_LIGHTS:
            if( !g_LightControl[g_nActiveLight].IsBeingDragged() )
            {
                WCHAR sz[100];
                swprintf_s( sz, 100, L"# Lights: %d", g_SampleUI.GetSlider( IDC_NUM_LIGHTS )->GetValue() );
                g_SampleUI.GetStatic( IDC_NUM_LIGHTS_STATIC )->SetText( sz );

                g_nNumActiveLights = g_SampleUI.GetSlider( IDC_NUM_LIGHTS )->GetValue();
                g_nActiveLight %= g_nNumActiveLights;
            }
            break;

        case IDC_LIGHT_SCALE:
            g_fLightScale = ( float )( g_SampleUI.GetSlider( IDC_LIGHT_SCALE )->GetValue() * 0.10f );

            swprintf_s( sz, 100, L"Light scale: %0.2f", g_fLightScale );
            g_SampleUI.GetStatic( IDC_LIGHT_SCALE_STATIC )->SetText( sz );
            break;

		
        case IDC_SPEED:
            g_dspeed = ( float )( g_SampleUI.GetSlider( IDC_SPEED )->GetValue() * 0.10f );
			if( s_model && s_model->m_curmotinfo ){
				s_model->SetMotionSpeed( g_dspeed );
			}

            swprintf_s( sz, 100, L"Motion Speed: %0.2f", g_dspeed );
            g_SampleUI.GetStatic( IDC_SPEED_STATIC )->SetText( sz );
            break;

		case IDC_COMBO_BONE:
			if( s_model ){
				pComboBox = g_SampleUI.GetComboBox( IDC_COMBO_BONE );
				tmpboneno = (int)PtrToUlong( pComboBox->GetSelectedData() );
				tmpbone = s_model->m_bonelist[ tmpboneno ];
				if( tmpbone ){
					s_curboneno = tmpboneno;
				}

				if( s_curboneno >= 0 ){
					int lineno = s_boneno2lineno[ s_curboneno ];
					if( lineno >= 0 ){
						s_owpTimeline->setCurrentLine( lineno, true );					
					}
				}
			}
			break;
		case IDC_COMBO_IKLEVEL:
			pComboBox = g_SampleUI.GetComboBox( IDC_COMBO_IKLEVEL );
			s_iklevel = (int)PtrToUlong( pComboBox->GetSelectedData() );
			break;
		case IDC_COMBO_SPLEVEL:
			pComboBox = g_SampleUI.GetComboBox( IDC_COMBO_SPLEVEL );
			s_splevel = (int)PtrToUlong( pComboBox->GetSelectedData() );
			break;
		case IDC_FK_XP:
			if( s_model && (s_curboneno >= 0) ){
				s_model->IKRotateAxisDelta( PICK_X, s_curboneno, radstep, 1 );
				s_model->SaveUndoMotion( s_curboneno, s_curbaseno );
			}
			break;
		case IDC_FK_XM:
			if( s_model && (s_curboneno >= 0) ){
				s_model->IKRotateAxisDelta( PICK_X, s_curboneno, -radstep, 1 );
				s_model->SaveUndoMotion( s_curboneno, s_curbaseno );
			}
			break;
		case IDC_FK_YP:
			if( s_model && (s_curboneno >= 0) ){
				s_model->IKRotateAxisDelta( PICK_Y, s_curboneno, radstep, 1 );
				s_model->SaveUndoMotion( s_curboneno, s_curbaseno );
			}
			break;
		case IDC_FK_YM:
			if( s_model && (s_curboneno >= 0) ){
				s_model->IKRotateAxisDelta( PICK_Y, s_curboneno, -radstep, 1 );
				s_model->SaveUndoMotion( s_curboneno, s_curbaseno );
			}
			break;
		case IDC_FK_ZP:
			if( s_model && (s_curboneno >= 0) ){
				AddBoneEul( 0.0f, 0.0f, radstep );
				s_model->SaveUndoMotion( s_curboneno, s_curbaseno );
			}
			break;
		case IDC_FK_ZM:
			if( s_model && (s_curboneno >= 0) ){
				AddBoneEul( 0.0f, 0.0f, -radstep );
				s_model->SaveUndoMotion( s_curboneno, s_curbaseno );
			}
			break;
		case IDC_T_XP:
			if( s_model && (s_curboneno >= 0) ){
				AddBoneTra( trastep, 0.0f, 0.0f );
				s_model->SaveUndoMotion( s_curboneno, s_curbaseno );
			}
			break;
		case IDC_T_XM:
			if( s_model && (s_curboneno >= 0) ){
				AddBoneTra( -trastep, 0.0f, 0.0f );
				s_model->SaveUndoMotion( s_curboneno, s_curbaseno );
			}
			break;
		case IDC_T_YP:
			if( s_model && (s_curboneno >= 0) ){
				AddBoneTra( 0.0f, trastep, 0.0f );
				s_model->SaveUndoMotion( s_curboneno, s_curbaseno );
			}
			break;
		case IDC_T_YM:
			if( s_model && (s_curboneno >= 0) ){
				AddBoneTra( 0.0, -trastep, 0.0f );
				s_model->SaveUndoMotion( s_curboneno, s_curbaseno );
			}
			break;
		case IDC_T_ZP:
			if( s_model && (s_curboneno >= 0) ){
				AddBoneTra( 0.0f, 0.0f, trastep );
				s_model->SaveUndoMotion( s_curboneno, s_curbaseno );
			}
			break;
		case IDC_T_ZM:
			if( s_model && (s_curboneno >= 0) ){
				AddBoneTra( 0.0f, 0.0f, -trastep );
				s_model->SaveUndoMotion( s_curboneno, s_curbaseno );
			}
			break;
		case IDC_CAMTARGET:
			s_camtargetflag = (int)s_CamTargetCheckBox->GetChecked();
			if( s_model && (s_curboneno >= 0) && s_camtargetflag ){
				CBone* curbone = s_model->m_bonelist[ s_curboneno ];
				_ASSERT( curbone );
				s_camtargetpos = curbone->m_childworld;
				weye = *(g_Camera.GetEyePt());
				g_Camera.SetViewParams( &weye, &s_camtargetpos );
			}			
			break;
		default:
			break;
	
	}

}


//--------------------------------------------------------------------------------------
// This callback function will be called immediately after the Direct3D device has 
// entered a lost state and before IDirect3DDevice9::Reset is called. Resources created
// in the OnResetDevice callback should be released here, which generally includes all 
// D3DPOOL_DEFAULT resources. See the "Lost Devices" section of the documentation for 
// information about lost devices.
//--------------------------------------------------------------------------------------
void CALLBACK OnLostDevice( void* pUserContext )
{
    g_DialogResourceManager.OnD3D9LostDevice();
    g_SettingsDlg.OnD3D9LostDevice();
    CDXUTDirectionWidget::StaticOnD3D9LostDevice();
    if( g_pFont )
        g_pFont->OnLostDevice();
    if( g_pEffect )
        g_pEffect->OnLostDevice();
    SAFE_RELEASE( g_pSprite );

	if( g_texbank ){
		CallF( g_texbank->Invalidate(), return );
	}

}


//--------------------------------------------------------------------------------------
// This callback function will be called immediately after the Direct3D device has 
// been destroyed, which generally happens as a result of application termination or 
// windowed/full screen toggles. Resources created in the OnCreateDevice callback 
// should be released here, which generally includes all D3DPOOL_MANAGED resources. 
//--------------------------------------------------------------------------------------
void CALLBACK OnDestroyDevice( void* pUserContext )
{

	CRdbIniFile* inifile = new CRdbIniFile();
	if( inifile ){
		WINPOS tmpwp[WINPOS_MAX];
		ZeroMemory( &tmpwp, sizeof( WINPOS ) * WINPOS_MAX );

		tmpwp[WINPOS_3D].posx = s_winpos[WINPOS_3D].posx;//OnFrameMoveでセットする
		tmpwp[WINPOS_3D].posy = s_winpos[WINPOS_3D].posy;		
		tmpwp[WINPOS_3D].width = s_winpos[WINPOS_3D].width;//OnResetDeviceでセットする
		tmpwp[WINPOS_3D].height = s_winpos[WINPOS_3D].height;


		tmpwp[WINPOS_TL].posx = s_timelineWnd->pos.x;
		tmpwp[WINPOS_TL].posy = s_timelineWnd->pos.y;
		tmpwp[WINPOS_TL].width = s_timelineWnd->size.x;
		tmpwp[WINPOS_TL].height = s_timelineWnd->size.y;
		tmpwp[WINPOS_TL].minwidth = s_timelineWnd->sizeMin.x;
		tmpwp[WINPOS_TL].minheight = s_timelineWnd->sizeMin.y;
		tmpwp[WINPOS_TL].dispflag = s_dispmw;

		tmpwp[WINPOS_MTL].posx = s_MtimelineWnd->pos.x;
		tmpwp[WINPOS_MTL].posy = s_MtimelineWnd->pos.y;
		tmpwp[WINPOS_MTL].width = s_MtimelineWnd->size.x;
		tmpwp[WINPOS_MTL].height = s_MtimelineWnd->size.y;
		tmpwp[WINPOS_MTL].minwidth = s_MtimelineWnd->sizeMin.x;
		tmpwp[WINPOS_MTL].minheight = s_MtimelineWnd->sizeMin.y;
		tmpwp[WINPOS_MTL].dispflag = s_Mdispmw;

		tmpwp[WINPOS_LTL].posx = s_LtimelineWnd->pos.x;
		tmpwp[WINPOS_LTL].posy = s_LtimelineWnd->pos.y;
		tmpwp[WINPOS_LTL].width = s_LtimelineWnd->size.x;
		tmpwp[WINPOS_LTL].height = s_LtimelineWnd->size.y;
		tmpwp[WINPOS_LTL].minwidth = s_LtimelineWnd->sizeMin.x;
		tmpwp[WINPOS_LTL].minheight = s_LtimelineWnd->sizeMin.y;
		tmpwp[WINPOS_LTL].dispflag = s_Ldispmw;

		tmpwp[WINPOS_TOOL].posx = s_toolWnd->pos.x;
		tmpwp[WINPOS_TOOL].posy = s_toolWnd->pos.y;
		tmpwp[WINPOS_TOOL].width = s_toolWnd->size.x;
		tmpwp[WINPOS_TOOL].height = s_toolWnd->size.y;
		tmpwp[WINPOS_TOOL].minwidth = s_toolWnd->sizeMin.x;
		tmpwp[WINPOS_TOOL].minheight = s_toolWnd->sizeMin.y;
		tmpwp[WINPOS_TOOL].dispflag = s_disptool;

		tmpwp[WINPOS_OBJ].posx = s_layerWnd->pos.x;
		tmpwp[WINPOS_OBJ].posy = s_layerWnd->pos.y;
		tmpwp[WINPOS_OBJ].width = s_layerWnd->size.x;
		tmpwp[WINPOS_OBJ].height = s_layerWnd->size.y;
		tmpwp[WINPOS_OBJ].minwidth = s_layerWnd->sizeMin.x;
		tmpwp[WINPOS_OBJ].minheight = s_layerWnd->sizeMin.y;
		tmpwp[WINPOS_OBJ].dispflag = s_dispobj;

		if( s_modelpanel.panel ){
			tmpwp[WINPOS_MODEL].posx = s_modelpanel.panel->pos.x;
			tmpwp[WINPOS_MODEL].posy = s_modelpanel.panel->pos.y;
			tmpwp[WINPOS_MODEL].width = s_modelpanel.panel->size.x;
			tmpwp[WINPOS_MODEL].height = s_modelpanel.panel->size.y;
			tmpwp[WINPOS_MODEL].minwidth = s_modelpanel.panel->sizeMin.x;
			tmpwp[WINPOS_MODEL].minheight = s_modelpanel.panel->sizeMin.y;
			tmpwp[WINPOS_MODEL].dispflag = 1;
		}else{
			MoveMemory( tmpwp + WINPOS_MODEL, inifile->m_defaultwp + WINPOS_MODEL, sizeof( WINPOS ) );
		}

		inifile->WriteRdbIniFile( tmpwp );

		delete inifile;
	}

    g_DialogResourceManager.OnD3D9DestroyDevice();
    g_SettingsDlg.OnD3D9DestroyDevice();
    CDXUTDirectionWidget::StaticOnD3D9DestroyDevice();
    SAFE_RELEASE( g_pEffect );
    SAFE_RELEASE( g_pFont );

	vector<MODELELEM>::iterator itrmodel;
	for( itrmodel = s_modelindex.begin(); itrmodel != s_modelindex.end(); itrmodel++ ){
		CModel* curmodel = itrmodel->modelptr;
		if( curmodel ){
			delete curmodel;
		}
	}
	s_modelindex.erase( s_modelindex.begin(), s_modelindex.end() );
	s_model = 0;

	int spacnt;
	for( spacnt = 0; spacnt < 3; spacnt++ ){
		if( s_spaxis[spacnt].sprite ){
			delete s_spaxis[spacnt].sprite;
			s_spaxis[spacnt].sprite = 0;
		}
	}

	if( s_select ){
		delete s_select;
		s_select = 0;
	}
	if( s_ground ){
		delete s_ground;
		s_ground = 0;
	}
	if( s_dummytri ){
		delete s_dummytri;
		s_dummytri = 0;
	}
	if( s_bmark ){
		delete s_bmark;
		s_bmark = 0;
	}
	if( s_bcircle ){
		delete s_bcircle;
		s_bcircle = 0;
	}
	if( s_kinsprite ){
		delete s_kinsprite;
		s_kinsprite = 0;
	}
	if( s_kinect ){
		delete s_kinect;
		s_kinect = 0;
	}

	if( g_texbank ){
		delete g_texbank;
		g_texbank = 0;
	}

	if( s_mainmenu ){
		DestroyMenu( s_mainmenu );
		s_mainmenu = 0;
	}

	DestroyTimeLine( 1 );

	if( s_timelineWnd ){
		delete s_timelineWnd;
		s_timelineWnd = 0;
	}
	if( s_MtimelineWnd ){
		delete s_MtimelineWnd;
		s_MtimelineWnd = 0;
	}
	if( s_LtimelineWnd ){
		delete s_LtimelineWnd;
		s_LtimelineWnd = 0;
	}

	if( s_toolWnd ){
		delete s_toolWnd;
		s_toolWnd = 0;
	}

	if( s_toolCopyB ){
		delete s_toolCopyB;
		s_toolCopyB = 0;
	}
	if( s_toolSymCopyB ){
		delete s_toolSymCopyB;
		s_toolSymCopyB = 0;
	}
	if( s_toolSymCopy2B ){
		delete s_toolSymCopy2B;
		s_toolSymCopy2B = 0;
	}
	if( s_toolCutB ){
		delete s_toolCutB;
		s_toolCutB = 0;
	}
	if( s_toolPasteB ){
		delete s_toolPasteB;
		s_toolPasteB = 0;
	}
	if( s_toolDeleteB ){
		delete s_toolDeleteB;
		s_toolDeleteB = 0;
	}
	if( s_toolNewKeyB ){
		delete s_toolNewKeyB;
		s_toolNewKeyB = 0;
	}
	if( s_toolNewKeyAllB ){
		delete s_toolNewKeyAllB;
		s_toolNewKeyAllB = 0;
	}
	if( s_toolMNewKeyB ){
		delete s_toolMNewKeyB;
		s_toolMNewKeyB = 0;
	}
	if( s_toolMNewKeyAllB ){
		delete s_toolMNewKeyAllB;
		s_toolMNewKeyAllB = 0;
	}
	if( s_toolInitKeyB ){
		delete s_toolInitKeyB;
		s_toolInitKeyB = 0;
	}

	if( s_owpTimeline ){
		delete s_owpTimeline;
		s_owpTimeline = 0;
	}
	if( s_owpPlayerButton ){
		delete s_owpPlayerButton;
		s_owpPlayerButton = 0;
	}
	if( s_owpMTimeline ){
		delete s_owpMTimeline;
		s_owpMTimeline = 0;
	}
	if( s_owpMPlayerButton ){
		delete s_owpMPlayerButton;
		s_owpMPlayerButton = 0;
	}
	if( s_owpLTimeline ){
		delete s_owpLTimeline;
		s_owpLTimeline = 0;
	}
	if( s_owpLPlayerButton ){
		delete s_owpLPlayerButton;
		s_owpLPlayerButton = 0;
	}

	if( s_layerWnd ){
		delete s_layerWnd;
		s_layerWnd = 0;
	}
	if( s_owpLayerTable ){
		delete s_owpLayerTable;
		s_owpLayerTable = 0;
	}

	if( s_morphWnd ){
		delete s_morphWnd;
		s_morphWnd = 0;
	}
	if( s_morphSlider ){
		delete s_morphSlider;
		s_morphSlider = 0;
	}
	if( s_morphOneB ){
		delete s_morphOneB;
		s_morphOneB = 0;
	}
	if( s_morphOneExB ){
		delete s_morphOneExB;
		s_morphOneExB = 0;
	}
	if( s_morphZeroB ){
		delete s_morphZeroB;
		s_morphZeroB = 0;
	}
	if( s_morphDelB ){
		delete s_morphDelB;
		s_morphDelB = 0;
	}
	if( s_morphCalcB ){
		delete s_morphCalcB;
		s_morphCalcB = 0;
	}
	if( s_morphCloseB ){
		delete s_morphCloseB;
		s_morphCloseB = 0;
	}

	if( s_tlprop_bmp != NULL ){
		DeleteObject( s_tlprop_bmp );
		s_tlprop_bmp = NULL;
	}
	if( s_tlscale_bmp != NULL ){
		DeleteObject( s_tlscale_bmp );
		s_tlscale_bmp = NULL;
	}


	DestroyModelPanel();

	DestroySdkObjects();

}

int DestroyTimeLine( int dellist )
{
	if( dellist ){
		EraseKeyList();
	}

	s_tlarray.clear();
	
	return 0;
}

int GetShaderHandle()
{
	if( !g_pEffect ){
		_ASSERT( 0 );
		return 1;
	}

	g_hRenderBoneL0 = g_pEffect->GetTechniqueByName( "RenderBoneL0" );
	_ASSERT( g_hRenderBoneL0 );
	g_hRenderBoneL1 = g_pEffect->GetTechniqueByName( "RenderBoneL1" );
	_ASSERT( g_hRenderBoneL1 );
	g_hRenderBoneL2 = g_pEffect->GetTechniqueByName( "RenderBoneL2" );
	_ASSERT( g_hRenderBoneL2 );
	g_hRenderBoneL3 = g_pEffect->GetTechniqueByName( "RenderBoneL3" );
	_ASSERT( g_hRenderBoneL3 );

	g_hRenderNoBoneL0 = g_pEffect->GetTechniqueByName( "RenderNoBoneL0" );
	_ASSERT( g_hRenderNoBoneL0 );
	g_hRenderNoBoneL1 = g_pEffect->GetTechniqueByName( "RenderNoBoneL1" );
	_ASSERT( g_hRenderNoBoneL1 );
	g_hRenderNoBoneL2 = g_pEffect->GetTechniqueByName( "RenderNoBoneL2" );
	_ASSERT( g_hRenderNoBoneL2 );
	g_hRenderNoBoneL3 = g_pEffect->GetTechniqueByName( "RenderNoBoneL3" );
	_ASSERT( g_hRenderNoBoneL3 );

	g_hRenderLine = g_pEffect->GetTechniqueByName( "RenderLine" );
	_ASSERT( g_hRenderLine );
	g_hRenderSprite = g_pEffect->GetTechniqueByName( "RenderSprite" );
	_ASSERT( g_hRenderSprite );


	g_hmBoneQ = g_pEffect->GetParameterByName( NULL, "g_mBoneQ" );
	_ASSERT( g_hmBoneQ );
	g_hmBoneTra = g_pEffect->GetParameterByName( NULL, "g_mBoneTra" );
	_ASSERT( g_hmBoneTra );
	g_hmWorld = g_pEffect->GetParameterByName( NULL, "g_mWorld" );
	_ASSERT( g_hmWorld );
	g_hmVP = g_pEffect->GetParameterByName( NULL, "g_mVP" );
	_ASSERT( g_hmVP );
	g_hEyePos = g_pEffect->GetParameterByName( NULL, "g_EyePos" );
	_ASSERT( g_hEyePos );


	g_hnNumLight = g_pEffect->GetParameterByName( NULL, "g_nNumLights" );
	_ASSERT( g_hnNumLight );
	g_hLightDir = g_pEffect->GetParameterByName( NULL, "g_LightDir" );
	_ASSERT( g_hLightDir );
	g_hLightDiffuse = g_pEffect->GetParameterByName( NULL, "g_LightDiffuse" );
	_ASSERT( g_hLightDiffuse );
	g_hLightAmbient = g_pEffect->GetParameterByName( NULL, "g_LightAmbient" );
	_ASSERT( g_hLightAmbient );

	g_hdiffuse = g_pEffect->GetParameterByName( NULL, "g_diffuse" );
	_ASSERT( g_hdiffuse );
	g_hambient = g_pEffect->GetParameterByName( NULL, "g_ambient" );
	_ASSERT( g_hambient );
	g_hspecular = g_pEffect->GetParameterByName( NULL, "g_specular" );
	_ASSERT( g_hspecular );
	g_hpower = g_pEffect->GetParameterByName( NULL, "g_power" );
	_ASSERT( g_hpower );
	g_hemissive = g_pEffect->GetParameterByName( NULL, "g_emissive" );
	_ASSERT( g_hemissive );
	g_hMeshTexture = g_pEffect->GetParameterByName( NULL, "g_MeshTexture" );
	_ASSERT( g_hMeshTexture );

	return 0;
}

int SetBaseDir()
{
	WCHAR filename[MAX_PATH] = {0};
	WCHAR* endptr = 0;

	int ret;
	ret = GetModuleFileNameW( NULL, filename, MAX_PATH );
	if( ret == 0 ){
		_ASSERT( 0 );
		return 1;
	}


    WCHAR* lasten = NULL;
    lasten = wcsrchr( filename, TEXT( '\\' ) );
	if( !lasten ){
		_ASSERT( 0 );
		DbgOut( L"SetMediaDir : strrchr error !!!\n" );
		return 1;
	}

	*lasten = 0;

	WCHAR* last2en = 0;
	WCHAR* last3en = 0;
	last2en = wcsrchr( filename, TEXT( '\\' ) );
	if( last2en ){
		*last2en = 0;
		last3en = wcsrchr( filename, TEXT( '\\' ) );
		if( last3en ){
			if( (wcscmp( last2en + 1, L"debug" ) == 0) ||
				(wcscmp( last2en + 1, L"Debug" ) == 0) ||
				(wcscmp( last2en + 1, L"DEBUG" ) == 0) ||
				(wcscmp( last2en + 1, L"release" ) == 0) ||
				(wcscmp( last2en + 1, L"Release" ) == 0) ||
				(wcscmp( last2en + 1, L"RELEASE" ) == 0)
				){

				endptr = last2en;
			}else{
				endptr = lasten;
			}
		}else{
			endptr = lasten;
		}
	}else{
		endptr = lasten;
	}

	*lasten = TEXT( '\\' );
	if( last2en )
		*last2en = TEXT( '\\' );
	if( last3en )
		*last3en = TEXT( '\\' );

	int leng;
	ZeroMemory( g_basedir, sizeof( WCHAR ) * MAX_PATH );
	leng = (int)( endptr - filename + 1 );
	wcsncpy_s( g_basedir, MAX_PATH, filename, leng );

	wcscpy_s( g_mediadir, MAX_PATH, g_basedir );
	WCHAR* mlasten = wcsrchr( g_mediadir, TEXT( '\\' ) );
	if( mlasten ){
		*mlasten = 0L;
		WCHAR* mlasten2 = wcsrchr( g_mediadir, TEXT( '\\' ) );
		if( mlasten2 ){
			*(mlasten2 + 1) = 0L;
			wcscat_s( g_mediadir, MAX_PATH, L"Media\\RDBMedia\\" );
		}
	}

	DbgOut( L"SetBaseDir : %s\r\n", g_basedir );
	DbgOut( L"SetBaseDir : MediaDir : %s\r\n", g_mediadir );

	return 0;
}

int OpenFile()
{
	int dlgret;
	dlgret = (int)DialogBoxW( (HINSTANCE)GetModuleHandle(NULL), MAKEINTRESOURCE( IDD_OPENMQODLG ), 
		s_mainwnd, (DLGPROC)OpenMqoDlgProc );
	if( (dlgret != IDOK) || !g_tmpmqopath[0] ){
		return 0;
	}


	WCHAR savepath[MULTIPATH];
	MoveMemory( savepath, g_tmpmqopath, sizeof( WCHAR ) * MULTIPATH );


	int leng;
	int namecnt = 0;
	leng = (int)wcslen( savepath );
	WCHAR* topchar = savepath + leng + 1;
	if( *topchar == TEXT( '\0' ) ){
		WCHAR* extptr = 0;
		extptr = wcsrchr( g_tmpmqopath, TEXT( '.' ) );
		if( !extptr ){
			return 0;
		}
		int cmprdb, cmpmqo, cmpqub;
		cmprdb = wcscmp( extptr, L".rdb" );
		cmpmqo = wcscmp( extptr, L".mqo" );
		cmpqub = wcscmp( extptr, L".qub" );
		if( cmprdb == 0 ){
			CallF( OpenRdbFile(), return 1 );
			s_filterindex = 1;
		}else if( cmpmqo == 0 ){
			OpenMQOFile();
			s_filterindex = 1;
		}else if( cmpqub == 0 ){
			CallF( OpenQubFile(), return 1 );
			s_filterindex = 2;
		}
	}else{
		int leng2;
		while( *topchar != TEXT( '\0' ) ){
			swprintf_s( g_tmpmqopath, MULTIPATH, L"%s\\%s", savepath, topchar );

			WCHAR* extptr = 0;
			extptr = wcsrchr( g_tmpmqopath, TEXT( '.' ) );
			if( !extptr ){
				return 0;
			}
			int cmprdb, cmpmqo, cmpqub;
			cmprdb = wcscmp( extptr, L".rdb" );
			cmpmqo = wcscmp( extptr, L".mqo" );
			cmpqub = wcscmp( extptr, L".qub" );
			if( cmprdb == 0 ){
				CallF( OpenRdbFile(), return 1 );
				s_filterindex = 1;
			}else if( cmpmqo == 0 ){
				OpenMQOFile();
				s_filterindex = 1;
			}else if( cmpqub == 0 ){
				CallF( OpenQubFile(), return 1 );
				s_filterindex = 2;
			}

			leng2 = (int)wcslen( topchar );
			topchar = topchar + leng2 + 1;
			namecnt++;
		}
	}

	s_cursorFlag = false;
	s_McursorFlag = false;
	return 0;
}

CModel* OpenMQOFileFromPnd( CPmCipherDll* cipher, char* modelfolder, char* mqopath )
{
	s_camtargetpos = D3DXVECTOR3( 0.0f, 0.0f, 0.0f );

	int propnum = cipher->GetPropertyNum();
	if( propnum <= 0 ){
		_ASSERT( 0 );
		return 0;
	}

	int findindex = -1;
	int propno;
	for( propno = 0; propno < propnum; propno++ ){
		PNDPROP prop;
		ZeroMemory( &prop, sizeof( PNDPROP ) );
		CallF( cipher->GetProperty( propno, &prop ), return 0 );
		int cmp;
		cmp = strcmp( prop.path, mqopath );
		if( cmp == 0 ){
			findindex = propno;
			break;
		}
	}
	if( findindex < 0 ){
		_ASSERT( 0 );
		return 0;
	}

	if( !g_texbank ){
		g_texbank = new CTexBank( s_pdev );
		if( !g_texbank ){
			_ASSERT( 0 );
			return 0;
		}
	}
	if( s_model && (s_curmodelmenuindex >= 0) && (s_modelindex.empty() == 0) ){
		s_modelindex[s_curmodelmenuindex].tlarray = s_tlarray;
		s_modelindex[s_curmodelmenuindex].lineno2boneno = s_lineno2boneno;
		s_modelindex[s_curmodelmenuindex].boneno2lineno = s_boneno2lineno;
	}

	DestroyTimeLine( 1 );

    // Load the mesh
	CModel* newmodel;
	newmodel = new CModel();
	if( !newmodel ){
		_ASSERT( 0 );
		return 0;
	}
	int ret;
	ret = newmodel->LoadMQOFromPnd( s_pdev, cipher, findindex, modelfolder, g_tmpmqomult );
	if( ret ){
		delete newmodel;
		if( s_owpTimeline && s_owpMTimeline ){
			refreshTimeline( *s_owpTimeline, *s_owpMTimeline );
		}
		return 0;
	}else{
		s_model = newmodel;
	}
	CallF( s_model->CalcInf(), return 0 );
	CallF( s_model->MakeDispObj(), return 0 );
	CallF( s_model->DbgDump(), return 0 );

	int mindex;
	mindex = s_modelindex.size();
	MODELELEM modelelem;
	modelelem.modelptr = s_model;
	modelelem.motmenuindex = 0;
	s_modelindex.push_back( modelelem );

    CDXUTComboBox* pComboBox = g_SampleUI.GetComboBox( IDC_COMBO_BONE );
	pComboBox->RemoveAllItems();

	map<int, CBone*>::iterator itrbone;
	for( itrbone = s_model->m_bonelist.begin(); itrbone != s_model->m_bonelist.end(); itrbone++ ){
		ULONG boneno = (ULONG)itrbone->first;
		CBone* curbone = itrbone->second;
		if( curbone && (boneno >= 0) ){
			char* nameptr = curbone->m_bonename;
			WCHAR wname[256];
			MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, nameptr, 256, wname, 256 );
			pComboBox->AddItem( wname, ULongToPtr( boneno ) );
		}
	}

	s_totalmb.center = D3DXVECTOR3( 0.0f, 0.0f, 0.0f );
	s_totalmb.max = D3DXVECTOR3( 0.0f, 0.0f, 0.0f );
	s_totalmb.min = D3DXVECTOR3( 0.0f, 0.0f, 0.0f );
	s_totalmb.r = 0.0f;

	vector<MODELELEM>::iterator itrmodel;
	for( itrmodel = s_modelindex.begin(); itrmodel != s_modelindex.end(); itrmodel++ ){
		CModel* curmodel = itrmodel->modelptr;
		MODELBOUND mb;
		curmodel->GetModelBound( &mb );
		AddModelBound( &s_totalmb, &mb );
	}

    FLOAT fObjectRadius;
	g_vCenter = s_totalmb.center;
	fObjectRadius = s_totalmb.r;

	s_cammvstep = fObjectRadius;

    D3DXMatrixTranslation( &g_mCenterWorld, -g_vCenter.x, -g_vCenter.y, -g_vCenter.z );

	s_projnear = fObjectRadius * 0.01f;
	g_initcamz = fObjectRadius * 3.0f;
	g_Camera.SetProjParams( D3DX_PI / 4, s_fAspectRatio, s_projnear, 3.0f * g_initcamz );


    for( int i = 0; i < MAX_LIGHTS; i++ )
		g_LightControl[i].SetRadius( fObjectRadius );


    D3DXVECTOR3 vecEye( 0.0f, 0.0f, -g_initcamz );
    D3DXVECTOR3 vecAt ( 0.0f, 0.0f, -0.0f );
    g_Camera.SetViewParams( &vecEye, &vecAt );
    g_Camera.SetRadius( fObjectRadius * 3.0f, fObjectRadius * 0.5f, fObjectRadius * 6.0f );

    for( int i = 0; i < MAX_LIGHTS; i++ )
		g_LightControl[i].SetRadius( fObjectRadius );


	CallF( AddMotion( 0 ), return 0 );

	s_modelindex[ mindex ].tlarray = s_tlarray;
	s_modelindex[ mindex ].lineno2boneno = s_lineno2boneno;
	s_modelindex[ mindex ].boneno2lineno = s_boneno2lineno;

	CallF( OnModelMenu( mindex, 0 ), return 0 );

	CallF( CreateModelPanel(), return 0 );

	s_cursorFlag = false;
	s_McursorFlag = false;


	return newmodel;
}


CModel* OpenMQOFile()
{
	s_camtargetpos = D3DXVECTOR3( 0.0f, 0.0f, 0.0f );

	static int modelcnt = 0;
	WCHAR modelname[MAX_PATH] = {0L};
	WCHAR* lasten;
	lasten = wcsrchr( g_tmpmqopath, TEXT('\\') );
	if( !lasten ){
		_ASSERT( 0 );
		return 0;
	}
	wcscpy_s( modelname, MAX_PATH, lasten + 1 );
	WCHAR* extptr;
	extptr = wcsrchr( modelname, TEXT('.') );
	if( !extptr ){
		_ASSERT( 0 );
		return 0;
	}
	*extptr = 0L;
	WCHAR modelfolder[MAX_PATH] = {0L};
	swprintf_s( modelfolder, MAX_PATH, L"%s_%d", modelname, modelcnt );
	modelcnt++;


	if( !g_texbank ){
		g_texbank = new CTexBank( s_pdev );
		if( !g_texbank ){
			_ASSERT( 0 );
			return 0;
		}
	}
	if( s_model && (s_curmodelmenuindex >= 0) && (s_modelindex.empty() == 0) ){
		s_modelindex[s_curmodelmenuindex].tlarray = s_tlarray;
		s_modelindex[s_curmodelmenuindex].lineno2boneno = s_lineno2boneno;
		s_modelindex[s_curmodelmenuindex].boneno2lineno = s_boneno2lineno;
	}

	DestroyTimeLine( 1 );

    // Load the mesh
	CModel* newmodel;
	newmodel = new CModel();
	if( !newmodel ){
		_ASSERT( 0 );
		return 0;
	}
	int ret;
	ret = newmodel->LoadMQO( s_pdev, g_tmpmqopath, modelfolder, g_tmpmqomult, 0 );
	if( ret ){
		delete newmodel;
		if( s_owpTimeline ){
			refreshTimeline( *s_owpTimeline, *s_owpMTimeline );
		}
		return 0;
	}else{
		s_model = newmodel;
	}
	CallF( s_model->CalcInf(), return 0 );
	CallF( s_model->MakeDispObj(), return 0 );
	CallF( s_model->DbgDump(), return 0 );

	int mindex;
	mindex = s_modelindex.size();
	MODELELEM modelelem;
	modelelem.modelptr = s_model;
	modelelem.motmenuindex = 0;
	s_modelindex.push_back( modelelem );

    CDXUTComboBox* pComboBox = g_SampleUI.GetComboBox( IDC_COMBO_BONE );
	pComboBox->RemoveAllItems();

	map<int, CBone*>::iterator itrbone;
	for( itrbone = s_model->m_bonelist.begin(); itrbone != s_model->m_bonelist.end(); itrbone++ ){
		ULONG boneno = (ULONG)itrbone->first;
		CBone* curbone = itrbone->second;
		if( curbone && (boneno >= 0) ){
			char* nameptr = curbone->m_bonename;
			WCHAR wname[256];
			MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, nameptr, 256, wname, 256 );
			pComboBox->AddItem( wname, ULongToPtr( boneno ) );
		}
	}

	s_totalmb.center = D3DXVECTOR3( 0.0f, 0.0f, 0.0f );
	s_totalmb.max = D3DXVECTOR3( 0.0f, 0.0f, 0.0f );
	s_totalmb.min = D3DXVECTOR3( 0.0f, 0.0f, 0.0f );
	s_totalmb.r = 0.0f;

	vector<MODELELEM>::iterator itrmodel;
	for( itrmodel = s_modelindex.begin(); itrmodel != s_modelindex.end(); itrmodel++ ){
		CModel* curmodel = itrmodel->modelptr;
		MODELBOUND mb;
		curmodel->GetModelBound( &mb );
		AddModelBound( &s_totalmb, &mb );
	}

    FLOAT fObjectRadius;
	g_vCenter = s_totalmb.center;
	fObjectRadius = s_totalmb.r;

	s_cammvstep = fObjectRadius;

    D3DXMatrixTranslation( &g_mCenterWorld, -g_vCenter.x, -g_vCenter.y, -g_vCenter.z );

	s_projnear = fObjectRadius * 0.01f;
	g_initcamz = fObjectRadius * 3.0f;
	g_Camera.SetProjParams( D3DX_PI / 4, s_fAspectRatio, s_projnear, 3.0f * g_initcamz );


    for( int i = 0; i < MAX_LIGHTS; i++ )
		g_LightControl[i].SetRadius( fObjectRadius );


    D3DXVECTOR3 vecEye( 0.0f, 0.0f, -g_initcamz );
    D3DXVECTOR3 vecAt ( 0.0f, 0.0f, -0.0f );
    g_Camera.SetViewParams( &vecEye, &vecAt );
    g_Camera.SetRadius( fObjectRadius * 3.0f, fObjectRadius * 0.5f, fObjectRadius * 6.0f );

	CallF( AddMotion( 0 ), return 0 );

	s_modelindex[ mindex ].tlarray = s_tlarray;
	s_modelindex[ mindex ].lineno2boneno = s_lineno2boneno;
	s_modelindex[ mindex ].boneno2lineno = s_boneno2lineno;

	CallF( OnModelMenu( mindex, 0 ), return 0 );

	CallF( CreateModelPanel(), return 0 );

	return newmodel;
}

int OpenQubFileFromPnd( CPmCipherDll* cipher, char* motpath )
{
	if( !s_model ){
		_ASSERT( 0 );
		return 0;
	}

	int propnum = cipher->GetPropertyNum();
	if( propnum <= 0 ){
		_ASSERT( 0 );
		return 0;
	}

	int findindex = -1;
	int propno;
	for( propno = 0; propno < propnum; propno++ ){
		PNDPROP prop;
		ZeroMemory( &prop, sizeof( PNDPROP ) );
		CallF( cipher->GetProperty( propno, &prop ), return 0 );
		int cmp;
		cmp = strcmp( prop.path, motpath );
		if( cmp == 0 ){
			findindex = propno;
			break;
		}
	}
	if( findindex < 0 ){
		_ASSERT( 0 );
		return 0;
	}

	int newmotid = -1;
	CallF( s_model->LoadQubFileFromPnd( cipher, findindex, &newmotid ), return 1 );

	MOTINFO* newmotinfo = s_model->m_motinfo[ newmotid ];

	CallF( AddTimeLine( newmotid ), return 1 );

	int selindex = s_tlarray.size() - 1;
	CallF( OnAnimMenu( selindex ), return 1 );


	return 0;
}


int OpenQubFile()
{
	if( !s_model ){
		_ASSERT( 0 );
		return 0;
	}

	int newmotid = -1;
	CallF( s_model->LoadQubFile( g_tmpmqopath, &newmotid ), return 1 );

	MOTINFO* newmotinfo = s_model->m_motinfo[ newmotid ];

	CallF( AddTimeLine( newmotid ), return 1 );

	int selindex = s_tlarray.size() - 1;
	CallF( OnAnimMenu( selindex ), return 1 );

	return 0;
}

int AddTimeLine( int newmotid )
{
	//EraseKeyList();

	if( !s_owpTimeline && s_model && (s_model->m_bonelist.size() > 0) ){
		OWP_Timeline* owpTimeline = 0;
		//タイムラインのGUIパーツを生成
		owpTimeline= new OWP_Timeline( L"testmotion" );

		//ウィンドウにタイムラインを関連付ける
		s_timelineWnd->addParts(*owpTimeline);

		// カーソル移動時のイベントリスナーに
		// カーソル移動フラグcursorFlagをオンにするラムダ関数を登録する
		owpTimeline->setCursorListener( [](){ s_cursorFlag=true; } );

		// キー選択時のイベントリスナーに
		// キー選択フラグselectFlagをオンにするラムダ関数を登録する
		owpTimeline->setSelectListener( [](){ s_selectFlag=true; } );


		// キー移動時のイベントリスナーに
		// キー移動フラグkeyShiftFlagをオンにして、キー移動量をコピーするラムダ関数を登録する
		owpTimeline->setKeyShiftListener( [](){
			s_keyShiftFlag= true;
			s_keyShiftTime= s_owpTimeline->getShiftKeyTime();
		} );

		// キー削除時のイベントリスナーに
		// 削除されたキー情報をスタックするラムダ関数を登録する
		owpTimeline->setKeyDeleteListener( [](const OWP_Timeline::KeyInfo &keyInfo){
			s_deletedKeyInfoList.push_back(keyInfo);
		} );

		s_owpTimeline = owpTimeline;
	}

	if( !s_owpMTimeline && s_model && (s_model->m_bonelist.size() > 0) ){
		OWP_Timeline* owpTimeline = 0;
		//タイムラインのGUIパーツを生成
		owpTimeline= new OWP_Timeline( L"testMmotion" );

		//ウィンドウにタイムラインを関連付ける
		s_MtimelineWnd->addParts(*owpTimeline);

		// カーソル移動時のイベントリスナーに
		// カーソル移動フラグcursorFlagをオンにするラムダ関数を登録する
		owpTimeline->setCursorListener( [](){ s_McursorFlag=true; } );

		// キー選択時のイベントリスナーに
		// キー選択フラグselectFlagをオンにするラムダ関数を登録する
		owpTimeline->setSelectListener( [](){ s_MselectFlag=true; } );


		// キー移動時のイベントリスナーに
		// キー移動フラグkeyShiftFlagをオンにして、キー移動量をコピーするラムダ関数を登録する
		owpTimeline->setKeyShiftListener( [](){
			s_MkeyShiftFlag= true;
			s_MkeyShiftTime= s_owpMTimeline->getShiftKeyTime();
		} );

		// キー削除時のイベントリスナーに
		// 削除されたキー情報をスタックするラムダ関数を登録する
		owpTimeline->setKeyDeleteListener( [](const OWP_Timeline::KeyInfo &keyInfo){
			s_MdeletedKeyInfoList.push_back(keyInfo);
		} );

		s_owpMTimeline = owpTimeline;

		s_owpMTimeline->setCallPropertyListener( []( int targetline, double targettime, POINT targetpos ){
			s_selmorph.mk = 0;
			s_selmorph.target = 0;

			HWND hwnd = s_MtimelineWnd->getHWnd();
			OWP_Timeline::KeyInfo ki = s_owpMTimeline->ExistKey( targetline, (double)( (int)targettime ) );			

			DbgOut( L"morphtl : property : targetline %d, targettime %f, targetpos (%d, %d), hwnd %x\r\n",
				targetline, targettime, targetpos.x, targetpos.y, hwnd );


			if( ki.lineIndex >= 0 ){
				CMorphKey* mk = (CMorphKey*)ki.object;
				CRMenuMain rmenu;
				CallF( rmenu.Create( s_mainwnd, mk, mk->m_baseobj->m_morphobj ), return );

				DbgOut( L"morph Prop : MorphKey : baseno %d, time %f\r\n", mk->m_baseobj->m_objectno, mk->m_frame );

				CMQOObject* curtarget = rmenu.TrackPopupMenu( hwnd, targetpos, mk->m_baseobj->m_morphobj );
				if( curtarget ){
					WCHAR wname[256];
					MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, curtarget->m_name, 256, wname, 256 );

					s_selmorph.mk = mk;
					s_selmorph.target = curtarget;
					s_oneditmorph = true;
					s_morphWnd->setVisible( true );

					map<CMQOObject*,float>::iterator itrfind;
					itrfind = mk->m_blendweight.find( curtarget );
					if( itrfind != mk->m_blendweight.end() ){
						s_morphSlider->setValue( mk->m_blendweight[ curtarget ] );
					}else{
						s_morphSlider->setValue( 0.0 );
					}
					s_morphWnd->callRewrite();

					DbgOut( L"RMenu select : target %s\r\n", wname );
				}

				CallF( rmenu.Destroy(), return );
				
			}
		} );
	}

	if( !s_owpLTimeline && s_model && (s_model->m_bonelist.size() > 0) ){
		s_owpLTimeline= new OWP_Timeline( L"LongTimeLine" );
		s_LtimelineWnd->addParts(*s_owpLTimeline);
		s_owpLTimeline->setCursorListener( [](){ s_LcursorFlag=true; } );
	}

	if( s_owpTimeline && (s_model->m_bonelist.size() > 0) ){
		int nextindex;
		nextindex = s_tlarray.size();

		TLELEM newtle;
		newtle.menuindex = nextindex;
		newtle.motionid = newmotid;
		s_tlarray.push_back( newtle );

		CallF( s_model->SetCurrentMotion( newmotid ), return 1 );
	}

	//タイムラインのキーを設定
	if( s_owpTimeline ){
		refreshTimeline(*s_owpTimeline, *s_owpMTimeline);
	}
	return 0;
}

//タイムラインにモーションデータのキーを設定する
void refreshTimeline(OWP_Timeline& timeline, OWP_Timeline& Mtimeline, int rbundoflag)
{
	s_selmorph.mk = 0;
	s_selmorph.target = 0;

	//時刻幅を設定
	if( s_model && (s_model->m_curmotinfo) ){
		timeline.setMaxTime( s_model->m_curmotinfo->frameleng );
		Mtimeline.setMaxTime( s_model->m_curmotinfo->frameleng );
		s_owpLTimeline->setMaxTime( s_model->m_curmotinfo->frameleng );
	}

	s_lineno2boneno.clear();
	s_boneno2lineno.clear();


	int labelwidth = 75;

	if( s_model && s_model->m_topbone ){
		CallF( s_model->FillTimeLine( timeline, Mtimeline, s_lineno2boneno, s_boneno2lineno, &labelwidth ), return );
	}else{
		//すべての行をクリア
		timeline.deleteLine();
		Mtimeline.deleteLine();

		WCHAR label[256];
		swprintf_s(label, 256, L"dummy%d", 0 );
		timeline.newLine(label);
		Mtimeline.newLine(label);
	}

	timeline.LABEL_SIZE_X = labelwidth;

	//選択時刻を設定
	if( rbundoflag == 0 ){
		timeline.setCurrentLine( 0 );
		Mtimeline.setCurrentLine( 0 );
		Mtimeline.setCurrentTime(0.0, true);
	}

	CDXUTComboBox* pComboBox = g_SampleUI.GetComboBox( IDC_COMBO_BONE );
	pComboBox->RemoveAllItems();
	if( s_model ){
		map<int, CBone*>::iterator itrbone;
		for( itrbone = s_model->m_bonelist.begin(); itrbone != s_model->m_bonelist.end(); itrbone++ ){
			ULONG boneno = (ULONG)itrbone->first;
			CBone* curbone = itrbone->second;
			if( curbone && (boneno >= 0) ){
				char* nameptr = curbone->m_bonename;
				WCHAR wname[256];
				MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, nameptr, 256, wname, 256 );
				pComboBox->AddItem( wname, ULongToPtr( boneno ) );
			}
		}
	}
}

int AddBoneEul( float addx, float addy, float addz )
{
	if( !s_model || (s_curboneno < 0) ){
		return 0;
	}

	if( (fabs( addx ) < 0.05f) && (fabs( addy ) < 0.05f) && (fabs( addz ) < 0.05f) ){
		return 0;
	}

	D3DXVECTOR3 cureul, neweul;

	CMotionPoint* newmp = 0;
	int existflag = 0;
	int calcflag = 1;
	_ASSERT( s_model->m_curmotinfo );
	newmp = s_model->AddMotionPoint( s_model->m_curmotinfo->motid, s_owpTimeline->getCurrentTime(), s_curboneno, calcflag, &existflag );
	if( !newmp ){
		_ASSERT( 0 );
		return 1;
	}

	CBone* curbone = s_model->m_bonelist[ s_curboneno ];
	_ASSERT( curbone );

	cureul = newmp->m_eul;
	neweul.x = cureul.x + addx;
	neweul.y = cureul.y + addy;
	neweul.z = cureul.z + addz;
	newmp->SetEul( &(curbone->m_axisq), neweul );

	if( existflag == 0 ){
		s_owpTimeline->newKey( curbone->m_wbonename, s_owpTimeline->getCurrentTime(), (void*)newmp );
	}

	return 0;
}

int AddBoneTra( float addx, float addy, float addz )
{
	if( !s_model || (s_curboneno < 0) ){
		return 0;
	}

	D3DXVECTOR3 curtra, newtra;

	CMotionPoint* newmp = 0;
	int existflag = 0;
	int calcflag = 1;
	_ASSERT( s_model->m_curmotinfo );
	newmp = s_model->AddMotionPoint( s_model->m_curmotinfo->motid, s_owpTimeline->getCurrentTime(), s_curboneno, calcflag, &existflag );
	if( !newmp ){
		_ASSERT( 0 );
		return 1;
	}

	curtra = newmp->m_tra;
	newtra.x = curtra.x + addx;
	newtra.y = curtra.y + addy;
	newtra.z = curtra.z + addz;
	newmp->m_tra = newtra;

	if( existflag == 0 ){
		CBone* curbone = s_model->m_bonelist[ s_curboneno ];
		_ASSERT( curbone );
		s_owpTimeline->newKey( curbone->m_wbonename, s_owpTimeline->getCurrentTime(), (void*)newmp );
	}

	return 0;
}

int DispMotionWindow()
{
	if( !s_timelineWnd ){
		return 0;
	}

	if( s_dispmw ){
		s_timelineWnd->setVisible( false );
		s_dispmw = false;
	}else{
		s_timelineWnd->setVisible( true );
		s_dispmw = true;
	}

	return 0;
}
int DispMorphMotionWindow()
{
	if( !s_MtimelineWnd ){
		return 0;
	}

	if( s_Mdispmw ){
		s_MtimelineWnd->setVisible( false );
		s_Mdispmw = false;
	}else{
		s_MtimelineWnd->setVisible( true );
		s_Mdispmw = true;
	}

	return 0;
}
int DispLongMotionWindow()
{
	if( !s_LtimelineWnd ){
		return 0;
	}

	if( s_Ldispmw ){
		s_LtimelineWnd->setVisible( false );
		s_Ldispmw = false;
	}else{
		s_LtimelineWnd->setVisible( true );
		s_Ldispmw = true;
	}

	return 0;
}

int DispToolWindow()
{
	if( !s_toolWnd ){
		return 0;
	}

	if( s_disptool ){
		s_toolWnd->setVisible( false );
		s_disptool = false;
	}else{
		s_toolWnd->setVisible( true );
		s_disptool = true;
	}

	return 0;
}
int DispObjPanel()
{
	if( !s_layerWnd ){
		return 0;
	}

	if( s_dispobj ){
		s_layerWnd->setVisible( false );
		s_dispobj = false;
	}else{
		s_layerWnd->setVisible( true );
		s_dispobj = true;
	}

	return 0;
}
int DispModelPanel()
{
	if( !s_modelpanel.panel ){
		return 0;
	}

	if( s_dispmodel ){
		s_modelpanel.panel->setVisible( false );
		s_dispmodel = false;
	}else{
		s_modelpanel.panel->setVisible( true );
		s_dispmodel = true;
	}

	return 0;
}


int EraseKeyList()
{
	s_copyKeyInfoList.clear();
	s_selectKeyInfoList.clear();
	s_deletedKeyInfoList.clear();

	s_McopyKeyInfoList.clear();
	s_MselectKeyInfoList.clear();
	s_MdeletedKeyInfoList.clear();

	return 0;
}

int AddMotion( WCHAR* wfilename )
{
	int motnum = s_tlarray.size();
	if( motnum >= MAXMOTIONNUM ){
		MessageBox( s_mainwnd, L"これ以上モーションを読み込めません。", L"モーション数が多すぎます。", MB_OK );
		return 0;
	}

	char motionname[256];
	ZeroMemory( motionname, 256 );
	SYSTEMTIME systime;
	GetLocalTime( &systime );
	sprintf_s( motionname, 256, "motion_%d_%d_%d_%d_%d_%d_%d",
		systime.wYear,
		systime.wMonth,
		systime.wDay,
		systime.wHour,
		systime.wMinute,
		systime.wSecond,
		systime.wMilliseconds
	);

	int newmotid = -1;
	double motleng = 100.0;
	CallF( s_model->AddMotion( motionname, wfilename, motleng, &newmotid ), return 1 );

	CallF( AddTimeLine( newmotid ), return 1 );

	int selindex = s_tlarray.size() - 1;
	CallF( OnAnimMenu( selindex ), return 1 );


	return 0;
}

int OnAnimMenu( int selindex, int saveundoflag )
{
	s_curmotmenuindex = selindex;

	if( selindex < 0 ){
		return 0;//!!!!!!!!!
	}

	_ASSERT( s_animmenu );

	int iAnimSet, cAnimSets;
	cAnimSets = GetMenuItemCount( s_animmenu );
	for (iAnimSet = 0; iAnimSet < cAnimSets; iAnimSet++)
	{
		RemoveMenu(s_animmenu, 0, MF_BYPOSITION);
	}


	if( !s_model || !s_owpTimeline ){
		return 0;//!!!!!!!!!!!!!!!!!!
	}

	cAnimSets = s_tlarray.size();

	if( cAnimSets <= 0 ){
		if( s_owpTimeline ){
			refreshTimeline(*s_owpTimeline, *s_owpMTimeline);
		}
		return 0;//!!!!!!!!!!!!!!!!!!!
	}

	char* szName;
	WCHAR wname[256];
	for(iAnimSet = 0; iAnimSet < cAnimSets; iAnimSet++)
	{
		int motid;
		motid = s_tlarray[iAnimSet].motionid;
		szName = s_model->m_motinfo[ motid ]->motname;
		MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, szName, 256, wname, 256 );

		if (szName != NULL)
			AppendMenu(s_animmenu, MF_STRING, 59900 + iAnimSet, wname);
		else
			AppendMenu(s_animmenu, MF_STRING, 59900 + iAnimSet, L"<No Animation Name>");
	}

	if( cAnimSets > 0 ){
		CheckMenuItem(s_mainmenu, 59900 + selindex, MF_CHECKED);

		int selmotid;
		selmotid = s_tlarray[ selindex ].motionid;
		if( selmotid > 0 ){
			CallF( s_model->SetCurrentMotion( selmotid ), return 1 );
			//EraseKeyList();
			if( saveundoflag == 1 ){
				s_owpTimeline->setCurrentLine( 0 );
				s_owpMTimeline->setCurrentLine( 0 );
				s_owpMTimeline->setCurrentTime( 0.0 );
			}
		}
	}

	if( s_owpTimeline ){
		//タイムラインのキーを設定
		refreshTimeline(*s_owpTimeline, *s_owpMTimeline);
		if( saveundoflag == 1 ){
			s_owpMTimeline->setCurrentTime( 0.0 );
		}
	}

	if( saveundoflag == 1 ){
		if( s_model ){
			s_model->SaveUndoMotion( s_curboneno, s_curbaseno );
		}
	}else{
		if( s_model ){
			double curframe = s_model->m_curmotinfo->curframe;
			s_owpTimeline->setCurrentTime( curframe, true );
			s_owpMTimeline->setCurrentTime( curframe, true );
			s_owpLTimeline->setCurrentTime( curframe, true );
		}
	}

	if( s_model && s_model->m_curmotinfo ){
		WCHAR wname[256];
		MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, s_model->m_curmotinfo->motname, 256, wname, 256 );
		WCHAR wtitle[256];
		swprintf_s( wtitle, 256, L"Mameroid3D : curmotion %s", wname );
		SetWindowText( s_mainwnd, wtitle );
	}

	return 0;
}

int OnModelMenu( int selindex, int callbymenu )
{
	if( callbymenu == 1 ){
		if( s_model && (s_curmodelmenuindex >= 0) && (s_modelindex.empty() == 0) ){
			s_modelindex[s_curmodelmenuindex].tlarray = s_tlarray;
			s_modelindex[s_curmodelmenuindex].lineno2boneno = s_lineno2boneno;
			s_modelindex[s_curmodelmenuindex].boneno2lineno = s_boneno2lineno;
		}
	}

	s_curmodelmenuindex = selindex;

	_ASSERT( s_modelmenu );
	int iMdlSet, cMdlSets;
	cMdlSets = GetMenuItemCount( s_modelmenu );
	for (iMdlSet = 0; iMdlSet < cMdlSets; iMdlSet++)
	{
		RemoveMenu(s_modelmenu, 0, MF_BYPOSITION);
	}

	if( (selindex < 0) || !s_model ){
		s_model = 0;
		s_curboneno = -1;
		s_curbaseno = -1;
		if( s_owpTimeline ){
			refreshTimeline(*s_owpTimeline, *s_owpMTimeline);
		}
		refreshModelPanel();
		return 0;//!!!!!!!!!
	}

	cMdlSets = s_modelindex.size();
	if( cMdlSets <= 0 ){
		s_model = 0;
		if( s_owpTimeline ){
			refreshTimeline(*s_owpTimeline, *s_owpMTimeline);
		}
		refreshModelPanel();
		return 0;//!!!!!!!!!!!!!!!!!!!
	}

	WCHAR* wname;
	for(iMdlSet = 0; iMdlSet < cMdlSets; iMdlSet++)
	{
		wname = s_modelindex[iMdlSet].modelptr->m_filename;

		if( *wname != 0 )
			AppendMenu(s_modelmenu, MF_STRING, 61000 + iMdlSet, wname);
		else
			AppendMenu(s_modelmenu, MF_STRING, 61000 + iMdlSet, L"<No Model Name>");
	}

	if( cMdlSets > 0 ){
		CheckMenuItem( s_mainmenu, 61000 + selindex, MF_CHECKED );

		s_model = s_modelindex[ selindex ].modelptr;
		s_tlarray = s_modelindex[ selindex ].tlarray;
		s_curmotmenuindex = s_modelindex[ selindex ].motmenuindex;
		s_lineno2boneno = s_modelindex[ selindex ].lineno2boneno;
		s_boneno2lineno = s_modelindex[ selindex ].boneno2lineno;

		OnAnimMenu( s_curmotmenuindex );
	}

	refreshModelPanel();

	return 0;
}



int OnDelMotion( int delmenuindex )
{
	int tlnum = s_tlarray.size();
	if( (tlnum <= 0) || (delmenuindex < 0) || (delmenuindex >= tlnum) ){
		return 0;
	}

	int delmotid = s_tlarray[ delmenuindex ].motionid;
	CallF( s_model->DeleteMotion( delmotid ), return 1 );


	int tlno;
	for( tlno = delmenuindex; tlno < (tlnum - 1); tlno++ ){
		s_tlarray[ tlno ] = s_tlarray[ tlno + 1 ];
	}
	s_tlarray.pop_back();

	int newtlnum = s_tlarray.size();
	if( newtlnum == 0 ){
		AddMotion( 0 );
	}else{
		OnAnimMenu( 0 );
	}

	return 0;
}

int OnDelModel( int delmenuindex )
{
	int mdlnum = s_modelindex.size();
	if( (mdlnum <= 0) || (delmenuindex < 0) || (delmenuindex >= mdlnum) ){
		return 0;
	}

	CModel* delmodel = s_modelindex[ delmenuindex ].modelptr;
	if( delmodel ){
		delete delmodel;
	}

	int mdlno;
	for( mdlno = delmenuindex; mdlno < (mdlnum - 1); mdlno++ ){
		s_modelindex[ mdlno ].tlarray.erase( s_modelindex[ mdlno ].tlarray.begin(), s_modelindex[ mdlno ].tlarray.end() );
		s_modelindex[ mdlno ].boneno2lineno.erase( s_modelindex[ mdlno ].boneno2lineno.begin(), s_modelindex[ mdlno ].boneno2lineno.end() );
		s_modelindex[ mdlno ].lineno2boneno.erase( s_modelindex[ mdlno ].lineno2boneno.begin(), s_modelindex[ mdlno ].lineno2boneno.end() );

		s_modelindex[ mdlno ] = s_modelindex[ mdlno + 1 ];
	}
	s_modelindex.pop_back();

	if( s_modelindex.empty() ){
		s_curboneno = -1;
		s_curbaseno = -1;
		s_model = 0;
		s_curmodelmenuindex = -1;
		s_tlarray.erase( s_tlarray.begin(), s_tlarray.end() );
		s_curmotmenuindex = -1;
		s_lineno2boneno.erase( s_lineno2boneno.begin(), s_lineno2boneno.end() );
		s_boneno2lineno.erase( s_boneno2lineno.begin(), s_boneno2lineno.end() );
	}else{
		s_curboneno = -1;
		s_curbaseno = -1;
		s_model = s_modelindex[ 0 ].modelptr;
		s_tlarray = s_modelindex[ 0 ].tlarray;
		s_curmotmenuindex = s_modelindex[ 0 ].motmenuindex;
		s_lineno2boneno = s_modelindex[ 0 ].lineno2boneno;
		s_boneno2lineno = s_modelindex[ 0 ].boneno2lineno;
	}

	OnModelMenu( 0, 0 );

	CreateModelPanel();

	return 0;
}

int OnDelAllModel()
{
	int mdlnum = s_modelindex.size();
	if( mdlnum <= 0 ){
		return 0;
	}

	vector<MODELELEM>::iterator itrmodel;
	for( itrmodel = s_modelindex.begin(); itrmodel != s_modelindex.end(); itrmodel++ ){
		CModel* delmodel = itrmodel->modelptr;
		if( delmodel ){
			delete delmodel;
		}
		itrmodel->tlarray.erase( itrmodel->tlarray.begin(), itrmodel->tlarray.end() );
		itrmodel->boneno2lineno.erase( itrmodel->boneno2lineno.begin(), itrmodel->boneno2lineno.end() );
		itrmodel->lineno2boneno.erase( itrmodel->lineno2boneno.begin(), itrmodel->lineno2boneno.end() );
	}

	s_modelindex.erase( s_modelindex.begin(), s_modelindex.end() );

	s_curboneno = -1;
	s_curbaseno = -1;
	s_model = 0;
	s_curmodelmenuindex = -1;
	s_tlarray.erase( s_tlarray.begin(), s_tlarray.end() );
	s_curmotmenuindex = -1;
	s_lineno2boneno.erase( s_lineno2boneno.begin(), s_lineno2boneno.end() );
	s_boneno2lineno.erase( s_boneno2lineno.begin(), s_boneno2lineno.end() );

	OnModelMenu( -1, 0 );

	CreateModelPanel();

	return 0;
}



int AddModelBound( MODELBOUND* mb, MODELBOUND* addmb )
{
	D3DXVECTOR3 newmin = mb->min;
	D3DXVECTOR3 newmax = mb->max;

	if( newmin.x > addmb->min.x ){
		newmin.x = addmb->min.x;
	}
	if( newmin.y > addmb->min.y ){
		newmin.y = addmb->min.y;
	}
	if( newmin.z > addmb->min.z ){
		newmin.z = addmb->min.z;
	}

	if( newmax.x < addmb->max.x ){
		newmax.x = addmb->max.x;
	}
	if( newmax.y < addmb->max.y ){
		newmax.y = addmb->max.y;
	}
	if( newmax.z < addmb->max.z ){
		newmax.z = addmb->max.z;
	}

	mb->center = ( newmin + newmax ) * 0.5f;
	mb->min = newmin;
	mb->max = newmax;

	D3DXVECTOR3 diff;
	diff = mb->center - newmin;
	mb->r = D3DXVec3Length( &diff );

	return 0;
}

int refreshModelPanel()
{
	//すべての行をクリア
	s_owpLayerTable->deleteLine();

	WCHAR label[256];
	int objnum = 0;

	if( s_model ){
		objnum = s_model->m_object.size();
	}else{
		objnum = 0;
	}
	
	if( objnum > 0 ){
		map<int, CMQOObject*>::iterator itrobj;
		for( itrobj = s_model->m_object.begin(); itrobj != s_model->m_object.end(); itrobj++ ){
			CMQOObject* curobj = itrobj->second;
			WCHAR wname[256];
			MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, curobj->m_name, 256, wname, 256 );
			s_owpLayerTable->newLine( wname, (void*)curobj );
			s_owpLayerTable->setVisible( wname, (curobj->m_dispflag > 0), true );
		}
	}else{
		wcscpy_s( label, 256, L"dummy name" );
		s_owpLayerTable->newLine( label, 0 );
	}

	return 0;
}

int RenderSelectMark()
{
	D3DXMATRIX mw = g_mCenterWorld * *g_Camera.GetWorldMatrix();
    D3DXMATRIX mp = *g_Camera.GetProjMatrix();
    D3DXMATRIX mv = *g_Camera.GetViewMatrix();
	D3DXMATRIX mvp = mv * mp;

	CBone* curboneptr = s_model->m_bonelist[ s_curboneno ];
	if( curboneptr ){
		D3DXMATRIX selm;
		D3DXMATRIX bonetra;
		selm = curboneptr->m_axisq.MakeRotMatX() * curboneptr->m_curmp.m_worldmat;

		D3DXVECTOR3 orgpos = curboneptr->m_vertpos[BT_CHILD];
		D3DXVECTOR3 bonepos = curboneptr->m_childworld;

		D3DXVECTOR3 cam0, cam1;
		D3DXMATRIX mwv = mw * mv;
		D3DXVec3TransformCoord( &cam0, &orgpos, &mwv );
		cam1 = cam0 + D3DXVECTOR3( 1.0f, 0.0f, 0.0f );

		D3DXVECTOR3 sc0, sc1;
		float selectscale;
		D3DXVec3TransformCoord( &sc0, &cam0, &mp );
		D3DXVec3TransformCoord( &sc1, &cam1, &mp );
		float lineleng = (sc0.x - sc1.x) * (sc0.x - sc1.x) + (sc0.y - sc1.y) * (sc0.y - sc1.y);
		if( lineleng != 0.0f ){
			lineleng = sqrt( lineleng );
			selectscale = 0.0020f / lineleng;
		}else{
			selectscale = 0.0f;
		}

		D3DXMATRIX scalemat;
		D3DXMatrixIdentity( &scalemat );
		scalemat._11 *= selectscale;
		scalemat._22 *= selectscale;
		scalemat._33 *= selectscale;

		D3DXMATRIX selworld;
		selworld = scalemat * selm;
		selworld._41 = bonepos.x;
		selworld._42 = bonepos.y;
		selworld._43 = bonepos.z;

		g_pEffect->SetMatrix( g_hmVP, &mvp );
		g_pEffect->SetMatrix( g_hmWorld, &selworld );
		s_select->UpdateMatrix( -1, &selworld, &mvp );

		if( s_dispselect ){
			s_select->SetShaderConst();
			int lightflag = 0;
			D3DXVECTOR4 diffusemult = D3DXVECTOR4(1.0f, 1.0f, 1.0f, 0.7f);
			s_pdev->SetRenderState( D3DRS_ZFUNC, D3DCMP_ALWAYS );
			s_select->OnRender( s_pdev, lightflag, diffusemult );
			s_pdev->SetRenderState( D3DRS_ZFUNC, D3DCMP_LESSEQUAL );
		}
	}

	return 0;
}

int CalcTargetPos( D3DXVECTOR3* dstpos )
{
	D3DXVECTOR3 start3d, end3d;
	CalcPickRay( &start3d, &end3d );

	//カメラの面とレイとの交点(targetpos)を求める。
	D3DXVECTOR3 sb, se, n;
	sb = s_pickinfo.objworld - start3d;
	se = end3d - start3d;
	n = *g_Camera.GetLookAtPt() - *g_Camera.GetEyePt();

	float t;
	t = D3DXVec3Dot( &sb, &n ) / D3DXVec3Dot( &se, &n );

	*dstpos = ( 1.0f - t ) * start3d + t * end3d;
	return 0;
}

int CalcPickRay( D3DXVECTOR3* startptr, D3DXVECTOR3* endptr )
{
	s_pickinfo.diffmouse.x = (float)( s_pickinfo.mousepos.x - s_pickinfo.mousebefpos.x );
	s_pickinfo.diffmouse.y = (float)( s_pickinfo.mousepos.y - s_pickinfo.mousebefpos.y );

	D3DXVECTOR3 mousesc;
	mousesc.x = s_pickinfo.objscreen.x + s_pickinfo.diffmouse.x;
	mousesc.y = s_pickinfo.objscreen.y + s_pickinfo.diffmouse.y;
	mousesc.z = s_pickinfo.objscreen.z;

	D3DXVECTOR3 startsc, endsc;
	float rayx, rayy;
	rayx = mousesc.x / (s_pickinfo.winx / 2.0f) - 1.0f;
	rayy = 1.0f - mousesc.y / (s_pickinfo.winy / 2.0f);

	startsc = D3DXVECTOR3( rayx, rayy, 0.0f );
	endsc = D3DXVECTOR3( rayx, rayy, 1.0f );
	
    D3DXMATRIX mView;
    D3DXMATRIX mProj;
    mProj = *g_Camera.GetProjMatrix();
    mView = *g_Camera.GetViewMatrix();
	D3DXMATRIX mVP, invmVP;
	mVP = mView * mProj;
	D3DXMatrixInverse( &invmVP, NULL, &mVP );

	D3DXVec3TransformCoord( startptr, &startsc, &invmVP );
	D3DXVec3TransformCoord( endptr, &endsc, &invmVP );

	return 0;
}

LRESULT CALLBACK PndLoadDlgProc(HWND hDlgWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	static OPENFILENAME ofn1;
	static WCHAR wkey[PNDKEYLENG] = {0};
	
	switch (msg) {
        case WM_INITDIALOG:
			ZeroMemory( s_pndpath, sizeof( WCHAR ) * MAX_PATH );
			ZeroMemory( wkey, sizeof( WCHAR ) * PNDKEYLENG );
			SetDlgItemText( hDlgWnd, IDC_PNDPATH, L"参照ボタンを押してファイルを指定してください。" );
			SetDlgItemText( hDlgWnd, IDC_PNDKEY, L"" );
            return FALSE;
        case WM_COMMAND:
            switch (LOWORD(wp)) {
                case IDOK:
					GetDlgItemText( hDlgWnd, IDC_PNDKEY, wkey, PNDKEYLENG );
					WideCharToMultiByte( CP_ACP, 0, wkey, -1, s_pndkey, PNDKEYLENG, NULL, NULL );

                    EndDialog(hDlgWnd, IDOK);
					return IDOK;
                    break;
                case IDCANCEL:
                    EndDialog(hDlgWnd, IDCANCEL);
					return IDCANCEL;
                    break;
				case IDC_REFPND:
					ofn1.lStructSize = sizeof(OPENFILENAME);
					ofn1.hwndOwner = hDlgWnd;
					ofn1.hInstance = 0;
					ofn1.lpstrFilter = L"暗号file(*.pnd)\0*.pnd\0";
					ofn1.lpstrCustomFilter = NULL;
					ofn1.nMaxCustFilter = 0;
					ofn1.nFilterIndex = 0;
					ofn1.lpstrFile = s_pndpath;
					ofn1.nMaxFile = MAX_PATH;
					ofn1.lpstrFileTitle = NULL;
					ofn1.nMaxFileTitle = 0;
					ofn1.lpstrInitialDir = NULL;
					ofn1.lpstrTitle = NULL;
					ofn1.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_EXPLORER;
					ofn1.nFileOffset = 0;
					ofn1.nFileExtension = 0;
					ofn1.lpstrDefExt =NULL;
					ofn1.lCustData = NULL;
					ofn1.lpfnHook = NULL;
					ofn1.lpTemplateName = NULL;

					if( GetOpenFileNameW(&ofn1) == IDOK ){
						SetDlgItemText( hDlgWnd, IDC_PNDPATH, s_pndpath );
					}
					break;
				default:
                    return FALSE;
            }
        default:
            return FALSE;
    }
    return TRUE;
}



LRESULT CALLBACK PndConvDlgProc(HWND hDlgWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	static OPENFILENAME ofn1;
	static OPENFILENAME ofn2;
	static WCHAR wkey[PNDKEYLENG] = {0};
	
	switch (msg) {
        case WM_INITDIALOG:
			ZeroMemory( s_rdbpath, sizeof( WCHAR ) * MAX_PATH );
			ZeroMemory( s_pndpath, sizeof( WCHAR ) * MAX_PATH );
			ZeroMemory( wkey, sizeof( WCHAR ) * PNDKEYLENG );
			SetDlgItemText( hDlgWnd, IDC_RDBNAME, L"参照ボタンを押してファイルを指定してください。" );
			SetDlgItemText( hDlgWnd, IDC_PNDPATH, L"参照ボタンを押してファイルを指定してください。" );
			SetDlgItemText( hDlgWnd, IDC_PNDKEY, L"" );
            return FALSE;
        case WM_COMMAND:
            switch (LOWORD(wp)) {
                case IDOK:
					GetDlgItemText( hDlgWnd, IDC_PNDKEY, wkey, PNDKEYLENG );
					WideCharToMultiByte( CP_ACP, 0, wkey, -1, s_pndkey, PNDKEYLENG, NULL, NULL );

                    EndDialog(hDlgWnd, IDOK);
					return IDOK;
                    break;
                case IDCANCEL:
                    EndDialog(hDlgWnd, IDCANCEL);
					return IDCANCEL;
                    break;
				case IDC_REFRDB:
					ofn1.lStructSize = sizeof(OPENFILENAME);
					ofn1.hwndOwner = hDlgWnd;
					ofn1.hInstance = 0;
					ofn1.lpstrFilter = L"project(*.rdb)\0*.rdb\0";
					ofn1.lpstrCustomFilter = NULL;
					ofn1.nMaxCustFilter = 0;
					ofn1.nFilterIndex = 0;
					ofn1.lpstrFile = s_rdbpath;
					ofn1.nMaxFile = MAX_PATH;
					ofn1.lpstrFileTitle = NULL;
					ofn1.nMaxFileTitle = 0;
					ofn1.lpstrInitialDir = NULL;
					ofn1.lpstrTitle = NULL;
					ofn1.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_EXPLORER;
					ofn1.nFileOffset = 0;
					ofn1.nFileExtension = 0;
					ofn1.lpstrDefExt =NULL;
					ofn1.lCustData = NULL;
					ofn1.lpfnHook = NULL;
					ofn1.lpTemplateName = NULL;

					if( GetOpenFileNameW(&ofn1) == IDOK ){
						SetDlgItemText( hDlgWnd, IDC_RDBNAME, s_rdbpath );
					}
					break;
				case IDC_REFPND:
					ofn2.lStructSize = sizeof(OPENFILENAME);
					ofn2.hwndOwner = hDlgWnd;
					ofn2.hInstance = 0;
					ofn2.lpstrFilter = L"pnd file(*.pnd)\0*.pnd\0";
					ofn2.lpstrCustomFilter = NULL;
					ofn2.nMaxCustFilter = 0;
					ofn2.nFilterIndex = 1;
					ofn2.lpstrFile = s_pndpath;
					ofn2.nMaxFile = MAX_PATH;
					ofn2.lpstrFileTitle = NULL;
					ofn2.nMaxFileTitle = 0;
					ofn2.lpstrInitialDir = NULL;
					ofn2.lpstrTitle = NULL;
					ofn2.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
					ofn2.nFileOffset = 0;
					ofn2.nFileExtension = 0;
					ofn2.lpstrDefExt =NULL;
					ofn2.lCustData = NULL;
					ofn2.lpfnHook = NULL;
					ofn2.lpTemplateName = NULL;
					if( GetOpenFileNameW(&ofn2) == IDOK ){
						SetDlgItemText( hDlgWnd, IDC_PNDPATH, s_pndpath );
					}
					break;
				default:
                    return FALSE;
            }
        default:
            return FALSE;
    }
    return TRUE;
}



LRESULT CALLBACK OpenMqoDlgProc(HWND hDlgWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	
	OPENFILENAME ofn;
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hDlgWnd;
	ofn.hInstance = 0;
	ofn.lpstrFilter = L"project(*.rdb)mikoto(*.mqo)\0*.rdb;*.mqo\0motion(*.qub)\0*.qub\0";
	ofn.lpstrCustomFilter = NULL;
	ofn.nMaxCustFilter = 0;
	ofn.nFilterIndex = s_filterindex;
	ofn.lpstrFile = g_tmpmqopath;
	ofn.nMaxFile = MULTIPATH;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.lpstrTitle = NULL;
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_EXPLORER | OFN_ALLOWMULTISELECT;
	ofn.nFileOffset = 0;
	ofn.nFileExtension = 0;
	ofn.lpstrDefExt =NULL;
	ofn.lCustData = NULL;
	ofn.lpfnHook = NULL;
	ofn.lpTemplateName = NULL;

	WCHAR strmult[256];
	wcscpy_s( strmult, 256, L"1.000" );
	
	switch (msg) {
        case WM_INITDIALOG:
			g_tmpmqomult = 1.0f;
			ZeroMemory( g_tmpmqopath, sizeof( WCHAR ) * MULTIPATH );
			SetDlgItemText( hDlgWnd, IDC_MULT, strmult );
			SetDlgItemText( hDlgWnd, IDC_FILEPATH, L"参照ボタンを押してファイルを指定してください。" );
            return FALSE;
        case WM_COMMAND:
            switch (LOWORD(wp)) {
                case IDOK:
					GetDlgItemText( hDlgWnd, IDC_MULT, strmult, 256 );
					g_tmpmqomult = (float)_wtof( strmult );
					//GetDlgItemText( hDlgWnd, IDC_FILEPATH, g_tmpmqopath, MULTIPATH );

                    EndDialog(hDlgWnd, IDOK);
                    break;
                case IDCANCEL:
                    EndDialog(hDlgWnd, IDCANCEL);
                    break;
				case IDC_REFMQO:
					if( GetOpenFileNameW(&ofn) == IDOK ){
						SetDlgItemText( hDlgWnd, IDC_FILEPATH, g_tmpmqopath );
					}
					break;
				default:
                    return FALSE;
            }
        default:
            return FALSE;
    }
    return TRUE;
}


LRESULT CALLBACK MotPropDlgProc(HWND hDlgWnd, UINT msg, WPARAM wp, LPARAM lp)
{

//static WCHAR s_tmpmotname[256] = {0L};
//static double s_tmpmotframeleng = 100.0f;
//static int s_tmpmotloop = 0;

	WCHAR strframeleng[256];

	switch (msg) {
        case WM_INITDIALOG:
			if( s_model && s_model->m_curmotinfo ){
				MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, 
					s_model->m_curmotinfo->motname, 256, s_tmpmotname, 256 );
				SetDlgItemText( hDlgWnd, IDC_MOTNAME, s_tmpmotname );

				s_tmpmotframeleng = s_model->m_curmotinfo->frameleng;
				swprintf_s( strframeleng, 256, L"%f", s_tmpmotframeleng );
				SetDlgItemText( hDlgWnd, IDC_FRAMELENG, strframeleng );

				s_tmpmotloop = s_model->m_curmotinfo->loopflag;
				SendMessage( GetDlgItem( hDlgWnd, IDC_LOOP ), BM_SETCHECK, (WPARAM)s_tmpmotloop, 0L);

			}
			//SetDlgItemText( hDlgWnd, IDC_MULT, strmult );
            return FALSE;
        case WM_COMMAND:
            switch (LOWORD(wp)) {
                case IDOK:
					GetDlgItemText( hDlgWnd, IDC_MOTNAME, s_tmpmotname, 256 );
					
					GetDlgItemText( hDlgWnd, IDC_FRAMELENG, strframeleng, 256 );
					s_tmpmotframeleng = (float)_wtof( strframeleng );

					if( IsDlgButtonChecked( hDlgWnd, IDC_LOOP ) == BST_CHECKED ){  
                        s_tmpmotloop = 1;
                    }else{ 
						s_tmpmotloop = 0;
					}

                    EndDialog(hDlgWnd, IDOK);
                    break;
                case IDCANCEL:
                    EndDialog(hDlgWnd, IDCANCEL);
                    break;
				default:
                    return FALSE;
            }
        default:
            return FALSE;
    }
    return TRUE;

}

int SaveProject()
{
	if( s_modelindex.empty() ){
		return 0;
	}

	int dlgret;
	dlgret = (int)DialogBoxW( (HINSTANCE)GetModuleHandle(NULL), MAKEINTRESOURCE( IDD_SAVERDBDLG ), 
		s_mainwnd, (DLGPROC)SaveRdbDlgProc );
	if( (dlgret != IDOK) || !s_projectname[0] || !s_projectdir[0] ){
		return 0;
	}

	CRdbFile rdbfile;
	CallF( rdbfile.WriteRdbFile( s_projectdir, s_projectname, s_modelindex ), return 1 );

	return 0;
}


LRESULT CALLBACK SaveRdbDlgProc(HWND hDlgWnd, UINT msg, WPARAM wp, LPARAM lp)
{

//	static WCHAR s_projectname[64] = {0L};
//	static WCHAR s_projectdir[MAX_PATH] = {0L};

	BROWSEINFO bi;
	LPITEMIDLIST curlpidl = 0;
	WCHAR dispname[MAX_PATH] = {0L};
	WCHAR selectname[MAX_PATH] = {0L};
	int iImage = 0;

	switch (msg) {
        case WM_INITDIALOG:
			if( s_model && s_model->m_curmotinfo ){
				if( s_rdbsavename[0] ){
					SetDlgItemText( hDlgWnd, IDC_PROJNAME, s_rdbsavename );
				}
				if( s_rdbsavedir[0] ){
					SetDlgItemText( hDlgWnd, IDC_DIRNAME, s_rdbsavedir );
				}else{
					if( s_projectdir[0] ){
						SetDlgItemText( hDlgWnd, IDC_DIRNAME, s_projectdir );
					}else{
						LPITEMIDLIST pidl;

						HWND hWnd = NULL;

						IMalloc *pMalloc;
						SHGetMalloc( &pMalloc );

						if( SUCCEEDED(SHGetSpecialFolderLocation( s_mainwnd, CSIDL_DESKTOPDIRECTORY, &pidl )) )
						{ 
							// パスに変換する
							SHGetPathFromIDList( pidl, s_projectdir );
							// 取得したIDLを解放する (CoTaskMemFreeでも可)
							pMalloc->Free( pidl );              
							SetDlgItemText( hDlgWnd, IDC_DIRNAME, s_projectdir );
						}
						pMalloc->Release();
					}
				}
			}
            return FALSE;
        case WM_COMMAND:
            switch (LOWORD(wp)) {
                case IDOK:
					GetDlgItemText( hDlgWnd, IDC_PROJNAME, s_projectname, 64 );
					GetDlgItemText( hDlgWnd, IDC_DIRNAME, s_projectdir, MAX_PATH );
					wcscpy_s( s_rdbsavename, 64, s_projectname );
					wcscpy_s( s_rdbsavedir, MAX_PATH, s_projectdir );
                    EndDialog(hDlgWnd, IDOK);
                    break;
                case IDCANCEL:
					s_projectname[0] = 0L;
					s_projectdir[0] = 0L;
                    EndDialog(hDlgWnd, IDCANCEL);
                    break;
				case IDC_REFDIR:
					bi.hwndOwner = hDlgWnd;
					bi.pidlRoot = NULL;//!!!!!!!
					bi.pszDisplayName = dispname;
					bi.lpszTitle = L"保存フォルダを選択してください。";
					//bi.ulFlags = BIF_EDITBOX | BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
					bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
					bi.lpfn = NULL;
					bi.lParam = 0;
					bi.iImage = iImage;

					curlpidl = SHBrowseForFolder( &bi );
					if( curlpidl ){
						//::MessageBox( m_hWnd, dispname, "フォルダー名", MB_OK );

						BOOL bret;
						bret = SHGetPathFromIDList( curlpidl, selectname );
						if( bret == FALSE ){
							_ASSERT( 0 );
							if( curlpidl )
								CoTaskMemFree( curlpidl );
							return 1;
						}

						if( curlpidl )
							CoTaskMemFree( curlpidl );

						wcscpy_s( s_projectdir, MAX_PATH, selectname );
						SetDlgItemText( hDlgWnd, IDC_DIRNAME, s_projectdir );
					}

					break;
				default:
                    return FALSE;
            }
        default:
            return FALSE;
    }
    return TRUE;

}

LRESULT CALLBACK ExportXDlgProc(HWND hDlgWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	WCHAR buf[MAX_PATH];
	OPENFILENAME ofn;
	buf[0] = 0L;
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = NULL;
	ofn.hInstance = 0;
	ofn.lpstrFilter = L"x File (*.x)\0*.x\0";
	ofn.lpstrCustomFilter = NULL;
	ofn.nMaxCustFilter = 0;
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = buf;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.lpstrTitle = NULL;
	ofn.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
	ofn.nFileOffset = 0;
	ofn.nFileExtension = 0;
	ofn.lpstrDefExt =NULL;
	ofn.lCustData = NULL;
	ofn.lpfnHook = NULL;
	ofn.lpTemplateName = NULL;

	WCHAR strmult[256];
	wcscpy_s( strmult, 256, L"1.000" );
	
	switch (msg) {
        case WM_INITDIALOG:
			g_tmpmqomult = 1.0f;
			ZeroMemory( g_tmpmqopath, sizeof( WCHAR ) * MULTIPATH );
			SetDlgItemText( hDlgWnd, IDC_MULT, strmult );
			SetDlgItemText( hDlgWnd, IDC_FILEPATH, L"参照ボタンを押してファイルを指定してください。" );
            return FALSE;
        case WM_COMMAND:
            switch (LOWORD(wp)) {
                case IDOK:
					GetDlgItemText( hDlgWnd, IDC_MULT, strmult, 256 );
					g_tmpmqomult = (float)_wtof( strmult );
					GetDlgItemText( hDlgWnd, IDC_FILEPATH, g_tmpmqopath, MULTIPATH );
                    EndDialog(hDlgWnd, IDOK);
                    break;
                case IDCANCEL:
                    EndDialog(hDlgWnd, IDCANCEL);
                    break;
				case IDC_REFX:
					if( GetSaveFileNameW(&ofn) == IDOK ){
						SetDlgItemText( hDlgWnd, IDC_FILEPATH, buf );
					}
					break;
				default:
                    return FALSE;
            }
        default:
            return FALSE;
    }
    return TRUE;
}

LRESULT CALLBACK ExportFBXDlgProc(HWND hDlgWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	WCHAR buf[MAX_PATH];
	OPENFILENAME ofn;
	buf[0] = 0L;
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = NULL;
	ofn.hInstance = 0;
	ofn.lpstrFilter = L"fbx File (*.fbx)\0*.fbx\0";
	ofn.lpstrCustomFilter = NULL;
	ofn.nMaxCustFilter = 0;
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = buf;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.lpstrTitle = NULL;
	ofn.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
	ofn.nFileOffset = 0;
	ofn.nFileExtension = 0;
	ofn.lpstrDefExt =NULL;
	ofn.lCustData = NULL;
	ofn.lpfnHook = NULL;
	ofn.lpTemplateName = NULL;

	WCHAR strmult[256];
	wcscpy_s( strmult, 256, L"1.000" );
	
	switch (msg) {
        case WM_INITDIALOG:
			g_tmpmqomult = 1.0f;
			ZeroMemory( g_tmpmqopath, sizeof( WCHAR ) * MULTIPATH );
			SetDlgItemText( hDlgWnd, IDC_MULT, strmult );
			SetDlgItemText( hDlgWnd, IDC_FILEPATH, L"参照ボタンを押してファイルを指定してください。" );
			s_fbxbunki = 1;
            return FALSE;
        case WM_COMMAND:
            switch (LOWORD(wp)) {
                case IDOK:
					GetDlgItemText( hDlgWnd, IDC_MULT, strmult, 256 );
					g_tmpmqomult = (float)_wtof( strmult );
					GetDlgItemText( hDlgWnd, IDC_FILEPATH, g_tmpmqopath, MULTIPATH );
					if( IsDlgButtonChecked( hDlgWnd, IDC_BUNKI ) == BST_CHECKED) {
						s_fbxbunki = 0;
					}else{
						s_fbxbunki = 1;
					}
                    EndDialog(hDlgWnd, IDOK);
                    break;
                case IDCANCEL:
                    EndDialog(hDlgWnd, IDCANCEL);
                    break;
				case IDC_REFFBX:
					if( GetSaveFileNameW(&ofn) == IDOK ){
						SetDlgItemText( hDlgWnd, IDC_FILEPATH, buf );
					}
					break;
				default:
                    return FALSE;
            }
        default:
            return FALSE;
    }
    return TRUE;
}

int OpenRdbFile()
{
	WCHAR* lasten = 0;
	lasten = wcsrchr( g_tmpmqopath, TEXT( '\\' ) );
	if( lasten ){
		int leng = (int)wcslen( lasten + 1 );
		if( leng < 64 ){
			wcscpy_s( s_rdbsavename, 64, lasten + 1 );
			WCHAR* peri = wcsrchr( s_rdbsavename, TEXT( '.' ) );
			if( peri ){
				*peri = 0L;
			}
		}
	}


	WCHAR tmpdir[MAX_PATH];
	wcscpy_s( tmpdir, MAX_PATH, g_tmpmqopath );
	WCHAR* lastend = 0;
	lastend = wcsrchr( tmpdir, TEXT( '\\' ) );
	if( lastend ){
		*lastend = 0L;
		WCHAR* lastend2 = 0;
		lastend2 = wcsrchr( tmpdir, TEXT( '\\' ) );
		if( lastend2 ){
			*lastend2 = 0L;
			int dirleng = (int)( lastend2 - tmpdir );
			if( dirleng < MAX_PATH ){
				wcscpy_s( s_rdbsavedir, MAX_PATH, tmpdir );
			}
		}
	}

	CRdbFile rdbfile;
	CallF( rdbfile.LoadRdbFile( g_tmpmqopath, OpenMQOFile, OpenQubFile, OnDelMotion ), return 1 );


	return 0;
}
int ExportXFile()
{
	if( !s_model ){
		return 0;
	}

	int dlgret;
	dlgret = (int)DialogBoxW( (HINSTANCE)GetModuleHandle(NULL), MAKEINTRESOURCE( IDD_EXPORTXDLG ), 
		s_mainwnd, (DLGPROC)ExportXDlgProc );
	if( (dlgret != IDOK) || !g_tmpmqopath[0] ){
		return 0;
	}

	CXFile xfile;
	CallF( xfile.WriteXFile( g_tmpmqopath, s_model, g_tmpmqomult ), return 1 );

	return 0;
}

int ExportFBXFile()
{
	if( !s_model ){
		return 0;
	}
	if( !s_dummytri ){
		return 0;
	}

	int dlgret;
	dlgret = (int)DialogBoxW( (HINSTANCE)GetModuleHandle(NULL), MAKEINTRESOURCE( IDD_EXPORTFBXDLG ), 
		s_mainwnd, (DLGPROC)ExportFBXDlgProc );
	if( (dlgret != IDOK) || !g_tmpmqopath[0] ){
		return 0;
	}
	
	char fbxpath[MAX_PATH] = {0};
	//WideCharToMultiByte( CP_ACP, 0, buf, -1, fbxpath, MAX_PATH, NULL, NULL );
	WideCharToMultiByte( CP_UTF8, 0, g_tmpmqopath, -1, fbxpath, MAX_PATH, NULL, NULL );	

	MODELBOUND mb;
	s_model->GetModelBound( &mb );

	CallF( WriteFBXFile( s_model, fbxpath, s_dummytri, mb, g_tmpmqomult, 1 ), return 1 );

	return 0;
}


void ActivatePanel( int state, int fromtlflag )
{
	if( state == 1 ){
		if( s_dispmw ){
			s_timelineWnd->setVisible( false );
			s_timelineWnd->setVisible( true );
		}
		if( s_Mdispmw ){
			s_MtimelineWnd->setVisible( false );
			s_MtimelineWnd->setVisible( true );
		}
		if( s_Ldispmw ){
			s_LtimelineWnd->setVisible( false );
			s_LtimelineWnd->setVisible( true );
		}
		if( s_disptool ){
			s_toolWnd->setVisible( true );
		}
		if( s_dispobj ){
			s_layerWnd->setVisible( true );
		}
		if( s_dispmodel && s_modelpanel.panel ){
			s_modelpanel.panel->setVisible( true );
		}
		if( s_oneditmorph ){
			s_morphWnd->setVisible( true );
		}
	}else if( state == 0 ){
		if( fromtlflag == 0 ){
			s_timelineWnd->setVisible( false );
			s_MtimelineWnd->setVisible( false );
			s_LtimelineWnd->setVisible( false );
		}
		s_toolWnd->setVisible( false );
		s_layerWnd->setVisible( false );
		if( s_modelpanel.panel ){
			s_modelpanel.panel->setVisible( false );
		}
		s_morphWnd->setVisible( false );
	}
}

int ExportColladaFile()
{
	if( s_model ){
		SaveColladaFile( s_model );
	}

	return 0;
}

int DestroyModelPanel()
{
	if( s_modelpanel.panel ){
		s_modelpanel.panel->setVisible( false );
		delete s_modelpanel.panel;
		s_modelpanel.panel = 0;
	}
	if( s_modelpanel.radiobutton ){
		delete s_modelpanel.radiobutton;
		s_modelpanel.radiobutton = 0;
	}
	if( s_modelpanel.separator ){
		delete s_modelpanel.separator;
		s_modelpanel.separator = 0;
	}
	if( !(s_modelpanel.checkvec.empty()) ){
		int checknum = s_modelpanel.checkvec.size();
		int checkno;
		for( checkno = 0; checkno < checknum; checkno++ ){
			delete s_modelpanel.checkvec[checkno];
		}
	}
	s_modelpanel.checkvec.clear();
	
	s_modelpanel.modelindex = -1;

	return 0;
}

int CreateModelPanel()
{
	DestroyModelPanel();

	int modelnum = s_modelindex.size();
	if( modelnum <= 0 ){
		return 0;
	}
	
	int classcnt = 0;
	WCHAR clsname[256];
	swprintf_s( clsname, 256, L"ModelPanel%d", classcnt );

	s_modelpanel.panel = new OrgWindow(
		1,
		clsname,		//ウィンドウクラス名
		GetModuleHandle(NULL),	//インスタンスハンドル
		WindowPos(s_winpos[WINPOS_MODEL].posx, s_winpos[WINPOS_MODEL].posy),		//位置
		WindowSize(s_winpos[WINPOS_MODEL].width, s_winpos[WINPOS_MODEL].height),	//サイズ
		L"ModelPanel",	//タイトル
		NULL,					//親ウィンドウハンドル
		true,					//表示・非表示状態
		70,50,70,				//カラー
		true,					//閉じられるか否か
		true);					//サイズ変更の可否

	s_modelpanel.panel->setSizeMin(WindowSize(s_winpos[WINPOS_MODEL].minwidth,s_winpos[WINPOS_MODEL].minheight));		// 最小サイズを設定

	int modelcnt;
	for( modelcnt = 0; modelcnt < modelnum; modelcnt++ ){
		CModel* curmodel = s_modelindex[modelcnt].modelptr;
		_ASSERT( curmodel );

		if( modelcnt == 0 ){
			s_modelpanel.radiobutton = new OWP_RadioButton( curmodel->m_filename );
		}else{
			s_modelpanel.radiobutton->addLine( curmodel->m_filename );
		}
	}

	s_modelpanel.separator =  new OWP_Separator();									// セパレータ1（境界線による横方向2分割）


	for( modelcnt = 0; modelcnt < modelnum; modelcnt++ ){
		CModel* curmodel = s_modelindex[modelcnt].modelptr;
		OWP_CheckBox *owpCheckBox= new OWP_CheckBox( L"表示/非表示", curmodel->m_modeldisp );
		s_modelpanel.checkvec.push_back( owpCheckBox );
	}

	s_modelpanel.panel->addParts( *(s_modelpanel.separator) );

	s_modelpanel.separator->addParts1( *(s_modelpanel.radiobutton) );

	for( modelcnt = 0; modelcnt < modelnum; modelcnt++ ){
		OWP_CheckBox* curcb = s_modelpanel.checkvec[modelcnt];
		s_modelpanel.separator->addParts2( *curcb );
	}

	s_modelpanel.modelindex = s_curmodelmenuindex;
	s_modelpanel.radiobutton->setSelectIndex( s_modelpanel.modelindex );

////////////
	s_modelpanel.panel->setCloseListener( [](){ s_closemodelFlag = true; } );


	for( modelcnt = 0; modelcnt < modelnum; modelcnt++ ){
		s_modelpanel.checkvec[modelcnt]->setButtonListener( [modelcnt](){
			CModel* curmodel = s_modelindex[modelcnt].modelptr;
			curmodel->m_modeldisp = s_modelpanel.checkvec[modelcnt]->getValue();
		} );
	}

	s_modelpanel.radiobutton->setSelectListener( [](){
		s_modelpanel.modelindex = s_modelpanel.radiobutton->getSelectIndex();
		OnModelMenu( s_modelpanel.modelindex, 1 );
	} );

	return 0;
}

int SetCamera6Angle()
{

	D3DXVECTOR3 weye, wdiff;
	weye = *(g_Camera.GetEyePt());
	wdiff = s_camtargetpos - weye;
	float camdist = D3DXVec3Length( &wdiff );

	D3DXVECTOR3 neweye;
	float delta = 0.10f;

	if( g_keybuf[ VK_F1 ] & 0x80 ){
		neweye.x = s_camtargetpos.x;
		neweye.y = s_camtargetpos.y;
		neweye.z = s_camtargetpos.z - camdist;

		g_Camera.SetViewParams( &neweye, &s_camtargetpos );
	}else if( g_keybuf[ VK_F2 ] & 0x80 ){
		neweye.x = s_camtargetpos.x;
		neweye.y = s_camtargetpos.y;
		neweye.z = s_camtargetpos.z + camdist;

		g_Camera.SetViewParams( &neweye, &s_camtargetpos );
	}else if( g_keybuf[ VK_F3 ] & 0x80 ){
		neweye.x = s_camtargetpos.x - camdist;
		neweye.y = s_camtargetpos.y;
		neweye.z = s_camtargetpos.z;

		g_Camera.SetViewParams( &neweye, &s_camtargetpos );
	}else if( g_keybuf[ VK_F4 ] & 0x80 ){
		neweye.x = s_camtargetpos.x + camdist;
		neweye.y = s_camtargetpos.y;
		neweye.z = s_camtargetpos.z;

		g_Camera.SetViewParams( &neweye, &s_camtargetpos );
	}else if( g_keybuf[ VK_F5 ] & 0x80 ){
		neweye.x = s_camtargetpos.x;
		neweye.y = s_camtargetpos.y + camdist;
		neweye.z = s_camtargetpos.z + delta;

		g_Camera.SetViewParams( &neweye, &s_camtargetpos );
	}else if( g_keybuf[ VK_F6 ] & 0x80 ){
		neweye.x = s_camtargetpos.x;
		neweye.y = s_camtargetpos.y - camdist;
		neweye.z = s_camtargetpos.z - delta;

		g_Camera.SetViewParams( &neweye, &s_camtargetpos );
	}

	return 0;
}

int PndConv()
{
	int dlgret;
	dlgret = (int)DialogBoxW( (HINSTANCE)GetModuleHandle(NULL), MAKEINTRESOURCE( IDD_PNDCONVDLG ), 
		s_mainwnd, (DLGPROC)PndConvDlgProc );
	if( (dlgret == IDOK) && s_rdbpath[0] && s_pndpath[0] ){
		char rdbpath[MAX_PATH] = {0};
		char pndpath[MAX_PATH] = {0};
		WideCharToMultiByte( CP_ACP, 0, s_rdbpath, -1, rdbpath, MAX_PATH, NULL, NULL );
		WideCharToMultiByte( CP_ACP, 0, s_pndpath, -1, pndpath, MAX_PATH, NULL, NULL );

		char* lasten = strrchr( rdbpath, '\\' );
		if( !lasten ){
			_ASSERT( 0 );
			return 1;
		}
		*lasten = 0;

		int keyleng = (int)strlen( s_pndkey );


		CPmCipherDll cipher;
		cipher.CreatePmCipher();
		CallF( cipher.Init( (unsigned char*)s_pndkey, keyleng ), return 1 );
		CallF( cipher.Encrypt( rdbpath, pndpath ), return 1 );
	}else{
		_ASSERT( 0 );
		::MessageBoxW( s_mainwnd, L"暗号化に失敗しました。\nダイアログの入力項目を確認して再試行してください。", L"エラー", MB_OK );
	}

	return 0;
}

int PndLoad()
{
	int dlgret;
	dlgret = (int)DialogBoxW( (HINSTANCE)GetModuleHandle(NULL), MAKEINTRESOURCE( IDD_OPENPNDDLG ), 
		s_mainwnd, (DLGPROC)PndLoadDlgProc );
	if( (dlgret == IDOK) && s_pndpath[0] ){
		m_frompnd = 1;

		char pndpath[MAX_PATH] = {0};
		WideCharToMultiByte( CP_ACP, 0, s_pndpath, -1, pndpath, MAX_PATH, NULL, NULL );

		int keyleng = (int)strlen( s_pndkey );

		CPmCipherDll cipher;
		cipher.CreatePmCipher();
		CallF( cipher.Init( (unsigned char*)s_pndkey, keyleng ), return 1 );
		CallF( cipher.ParseCipherFile( pndpath ), return 1 );

		int propnum = 0;
		propnum = cipher.GetPropertyNum();
		if( propnum <= 0 ){
			return 0;
		}

		int rdbindex = -1;
		int propno;
		for( propno = 0; propno < propnum; propno++ ){
			PNDPROP curprop;
			ZeroMemory( &curprop, sizeof( PNDPROP ) );
			CallF( cipher.GetProperty( propno, &curprop ), return 1 );

//			WCHAR wname[MAX_PATH];
//			ZeroMemory( wname, sizeof( WCHAR ) * MAX_PATH );
//			MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, curprop.path, MAX_PATH, wname, MAX_PATH );
//			DbgOut( L"pnd filename %s\r\n", wname );

			char* rdbpat;
			rdbpat = (char*)strstr( curprop.filename, ".rdb" );
			if( rdbpat != 0 ){
				rdbindex = propno;
//				DbgOut( L"pnd rdb size %d\r\n", curprop.sourcesize );
				break;
			}
		}
		if( rdbindex < 0 ){
			_ASSERT( 0 );
			return 1;
		}


		CRdbFile rdbfile;
		CallF( rdbfile.LoadRdbFileFromPnd( &cipher, rdbindex, OpenMQOFileFromPnd, OpenQubFileFromPnd, OnDelMotion ), return 1 );

	}

	return 0;
}

int LineNo2BaseNo( int lineno, int* basenoptr )
{
	map<int, CMQOObject*>::iterator itrmbase = s_model->m_mbaseobject.begin();
	int i;
	for( i = 0; i < lineno; i++ ){
		itrmbase++;
		if( itrmbase == s_model->m_mbaseobject.end() ){
			*basenoptr = -1;
			return 1;
		}
	}
	*basenoptr = itrmbase->second->m_objectno;

	return 0;
}

int SetSpAxisParams()
{
	if( !(s_spaxis[0].sprite) ){
		return 0;
	}

	float spawidth = 32.0f;
	int spashift = 12;
	s_spaxis[0].dispcenter.x = (int)( 320.0f * ( (float)s_mainwidth / 620.0f ) );
	s_spaxis[0].dispcenter.y = (int)( 30.0f * ( (float)s_mainheight / 620.0f ) );
	spashift = (int)( (float)spashift * ( (float)s_mainwidth / 620.0f) ); 

	s_spaxis[1].dispcenter.x = s_spaxis[0].dispcenter.x + (int)( spawidth ) + spashift;
	s_spaxis[1].dispcenter.y = s_spaxis[0].dispcenter.y;

	s_spaxis[2].dispcenter.x = s_spaxis[1].dispcenter.x + (int)( spawidth ) + spashift;
	s_spaxis[2].dispcenter.y = s_spaxis[0].dispcenter.y;

	int spacnt;
	for( spacnt = 0; spacnt < 3; spacnt++ ){
		D3DXVECTOR3 disppos;
		disppos.x = (float)( s_spaxis[spacnt].dispcenter.x ) / ((float)s_mainwidth / 2.0f) - 1.0f;
		disppos.y = -((float)( s_spaxis[spacnt].dispcenter.y ) / ((float)s_mainheight / 2.0f) - 1.0f);
		disppos.z = 0.0f;
		D3DXVECTOR2 dispsize = D3DXVECTOR2( spawidth / (float)s_mainwidth * 2.0f, spawidth / (float)s_mainheight * 2.0f );
		CallF( s_spaxis[spacnt].sprite->SetPos( disppos ), return 1 );
		CallF( s_spaxis[spacnt].sprite->SetSize( dispsize ), return 1 );
	}

	return 0;

}

int PickSpAxis( POINT srcpos )
{
	int kind = 0;

	int starty = s_spaxis[0].dispcenter.y - 16;
	int endy = starty + 32;

	if( (srcpos.y >= starty) && (srcpos.y <= endy) ){
		int spacnt;
		for( spacnt = 0; spacnt < 3; spacnt++ ){
			int startx = s_spaxis[spacnt].dispcenter.x - 16;
			int endx = startx + 32;

			if( (srcpos.x >= startx) && (srcpos.x <= endx) ){
				switch( spacnt ){
				case 0:
					kind = PICK_SPA_X;
					break;
				case 1:
					kind = PICK_SPA_Y;
					break;
				case 2:
					kind = PICK_SPA_Z;
					break;
				}
				break;
			}
		}
	}


//DbgOut( L"pickspaxis : kind %d, mouse (%d, %d), starty %d, endy %d\r\n",
//	kind, srcpos.x, srcpos.y, starty, endy );
//int spacnt;
//for( spacnt = 0; spacnt < 3; spacnt++ ){
//	DbgOut( L"\tspa %d : startx %d, endx %d\r\n", spacnt, s_spaxis[spacnt].dispcenter.x, s_spaxis[spacnt].dispcenter.x + 32 );
//}

	return kind;
}

int OnPreviewStop()
{
	s_previewFlag = 0;

	vector<MODELELEM>::iterator itrmodel;
	for( itrmodel = s_modelindex.begin(); itrmodel != s_modelindex.end(); itrmodel++ ){
		CModel* curmodel = itrmodel->modelptr;
		if( curmodel && curmodel->m_curmotinfo ){
			curmodel->SetMotionFrame( 0.0 );
		}
	}
	if( s_owpTimeline ){
		s_owpTimeline->setCurrentTime( 0.0, true );
	}
	if( s_owpMTimeline ){
		s_owpMTimeline->setCurrentTime( 0.0, true );
	}
	if( s_owpLTimeline ){
		s_owpLTimeline->setCurrentTime( 0.0, true );
	}

	return 0;
}


int SetSelectState()
{
	if( !s_select || !s_model || s_previewFlag ){
		return 0;
	}

    CDXUTRadioButton* pRadioButton = g_SampleUI.GetRadioButton( IDC_IK_ROT );
	int chkrot = pRadioButton->GetChecked();
	if( chkrot ){
		s_ikkind = 0;

		s_select->SetDispFlag( "ringX", 1 );
		s_select->SetDispFlag( "ringY", 1 );
		s_select->SetDispFlag( "ringZ", 1 );
	}else{
		CDXUTRadioButton* pRadioButtonMV = g_SampleUI.GetRadioButton( IDC_IK_MV );
		int chkmv = pRadioButtonMV->GetChecked();
		if( chkmv ){
			s_ikkind = 1;

			s_select->SetDispFlag( "ringX", 0 );
			s_select->SetDispFlag( "ringY", 0 );
			s_select->SetDispFlag( "ringZ", 0 );
		}else{
			s_ikkind = 2;
			s_displightarrow = true;
			s_LightCheckBox->SetChecked( true );
		}
	}
////////
	PICKINFO pickinfo;
	ZeroMemory( &pickinfo, sizeof( PICKINFO ) );
	POINT ptCursor;
	GetCursorPos( &ptCursor );
	::ScreenToClient( s_mainwnd, &ptCursor );
	pickinfo.clickpos = ptCursor;
	pickinfo.mousepos = ptCursor;
	pickinfo.mousebefpos = ptCursor;
	pickinfo.diffmouse = D3DXVECTOR2( 0.0f, 0.0f );

	pickinfo.winx = (int)DXUTGetWindowWidth();
	pickinfo.winy = (int)DXUTGetWindowHeight();
	pickinfo.pickrange = 6;
	pickinfo.buttonflag = 0;

	pickinfo.pickobjno = s_curboneno;

	int spakind = PickSpAxis( ptCursor );
	if( spakind != 0 ){
		pickinfo.pickobjno = s_curboneno;
		pickinfo.buttonflag = spakind;
	}else{
	
		if( g_controlkey == false ){
			CallF( s_model->PickBone( &pickinfo ), return 1 );
		}

		if( pickinfo.pickobjno >= 0 ){
			pickinfo.buttonflag = PICK_CENTER;//!!!!!!!!!!!!!
		}else{
			if( s_dispselect ){
				int colliobjx, colliobjy, colliobjz, colliringx, colliringy, colliringz;
				colliobjx = 0;
				colliobjy = 0;
				colliobjz = 0;
				colliringx = 0;
				colliringy = 0;
				colliringz = 0;
						
				colliobjx = s_select->CollisionNoBoneObj_Mouse( &pickinfo, "objX" );
				colliobjy = s_select->CollisionNoBoneObj_Mouse( &pickinfo, "objY" );
				colliobjz = s_select->CollisionNoBoneObj_Mouse( &pickinfo, "objZ" );
				colliringx = s_select->CollisionNoBoneObj_Mouse( &pickinfo, "ringX" );
				colliringy = s_select->CollisionNoBoneObj_Mouse( &pickinfo, "ringY" );
				colliringz = s_select->CollisionNoBoneObj_Mouse( &pickinfo, "ringZ" );

				if( colliobjx || colliringx || colliobjy || colliringy || colliobjz || colliringz ){
					pickinfo.pickobjno = s_curboneno;
				}

				if( colliobjx || colliringx ){
					pickinfo.buttonflag = PICK_X;
				}else if( colliobjy || colliringy ){
					pickinfo.buttonflag = PICK_Y;
				}else if( colliobjz || colliringz ){
					pickinfo.buttonflag = PICK_Z;
				}else{
					ZeroMemory( &pickinfo, sizeof( PICKINFO ) );
					pickinfo.pickobjno = -1;
				}
			}else{
				ZeroMemory( &pickinfo, sizeof( PICKINFO ) );
				pickinfo.pickobjno = -1;
			}
		}
	}

	if( s_pickinfo.buttonflag != 0 ){
		return 0;//!!!!!!!!!!!!!!!!!!!!!!!
	}

	float hirate = 1.0f;
	float lowrate = 0.3f;

	float hia = 1.0f;
	float lowa = 0.7f;

	CMQOMaterial* matred = s_select->m_materialname[ "matred" ];
	_ASSERT( matred );
	CMQOMaterial* ringred = s_select->m_materialname[ "ringred" ];
	_ASSERT( ringred );
	CMQOMaterial* matblue = s_select->m_materialname[ "matblue" ];
	_ASSERT( matblue );
	CMQOMaterial* ringblue = s_select->m_materialname[ "ringblue" ];
	_ASSERT( ringblue );
	CMQOMaterial* matgreen = s_select->m_materialname[ "matgreen" ];
	_ASSERT( matgreen );
	CMQOMaterial* ringgreen = s_select->m_materialname[ "ringgreen" ];
	_ASSERT( ringgreen );
	CMQOMaterial* matyellow = s_select->m_materialname[ "matyellow" ];
	_ASSERT( matyellow );

	if( matred && ringred && matblue && ringblue && matgreen && ringgreen && matyellow ){
		if( (pickinfo.pickobjno >= 0) && (s_curboneno == pickinfo.pickobjno) ){

			if( (pickinfo.buttonflag == PICK_X) || (pickinfo.buttonflag == PICK_SPA_X) ){//red
				matred->dif4f = matred->col * hirate;
				ringred->dif4f = ringred->col * hirate;
				matred->dif4f.w = hia;
				ringred->dif4f.w = hia;

				matblue->dif4f = matblue->col * lowrate;
				ringblue->dif4f = ringblue->col * lowrate;
				matblue->dif4f.w = lowa;
				ringblue->dif4f.w = lowa;


				matgreen->dif4f = matgreen->col * lowrate;
				ringgreen->dif4f = ringgreen->col * lowrate;
				matgreen->dif4f.w = lowa;
				ringgreen->dif4f.w = lowa;

				matyellow->dif4f = matyellow->dif4f * lowrate;
				matyellow->dif4f.w = lowa;

			}else if( (pickinfo.buttonflag == PICK_Y) || (pickinfo.buttonflag == PICK_SPA_Y) ){//green
				matred->dif4f = matred->col * lowrate;
				ringred->dif4f = ringred->col * lowrate;
				matred->dif4f.w = lowa;
				ringred->dif4f.w = lowa;

				matblue->dif4f = matblue->col * lowrate;
				ringblue->dif4f = ringblue->col * lowrate;
				matblue->dif4f.w = lowa;
				ringblue->dif4f.w = lowa;

				matgreen->dif4f = matgreen->col * hirate;
				ringgreen->dif4f = ringgreen->col * hirate;
				matgreen->dif4f.w = hia;
				ringgreen->dif4f.w = hia;

				matyellow->dif4f = matyellow->dif4f * lowrate;
				matyellow->dif4f.w = lowa;

			}else if( (pickinfo.buttonflag == PICK_Z) || (pickinfo.buttonflag == PICK_SPA_Z) ){//blue
				matred->dif4f = matred->col * lowrate;
				ringred->dif4f = ringred->col * lowrate;
				matred->dif4f.w = lowa;
				ringred->dif4f.w = lowa;

				matblue->dif4f = matblue->col * hirate;
				ringblue->dif4f = ringblue->col * hirate;
				matblue->dif4f.w = hia;
				ringblue->dif4f.w = hia;

				matgreen->dif4f = matgreen->col * lowrate;
				ringgreen->dif4f = ringgreen->col * lowrate;
				matgreen->dif4f.w = lowa;
				ringgreen->dif4f.w = lowa;

				matyellow->dif4f = matyellow->dif4f * lowrate;
				matyellow->dif4f.w = lowa;

			}else if( pickinfo.buttonflag == PICK_CENTER ){//yellow
				matred->dif4f = matred->col * lowrate;
				ringred->dif4f = ringred->col * lowrate;
				matred->dif4f.w = lowa;
				ringred->dif4f.w = lowa;

				matblue->dif4f = matblue->col * lowrate;
				ringblue->dif4f = ringblue->col * lowrate;
				matblue->dif4f.w = lowa;
				ringblue->dif4f.w = lowa;

				matgreen->dif4f = matgreen->col * lowrate;
				ringgreen->dif4f = ringgreen->col * lowrate;
				matgreen->dif4f.w = lowa;
				ringgreen->dif4f.w = lowa;

				matyellow->dif4f = matyellow->dif4f * hirate;
				matyellow->dif4f.w = hia;

			}
		}else{
			matred->dif4f = matred->col * lowrate;
			ringred->dif4f = ringred->col * lowrate;
			matred->dif4f.w = lowa;
			ringred->dif4f.w = lowa;

			matblue->dif4f = matblue->col * lowrate;
			ringblue->dif4f = ringblue->col * lowrate;
			matblue->dif4f.w = lowa;
			ringblue->dif4f.w = lowa;

			matgreen->dif4f = matgreen->col * lowrate;
			ringgreen->dif4f = ringgreen->col * lowrate;
			matgreen->dif4f.w = lowa;
			ringgreen->dif4f.w = lowa;

			matyellow->dif4f = matyellow->dif4f * lowrate;
			matyellow->dif4f.w = lowa;

		}
	}

	return 0;
}

int ChangeTimeLineScale()
{
	if( s_owpTimeline ){
		double tmpscale;
		double curscale = s_owpTimeline->timeSize;
		if( curscale >= 5.0 ){
			tmpscale = 4.0;
		}else{
			tmpscale = 8.0;
		}
		s_owpTimeline->timeSize = tmpscale;
		s_owpTimeline->callRewrite();						//再描画
		s_owpTimeline->setRewriteOnChangeFlag(true);		//再描画要求を再開
	}

	return 0;
}
int ChangeMTimeLineScale()
{
	if( s_owpMTimeline ){
		double tmpscale;
		double curscale = s_owpMTimeline->timeSize;
		if( curscale >= 5.0 ){
			tmpscale = 4.0;
		}else{
			tmpscale = 8.0;
		}
		s_owpMTimeline->timeSize = tmpscale;
		s_owpMTimeline->callRewrite();						//再描画
		s_owpMTimeline->setRewriteOnChangeFlag(true);		//再描画要求を再開
	}
	return 0;
}
int ChangeLTimeLineScale()
{
	if( s_owpLTimeline ){
		double tmpscale;
		double curscale = s_owpLTimeline->timeSize;
		if( curscale >= 5.0 ){
			tmpscale = 4.0;
		}else{
			tmpscale = 8.0;
		}
		s_owpLTimeline->timeSize = tmpscale;
		s_owpLTimeline->callRewrite();						//再描画
		s_owpLTimeline->setRewriteOnChangeFlag(true);		//再描画要求を再開
	}
	return 0;
}

int ChangeTimeScale()
{
	double tmpscale;
	if( s_timescale >= 5.0 ){
		tmpscale = 4.0;
	}else{
		tmpscale = 8.0;
	}
	s_timescale = tmpscale;


	if( s_owpTimeline ){
		s_owpTimeline->timeSize = s_timescale;
		s_owpTimeline->callRewrite();						//再描画
		s_owpTimeline->setRewriteOnChangeFlag(true);		//再描画要求を再開
	}
	if( s_owpMTimeline ){
		s_owpMTimeline->timeSize = s_timescale;
		s_owpMTimeline->callRewrite();						//再描画
		s_owpMTimeline->setRewriteOnChangeFlag(true);		//再描画要求を再開
	}
	if( s_owpLTimeline ){
		s_owpLTimeline->timeSize = s_timescale;
		s_owpLTimeline->callRewrite();						//再描画
		s_owpLTimeline->setRewriteOnChangeFlag(true);		//再描画要求を再開
	}

	return 0;
}



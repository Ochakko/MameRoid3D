#ifndef KinectMainH
#define KinectMainH

#include <d3dx9.h>

class CKbs;
class CRps;
class CModel;
class CBone;
class CConvSetDlg;
class CQuaternion;
class CMySprite;

class CKinectMain
{
public:
	CKinectMain( LPDIRECT3DDEVICE9 pdev );
	~CKinectMain();

	int SetFilePath( WCHAR* pluginpath );
	int LoadPlugin( HWND srchwnd );

	int LoadKbs( CModel* srcmodel );
	int EditKbs( CModel* srcmodel );

	int StartCapture( CModel* srcmodel, int capmode );
	int EndCapture();

	int Update( CModel* srcmodel, double* dstframe );

	int ApplyFilter( CModel* srcmodel );
private:
	int UnloadPlugin();
	void DestroyRps();
	void DestroyKbs();

	int CreateMotionPoints( int cookie, double startframe, double endframe, int forceflag );

	int AvrgMotion( int motid, double frameleng, int type, int filter_size );

	//ガウシアンフィルタ用
	int combi( int N, int rp );

	int QtoEul( CModel* srcmodel, CQuaternion srcq, D3DXVECTOR3 befeul, int boneno, D3DXVECTOR3* eulptr, CQuaternion* axisqptr );

public:
	CModel* m_model;
	LPDIRECT3DDEVICE9 m_pdev;
	IDirect3DTexture9* m_ptex;
	HWND m_hwnd;
	int m_texwidth;
	int m_texheight;


	HMODULE m_hModule;
	WCHAR m_filepath[_MAX_PATH];
	WCHAR m_filename[_MAX_PATH];
	float m_pluginversion;

	int m_validflag;
	bool (*OpenNIInit)(int fullscflag,HWND hWnd,bool EngFlag,LPDIRECT3DDEVICE9 lpDevice,CHAR* f_path,CHAR* onifilename);
	void (*OpenNIClean)();
	void (*OpenNIDrawDepthMap)(bool waitflag);
	void (*OpenNIDepthTexture)(IDirect3DTexture9** ppTex);
	void (*OpenNIGetSkeltonJointPosition)(int num,D3DXVECTOR3* vec);
	void (*OpenNIIsTracking)(bool* lpb);
	void (*OpenNIGetVersion)(float* ver);

	int m_rendercnt;


	int m_capmode;
	int m_capmotid;
	double m_capstartframe;
	double m_capendframe;
	double m_capframe;
	double m_capstep;

	int m_timermax;
	int m_curtimer;

	CConvSetDlg* m_dlg;
};

#endif


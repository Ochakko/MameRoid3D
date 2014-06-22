#ifndef RpsH
#define RpsH

#include <d3dx9.h>
#include <coef.h>

#define RPSLINELEN	1024

class CTraQ;
class CModel;
class CBone;
class CMotionPoint;
class CQuaternion;

class CRps
{
public:
	CRps( CModel* srcmodel );
	~CRps();

	int CreateParams();
	int CalcTraQ( TSELEM* tsptr, int chksym );
	int SetMotion( MODELBOUND srcmb, TSELEM* tsptr, int motid, double setframe );

	int SetRpsElem( int frameno, D3DXVECTOR3* srcpos );
	int InitArMp( TSELEM* tsptr, int motid, double frameno );



private:
	int InitParams();
	int DestroyObjs();

	int SetBuffer( char* filename );

	int CreateRpsElem();

	int SetSkipFlag();
	int ResetOutputFlag( D3DXVECTOR3* keyeul, D3DXVECTOR3* keymv );

	int SetSym( int skelno, int sksym, TSELEM* tsptr, CModel* srcmodel );
	int CalcSymPose( TSELEM* tsptr, CModel* srcmodel, CTraQ* traq, 
		CQuaternion* newqptr, D3DXVECTOR3* newtraptr, D3DXVECTOR3* neweulptr, D3DXVECTOR3 befeul );


private:
	CModel* m_model;

	//file操作用
	HANDLE m_hfile;
	char* m_buf;
	DWORD m_pos;
	DWORD m_bufleng;
	char m_line[ RPSLINELEN ];


	RPSELEM* m_pelem;
	CTraQ* m_traq;

	int m_skipflag[ SKEL_MAX ];

	//ガウシアンフィルタ用
	//int combi( int N, int rp );


	CMotionPoint* m_armp[ SKEL_MAX ];


};


#endif
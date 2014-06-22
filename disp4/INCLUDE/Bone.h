#ifndef BONEH
#define BONEH

#include <d3dx9.h>
#include <Coef.h>
#include <MotionPoint.h>
#include <quaternion.h>

#include <map>
#include <string>

using namespace std;

class CMQOFace;
class CInfScope;
class CMotionPoint;

class CBone
{
public:
	CBone();
	~CBone();

	int SetBoneTri( CMQOFace* srcface, D3DXVECTOR3* srcpoint );
	int AddChild( CBone* childptr );

	static int ResetBoneCnt();

	int AddInfScope( CInfScope* pinfscope );
	int CalcTargetCenter();

	int UpdateMatrix( int srcmotid, double srcframe, D3DXMATRIX* wmat, D3DXMATRIX* vpmat, D3DXMATRIX* parmat, CQuaternion* parq );
	int GetBoneMarkMatrix( D3DXMATRIX* srcworld, D3DXMATRIX* bmmatptr );
	int GetBoneCircleMatrix( D3DXMATRIX* bcmatptr );

	int MakeFirstMP( int motid );
	CMotionPoint* AddMotionPoint( int srcmotid, double srcframe, int calcflag, int* existptr );

	int CalcMotionPoint( int srcmotid, double srcframe, CMotionPoint* dstmpptr, int* existptr );
	int GetBefNextMP( int srcmotid, double srcframe, CMotionPoint** ppbef, CMotionPoint** ppnext, int* existptr );
	int CalcFrameMP( double srcframe, CMotionPoint* befptr, CMotionPoint* nextptr, int existflag, CMotionPoint* dstmpptr );
	int OrderMotionPoint( int srcmotid, double settime, CMotionPoint* putmp, int samedel );

	int DeleteMotion( int srcmotid );

	int MultQ( int srcmotid, double srcframe, CQuaternion rotq );
	int AddTra( int srcmotid, double srcframe, float mvx, float mvy, float mvz );

	int DeleteMPOutOfRange( int motid, double srcleng );

	int CalcXOffsetMatrix( float mult );
	int GetKeyFrameXSRT( int motid, float* keyframearray, 
		D3DXQUATERNION* rotarray, D3DXVECTOR3* traarray, int arrayleng, int* getnum );

	int SetMiko3rdVec( D3DXVECTOR3 srcvec );

	int GetBoneNum();
	int DestroyMotionKey( int srcmotid );

private:
	int InitParams();
	int DestroyObjs();
	int CalcAxisQ();

public:
	int m_boneno;
	int m_topboneflag;
	char m_bonename[256];
	WCHAR m_wbonename[256];
	char m_engbonename[256];

	int m_bonematerialno;
	D3DXVECTOR3 m_vertpos[BT_MAX];
	CMQOFace* m_faceptr;

	D3DXVECTOR3 m_childworld;
	D3DXVECTOR3 m_childscreen;

	int m_isnum;
	CInfScope* m_isarray[ INFSCOPEMAX ];

	map<int, CMotionPoint*> m_motionkey;
	CMotionPoint m_curmp;
	CQuaternion m_axisq;
	D3DXVECTOR3 m_3rdvec;
	int m_3rdvecflag;

	int m_selectflag;

	D3DXMATRIX m_xoffsetmat;
	D3DXMATRIX m_xtransmat;
	D3DXMATRIX m_xconbinedtransmat;

	CQuaternion m_kinectq;

	CBone* m_parent;
	CBone* m_child;
	CBone* m_brother;

};

#endif
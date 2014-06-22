#ifndef MODELH
#define MODELH

#include <d3dx9.h>
#include <wchar.h>
#include <Coef.h>
#include <string>
#include <map>

#include <OrgWindow.h>
#include <UndoMotion.h>

using namespace std;

class CMQOMaterial;
class CMQOObject;
class CMQOFace;
class CBone;
class CInfScope;
class CMySprite;
class CMotionPoint;
class CQuaternion;
class CKbsFile;
class CRps;
class CPmCipherDll;

typedef struct tag_newmpelem
{
	CBone* boneptr;
	CMotionPoint* mpptr;
}NEWMPELEM;


class CModel
{
public:
	CModel();
	~CModel();

	int LoadMQO( LPDIRECT3DDEVICE9 pdev, WCHAR* wfile, WCHAR* modelfolder, float srcmult, int ismedia );
	int LoadMQOFromPnd( LPDIRECT3DDEVICE9 pdev, CPmCipherDll* cipher, int mqoindex, char* modelfolder, float srcmult );

	int OnRender( LPDIRECT3DDEVICE9 pdev, int lightflag, D3DXVECTOR4 diffusemult );
	int RenderBoneMark( LPDIRECT3DDEVICE9 pdev, CModel* bmarkptr, CMySprite* bcircleptr, int selboneno );
	int GetModelBound( MODELBOUND* dstb );

	int MakeObjectName();
	int MakeBoneTri();
	int MakeInfScope();
	int MakePolyMesh3();
	int MakeExtLine();
	int MakeDispObj();

	int CalcInf();

	int DbgDump();

	int UpdateMatrix( int startbone, D3DXMATRIX* wmat, D3DXMATRIX* vpmat );
	int SetShaderConst();

	int SetBoneEul( int boneno, D3DXVECTOR3 srceul );
	int GetBoneEul( int boneno, D3DXVECTOR3* dsteul );

	int FillTimeLine( OrgWinGUI::OWP_Timeline& timeline, OrgWinGUI::OWP_Timeline& Mtimeline, map<int, int>& lineno2boneno, map<int, int>& boneno2lineno, int* labelwidth );

	int AddMotion( char* srcname, WCHAR* wfilename, double srcleng, int* dstid );
	CMotionPoint* AddMotionPoint( int srcmotid, double srcframe, int srcboneno, int calcflag, int* existptr );
	CMorphKey* AddMorphKey( int srcmotid, double srcframe, int srcbaseno, int* existptr );

	int SetCurrentMotion( int srcmotid );
	int SetMotionFrame( double srcframe );
	int GetMotionFrame( double* dstframe );
	int SetMotionSpeed( double srcspeed );
	int GetMotionSpeed( double* dstspeed );

	int DeleteMotion( int motid );
	int GetSymBoneNo( int srcboneno, int* dstboneno, int* existptr );

	int PickBone( PICKINFO* pickinfo );
	int IKRotate( int srcboneno, D3DXVECTOR3 targetpos, int maxlevel );
	int IKRotateAxisDelta( int axiskind, int srcboneno, float delta, int maxlevel );
	int IKMove( int srcboneno, D3DXVECTOR3 targetpos );
	int IKMoveAxisDelta( int axiskind, int srcboneno, float deltax );

	int CollisionNoBoneObj_Mouse( PICKINFO* pickinfo, char* objnameptr );
	int TransformBone( int winx, int winy, int srcboneno, D3DXVECTOR3* worldptr, D3DXVECTOR3* screenptr, D3DXVECTOR3* dispptr );
	int ChangeMotFrameLeng( int motid, double srcleng );
	int AdvanceTime( int previewflag, double difftime, double* nextframeptr, int* endflagptr );

	int SaveQubFile( WCHAR* savepath, int savemotid, char* mmotname );
	int LoadQubFile( WCHAR* loadpath, int* newmotid );
	int LoadQubFileFromPnd( CPmCipherDll* cipher, int qubindex, int* newmotid );

	int MakeEnglishName();
	int CalcXTransformMatrix( float mult );
	int AddDefMaterial();

	int SetMiko3rdVec( int boneno, D3DXVECTOR3 srcvec );
	int UpdateMatrixNext();
	int SetKinectFirstPose();

	int SetMorphBaseMap();
	int SetDispFlag( char* srcobjname, int srcflag );

	int InitUndoMotion( int saveflag );
	int SaveUndoMotion( int curboneno, int curbaseno );
	int RollBackUndoMotion( int redoflag, int* curboneno, int* curbaseno );

private:
	int InitParams();
	int DestroyObjs();
	int CreateMaterialTexture();
	int CreateMaterialTexture( CPmCipherDll* cipher );

	int AddModelBound( MODELBOUND* mb, MODELBOUND* addmb );

	int DestroyMaterial();
	int DestroyObject();
	int DestroyAncObj();
	int DestroyAllMotionInfo();

	void MakeBoneReq( CBone* parbone, CMQOFace* curface, D3DXVECTOR3* pointptr, int broflag, int* errcntptr );

	int SetShapeNoReq( CMQOFace** ppface, int facenum, int searchp, int shapeno, int* setfacenum);
	int SetFaceOfShape( CMQOFace** ppface, int facenum, int shapeno, CMQOFace** ppface2, int setfacenum );

	int DbgDumpBoneReq( CBone* boneptr, int broflag );

	int UpdateMorph( int srcmotid, double srcframe );
	void UpdateMatrixReq( int srcmotid, double srcframe, D3DXMATRIX* wmat, D3DXMATRIX* vpmat, 
		D3DXMATRIX* parmat, CQuaternion* parq, CBone* srcbone, int broflag );

	void FillTimelineReq( int* bytemax, OrgWinGUI::OWP_Timeline& timeline, CBone* curbone, int* linenoptr, 
		map<int, int>& lineno2boneno, map<int, int>& boneno2lineno, int broflag );

	void SetSelectFlagReq( CBone* boneptr, int broflag );
	int CalcMouseLocalRay( PICKINFO* pickinfo, D3DXVECTOR3* startptr, D3DXVECTOR3* dirptr );
	CBone* GetCalcRootBone( CBone* firstbone, int maxlevel );
	void CalcXTransformMatrixReq( CBone* srcbone, D3DXMATRIX parenttra, float mult );

	int GetValidUndoID( int* rbundoid );
	int GetValidRedoID( int* rbundoid );

public:
	int m_modelno;

	LPDIRECT3DDEVICE9 m_pdev;

	bool m_modeldisp;
	map<int, CMQOMaterial*> m_material;
	map<string, CMQOMaterial*> m_materialname;
	map<int, CMQOObject*> m_object;
	map<string, CMQOObject*> m_objectname;

	map<int, CMQOMaterial*> m_ancmaterial;
	map<int, CMQOObject*> m_ancobject;
	map<int, CMQOObject*> m_boneobject;
	map<string, CBone*> m_bonename;

	map<int, CBone*> m_bonelist;
	map<int, CMQOObject*> m_mbaseobject;

	//int m_topbonenum;
	CBone* m_topbone;

	map<int, CInfScope*> m_infscope;


	WCHAR m_filename[MAX_PATH];
	WCHAR m_dirname[MAX_PATH];
	WCHAR m_modelfolder[MAX_PATH];

	float m_loadmult;

	D3DXMATRIX m_bonemat[ MAXBONENUM ];

	D3DXMATRIX m_matWorld;
	D3DXMATRIX m_matVP;

	map<int, MOTINFO*> m_motinfo;
	MOTINFO* m_curmotinfo;

	vector<NEWMPELEM> m_newmplist;

	CKbsFile* m_kbs;
	CRps* m_rps;
	int m_frompnd;

	CUndoMotion m_undomotion[ UNDOMAX ];
	int m_undoid;
};

#endif
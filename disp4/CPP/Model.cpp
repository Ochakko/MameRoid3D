//#include <stdafx.h>

#include <stdio.h>
#include <stdarg.h>
#include <math.h>
#include <string.h>
#include <wchar.h>
#include <ctype.h>
#include <malloc.h>
#include <memory.h>

#include <windows.h>
#include <crtdbg.h>

#include <Model.h>
#include <polymesh3.h>
#include <ExtLine.h>

#include <GetMaterial.h>

#include <mqofile.h>
#include <mqomaterial.h>
#include <mqoobject.h>
#include <Bone.h>
#include <mqoface.h>

#define DBGH
#include <dbg.h>

#include <DXUT.h>
#include <DXUTcamera.h>
#include <DXUTgui.h>
#include <DXUTsettingsdlg.h>
#include <SDKmisc.h>

#include <DispObj.h>
#include <InfScope.h>
#include <MySprite.h>

#include <KbsFile.h>
#include <rps.h>

#include <MotionPoint.h>
#include <quaternion.h>
#include <VecMath.h>
#include <QubFile.h>

#include <Collision.h>
#include <EngName.h>


#include <string>
#include <PmCipherDll.h>

#include <MorphKey.h>

using namespace OrgWinGUI;


static int s_alloccnt = 0;

extern WCHAR g_basedir[ MAX_PATH ];
extern ID3DXEffect*	g_pEffect;
extern D3DXHANDLE g_hmBoneQ;
extern D3DXHANDLE g_hmBoneTra;
extern D3DXHANDLE g_hmWorld;


int g_dbgflag = 0;

CModel::CModel() : m_newmplist()
{
	InitParams();
	s_alloccnt++;
	m_modelno = s_alloccnt;
}
CModel::~CModel()
{
	DestroyObjs();
}
int CModel::InitParams()
{
	m_modelno = 0;
	m_pdev = 0;

	ZeroMemory( m_filename, sizeof( WCHAR ) * MAX_PATH );
	ZeroMemory( m_dirname, sizeof( WCHAR ) * MAX_PATH );
	ZeroMemory( m_modelfolder, sizeof( WCHAR ) * MAX_PATH );
	m_loadmult = 1.0f;

	ZeroMemory( m_bonemat, sizeof( D3DXMATRIX ) * MAXBONENUM );
	D3DXMatrixIdentity( &m_matWorld );
	D3DXMatrixIdentity( &m_matVP );

	m_curmotinfo = 0;

	m_newmplist.erase( m_newmplist.begin(), m_newmplist.end() );

	m_kbs = 0;
	m_rps = 0;

	m_modeldisp = true;
	m_topbone = 0;

	m_frompnd = 0;

	InitUndoMotion( 0 );

	return 0;
}
int CModel::DestroyObjs()
{
	DestroyMaterial();
	DestroyObject();
	DestroyAncObj();
	DestroyAllMotionInfo();

	if( m_rps ){
		delete m_rps;
		m_rps = 0;
	}
	if( m_kbs ){
		delete m_kbs;
		m_kbs = 0;
	}

	InitParams();

	return 0;
}

int CModel::DestroyAllMotionInfo()
{
	map<int, MOTINFO*>::iterator itrmi;
	for( itrmi = m_motinfo.begin(); itrmi != m_motinfo.end(); itrmi++ ){
		MOTINFO* miptr = itrmi->second;
		if( miptr ){
			free( miptr );
		}
	}
	m_motinfo.erase( m_motinfo.begin(), m_motinfo.end() );

	m_curmotinfo = 0;

	return 0;
}

int CModel::DestroyMaterial()
{
	map<int, CMQOMaterial*>::iterator itr;
	for( itr = m_material.begin(); itr != m_material.end(); itr++ ){
		CMQOMaterial* delmat = itr->second;
		if( delmat ){
			delete delmat;
		}
	}
	m_material.erase( m_material.begin(), m_material.end() );

	return 0;
}
int CModel::DestroyObject()
{

	map<int, CMQOObject*>::iterator itr;
	for( itr = m_object.begin(); itr != m_object.end(); itr++ ){
		CMQOObject* delobj = itr->second;
		if( delobj ){
			delete delobj;
		}
	}
	m_object.erase( m_object.begin(), m_object.end() );

	return 0;
}

int CModel::DestroyAncObj()
{
	map<int, CMQOMaterial*>::iterator itrmat;
	for( itrmat = m_ancmaterial.begin(); itrmat != m_ancmaterial.end(); itrmat++ ){
		CMQOMaterial* delmat = itrmat->second;
		if( delmat ){
			delete delmat;
		}
	}
	m_ancmaterial.erase( m_ancmaterial.begin(), m_ancmaterial.end() );

	map<int, CMQOObject*>::iterator itrobj;
	for( itrobj = m_ancobject.begin(); itrobj != m_ancobject.end(); itrobj++ ){
		CMQOObject* delobj = itrobj->second;
		if( delobj ){
			delete delobj;
		}
	}
	m_ancobject.erase( m_ancobject.begin(), m_ancobject.end() );


	map<int, CMQOObject*>::iterator itrobj2;
	for( itrobj2 = m_boneobject.begin(); itrobj2 != m_boneobject.end(); itrobj2++ ){
		CMQOObject* delobj = itrobj2->second;
		if( delobj ){
			delete delobj;
		}
	}
	m_boneobject.erase( m_boneobject.begin(), m_boneobject.end() );


	map<int, CBone*>::iterator itrbone;
	for( itrbone = m_bonelist.begin(); itrbone != m_bonelist.end(); itrbone++ ){
		CBone* delbone = itrbone->second;
		if( delbone ){
			delete delbone;
		}
	}
	m_bonelist.erase( m_bonelist.begin(), m_bonelist.end() );

	m_topbone = 0;

	map<int, CInfScope*>::iterator itris;
	for( itris = m_infscope.begin(); itris != m_infscope.end(); itris++ ){
		CInfScope* delis = itris->second;
		if( delis ){
			delete delis;
		}
	}
	m_infscope.erase( m_infscope.begin(), m_infscope.end() );

	m_objectname.erase( m_objectname.begin(), m_objectname.end() );
	m_bonename.erase( m_bonename.begin(), m_bonename.end() );

	return 0;
}

int CModel::LoadMQOFromPnd( LPDIRECT3DDEVICE9 pdev, CPmCipherDll* cipher, int mqoindex, char* modelfolder, float srcmult )
{
	m_frompnd = 1;

	MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, modelfolder, -1, m_modelfolder, MAX_PATH );
	m_loadmult = srcmult;
	m_pdev = pdev;

	PNDPROP prop;
	ZeroMemory( &prop, sizeof( PNDPROP ) );
	CallF( cipher->GetProperty( mqoindex, &prop ), return 1 );


	WCHAR fullname[MAX_PATH] = {0};
	MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, prop.path, -1, fullname, MAX_PATH );

    WCHAR* strLastSlash = NULL;
    strLastSlash = wcsrchr( fullname, TEXT( '\\' ) );
    if( strLastSlash )
    {
		*strLastSlash = 0;
		wcscpy_s( m_dirname, MAX_PATH, fullname );
		wcscpy_s( m_filename, MAX_PATH, strLastSlash + 1 );
	}else{
		ZeroMemory( m_dirname, sizeof( WCHAR ) * MAX_PATH );
		wcscpy_s( m_filename, MAX_PATH, fullname );
	}


	//SetCurrentDirectory( m_dirname );


	DestroyMaterial();
	DestroyObject();

	CMQOFile mqofile;
	CallF( mqofile.LoadMQOFileFromPnd( m_pdev, cipher, mqoindex, srcmult, this ), return 1 );

	CallF( MakeObjectName(), return 1 );


	CallF( CreateMaterialTexture( cipher ), return 1 );


	return 0;
}

int CModel::LoadMQO( LPDIRECT3DDEVICE9 pdev, WCHAR* wfile, WCHAR* modelfolder, float srcmult, int ismedia )
{
	if( modelfolder ){
		wcscpy_s( m_modelfolder, MAX_PATH, modelfolder );
	}else{
		ZeroMemory( m_modelfolder, sizeof( WCHAR ) * MAX_PATH );
	}
	m_loadmult = srcmult;
	m_pdev = pdev;

	WCHAR fullname[MAX_PATH];

	if( ismedia == 1 ){
		WCHAR str[MAX_PATH];
		HRESULT hr;
		hr = DXUTFindDXSDKMediaFileCch( str, MAX_PATH, wfile );
		if( hr != S_OK ){
			::MessageBoxA( NULL, "media not found error !!!", "load error", MB_OK );
			_ASSERT( 0 );
			return 1;
		}
		wcscpy_s( fullname, MAX_PATH, g_basedir );
		wcscat_s( fullname, MAX_PATH, str );

	}else{
		wcscpy_s( fullname, MAX_PATH, wfile );
	}

    WCHAR* strLastSlash = NULL;
    strLastSlash = wcsrchr( fullname, TEXT( '\\' ) );
    if( strLastSlash )
    {
		*strLastSlash = 0;
		wcscpy_s( m_dirname, MAX_PATH, fullname );
		wcscpy_s( m_filename, MAX_PATH, strLastSlash + 1 );
	}else{
		ZeroMemory( m_dirname, sizeof( WCHAR ) * MAX_PATH );
		wcscpy_s( m_filename, MAX_PATH, fullname );
	}


	SetCurrentDirectory( m_dirname );


	DestroyMaterial();
	DestroyObject();

	CMQOFile mqofile;
	D3DXVECTOR3 vop( 0.0f, 0.0f, 0.0f );
	D3DXVECTOR3 vor( 0.0f, 0.0f, 0.0f );
	CallF( mqofile.LoadMQOFile( m_pdev, srcmult, m_filename, vop, vor, this ), return 1 );

	CallF( MakeObjectName(), return 1 );


	CallF( CreateMaterialTexture(), return 1 );


	return 0;
}

int CModel::CreateMaterialTexture( CPmCipherDll* cipher )
{
	map<int,CMQOMaterial*>::iterator itr;
	for( itr = m_material.begin(); itr != m_material.end(); itr++ ){
		CMQOMaterial* curmat = itr->second;
		CallF( curmat->CreateTexture( cipher, m_dirname ), return 1 );
	}

	return 0;
}


int CModel::CreateMaterialTexture()
{
	map<int,CMQOMaterial*>::iterator itr;
	for( itr = m_material.begin(); itr != m_material.end(); itr++ ){
		CMQOMaterial* curmat = itr->second;
		CallF( curmat->CreateTexture( m_dirname ), return 1 );
	}

	return 0;
}

int CModel::OnRender( LPDIRECT3DDEVICE9 pdev, int lightflag, D3DXVECTOR4 diffusemult )
{
	map<int,CMQOObject*>::iterator itr;
	for( itr = m_object.begin(); itr != m_object.end(); itr++ ){
		CMQOObject* curobj = itr->second;
		if( curobj && curobj->m_dispflag ){
			if( curobj->m_dispobj ){
				CMQOObject* morphbaseobj = 0;
				if( curobj->m_morphobj.empty() ){
					morphbaseobj = 0;
				}else{
					morphbaseobj = curobj;
				}
				CallF( curobj->m_dispobj->RenderNormal( lightflag, diffusemult, morphbaseobj ), return 1 );
			}
			if( curobj->m_displine ){
				CallF( curobj->m_displine->RenderLine( diffusemult ), return 1 );
			}
		}
	}

	return 0;
}

int CModel::GetModelBound( MODELBOUND* dstb )
{
	MODELBOUND mb;
	MODELBOUND addmb;
	ZeroMemory( &mb, sizeof( MODELBOUND ) );

	int calcflag = 0;
	map<int,CMQOObject*>::iterator itr;
	for( itr = m_object.begin(); itr != m_object.end(); itr++ ){
		CMQOObject* curobj = itr->second;
		if( curobj->m_pm3 && curobj->m_pm3->m_infbone ){
			curobj->m_pm3->CalcBound();
			if( calcflag == 0 ){
				mb = curobj->m_pm3->m_bound;
			}else{
				addmb = curobj->m_pm3->m_bound;
				AddModelBound( &mb, &addmb );
			}
			calcflag++;
		}
		if( curobj->m_extline ){
			curobj->m_extline->CalcBound();
			if( calcflag == 0 ){
				mb = curobj->m_extline->m_bound;
			}else{
				addmb = curobj->m_extline->m_bound;
				AddModelBound( &mb, &addmb );
			}
			calcflag++;
		}
	}

	*dstb = mb;

	return 0;
}

int CModel::AddModelBound( MODELBOUND* mb, MODELBOUND* addmb )
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

int CModel::MakeBoneTri()
{
	if( m_boneobject.begin() == m_boneobject.end() ){
		m_topbone = 0;
		return 0;//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	}


	CMQOFace* toplevelface[ MAXBONENUM ];
	int toplevelnum = 0;

	map<int, CMQOObject*>::iterator itrobj;
	itrobj = m_boneobject.begin();
	CMQOObject* curobj = itrobj->second;
	if( curobj ){
		CallF( curobj->SetMikoBoneIndex3(), return 1 );
		CallF( curobj->SetMikoBoneIndex2(), return 1 );
		CallF( curobj->CheckSameMikoBone(), return 1 );
		CallF( curobj->GetTopLevelMikoBone( toplevelface, &toplevelnum, MAXBONENUM ), return 1 );

		CallF( curobj->InitFaceDirtyFlag(), return 1 );
		int topno;
		for( topno = 0; topno < toplevelnum; topno++ ){
			CallF( curobj->SetTreeMikoBone( toplevelface[ topno ] ), return 1 );
		}

		CallF( curobj->InitFaceDirtyFlag(), return 1 );
		int islooped = 0;
		int jointnum = 0;
		int totaljointnum = 0;
		for( topno = 0; topno < toplevelnum; topno++ ){
			jointnum = 0;
			CallF( curobj->CheckLoopedMikoBoneReq( toplevelface[ topno ], &islooped, &jointnum ), return 1 );
			if( islooped ){
				::MessageBox( NULL, L"ループ又は重複しているボーン構造が見つかりました。(誤判定かも（汗）)読み込めません。", L"エラー", MB_OK );
				_ASSERT( 0 );
				return 1;
			}
			totaljointnum += jointnum;
			if( totaljointnum >= MAXBONENUM ){
				WCHAR wmes[256];
				swprintf_s( wmes, 256, L"ボーンの数が多すぎます。%d個以下にしてください。", MAXBONENUM );
				::MessageBox( NULL, wmes, L"エラー", MB_OK );
				_ASSERT( 0 );
				return 1;
			}
		}

		CallF( curobj->SetMikoBoneName( m_ancmaterial ), return 1 );
		CallF( curobj->SetMikoFloatBoneName(), return 1 );


		CBone::ResetBoneCnt();
		m_topbone = new CBone;
		if( !m_topbone ){
			_ASSERT( 0 );
			return 1;
		}
		m_topbone->m_topboneflag = 1;
		CallF( m_topbone->SetBoneTri( 0, curobj->m_pointbuf ), return 1 );
		m_bonelist[m_topbone->m_boneno] = m_topbone;
		m_bonename[m_topbone->m_bonename] = m_topbone;


		int secbonenum = toplevelnum;
		int errcnt = 0;
		int secno;
		for( secno = 0; secno < toplevelnum; secno++ ){
			CBone* cursec = new CBone;
			if( !cursec ){
				_ASSERT( 0 );
				return 1;
			}
			cursec->m_topboneflag = 2;
			CMQOFace* topface = toplevelface[ secno ];
			CallF( cursec->SetBoneTri( topface, curobj->m_pointbuf ), return 1 );
			m_bonelist[cursec->m_boneno] = cursec;

			CallF( m_topbone->AddChild( cursec ), return 1 );
			m_bonename[ cursec->m_bonename ] = cursec;

			MakeBoneReq( cursec, topface, curobj->m_pointbuf, 0, &errcnt );
			if( errcnt > 0 ){
				_ASSERT( 0 );
				return 1;
			}
		}
	}

	return 0;
}

void CModel::MakeBoneReq( CBone* parbone, CMQOFace* curface, D3DXVECTOR3* pointptr, int broflag, int* errcntptr )
{
	if( *errcntptr > 0 ){
		return;
	}

	if( (curface->m_bonetype != MIKOBONE_NORMAL) && (curface->m_bonetype != MIKOBONE_FLOAT) ){
		return;
	}

	CBone* newbone = new CBone();
	if( !newbone ){
		_ASSERT( 0 );
		(*errcntptr)++;
		return;
	}
	m_bonelist[ newbone->m_boneno ] = newbone;

	int ret;
	ret = newbone->SetBoneTri( curface, pointptr );
	if( ret ){
		_ASSERT( 0 );
		(*errcntptr)++;
		return;
	}

	ret = parbone->AddChild( newbone );
	if( ret ){
		_ASSERT( 0 );
		(*errcntptr)++;
		return;
	}
	m_bonename[ newbone->m_bonename ] = newbone;


/////////
	if( curface->m_child ){
		MakeBoneReq( newbone, curface->m_child, pointptr, 1, errcntptr );
	}
	if( broflag && curface->m_brother ){
		MakeBoneReq( parbone, curface->m_brother, pointptr, 1, errcntptr );
	}
}

int CModel::MakeInfScope()
{
	map<int, CMQOObject*>::iterator itrobj;
	for( itrobj = m_ancobject.begin(); itrobj != m_ancobject.end(); itrobj++ ){
		CMQOObject* curobj = itrobj->second;
		if( curobj ){
			int matnum = 0;
			CallF( curobj->GetMaterialNoInUse( 0, 0, &matnum ), return 1 );

			if( matnum > 0 ){
				CMQOObject* targetobjptr = 0;
				CBone* applynamebone = 0;

				char* ancnameptr = curobj->m_name;
				char* sepptr = 0;
				sepptr = strchr( ancnameptr, '|' );
				if( sepptr ){
					char* startptr = sepptr + 1;
					int leng = (int)strlen( startptr );
					string findname( startptr, startptr + leng );
					targetobjptr = m_objectname[ findname ];
				}



				int* matarray = (int*)malloc( sizeof( int ) * matnum );
				if( !matarray ){
					_ASSERT( 0 );
					return 1;
				}
				ZeroMemory( matarray, sizeof( int ) * matnum );

				int chknum = 0;
				CallF( curobj->GetMaterialNoInUse( matarray, matnum, &chknum ), return 1 );

				int matcnt;
				for( matcnt = 0; matcnt < matnum; matcnt++ ){
					int matno = *( matarray + matcnt );

					int facenum = 0;
					CallF( curobj->GetFaceInMaterial( matno, 0, 0, &facenum ), return 1 );
					if( facenum > 0 ){
						CMQOFace** ppface = (CMQOFace**)malloc( sizeof( CMQOFace* ) * facenum );
						ZeroMemory( ppface, sizeof( CMQOFace* ) * facenum );
						int chkfnum = 0;
						CallF( curobj->GetFaceInMaterial( matno, ppface, facenum, &chkfnum ), return 1 );

						int setfacenum = 0;
						int setfirst = 1;
						int shapeno = 0;
						while( (setfirst == 1) || (setfacenum > 0) ){
							setfacenum = 0;

							CMQOFace* startface = 0;
							int chkf;
							for( chkf = 0; chkf < facenum; chkf++ ){
								if( (*( ppface + chkf ))->m_shapeno == -1 ){
									startface = *(ppface + chkf);
									break;
								}
							}
							if( startface ){
								int si;
								for( si = 0; si < startface->m_pointnum; si++ ){
									int searchp = startface->m_index[ si ];
									CallF( SetShapeNoReq( ppface, facenum, searchp, shapeno, &setfacenum), return 1 );
								}
								if( setfacenum > 0 ){
									CMQOFace** ppface2 = (CMQOFace**)malloc( sizeof( CMQOFace* ) * setfacenum );
									if( !ppface2 ){
										_ASSERT( 0 );
										return 1;
									}
									CallF( SetFaceOfShape( ppface, facenum, shapeno, ppface2, setfacenum), return 1 );

									CInfScope* newis = new CInfScope();
									if( !newis ){
										_ASSERT( 0 );
										return 1;
									}
									newis->m_materialno = matno;
									newis->m_facenum = setfacenum;
									newis->m_ppface = ppface2;
									newis->m_vnum = curobj->m_vertex;
									newis->m_pvert = curobj->m_pointbuf;
									newis->m_targetobj = targetobjptr;
									if( applynamebone ){
										newis->m_tmpappbone = applynamebone;
									}
									m_infscope[ newis->m_serialno ] = newis;

									CallF( newis->CalcCenter(), return 1 );

									shapeno++;

								}
			
							}else{
								break;
							}

							setfirst = 0;
						}
						free( ppface );
					}
				}
				free( matarray );
			}
		}
	}


	D3DXVECTOR3 bonediff;
	float bonedist, tmpdist;

	map<int, CInfScope*>::iterator itris;
	for( itris = m_infscope.begin(); itris != m_infscope.end(); itris++ ){
		CInfScope* curis = itris->second;
		if( curis->m_tmpappbone == 0 ){
			CMQOMaterial* curmaterial = GetMaterialFromNo( m_ancmaterial, curis->m_materialno );
			if( curmaterial ){
				int setboneno = -1;
				map<int, CBone*>::iterator itrbone;
				for( itrbone = m_bonelist.begin(); itrbone != m_bonelist.end(); itrbone++ ){
					CBone* cmpbone = itrbone->second;
					if( cmpbone->m_bonematerialno == curmaterial->materialno ){
						if( setboneno == -1 ){
							setboneno = cmpbone->m_boneno;
							bonediff = (cmpbone->m_vertpos[BT_PARENT] + cmpbone->m_vertpos[BT_CHILD]) * 0.5f - curis->m_center;
							bonedist = D3DXVec3Length( &bonediff );
						}else{
							bonediff = (cmpbone->m_vertpos[BT_PARENT] + cmpbone->m_vertpos[BT_CHILD]) * 0.5f - curis->m_center;
							tmpdist = D3DXVec3Length( &bonediff );
							if( tmpdist < bonedist ){
								setboneno = cmpbone->m_boneno;
								bonedist = tmpdist;
							}
						}
					}
				}

				if( setboneno >= 0 ){
					curis->m_applyboneno = setboneno;
					curis->m_validflag = 1;
		
					CallF( m_bonelist[ setboneno ]->AddInfScope( curis ), return 1 );
				}
			}
		}else{
			curis->m_applyboneno = curis->m_tmpappbone->m_boneno;
			curis->m_validflag = 1;

			CallF( m_bonelist[ curis->m_applyboneno ]->AddInfScope( curis ), return 1 );

		}
	}

	map<int, CBone*>::iterator itrb;
	for( itrb = m_bonelist.begin(); itrb != m_bonelist.end(); itrb++ ){
		CBone* curbone = itrb->second;
		if( curbone ){
			CallF( curbone->CalcTargetCenter(), return 1 );
		}
	}

	return 0;
}


int CModel::CalcInf()
{
	if( m_bonelist.empty() ){
		return 0;
	}

	map<int, CMQOObject*>::iterator itrobj;
	for( itrobj = m_object.begin(); itrobj != m_object.end(); itrobj++ ){
		CMQOObject* curobj = itrobj->second;
		if( curobj && curobj->m_pm3 ){
			CBone* applybone = 0;
			char* nameptr = curobj->m_name;
			char* applyptr = strchr( nameptr, '-' );
			if( applyptr ){
				int leng = (int)strlen( applyptr + 1 );
				string strapply( applyptr + 1, applyptr + 1 + leng );
				applybone = m_bonename[ strapply ];
			}

			if( applybone ){
				CallF( curobj->m_pm3->CalcInfNoSkin( applybone ), return 1 );
			}else{
				CallF( curobj->m_pm3->CalcInf( curobj, m_bonelist ), return 1; );
			}
		}
	}


	return 0;
}


int CModel::SetShapeNoReq( CMQOFace** ppface, int facenum, int searchp, int shapeno, int* setfacenum )
{

	int fno;
	CMQOFace* findface[200];
	ZeroMemory( findface, sizeof( CMQOFace* ) * 200 );
	int findnum = 0;

	for( fno = 0; fno < facenum; fno++ ){
		CMQOFace* curface = *( ppface + fno );
		if( curface->m_shapeno != -1 ){
			continue;
		}

		int chki;
		for( chki = 0; chki < curface->m_pointnum; chki++ ){
			if( searchp == curface->m_index[ chki ] ){
				if( findnum >= 200 ){
					_ASSERT( 0 );
					return 1;
				}
				curface->m_shapeno = shapeno;
				findface[findnum] = curface;
				findnum++;
				break;
			}
		}
	}

	if( findnum > 0 ){
		(*setfacenum) += findnum;

		int findno;
		for( findno = 0; findno < findnum; findno++ ){
			CMQOFace* fface = findface[ findno ];
			int i;
			for( i = 0; i < fface->m_pointnum; i++ ){
				int newsearch = fface->m_index[ i ];
				if( newsearch != searchp ){
					SetShapeNoReq( ppface, facenum, newsearch, shapeno, setfacenum );
				}
			}
		}
	}
	return 0;
}

int CModel::SetFaceOfShape( CMQOFace** ppface, int facenum, int shapeno, CMQOFace** ppface2, int setfacenum )
{
	int setno = 0;
	int fno;
	for( fno = 0; fno < facenum; fno++ ){
		CMQOFace* curface = *( ppface + fno );
		if( curface->m_shapeno == shapeno ){
			if( setno >= setfacenum ){
				_ASSERT( 0 );
				return 1;
			}
			*( ppface2 + setno ) = curface;
			setno++;
		}
	}

	_ASSERT( setno == setfacenum );

	return 0;
}

int CModel::MakeObjectName()
{
	m_objectname.clear();
	map<int, CMQOObject*>::iterator itrobj;
	for( itrobj = m_object.begin(); itrobj != m_object.end(); itrobj++ ){
		CMQOObject* curobj = itrobj->second;
		if( curobj ){
			char* nameptr = curobj->m_name;
			int sdefcmp, bdefcmp;
			sdefcmp = strncmp( nameptr, "sdef:", 5 );
			bdefcmp = strncmp( nameptr, "bdef:", 5 );
			if( (sdefcmp != 0) && (bdefcmp != 0) ){
				int leng = (int)strlen( nameptr );
				string firstname( nameptr, nameptr + leng );
				m_objectname[ firstname ] = curobj;
				curobj->m_dispname = firstname;
			}else{
				char* startptr = nameptr + 5;
				int leng = (int)strlen( startptr );
				string firstname( startptr, startptr + leng );
				m_objectname[ firstname ] = curobj;
				curobj->m_dispname = firstname;
			}
		}
	}


	m_materialname.clear();
	map<int, CMQOMaterial*>::iterator itrmat;
	for( itrmat = m_material.begin(); itrmat != m_material.end(); itrmat++ ){
		CMQOMaterial* curmat = itrmat->second;
		if( curmat ){
			m_materialname[ curmat->name ] = curmat;
		}
	}

	return 0;
}

int CModel::DbgDump()
{
	DbgOut( L"######start DbgDump\r\n" );

	DbgOut( L"Dump Bone And InfScope\r\n" );

	if( m_topbone ){
		DbgDumpBoneReq( m_topbone, 0 );
	}

	DbgOut( L"\r\n\r\nDump Object And MorphObj\r\n" );


	if( m_object.empty() == 0 ){
		map<int,CMQOObject*>::iterator itrobj;
		for( itrobj = m_object.begin(); itrobj != m_object.end(); itrobj++ ){
			CMQOObject* curobj = itrobj->second;
			_ASSERT( curobj );

			WCHAR wname[256];
			MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, curobj->m_name, 256, wname, 256 );
			DbgOut( L"objname %s, morph obj %d\r\n", wname, curobj->m_morphobj.size() );

			map<int,CMQOObject*>::iterator itrmobj;
			for( itrmobj = curobj->m_morphobj.begin(); itrmobj != curobj->m_morphobj.end(); itrmobj++ ){
				CMQOObject* mobj = itrmobj->second;
				_ASSERT( mobj );

				WCHAR wname2[256];
				MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, mobj->m_name, 256, wname2, 256 );
				DbgOut( L"\tmorph obj name %s\r\n", wname2 );
			}
		}
	}


//	MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, name, 256, wname, 256 );

	DbgOut( L"######end DbgDump\r\n" );


	return 0;
}

int CModel::DbgDumpBoneReq( CBone* boneptr, int broflag )
{
	char mes[1024];
	WCHAR wmes[1024];

	sprintf_s( mes, 1024, "\tboneno %d, bonename %s\r\n", boneptr->m_boneno, boneptr->m_bonename );
	MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, mes, 1024, wmes, 1024 );
	DbgOut( wmes );

	DbgOut( L"\t\tbonepos (%f, %f, %f), (%f, %f, %f)\r\n", 
		boneptr->m_vertpos[ BT_PARENT ].x, boneptr->m_vertpos[ BT_PARENT ].y, boneptr->m_vertpos[ BT_PARENT ].z,
		boneptr->m_vertpos[ BT_CHILD ].x, boneptr->m_vertpos[ BT_CHILD ].y, boneptr->m_vertpos[ BT_CHILD ].z );

	
	DbgOut( L"\t\tinfscopenum %d\r\n", boneptr->m_isnum );
	int isno;
	for( isno = 0; isno < boneptr->m_isnum; isno++ ){
		CInfScope* curis = boneptr->m_isarray[ isno ];
		CBone* infbone = 0;
		if( curis->m_applyboneno >= 0 ){
			infbone = m_bonelist[ curis->m_applyboneno ];
			sprintf_s( mes, 1024, "\t\tInfScope %d, validflag %d, facenum %d, applybone %s\r\n", 
				isno, curis->m_validflag, curis->m_facenum, infbone->m_bonename );
			MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, mes, 1024, wmes, 1024 );
			DbgOut( wmes );

		}else{
			DbgOut( L"\t\tInfScope %d, validflag %d, facenum %d, applybone is none\r\n", 
				isno, curis->m_validflag, curis->m_facenum );
		}

		if( curis->m_targetobj ){
			sprintf_s( mes, 1024, "\t\t\ttargetobj %s\r\n", curis->m_targetobj->m_name );
			MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, mes, 1024, wmes, 1024 );
			DbgOut( wmes );
		}else{
			DbgOut( L"\t\t\ttargetobj is none\r\n" );
		}
	}

//////////
	if( boneptr->m_child ){
		DbgDumpBoneReq( boneptr->m_child, 1 );
	}
	if( (broflag == 1) && boneptr->m_brother ){
		DbgDumpBoneReq( boneptr->m_brother, 1 );
	}



	return 0;
}

int CModel::MakePolyMesh3()
{
	map<int,CMQOObject*>::iterator itr;
	for( itr = m_object.begin(); itr != m_object.end(); itr++ ){
		CMQOObject* curobj = itr->second;
		if( curobj ){
			CallF( curobj->MakePolymesh3( m_pdev, m_material ), return 1 );
		}
	}

	return 0;
}

int CModel::MakeExtLine()
{
	map<int,CMQOObject*>::iterator itr;
	for( itr = m_object.begin(); itr != m_object.end(); itr++ ){
		CMQOObject* curobj = itr->second;
		if( curobj ){
			CallF( curobj->MakeExtLine(), return 1 );
		}
	}

	return 0;
}

int CModel::MakeDispObj()
{
	int hasbone;
	if( m_bonelist.empty() ){
		hasbone = 0;
	}else{
		hasbone = 1;
	}

	map<int,CMQOObject*>::iterator itr;
	for( itr = m_object.begin(); itr != m_object.end(); itr++ ){
		CMQOObject* curobj = itr->second;
		if( curobj ){
			CallF( curobj->MakeDispObj( m_pdev, m_material, hasbone ), return 1 );
		}
	}

	return 0;
}

int CModel::UpdateMatrix( int startbone, D3DXMATRIX* wmat, D3DXMATRIX* vpmat )
{
	m_matWorld = *wmat;
	m_matVP = *vpmat;

	if( !m_curmotinfo ){
		return 0;//!!!!!!!!!!!!
	}

	int curmotid = m_curmotinfo->motid;
	double curframe = m_curmotinfo->curframe;


	CallF( UpdateMorph( curmotid, curframe ), return 1 );

	D3DXMATRIX inimat;
	D3DXMatrixIdentity( &inimat );
	CQuaternion iniq;
	iniq.SetParams( 1.0f, 0.0f, 0.0f, 0.0f );

	if( startbone < 0 ){
		UpdateMatrixReq( curmotid, curframe, wmat, vpmat, &inimat, &iniq, m_topbone, 0 );
	}else{
		CBone* startboneptr = m_bonelist[ startbone ];
		if( !startboneptr ){
			_ASSERT( 0 );
			return 1;
		}
		CBone* parbone = startboneptr->m_parent;
		if( parbone ){
			UpdateMatrixReq( curmotid, curframe, wmat, vpmat, &(parbone->m_curmp.m_totalmat), &(parbone->m_curmp.m_totalq), startboneptr, 0 );
		}else{
			UpdateMatrixReq( curmotid, curframe, wmat, vpmat, &inimat, &iniq, startboneptr, 0 );
		}
	}

	return 0;
}

void CModel::UpdateMatrixReq( int srcmotid, double srcframe, D3DXMATRIX* wmat, D3DXMATRIX* vpmat, 
	D3DXMATRIX* parmat, CQuaternion* parq, CBone* srcbone, int broflag )
{
	if( srcbone && wmat && parmat ){
		srcbone->UpdateMatrix( srcmotid, srcframe, wmat, vpmat, parmat, parq );
	}else{
		return;
	}

	if( srcbone->m_child ){
		UpdateMatrixReq( srcmotid, srcframe, wmat, vpmat, &(srcbone->m_curmp.m_totalmat),&(srcbone->m_curmp.m_totalq), srcbone->m_child, 1 );
	}
	if( srcbone->m_brother && (broflag == 1) ){
		UpdateMatrixReq( srcmotid, srcframe, wmat, vpmat, parmat, parq, srcbone->m_brother, 1 );
	}
}

int CModel::SetShaderConst()
{
	D3DXQUATERNION boneq[MAXBONENUM];
	D3DXVECTOR3 bonetra[MAXBONENUM];

	ZeroMemory( boneq, sizeof( D3DXQUATERNION ) * MAXBONENUM );
	ZeroMemory( bonetra, sizeof( D3DXVECTOR3 ) * MAXBONENUM );

	map<int, CBone*>::iterator itrbone;
	int setcnt = 0;
	for( itrbone = m_bonelist.begin(); itrbone != m_bonelist.end(); itrbone++ ){
		int boneno = itrbone->first;
		CBone* curbone = itrbone->second;
		if( curbone && (boneno >= 0) && (boneno < MAXBONENUM) ){
			boneq[boneno].x = curbone->m_curmp.m_dispq.x;
			boneq[boneno].y = curbone->m_curmp.m_dispq.y;
			boneq[boneno].z = curbone->m_curmp.m_dispq.z;
			boneq[boneno].w = curbone->m_curmp.m_dispq.w;
			bonetra[boneno] = curbone->m_curmp.m_disptra;
			setcnt++;
		}
	}

	if( setcnt > 0 ){
		HRESULT hr;
		hr = g_pEffect->SetValue( g_hmBoneQ, (void*)boneq, sizeof( D3DXQUATERNION ) * MAXBONENUM );
		if( hr != D3D_OK ){
			_ASSERT( 0 );
			return 1;
		}
		hr = g_pEffect->SetValue( g_hmBoneTra, (void*)bonetra, sizeof( D3DXVECTOR3 ) * MAXBONENUM );
		if( hr != D3D_OK ){
			_ASSERT( 0 );
			return 1;
		}
	}
	return 0;
}

int CModel::SetBoneEul( int boneno, D3DXVECTOR3 srceul )
{
	if( boneno < 0 ){
		_ASSERT( 0 );
		return 1;
	}

	CBone* curbone;
	curbone = m_bonelist[ boneno ];
	if( curbone ){
		curbone->m_curmp.SetEul( &(curbone->m_axisq), srceul );
	}

	return 0;
}

int CModel::GetBoneEul( int boneno, D3DXVECTOR3* dsteul )
{
	if( boneno < 0 ){
		_ASSERT( 0 );
		return 1;
	}

	CBone* curbone;
	curbone = m_bonelist[ boneno ];
	if( curbone ){
		*dsteul = curbone->m_curmp.m_eul;
	}else{
		*dsteul = D3DXVECTOR3( 0.0f, 0.0f, 0.0f );
	}

	return 0;
}

int CModel::RenderBoneMark( LPDIRECT3DDEVICE9 pdev, CModel* bmarkptr, CMySprite* bcircleptr, int selboneno )
{
	if( m_bonelist.empty() ){
		return 0;
	}

	map<int, CBone*>::iterator itrb;
	for( itrb = m_bonelist.begin(); itrb != m_bonelist.end(); itrb++ ){
		CBone* curbone = itrb->second;
		if( curbone ){
			curbone->m_selectflag = 0;
		}
	}

	if( selboneno > 0 ){
		CBone* selbone = m_bonelist[ selboneno ];
		if( selbone ){
			SetSelectFlagReq( selbone, 0 );
			selbone->m_selectflag = 2;
		}
	}


	pdev->SetRenderState( D3DRS_ZFUNC, D3DCMP_ALWAYS );

	if( bmarkptr ){
		map<int, CBone*>::iterator itrbone;
		for( itrbone = m_bonelist.begin(); itrbone != m_bonelist.end(); itrbone++ ){
			CBone* boneptr = itrbone->second;
			if( boneptr ){

				D3DXMATRIX bmmat;
				D3DXMatrixIdentity( &bmmat );
				CallF( boneptr->GetBoneMarkMatrix( &m_matWorld, &bmmat ), return 1 );

				g_pEffect->SetMatrix( g_hmWorld, &bmmat );
				bmarkptr->UpdateMatrix( -1, &bmmat, &m_matVP );
				D3DXVECTOR4 difmult;
				if( boneptr->m_selectflag == 2 ){
					difmult = D3DXVECTOR4( 1.0f, 0.0f, 0.0f, 0.5f );
				}else{
					difmult = D3DXVECTOR4( 0.25f, 0.5f, 0.5f, 0.5f );
				}
				CallF( bmarkptr->OnRender( pdev, 0, difmult ), return 1 );
			}
		}
	}

	if( bcircleptr ){
		map<int, CBone*>::iterator itrbone;
		for( itrbone = m_bonelist.begin(); itrbone != m_bonelist.end(); itrbone++ ){
			CBone* boneptr = itrbone->second;
			if( boneptr ){

				D3DXMATRIX bcmat;
				D3DXMatrixIdentity( &bcmat );
				CallF( boneptr->GetBoneCircleMatrix( &bcmat ), return 1 );

				D3DXMATRIX transmat = bcmat * m_matVP;
				D3DXVECTOR3 scpos;
				D3DXVec3TransformCoord( &scpos, &(boneptr->m_vertpos[BT_CHILD]), &transmat );
				scpos.z = 0.0f;
				bcircleptr->SetPos( scpos );
				D3DXVECTOR2 bsize( 0.025f, 0.025f );
				bcircleptr->SetSize( bsize );
				if( boneptr->m_selectflag >= 1 ){
					bcircleptr->SetColor( D3DXVECTOR4( 1.0f, 0.0f, 0.0f, 0.7f ) );
				}else{
					bcircleptr->SetColor( D3DXVECTOR4( 1.0f, 1.0f, 1.0f, 0.7f ) );
				}
				CallF( bcircleptr->OnRender(), return 1 );

			}
		}
	}

	pdev->SetRenderState( D3DRS_ZFUNC, D3DCMP_LESSEQUAL );

	return 0;
}

int CModel::FillTimeLine( OrgWinGUI::OWP_Timeline& timeline, OrgWinGUI::OWP_Timeline& Mtimeline, map<int, int>& lineno2boneno, map<int, int>& boneno2lineno, int* labelwidth )
{
	*labelwidth = 75;

	lineno2boneno.clear();
	boneno2lineno.clear();


	timeline.deleteLine();
	Mtimeline.deleteLine();

	if( m_mbaseobject.empty() ){
		WCHAR label[256];
		swprintf_s(label, 256, L"dummy%d", 0 );
		Mtimeline.newLine(label);
	}

	if( m_bonelist.empty() ){
		WCHAR label[256];
		swprintf_s(label, 256, L"dummy%d", 0 );
		timeline.newLine(label);
		return 0;
	}

	int labelbytemax = 0;
	int lineno = 0;
	FillTimelineReq( &labelbytemax, timeline, m_topbone, &lineno, lineno2boneno, boneno2lineno, 0 );
	timeline.setCurrentLineName( m_topbone->m_wbonename );

	int labelsize = 5 * labelbytemax;
	*labelwidth = max( 75, labelsize );


	if( m_mbaseobject.empty() ){
		return 0;
	}

	map<int,CMQOObject*>::iterator itrmbase;
	for( itrmbase = m_mbaseobject.begin(); itrmbase != m_mbaseobject.end(); itrmbase++ ){
		CMQOObject* curbase = itrmbase->second;
		_ASSERT( curbase );

		WCHAR wname[256] = {0L};
		MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, curbase->m_name, 256, wname, 256 );
		Mtimeline.newLine( wname );

		_ASSERT( m_curmotinfo );
		CMorphKey* curmk = curbase->m_morphkey[ m_curmotinfo->motid ];
		while( curmk ){
			Mtimeline.newKey( wname, curmk->m_frame, (void*)curmk );
			curmk = curmk->m_next;
		}
	}

	return 0;
}

void CModel::FillTimelineReq( int* bytemax, OrgWinGUI::OWP_Timeline& timeline, CBone* curbone, int* linenoptr, 
	map<int, int>& lineno2boneno, map<int, int>& boneno2lineno, int broflag )
{
	int curbyte = (int)strlen( curbone->m_bonename );
	if( *bytemax < curbyte ){
		*bytemax = curbyte;
	}

	//行を追加
	timeline.newLine( curbone->m_wbonename );

	lineno2boneno[ *linenoptr ] = curbone->m_boneno;
	boneno2lineno[ curbone->m_boneno ] = *linenoptr;
	(*linenoptr)++;

	_ASSERT( m_curmotinfo );
	CMotionPoint* curmp = curbone->m_motionkey[ m_curmotinfo->motid ];
	while( curmp ){
		timeline.newKey( curbone->m_wbonename, curmp->m_frame, (void*)curmp );

//if( !strcmp( curbone->m_bonename, "頭" ) ){
//	DbgOut( L"check !!! : name %s boneno %d : frame %f : eul (%f, %f, %f)\r\n", 
//		curbone->m_wbonename, curbone->m_boneno, 
//		curmp->m_frame, curmp->m_eul.x, curmp->m_eul.y, curmp->m_eul.z );
//}
		curmp = curmp->m_next;
	}

	if( curbone->m_child ){
		FillTimelineReq( bytemax, timeline, curbone->m_child, linenoptr, lineno2boneno, boneno2lineno, 1 );
	}
	if( broflag && curbone->m_brother ){
		FillTimelineReq( bytemax, timeline, curbone->m_brother, linenoptr, lineno2boneno, boneno2lineno, 1 );
	}
}

int CModel::AddMotion( char* srcname, WCHAR* wfilename, double srcleng, int* dstid )
{
	*dstid = -1;
	int leng = (int)strlen( srcname );

	int maxid = 0;
	map<int, MOTINFO*>::iterator itrmi;
	for( itrmi = m_motinfo.begin(); itrmi != m_motinfo.end(); itrmi++ ){
		MOTINFO* chkmi = itrmi->second;
		if( chkmi ){
			if( maxid < chkmi->motid ){
				maxid = chkmi->motid;
			}
		}
	}
	int newid = maxid + 1;


	MOTINFO* newmi = (MOTINFO*)malloc( sizeof( MOTINFO ) );
	if( !newmi ){
		_ASSERT( 0 );
		return 1;
	}
	ZeroMemory( newmi, sizeof( MOTINFO ) );

	strcpy_s( newmi->motname, 256, srcname );
	if( wfilename ){
		wcscpy_s( newmi->wfilename, MAX_PATH, wfilename );
	}else{
		ZeroMemory( newmi->wfilename, sizeof( WCHAR ) * MAX_PATH );
	}
	ZeroMemory( newmi->engmotname, sizeof( char ) * 256 );

	newmi->motid = newid;
	newmi->frameleng = srcleng;
	newmi->curframe = 0.0;
	newmi->speed = 1.0;
	newmi->loopflag = 0;

	m_motinfo[ newid ] = newmi;

///////////////
	map<int, CBone*>::iterator itrbone;
	for( itrbone = m_bonelist.begin(); itrbone != m_bonelist.end(); itrbone++ ){
		CBone* curbone = itrbone->second;
		if( curbone ){
			CallF( curbone->MakeFirstMP( newid ), return 1 );
		}
	}

	map<int, CMQOObject*>::iterator itrbase;
	for( itrbase = m_mbaseobject.begin(); itrbase != m_mbaseobject.end(); itrbase++ ){
		CMQOObject* curbase = itrbase->second;
		if( curbase ){
			CallF( curbase->MakeFirstMK( newid ), return 1 );
		}
	}


	*dstid = newid;

	return 0;
}

CMotionPoint* CModel::AddMotionPoint( int srcmotid, double srcframe, int srcboneno, int calcflag, int* existptr )
{
	if( srcboneno < 0 ){
		return 0;
	}

	CBone* curbone = m_bonelist[ srcboneno ];
	if( curbone ){
		return curbone->AddMotionPoint( srcmotid, srcframe, calcflag, existptr );
	}else{
		return 0;
	}

}

CMorphKey* CModel::AddMorphKey( int srcmotid, double srcframe, int srcbaseno, int* existptr )
{
	if( srcbaseno < 0 ){
		return 0;
	}

	CMQOObject* curbase = m_mbaseobject[ srcbaseno ];
	if( curbase ){
		return curbase->AddMorphKey( srcmotid, srcframe, existptr );
	}else{
		return 0;
	}
}


int CModel::SetCurrentMotion( int srcmotid )
{
	m_curmotinfo = m_motinfo[ srcmotid ];
	if( !m_curmotinfo ){
		_ASSERT( 0 );
		return 1;
	}else{
		return 0;
	}
}
int CModel::SetMotionFrame( double srcframe )
{
	if( !m_curmotinfo ){
		_ASSERT( 0 );
		return 1;
	}

	m_curmotinfo->curframe = max( 0.0, min( m_curmotinfo->frameleng, srcframe ) );

	return 0;
}
int CModel::GetMotionFrame( double* dstframe )
{
	if( !m_curmotinfo ){
		_ASSERT( 0 );
		return 1;
	}

	*dstframe = m_curmotinfo->curframe;

	return 0;
}
int CModel::SetMotionSpeed( double srcspeed )
{
	if( !m_curmotinfo ){
		_ASSERT( 0 );
		return 1;
	}

	m_curmotinfo->speed = srcspeed;

	return 0;
}
int CModel::GetMotionSpeed( double* dstspeed )
{
	if( !m_curmotinfo ){
		_ASSERT( 0 );
		return 1;
	}

	*dstspeed = m_curmotinfo->speed;
	return 0;
}

int CModel::DeleteMotion( int motid )
{
	map<int, CBone*>::iterator itrbone;
	for( itrbone = m_bonelist.begin(); itrbone != m_bonelist.end(); itrbone++ ){
		CBone* curbone = itrbone->second;
		if( curbone ){
			CallF( curbone->DeleteMotion( motid ), return 1 );
		}
	}

	map<int, CMQOObject*>::iterator itrmbase;
	for( itrmbase = m_mbaseobject.begin(); itrmbase != m_mbaseobject.end(); itrmbase++ ){
		CMQOObject* curbase = itrmbase->second;
		if( curbase ){
			CallF( curbase->DeleteMorphMotion( motid ), return 1 );
		}
	}


	map<int, MOTINFO*>::iterator itrmi;
	itrmi = m_motinfo.find( motid );
	if( itrmi != m_motinfo.end() ){
		MOTINFO* delmi = itrmi->second;
		if( delmi ){
			delete delmi;
		}
		m_motinfo.erase( itrmi );
	}

	int undono;
	for( undono = 0; undono < UNDOMAX; undono++ ){
		if( m_undomotion[undono].m_savemotinfo.motid == motid ){
			m_undomotion[undono].m_validflag = 0;
		}
	}

	return 0;
}

int CModel::GetSymBoneNo( int srcboneno, int* dstboneno, int* existptr )
{
	*existptr = 0;

	CBone* srcbone = m_bonelist[ srcboneno ];
	if( !srcbone ){
		*dstboneno = -1;
		return 0;
	}

	int findflag = 0;
	WCHAR findname[256];
	ZeroMemory( findname, sizeof( WCHAR ) * 256 );
	wcscpy_s( findname, 256, srcbone->m_wbonename );

	WCHAR* lpat = wcsstr( findname, L"[L]" );
	if( lpat ){
		*lpat = 0L;
		wcscat_s( findname, 256, L"[R]" );
		findflag = 1;		
	}else{
		WCHAR* rpat = wcsstr( findname, L"[R]" );
		if( rpat ){
			*rpat = 0;
			wcscat_s( findname, 256, L"[L]" );
			findflag = 1;
		}
	}

	if( findflag == 0 ){
		*dstboneno = srcboneno;
		*existptr = 0;
	}else{
		CBone* dstbone = 0;
		map<int, CBone*>::iterator itrbone;
		for( itrbone = m_bonelist.begin(); itrbone != m_bonelist.end(); itrbone++ ){
			CBone* chkbone = itrbone->second;
			if( chkbone && (wcscmp( findname, chkbone->m_wbonename ) == 0) ){
				dstbone = chkbone;
				break;
			}
		}
		if( dstbone ){
			*dstboneno = dstbone->m_boneno;
			*existptr = 1;
		}else{
			*dstboneno = srcboneno;
			*existptr = 0;
		}
	}

	return 0;
}

int CModel::PickBone( PICKINFO* pickinfo )
{
	pickinfo->pickobjno = -1;

	float fw, fh;
	fw = (float)pickinfo->winx / 2.0f;
	fh = (float)pickinfo->winy / 2.0f;

	int minno = -1;
	D3DXVECTOR3 cmpsc;
	D3DXVECTOR3 picksc = D3DXVECTOR3( 0.0f, 0.0f, 0.0f );
	D3DXVECTOR3 pickworld = D3DXVECTOR3( 0.0f, 0.0f, 0.0f );
	float cmpdist;
	float mindist = 0.0f;
	int firstflag = 1;


	map<int, CBone*>::iterator itrbone;
	for( itrbone = m_bonelist.begin(); itrbone != m_bonelist.end(); itrbone++ ){
		CBone* curbone = itrbone->second;
		if( curbone ){
			cmpsc.x = ( 1.0f + curbone->m_childscreen.x ) * fw;
			cmpsc.y = ( 1.0f - curbone->m_childscreen.y ) * fh;
			cmpsc.z = curbone->m_childscreen.z;

			if( (cmpsc.z >= 0.0f) && (cmpsc.z <= 1.0f) ){
				float mag;
				mag = ( (float)pickinfo->clickpos.x - cmpsc.x ) * ( (float)pickinfo->clickpos.x - cmpsc.x ) + 
					( (float)pickinfo->clickpos.y - cmpsc.y ) * ( (float)pickinfo->clickpos.y - cmpsc.y );
				if( mag != 0.0f ){
					cmpdist = sqrtf( mag );
				}else{
					cmpdist = 0.0f;
				}

				if( (firstflag || (cmpdist <= mindist)) && (cmpdist <= (float)pickinfo->pickrange ) ){
					minno = curbone->m_boneno;
					mindist = cmpdist;
					picksc = cmpsc;
					pickworld = curbone->m_childworld;
					firstflag = 0;
				}
			}
		}
	}

	pickinfo->pickobjno = minno;
	if( minno >= 0 ){
		pickinfo->objscreen = picksc;
		pickinfo->objworld = pickworld;
	}

	return 0;
}

void CModel::SetSelectFlagReq( CBone* boneptr, int broflag )
{
	boneptr->m_selectflag = 1;

	if( boneptr->m_child ){
		SetSelectFlagReq( boneptr->m_child, 1 );
	}
	if( boneptr->m_brother && broflag ){
		SetSelectFlagReq( boneptr->m_brother, 1 );
	}
}

int CModel::IKRotate( int srcboneno, D3DXVECTOR3 targetpos, int maxlevel )
{
	m_newmplist.erase( m_newmplist.begin(), m_newmplist.end() );

	CBone* firstbone = m_bonelist[ srcboneno ];
	if( !firstbone ){
		_ASSERT( 0 );
		return 1;
	}

	int calccnt;
	for( calccnt = 0; calccnt < 3; calccnt++ ){
		int levelcnt = 0;

		CBone* curbone = firstbone;
		while( curbone && ((maxlevel == 0) || (levelcnt < maxlevel)) && 
			( !curbone->m_faceptr || (curbone->m_faceptr && curbone->m_faceptr->m_bonetype == MIKOBONE_NORMAL) ) )
		{
			if( curbone->m_vertpos[BT_CHILD] != curbone->m_vertpos[BT_PARENT] ){
				D3DXVECTOR3 parworld, chilworld;
				chilworld = firstbone->m_childworld;
				D3DXVec3TransformCoord( &parworld, &(curbone->m_vertpos[BT_PARENT]), &(curbone->m_curmp.m_worldmat) );

				D3DXVECTOR3 parbef, chilbef, tarbef;
				CBone* parbone = curbone->m_parent;
				if( parbone ){
					D3DXMATRIX invmat;
					D3DXMatrixInverse( &invmat, NULL, &(parbone->m_curmp.m_worldmat) );
					D3DXVec3TransformCoord( &parbef, &parworld, &invmat );
					D3DXVec3TransformCoord( &chilbef, &chilworld, &invmat );
					D3DXVec3TransformCoord( &tarbef, &targetpos, &invmat );
				}else{
					parbef = parworld;
					chilbef = chilworld;
					tarbef = targetpos;
				}

				D3DXVECTOR3 vec0, vec1;
				vec0 = chilbef - parbef;
				D3DXVec3Normalize( &vec0, &vec0 );
				vec1 = tarbef - parbef;
				D3DXVec3Normalize( &vec1, &vec1 );

				D3DXVECTOR3 rotaxis;
				D3DXVec3Cross( &rotaxis, &vec0, &vec1 );
				D3DXVec3Normalize( &rotaxis, &rotaxis );

				float rotdot, rotrad;
				rotdot = DXVec3Dot( &vec0, &vec1 );
				rotdot = min( 1.0f, rotdot );
				rotdot = max( -1.0f, rotdot );
				rotrad = (float)acos( rotdot );
				if( rotrad > 1.0e-4 ){
					CQuaternion rotq;
					rotq.SetAxisAndRot( rotaxis, rotrad );
					CMotionPoint* newmp = 0;
					int existflag = 0;
					newmp = curbone->AddMotionPoint( m_curmotinfo->motid, m_curmotinfo->curframe, 1, &existflag );
					CQuaternion q0 = newmp->m_q;
					D3DXVECTOR3 eul0 = newmp->m_eul;

					if( existflag == 0 ){
						NEWMPELEM newmpe;
						newmpe.boneptr = curbone;
						newmpe.mpptr = newmp;
						m_newmplist.push_back( newmpe );
					}

					D3DXVECTOR3 firstdiff = firstbone->m_childworld - targetpos;
					float firstdist = DXVec3Length( &firstdiff );

					curbone->MultQ( m_curmotinfo->motid, m_curmotinfo->curframe, rotq );
					UpdateMatrix( curbone->m_boneno, &m_matWorld, &m_matVP );

					D3DXVECTOR3 aftdiff = firstbone->m_childworld - targetpos;
					float aftdist = DXVec3Length( &aftdiff );
					//if( aftdist >= firstdist ){
					//	newmp->m_q = q0;
					//	newmp->m_eul = eul0;
					//	UpdateMatrix( curbone->m_boneno, &m_matWorld, &m_matVP );
					//}
				}
			}

			curbone = curbone->m_parent;
			levelcnt++;
		}
	}

	return 0;
}

int CModel::CollisionNoBoneObj_Mouse( PICKINFO* pickinfo, char* objnameptr )
{
	//当たったら１、当たらなかったら０を返す。エラーも０を返す。

	CMQOObject* curobj = m_objectname[ objnameptr ];
	if( !curobj ){
		_ASSERT( 0 );
		return 0;
	}

	D3DXVECTOR3 startlocal, dirlocal;
	CalcMouseLocalRay( pickinfo, &startlocal, &dirlocal );

	int colli = 0;
	if( curobj->m_dispflag ){
		colli = curobj->CollisionLocal_Ray( startlocal, dirlocal );
	}

	return colli;
}

int CModel::CalcMouseLocalRay( PICKINFO* pickinfo, D3DXVECTOR3* startptr, D3DXVECTOR3* dirptr )
{
	D3DXVECTOR3 startsc, endsc;
	float rayx, rayy;
	rayx = (float)pickinfo->clickpos.x / ((float)pickinfo->winx / 2.0f) - 1.0f;
	rayy = 1.0f - (float)pickinfo->clickpos.y / ((float)pickinfo->winy / 2.0f);

	startsc = D3DXVECTOR3( rayx, rayy, 0.0f );
	endsc = D3DXVECTOR3( rayx, rayy, 1.0f );
	
    D3DXMATRIX mWVP, invmWVP;
	mWVP = m_matWorld * m_matVP;
	D3DXMatrixInverse( &invmWVP, NULL, &mWVP );

	D3DXVECTOR3 startlocal, endlocal;

	D3DXVec3TransformCoord( &startlocal, &startsc, &invmWVP );
	D3DXVec3TransformCoord( &endlocal, &endsc, &invmWVP );

	D3DXVECTOR3 dirlocal = endlocal - startlocal;
	DXVec3Normalize( &dirlocal, &dirlocal );

	*startptr = startlocal;
	*dirptr = dirlocal;

	return 0;
}

CBone* CModel::GetCalcRootBone( CBone* firstbone, int maxlevel )
{
	int levelcnt = 0;
	CBone* retbone = firstbone;
	CBone* curbone = firstbone;
	while( curbone && ((maxlevel == 0) || (levelcnt <= maxlevel)) )
	{
		retbone = curbone;
		if( curbone->m_faceptr && (curbone->m_faceptr && curbone->m_faceptr->m_bonetype != MIKOBONE_NORMAL) ){
			break;
		}
		curbone = curbone->m_parent;
		levelcnt++;
	}

	return retbone;
}


int CModel::IKRotateAxisDelta( int axiskind, int srcboneno, float delta, int maxlevel )
{
	m_newmplist.erase( m_newmplist.begin(), m_newmplist.end() );

	int levelcnt = 0;
	CBone* firstbone = m_bonelist[ srcboneno ];
	if( !firstbone ){
		_ASSERT( 0 );
		return 1;
	}
	D3DXVECTOR3 firstpos = firstbone->m_childworld;

	CBone* calcrootbone = GetCalcRootBone( firstbone, maxlevel );
	_ASSERT( calcrootbone );
	D3DXVECTOR3 rootpos = calcrootbone->m_childworld;

	D3DXVECTOR3 axis0, rotaxis;
	D3DXVECTOR3 targetpos;
	D3DXMATRIX selectmat;
	D3DXMATRIX mat, befrotmat, rotmat, aftrotmat;

	selectmat = firstbone->m_axisq.MakeRotMatX() * firstbone->m_curmp.m_worldmat; 
	selectmat._41 = 0.0f;
	selectmat._42 = 0.0f;
	selectmat._43 = 0.0f;

	float rotrad;
	if( axiskind == PICK_X ){
		axis0 = D3DXVECTOR3( 1.0f, 0.0f, 0.0f );
		D3DXVec3TransformCoord( &rotaxis, &axis0, &selectmat );
		D3DXVec3Normalize( &rotaxis, &rotaxis );
		rotrad = delta / 10.0f * (float)PAI / 12.0f;
	}else if( axiskind == PICK_Y ){
		axis0 = D3DXVECTOR3( 0.0f, 1.0f, 0.0f );
		D3DXVec3TransformCoord( &rotaxis, &axis0, &selectmat );
		D3DXVec3Normalize( &rotaxis, &rotaxis );
		rotrad = delta / 10.0f * (float)PAI / 12.0f;
	}else if( axiskind == PICK_Z ){
		axis0 = D3DXVECTOR3( 0.0f, 0.0f, 1.0f );
		D3DXVec3TransformCoord( &rotaxis, &axis0, &selectmat );
		D3DXVec3Normalize( &rotaxis, &rotaxis );
		rotrad = delta / 400.0f * (float)PAI / 12.0f;
	}else{
		_ASSERT( 0 );
		return 1;
	}

	if( fabs(rotrad) < (0.05f * (float)DEG2PAI) ){
		return 0;
	}


	if( (firstbone == calcrootbone) && !firstbone->m_parent ){
		//一番親のジョイントの回転
		CQuaternion multq;
		multq.SetAxisAndRot( rotaxis, rotrad );
		int existflag = 0;
		CMotionPoint* newmp = 0;
		newmp = firstbone->AddMotionPoint( m_curmotinfo->motid, m_curmotinfo->curframe, 1, &existflag );
		if( !newmp ){
			_ASSERT( 0 );
			return 1;
		}

		firstbone->MultQ( m_curmotinfo->motid, m_curmotinfo->curframe, multq );
		UpdateMatrix( firstbone->m_boneno, &m_matWorld, &m_matVP );

		if( existflag == 0 ){
			NEWMPELEM newmpe;
			newmpe.boneptr = firstbone;
			newmpe.mpptr = newmp;
			m_newmplist.push_back( newmpe );
		}

		return 0;//!!!!!!!!!!!!!!!!!
	}

	D3DXMatrixTranslation( &befrotmat, -rootpos.x, -rootpos.y, -rootpos.z );
	D3DXMatrixTranslation( &aftrotmat, rootpos.x, rootpos.y, rootpos.z );
	D3DXMatrixRotationAxis( &rotmat, &rotaxis, rotrad );
	mat = befrotmat * rotmat * aftrotmat;
	D3DXVec3TransformCoord( &targetpos, &firstpos, &mat );

	CBone* curbone = firstbone;
	while( curbone && ((maxlevel == 0) || (levelcnt < maxlevel)) && 
		( !curbone->m_faceptr || (curbone->m_faceptr && curbone->m_faceptr->m_bonetype == MIKOBONE_NORMAL) ) )
	{
		if( curbone->m_vertpos[BT_CHILD] != curbone->m_vertpos[BT_PARENT] ){
			D3DXVECTOR3 parworld, chilworld;
			chilworld = firstbone->m_childworld;
			D3DXVec3TransformCoord( &parworld, &(curbone->m_vertpos[BT_PARENT]), &(curbone->m_curmp.m_worldmat) );

			D3DXVECTOR3 chilaxis, taraxis;
			CalcShadowToPlane( chilworld, rotaxis, parworld, &chilaxis );
			CalcShadowToPlane( targetpos, rotaxis, parworld, &taraxis );

			D3DXVECTOR3 parbef, chilbef, tarbef;
			CBone* parbone = curbone->m_parent;
			if( parbone ){
				D3DXMATRIX invmat;
				D3DXMatrixInverse( &invmat, NULL, &(parbone->m_curmp.m_worldmat) );
				D3DXVec3TransformCoord( &parbef, &parworld, &invmat );
				D3DXVec3TransformCoord( &chilbef, &chilaxis, &invmat );
				D3DXVec3TransformCoord( &tarbef, &taraxis, &invmat );
			}else{
				parbef = parworld;
				chilbef = chilaxis;
				tarbef = taraxis;
			}

			D3DXVECTOR3 vec0, vec1;
			vec0 = chilbef - parbef;
			D3DXVec3Normalize( &vec0, &vec0 );
			vec1 = tarbef - parbef;
			D3DXVec3Normalize( &vec1, &vec1 );

			D3DXVECTOR3 rotaxisF;
			D3DXVec3Cross( &rotaxisF, &vec0, &vec1 );
			D3DXVec3Normalize( &rotaxisF, &rotaxisF );

			float rotdot, rotrad;
			rotdot = DXVec3Dot( &vec0, &vec1 );
			rotdot = min( 1.0f, rotdot );
			rotdot = max( -1.0f, rotdot );
			rotrad = (float)acos( rotdot );
			if( rotrad > 1.0e-4 ){
				CQuaternion rotq;
				rotq.SetAxisAndRot( rotaxisF, rotrad );
				CMotionPoint* newmp = 0;
				int existflag = 0;
				newmp = curbone->AddMotionPoint( m_curmotinfo->motid, m_curmotinfo->curframe, 1, &existflag );
				CQuaternion q0 = newmp->m_q;
				D3DXVECTOR3 eul0 = newmp->m_eul;

				if( existflag == 0 ){
					NEWMPELEM newmpe;
					newmpe.boneptr = curbone;
					newmpe.mpptr = newmp;
					m_newmplist.push_back( newmpe );
				}


				D3DXVECTOR3 firstdiff = firstbone->m_childworld - targetpos;
				float firstdist = DXVec3Length( &firstdiff );

				curbone->MultQ( m_curmotinfo->motid, m_curmotinfo->curframe, rotq );
				UpdateMatrix( curbone->m_boneno, &m_matWorld, &m_matVP );

				D3DXVECTOR3 aftdiff = firstbone->m_childworld - targetpos;
				float aftdist = DXVec3Length( &aftdiff );
				if( (axiskind == PICK_Z) && (aftdist >= firstdist) ){
					newmp->m_q = q0;
					newmp->m_eul = eul0;
					UpdateMatrix( curbone->m_boneno, &m_matWorld, &m_matVP );
				}
			}
		}

		curbone = curbone->m_parent;
		levelcnt++;
	}

	return 0;
}

int CModel::IKMove( int srcboneno, D3DXVECTOR3 targetworld )
{
	m_newmplist.erase( m_newmplist.begin(), m_newmplist.end() );

	CBone* curbone = m_bonelist[ srcboneno ];
	if( !curbone ){
		_ASSERT( 0 );
		return 1;
	}
	D3DXVECTOR3 orgobj;
	orgobj = curbone->m_vertpos[BT_CHILD];

	//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	//!!!! この関数では、ボーン変形を考慮した、ローカル座標で計算を行う。
	//!!!! matWorldが初期状態でない場合は、targetを、invmatworldで、逆変換してから渡す。
	//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

	D3DXMATRIX invworld;
	D3DXMatrixInverse( &invworld, NULL, &m_matWorld );
	D3DXVECTOR3 target;
	D3DXVec3TransformCoord( &target, &targetworld, &invworld );


	D3DXMATRIX raw1, par1;
	raw1 = curbone->m_curmp.m_mat;

	CBone* parbone = curbone->m_parent;
	if( parbone ){
		par1 = parbone->m_curmp.m_totalmat;
	}else{
		D3DXMatrixIdentity( &par1 );
	}

///////
	// org * raw1 * mvm * par1 = target

	float alpha, beta, ganma;
	alpha = target.x;
	beta = target.y;
	ganma = target.z;

////////
	float a11, a12, a13, a21, a22, a23, a31, a32, a33, a41, a42, a43;
	a11 = raw1._11;
	a12 = raw1._12;
	a13 = raw1._13;

	a21 = raw1._21;
	a22 = raw1._22;
	a23 = raw1._23;

	a31 = raw1._31;
	a32 = raw1._32;
	a33 = raw1._33;

	a41 = raw1._41;
	a42 = raw1._42;
	a43 = raw1._43;

	float b11, b12, b13, b21, b22, b23, b31, b32, b33, b41, b42, b43;
	b11 = par1._11;
	b12 = par1._12;
	b13 = par1._13;

	b21 = par1._21;
	b22 = par1._22;
	b23 = par1._23;

	b31 = par1._31;
	b32 = par1._32;
	b33 = par1._33;

	b41 = par1._41;
	b42 = par1._42;
	b43 = par1._43;

	float c11, c12, c13, c21, c22, c23, c31, c32, c33, c41, c42, c43;
	c11 = a11*b11 + a12*b21 + a13*b31;
	c12 = a11*b12 + a12*b22 + a13*b32;
	c13 = a11*b13 + a12*b23 + a13*b33;

	c21 = a21*b11 + a22*b21 + a23*b31;
	c22 = a21*b12 + a22*b22 + a23*b32;
	c23 = a21*b13 + a22*b23 + a23*b33;

	c31 = a31*b11 + a32*b21 + a33*b31;
	c32 = a31*b12 + a32*b22 + a33*b32;
	c33 = a31*b13 + a32*b23 + a33*b33;

	c41 = b11*a41 + b21*a42 + b31*a43 + b41;
	c42 = b12*a41 + b22*a42 + b32*a43 + b42;
	c43 = b13*a41 + b23*a42 + b33*a43 + b43;

	//zero div check
	if( b33 == 0.0f )
		return 0;
	float B33 = 1.0f / b33;

	float p, q, r, s, t, v, w;
	float x, y, z;

	x = orgobj.x;
	y = orgobj.y;
	z = orgobj.z;


	//zero div check
	if( (b11 - b31 * B33 * b13) == 0.0f )
		return 0;

	p = B33 * ( ganma - c13 * x - c23 * y - c33 * z - c43 );
	q = 1.0f / (b11 - b31 * B33 * b13);
	r = q * (alpha - c11 * x - c21 * y - c31 * z - c41 - b31 * p);

	s = b12 - b32 * B33 * b13;
	t = beta - c12 * x - c22 * y - c32 * z - c42 - b32 * p;

	v = b21 - b31 * B33 * b23;
	w = b22 - b32 * B33 * b23;

	if( (w - s * q * v) == 0.0f ){
		return 0;
	}

	float mvx, mvy, mvz;
	mvy = ( t -  s * r ) / ( w - s * q * v );
	mvx = -q * ( b21 - b31 * B33 * b23 ) * mvy + r;
	mvz = p - B33 * b13 * mvx - B33 * b23 * mvy;

	CMotionPoint* newmp = 0;
	int existflag = 0;
	newmp = curbone->AddMotionPoint( m_curmotinfo->motid, m_curmotinfo->curframe, 1, &existflag );
	if( existflag == 0 ){
		NEWMPELEM newmpe;
		newmpe.boneptr = curbone;
		newmpe.mpptr = newmp;
		m_newmplist.push_back( newmpe );
	}

	curbone->AddTra( m_curmotinfo->motid, m_curmotinfo->curframe, mvx, mvy, mvz );
	UpdateMatrix( curbone->m_boneno, &m_matWorld, &m_matVP );
	
	return 0;
}

int CModel::TransformBone( int winx, int winy, int srcboneno, D3DXVECTOR3* worldptr, D3DXVECTOR3* screenptr, D3DXVECTOR3* dispptr )
{					
	CBone* curbone;
	curbone = m_bonelist[ srcboneno ];
	*worldptr = curbone->m_childworld;
	D3DXMATRIX mWVP = curbone->m_curmp.m_worldmat * m_matVP;
	D3DXVec3TransformCoord( screenptr, &curbone->m_vertpos[BT_CHILD], &mWVP );

	float fw, fh;
	fw = (float)winx / 2.0f;
	fh = (float)winy / 2.0f;
	dispptr->x = ( 1.0f + screenptr->x ) * fw;
	dispptr->y = ( 1.0f - screenptr->y ) * fh;
	dispptr->z = screenptr->z;

	return 0;
}

int CModel::IKMoveAxisDelta( int axiskind, int srcboneno, float delta )
{
	m_newmplist.erase( m_newmplist.begin(), m_newmplist.end() );

	CBone* curbone = m_bonelist[ srcboneno ];
	if( !curbone ){
		_ASSERT( 0 );
		return 1;
	}
	D3DXVECTOR3 curpos = curbone->m_childworld;

	D3DXMATRIX selectmat;
	selectmat = curbone->m_axisq.MakeRotMatX() * curbone->m_curmp.m_worldmat; 
	selectmat._41 = 0.0f;
	selectmat._42 = 0.0f;
	selectmat._43 = 0.0f;

	D3DXVECTOR3 mvdelta;
	D3DXVECTOR3 axis0, curaxis;
	if( axiskind == PICK_X ){
		axis0 = D3DXVECTOR3( 1.0f, 0.0f, 0.0f );
	}else if( axiskind == PICK_Y ){
		axis0 = D3DXVECTOR3( 0.0f, 1.0f, 0.0f );
	}else if( axiskind == PICK_Z ){
		axis0 = D3DXVECTOR3( 0.0f, 0.0f, 1.0f );
	}else{
		_ASSERT( 0 );
		return 1;
	}
	D3DXVec3TransformCoord( &curaxis, &axis0, &selectmat );
	D3DXVec3Normalize( &curaxis, &curaxis );
	//mvdelta = delta / 8.0f * curaxis;
	mvdelta = delta / 32.0f * curaxis;

	D3DXVECTOR3 newtarget = curpos + mvdelta;

	IKMove( srcboneno, newtarget );

	return 0;
}

int CModel::ChangeMotFrameLeng( int motid, double srcleng )
{
	MOTINFO* dstmi = m_motinfo[ motid ];
	if( dstmi ){
		double befleng = dstmi->frameleng;

		dstmi->frameleng = srcleng;

		if( befleng > srcleng ){
			map<int, CBone*>::iterator itrbone;
			for( itrbone = m_bonelist.begin(); itrbone != m_bonelist.end(); itrbone++ ){
				CBone* curbone = itrbone->second;
				if( curbone ){
					curbone->DeleteMPOutOfRange( motid, srcleng - 1.0 );
				}
			}

			map<int, CMQOObject*>::iterator itrmbase;
			for( itrmbase = m_mbaseobject.begin(); itrmbase != m_mbaseobject.end(); itrmbase++ ){
				CMQOObject* curbase = itrmbase->second;
				if( curbase ){
					CallF( curbase->DeleteMKOutOfRange( motid, srcleng - 1.0 ), return 1 );
				}
			}

		}
	}
	return 0;
}

int CModel::AdvanceTime( int previewflag, double difftime, double* nextframeptr, int* endflagptr )
{
	*endflagptr = 0;

	double curspeed, curframe;
	curspeed = m_curmotinfo->speed;
	curframe = m_curmotinfo->curframe;

	double nextframe;
	double oneframe = 1.0 / 60.0;

	if( previewflag > 0 ){
		nextframe = curframe + difftime / oneframe * curspeed;
		if( nextframe > ( m_curmotinfo->frameleng - 1.0 ) ){
			if( m_curmotinfo->loopflag == 0 ){
				nextframe = m_curmotinfo->frameleng - 1.0;
				*endflagptr = 1;
			}else{
				nextframe = 0.0;
			}
		}
	}else{
		nextframe = curframe - difftime / oneframe * curspeed;
		if( nextframe < 0.0 ){
			if( m_curmotinfo->loopflag == 0 ){
				nextframe = 0.0;
				*endflagptr = 1;
			}else{
				nextframe = m_curmotinfo->frameleng - 1.0;
			}
		}
	}

	*nextframeptr = nextframe;

	return 0;
}

int CModel::SaveQubFile( WCHAR* savepath, int savemotid, char* mmotname )
{
	MOTINFO* savemotinfo = m_motinfo[ savemotid ];
	if( !savemotinfo ){
		_ASSERT( 0 );
		return 1;
	}

	CQubFile qubfile;
	CallF( qubfile.WriteQubFile( savepath, savemotinfo, m_topbone, m_bonelist.size(), m_mbaseobject, mmotname ), return 1 );

	return 0;
}

int CModel::LoadQubFileFromPnd( CPmCipherDll* cipher, int qubindex, int* newmotid )
{
	*newmotid = -1;

	CQubFile qubfile;
	CallF( qubfile.LoadQubFileFromPnd( cipher, qubindex, this, newmotid ), return 1 );

	return 0;
}


int CModel::LoadQubFile( WCHAR* loadpath, int* newmotid )
{
	*newmotid = -1;

	CQubFile qubfile;
	CallF( qubfile.LoadQubFile( loadpath, this, newmotid ), return 1 );

	return 0;
}

int CModel::MakeEnglishName()
{
	map<int, CMQOObject*>::iterator itrobj;
	for( itrobj = m_object.begin(); itrobj != m_object.end(); itrobj++ ){
		CMQOObject* curobj = itrobj->second;
		if( curobj ){
			CallF( ConvEngName( ENGNAME_DISP, curobj->m_name, 256, curobj->m_engname, 256 ), return 1 );
			map<int, CMQOObject*>::iterator itrtarget;
			for( itrtarget = curobj->m_morphobj.begin(); itrtarget != curobj->m_morphobj.end(); itrtarget++ ){
				CMQOObject* curtarget = itrtarget->second;
				CallF( ConvEngName( ENGNAME_DISP, curtarget->m_name, 256, curtarget->m_engname, 256 ), return 1 );			
			}
		}
	}


	map<int, CBone*>::iterator itrbone;
	for( itrbone = m_bonelist.begin(); itrbone != m_bonelist.end(); itrbone++ ){
		CBone* curbone = itrbone->second;
		if( curbone ){
			CallF( ConvEngName( ENGNAME_BONE, curbone->m_bonename, 256, curbone->m_engbonename, 256 ), return 1 );
		}
	}

	map<int, MOTINFO*>::iterator itrmi;
	for( itrmi = m_motinfo.begin(); itrmi != m_motinfo.end(); itrmi++ ){
		MOTINFO* curmi = itrmi->second;
		if( curmi ){
			CallF( ConvEngName( ENGNAME_MOTION, curmi->motname, 256, curmi->engmotname, 256 ), return 1 );
		}
	}

	return 0;
}

int CModel::CalcXTransformMatrix( float mult )
{
	if( m_topbone ){
		D3DXMATRIX inimat;
		D3DXMatrixIdentity( &inimat );
		CalcXTransformMatrixReq( m_topbone, inimat, mult );
	}

	return 0;
}

void CModel::CalcXTransformMatrixReq( CBone* srcbone, D3DXMATRIX parenttra, float mult )
{
	CallF( srcbone->CalcXOffsetMatrix( mult ), return );
	D3DXMATRIX offsetmat;
	offsetmat = srcbone->m_xoffsetmat;

	D3DXMATRIX offsetinv;
	D3DXMatrixInverse( &offsetinv, NULL, &offsetmat );

	D3DXMATRIX parinv;
	D3DXMatrixInverse( &parinv, NULL, &parenttra );

	srcbone->m_xtransmat = offsetinv * parinv;
	srcbone->m_xconbinedtransmat = srcbone->m_xtransmat * parenttra;

	if( srcbone->m_brother ){
		CalcXTransformMatrixReq( srcbone->m_brother, parenttra, mult );
	}

	if( srcbone->m_child ){
		CalcXTransformMatrixReq( srcbone->m_child, srcbone->m_xconbinedtransmat, mult );
	}
}

int CModel::AddDefMaterial()
{

	CMQOMaterial* dummymat = new CMQOMaterial();
	if( !dummymat ){
		_ASSERT( 0 );
		return 1;
	}

	int defmaterialno = m_material.size();
	dummymat->materialno = defmaterialno;
	strcpy_s( dummymat->name, 256, "dummyMaterial" );

	m_material[defmaterialno] = dummymat;

	return 0;
}

int CModel::SetMiko3rdVec( int boneno, D3DXVECTOR3 srcvec )
{
	CBone* curbone = m_bonelist[boneno];
	if( !curbone ){
		_ASSERT( 0 );
		return 1;
	}
	curbone->SetMiko3rdVec( srcvec );

	return 0;
}

int CModel::SetKinectFirstPose()
{
	map<int, CBone*>::iterator itrbone;
	for( itrbone = m_bonelist.begin(); itrbone != m_bonelist.end(); itrbone++ ){
		CBone* curbone = itrbone->second;
		_ASSERT( curbone );
		CMotionPoint calcmp;
		int existflag = 0;
		curbone->CalcMotionPoint( m_curmotinfo->motid, m_curmotinfo->curframe, &calcmp, &existflag );
		curbone->m_kinectq = calcmp.m_q;
	}

	return 0;
}

int CModel::SetMorphBaseMap()
{
	m_mbaseobject.clear();

	map<int, CMQOObject*>::iterator itrobj;
	for( itrobj = m_object.begin(); itrobj != m_object.end(); itrobj++ ){
		CMQOObject* curobj = itrobj->second;
		if( curobj ){
			if( !curobj->m_morphobj.empty() ){
				m_mbaseobject[ curobj->m_objectno ] = curobj;
			}
		}
	}

	return 0;
}

int CModel::UpdateMorph( int srcmotid, double srcframe )
{
	int hasbone = 0;
	if( m_bonelist.empty() ){
		hasbone = 0;
	}else{
		hasbone = 1;
	}

	map<int, CMQOObject*>::iterator itrmbase;
	for( itrmbase = m_mbaseobject.begin(); itrmbase != m_mbaseobject.end(); itrmbase++ ){
		CMQOObject* mbaseobj = itrmbase->second;
		if( mbaseobj ){
			CallF( mbaseobj->UpdateMorph( m_pdev, hasbone, srcmotid, srcframe, m_material ), return 1 );
		}
	}
	return 0;
}

int CModel::SetDispFlag( char* srcobjname, int srcflag )
{
	CMQOObject* curobj = m_objectname[ srcobjname ];
	if( curobj ){
		curobj->m_dispflag = srcflag;
	}

	return 0;
}

int CModel::InitUndoMotion( int saveflag )
{
	int undono;
	for( undono = 0; undono < UNDOMAX; undono++ ){
		m_undomotion[ undono ].ClearData();
	}

	m_undoid = 0;

	if( saveflag ){
		m_undomotion[0].SaveUndoMotion( this, -1, -1 );
	}


	return 0;
}

int CModel::SaveUndoMotion( int curboneno, int curbaseno )
{
	if( m_bonelist.empty() || !m_curmotinfo ){
		return 0;
	}

	int nextundoid;
	nextundoid = m_undoid + 1;
	if( nextundoid >= UNDOMAX ){
		nextundoid = 0;
	}
	m_undoid = nextundoid;

	CallF( m_undomotion[m_undoid].SaveUndoMotion( this, curboneno, curbaseno ), return 1 );

	return 0;
}
int CModel::RollBackUndoMotion( int redoflag, int* curboneno, int* curbaseno )
{
	if( m_bonelist.empty() || !m_curmotinfo ){
		return 0;
	}

	int rbundoid = -1;
	if( redoflag == 0 ){
		GetValidUndoID( &rbundoid );
	}else{
		GetValidRedoID( &rbundoid );
	}

	if( rbundoid >= 0 ){
		m_undomotion[ rbundoid ].RollBackMotion( this, curboneno, curbaseno );
		m_undoid = rbundoid;
	}

	return 0;
}
int CModel::GetValidUndoID( int* rbundoid )
{
	int retid = -1;

	int chkcnt;
	int curid = m_undoid;
	for( chkcnt = 0; chkcnt < UNDOMAX; chkcnt++ ){
		curid = curid - 1;
		if( curid < 0 ){
			curid = UNDOMAX - 1;
		}

		if( m_undomotion[curid].m_validflag == 1 ){
			retid = curid;
			break;
		}
	}
	*rbundoid = retid;
	return 0;
}
int CModel::GetValidRedoID( int* rbundoid )
{
	int retid = -1;

	int chkcnt;
	int curid = m_undoid;
	for( chkcnt = 0; chkcnt < UNDOMAX; chkcnt++ ){
		curid = curid + 1;
		if( curid >= UNDOMAX ){
			curid = 0;
		}

		if( m_undomotion[curid].m_validflag == 1 ){
			retid = curid;
			break;
		}
	}
	*rbundoid = retid;

	return 0;
}


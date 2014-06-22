#include <stdio.h>
#include <stdarg.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <malloc.h>
#include <memory.h>

#include <windows.h>
#include <crtdbg.h>

#include <Model.h>
#include <mqomaterial.h>
#include <mqoobject.h>

#define DBGH
#include <dbg.h>

#include <Bone.h>
#include <MQOFace.h>

#include <InfScope.h>
#include <MotionPoint.h>

#include <VecMath.h>

static int s_bonecnt = 0;

CBone::CBone() : m_curmp(), m_axisq()
{
	InitParams();
	m_boneno = s_bonecnt;
	s_bonecnt++;
}

CBone::~CBone()
{
	DestroyObjs();
}

int CBone::InitParams()
{
	m_boneno = 0;
	m_topboneflag = 0;
	ZeroMemory( m_bonename, sizeof( char ) * 256 );
	ZeroMemory( m_wbonename, sizeof( WCHAR ) * 256 );
	ZeroMemory( m_engbonename, sizeof( char ) * 256 );
	ZeroMemory( m_vertpos, sizeof( D3DXVECTOR3 ) * BT_MAX );
	m_faceptr = 0;

	m_childworld = D3DXVECTOR3( 0.0f, 0.0f, 0.0f );
	m_childscreen = D3DXVECTOR3( 0.0f, 0.0f, 0.0f );

	m_bonematerialno = -1;

	m_parent = 0;
	m_child = 0;
	m_brother = 0;

	m_isnum = 0;
	ZeroMemory( m_isarray, sizeof( CInfScope* ) * INFSCOPEMAX );

	m_selectflag = 0;

	D3DXMatrixIdentity( &m_xoffsetmat );
	D3DXMatrixIdentity( &m_xtransmat );
	D3DXMatrixIdentity( &m_xconbinedtransmat );

	m_3rdvecflag = 0;
	m_3rdvec = D3DXVECTOR3( 0.0f, 0.0f, 1.0f );

	m_kinectq.SetParams( 1.0f, 0.0f, 0.0f, 0.0f );

	return 0;
}
int CBone::DestroyObjs()
{
	map<int, CMotionPoint*>::iterator itrmp;
	for( itrmp = m_motionkey.begin(); itrmp != m_motionkey.end(); itrmp++ ){
		CMotionPoint* topkey = itrmp->second;
		if( topkey ){
			CMotionPoint* curmp = topkey;
			CMotionPoint* nextmp = 0;
			while( curmp ){
				nextmp = curmp->m_next;

				delete curmp;

				curmp = nextmp;
			}
		}
	}

	m_motionkey.erase( m_motionkey.begin(), m_motionkey.end() );

	return 0;
}

int CBone::ResetBoneCnt()
{
	s_bonecnt = 0;
	return 0;
}

int CBone::SetBoneTri( CMQOFace* srcface, D3DXVECTOR3* srcpoint )
{
	if( m_topboneflag == 0 ){
		sprintf_s( m_bonename, 256, srcface->m_bonename );

		if( srcface->m_bonetype == MIKOBONE_NORMAL ){
			*( m_vertpos + BT_PARENT ) = *( srcpoint + srcface->m_parentindex );
			*( m_vertpos + BT_CHILD ) = *( srcpoint + srcface->m_childindex );
			*( m_vertpos + BT_3RD ) = *( srcpoint + srcface->m_hindex );
		}else if( srcface->m_bonetype == MIKOBONE_FLOAT ){
			*( m_vertpos + BT_PARENT ) = *( srcpoint + srcface->m_parentindex );
			*( m_vertpos + BT_CHILD ) = *( srcpoint + srcface->m_childindex );
			*( m_vertpos + BT_3RD ) = D3DXVECTOR3( 0.0f, 0.0f, 0.0f );
		}else{
			ZeroMemory( m_vertpos, sizeof( D3DXVECTOR3 ) * BT_MAX );
			_ASSERT( 0 );
			return 1;
		}

		m_bonematerialno = srcface->m_materialno;

	}else if( m_topboneflag == 1 ){
		strcpy_s( m_bonename, 256, "ModelRootBone" );

		int bt;
		for( bt = 0; bt < BT_MAX; bt++ ){
			*( m_vertpos + bt ) = D3DXVECTOR3( 0.0f, 0.0f, 0.0f );
		}

		m_bonematerialno = -1;

	}else if( m_topboneflag == 2 ){
		sprintf_s( m_bonename, 256, "TOP_%s", srcface->m_bonename );

		*( m_vertpos + BT_PARENT ) = D3DXVECTOR3( 0.0f, 0.0f, 0.0f );
		*( m_vertpos + BT_CHILD ) = *( srcpoint + srcface->m_parentindex );
		*( m_vertpos + BT_3RD ) = D3DXVECTOR3( 0.0f, 0.0f, 1.0f );

		m_bonematerialno = -1;
	}
	m_faceptr = srcface;

	MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, m_bonename, 256, m_wbonename, 256 );

	CalcAxisQ();

	return 0;
}

int CBone::AddChild( CBone* childptr )
{
	childptr->m_parent = this;
	if( !m_child ){
		m_child = childptr;
	}else{
		CBone* broptr = m_child;
		while( broptr->m_brother ){
			broptr = broptr->m_brother;
		}
		broptr->m_brother = childptr;
	}

	return 0;
}

int CBone::AddInfScope( CInfScope* pinfscope )
{
	if( m_isnum >= INFSCOPEMAX ){
		_ASSERT( 0 );
		return 1;
	}

	m_isarray[ m_isnum ] = pinfscope;
	m_isnum++;

	return 0;
}

int CBone::CalcTargetCenter()
{
	int isno;
	for( isno = 0; isno < m_isnum; isno++ ){
		int setisno[INFSCOPEMAX];
		int setnum = 0;
		ZeroMemory( setisno, sizeof( int ) * INFSCOPEMAX );


		CInfScope* curis = m_isarray[ isno ];
		_ASSERT( curis );
		if( curis->m_settminmax != 0 ){
			continue;
		}

		setisno[ 0 ] = isno;
		setnum = 1;

		CMQOObject* curtarget = curis->m_targetobj;
		D3DXVECTOR3 tmpmin, tmpmax;
		tmpmin = curis->m_minv;
		tmpmax = curis->m_maxv;

		int isno2;
		for( isno2 = 0; isno2 < m_isnum; isno2++ ){
			CInfScope* calcis = m_isarray[ isno2 ];
			if( (isno2 != isno) && (calcis->m_settminmax == 0) && (calcis->m_targetobj == curtarget) ){

				if( setnum < INFSCOPEMAX ){
					setisno[setnum] = isno2;
					setnum++;
				}else{
					_ASSERT( 0 );
				}

				if( tmpmin.x > calcis->m_minv.x ){
					tmpmin.x = calcis->m_minv.x;
				}
				if( tmpmin.y > calcis->m_minv.y ){
					tmpmin.y = calcis->m_minv.y;
				}
				if( tmpmin.z > calcis->m_minv.z ){
					tmpmin.z = calcis->m_minv.z;
				}
				if( tmpmax.x < calcis->m_maxv.x ){
					tmpmax.x = calcis->m_maxv.x;
				}
				if( tmpmax.y < calcis->m_maxv.y ){
					tmpmax.y = calcis->m_maxv.y;
				}
				if( tmpmax.z < calcis->m_maxv.z ){
					tmpmax.z = calcis->m_maxv.z;
				}
			}
		}

		D3DXVECTOR3 tmpcenter, tmpdiff;
		float tmpmaxdist;
		tmpcenter = ( tmpmin + tmpmax ) * 0.5f;
		tmpdiff = tmpmin - tmpcenter;
		tmpmaxdist = D3DXVec3Length( &tmpdiff );

		int sno;
		for( sno = 0; sno < setnum; sno++ ){
			int setindex = setisno[ sno ];
			CInfScope* setis = m_isarray[ setindex ];
			setis->m_tminv = tmpmin;
			setis->m_tmaxv = tmpmax;
			setis->m_tcenter = tmpcenter;
			setis->m_tmaxdist = tmpmaxdist;
			setis->m_settminmax = 1;
		}
	}

	return 0;
}

int CBone::UpdateMatrix( int srcmotid, double srcframe, D3DXMATRIX* wmat, D3DXMATRIX* vpmat, D3DXMATRIX* parmat, CQuaternion* parq )
{
	int existflag = 0;
	if( srcframe >= 0.0 ){
		CallF( CalcMotionPoint( srcmotid, srcframe, &m_curmp, &existflag ), return 1 );
		CallF( m_curmp.UpdateMatrix( wmat, parmat, parq, this ), return 1 );
	}else{
		m_curmp.InitMotion();
		CallF( m_curmp.UpdateMatrix( wmat, parmat, parq, this ), return 1 );
	}

	D3DXVec3TransformCoord( &m_childworld, &(m_vertpos[BT_CHILD]), &m_curmp.m_worldmat );
	D3DXVec3TransformCoord( &m_childscreen, &m_childworld, vpmat );

	return 0;
}

int CBone::GetBoneMarkMatrix( D3DXMATRIX* srcworld, D3DXMATRIX* bmmatptr )
{
	D3DXVECTOR3 aftpar, aftchil;
	if( m_parent ){
		D3DXVec3TransformCoord( &aftpar, &(m_vertpos[BT_PARENT]), &( m_parent->m_curmp.m_worldmat ) );
	}else{
		D3DXVec3TransformCoord( &aftpar, &(m_vertpos[BT_PARENT]), srcworld );
	}
	D3DXVec3TransformCoord( &aftchil, &(m_vertpos[BT_CHILD]), &(m_curmp.m_worldmat) );

	D3DXVECTOR3 diffvec = aftchil - aftpar;
	float leng = DXVec3Length( &diffvec );
	if( (leng <= 0.000001f) || (m_vertpos[BT_PARENT] == m_vertpos[BT_CHILD]) ){
		ZeroMemory( bmmatptr, sizeof( D3DXMATRIX ) );
		return 0;
	}


	D3DXVECTOR3 basevec( 0.0f, 0.0f, 1.0f ); 
	D3DXVECTOR3 ndiffvec = diffvec / leng;
	CQuaternion rotq;
	rotq.RotationArc( basevec, ndiffvec );
	D3DXMATRIX rotmat = rotq.MakeRotMatX();


	D3DXMATRIX scalem;
	float scale;
	scale = leng / 50.0f;
	D3DXMatrixScaling( &scalem, scale, scale, scale );

	*bmmatptr = scalem * rotmat;

	bmmatptr->_41 += aftpar.x;
	bmmatptr->_42 += aftpar.y;
	bmmatptr->_43 += aftpar.z;

	return 0;
}

int CBone::GetBoneCircleMatrix( D3DXMATRIX* bcmatptr )
{
	*bcmatptr = m_curmp.m_worldmat;

	return 0;
}

int CBone::MakeFirstMP( int motid )
{
	if( m_motionkey[motid] != NULL ){
		_ASSERT( 0 );
		return 1;
	}

	CMotionPoint* newmp = new CMotionPoint();
	if( !newmp ){
		_ASSERT( 0 );
		return 1;
	}

	m_motionkey[motid] = newmp;

	return 0;
}

CMotionPoint* CBone::AddMotionPoint( int srcmotid, double srcframe, int calcflag, int* existptr )
{
	*existptr = 0;

	CMotionPoint* newmp = 0;
	CMotionPoint* pbef = 0;
	CMotionPoint* pnext = 0;
	CallF( GetBefNextMP( srcmotid, srcframe, &pbef, &pnext, existptr ), return 0 );

	if( *existptr ){
		if( !calcflag ){
			pbef->InitMotion();
			pbef->m_frame = srcframe;
		}
		newmp = pbef;

	}else{
		newmp = new CMotionPoint();
		if( !newmp ){
			_ASSERT( 0 );
			return 0;
		}
		newmp->m_frame = srcframe;

		if( calcflag ){
			CallF( CalcFrameMP( srcframe, pbef, pnext, *existptr, newmp ), return 0 );
		}else{
			newmp->InitMotion();
			newmp->m_frame = srcframe;
		}
		if( pbef ){
			CallF( pbef->AddToNext( newmp ), return 0 );
		}else{
			m_motionkey[srcmotid] = newmp;
			if( pnext ){
				newmp->m_next = pnext;
			}
		}
	}

	return newmp;
}


int CBone::CalcMotionPoint( int srcmotid, double srcframe, CMotionPoint* dstmpptr, int* existptr )
{
	CMotionPoint* befptr = 0;
	CMotionPoint* nextptr = 0;
	CallF( GetBefNextMP( srcmotid, srcframe, &befptr, &nextptr, existptr ), return 1 );
	CallF( CalcFrameMP( srcframe, befptr, nextptr, *existptr, dstmpptr ), return 1 );

	return 0;
}

int CBone::GetBefNextMP( int srcmotid, double srcframe, CMotionPoint** ppbef, CMotionPoint** ppnext, int* existptr )
{
	*existptr = 0;

	CMotionPoint* pbef = 0;
	CMotionPoint* pcur = m_motionkey[srcmotid];

	while( pcur ){

		if( (pcur->m_frame >= srcframe - TIME_ERROR_WIDTH) && (pcur->m_frame <= srcframe + TIME_ERROR_WIDTH) ){
			*existptr = 1;
			pbef = pcur;
			break;
		}else if( pcur->m_frame > srcframe ){
			break;
		}else{
			pbef = pcur;
			pcur = pcur->m_next;
		}
	}
	*ppbef = pbef;

	if( *existptr ){
		*ppnext = pbef->m_next;
	}else{
		*ppnext = pcur;
	}

	return 0;
}

int CBone::CalcFrameMP( double srcframe, CMotionPoint* befptr, CMotionPoint* nextptr, int existflag, CMotionPoint* dstmpptr )
{

	if( existflag == 1 ){
		*dstmpptr = *befptr;
		return 0;
	}else if( !befptr ){
		dstmpptr->InitMotion();
		dstmpptr->m_frame = srcframe;
		return 0;
	}else if( !nextptr ){
		*dstmpptr = *befptr;
		dstmpptr->m_frame = srcframe;
		return 0;
	}else{
		double diffframe = nextptr->m_frame - befptr->m_frame;
		_ASSERT( diffframe != 0.0 );
		double t = ( srcframe - befptr->m_frame ) / diffframe;
		D3DXVECTOR3 calceul;
		calceul = befptr->m_eul + ( nextptr->m_eul - befptr->m_eul ) * (float)t;
		D3DXVECTOR3 calctra;
		calctra = befptr->m_tra + ( nextptr->m_tra - befptr->m_tra ) * (float)t;

		dstmpptr->InitMotion();
		dstmpptr->SetEul( &m_axisq, calceul );
		dstmpptr->m_tra = calctra;
		dstmpptr->m_frame = srcframe;

		dstmpptr->m_prev = befptr;
		dstmpptr->m_next = nextptr;
		return 0;
	}
}

int CBone::OrderMotionPoint( int srcmotid, double settime, CMotionPoint* putmp, int samedel )
{
//	putmp->LeaveFromChain( srcmotid, this );//<---呼び出し前に処理しておく。(複数いっぺんに処理する場合があるので外でやる。)
	CMotionPoint* pbef = 0;
	CMotionPoint* pnext = 0;
	int existflag = 0;
	GetBefNextMP( srcmotid, settime, &pbef, &pnext, &existflag );

	while( (samedel == 1) && (existflag == 1) ){
		pbef->LeaveFromChain( srcmotid, this );
		delete pbef;

		pbef = 0;
		pnext = 0;
		existflag = 0;
		GetBefNextMP( srcmotid, settime, &pbef, &pnext, &existflag );
	}

	putmp->m_frame = settime;

	if( pbef ){
		CallF( pbef->AddToNext( putmp ), return 0 );
	}else{
		m_motionkey[srcmotid] = putmp;
		if( pnext ){
			putmp->m_next = pnext;
		}
	}

	return 0;
}

int CBone::DeleteMotion( int srcmotid )
{
	map<int, CMotionPoint*>::iterator itrmp;
	itrmp = m_motionkey.find( srcmotid );
	if( itrmp != m_motionkey.end() ){
		CMotionPoint* topkey = itrmp->second;
		if( topkey ){
			CMotionPoint* curmp = topkey;
			CMotionPoint* nextmp = 0;
			while( curmp ){
				nextmp = curmp->m_next;

				delete curmp;

				curmp = nextmp;
			}
		}
	}

	m_motionkey.erase( itrmp );

	return 0;
}

int CBone::CalcAxisQ()
{
	if( !m_faceptr ){
		m_axisq.SetParams( 1.0f, 0.0f, 0.0f, 0.0f );
		return 0;
	}
	if( ((m_faceptr->m_bonetype != MIKOBONE_NORMAL) && (m_faceptr->m_bonetype != MIKOBONE_FLOAT)) || 
		(m_vertpos[BT_CHILD] == m_vertpos[BT_PARENT]) ){
		
		if( m_parent ){
			m_axisq = m_parent->m_axisq;
		}else{
			m_axisq.SetParams( 1.0f, 0.0f, 0.0f, 0.0f );
		}
		return 0;
	}

	D3DXVECTOR3 upvec;
	D3DXVECTOR3 vecx1, vecy1, vecz1;

	D3DXVECTOR3 bonevec;
	bonevec = m_vertpos[BT_CHILD] - m_vertpos[BT_PARENT];
	D3DXVec3Normalize( &bonevec, &bonevec );

	if( m_faceptr->m_bonetype == MIKOBONE_FLOAT ){
		if( (bonevec.x != 0.0f) || (bonevec.y != 0.0f) ){
			upvec.x = 0.0f;
			upvec.y = 0.0f;
			upvec.z = 1.0f;
		}else{
			upvec.x = 1.0f;
			upvec.y = 0.0f;
			upvec.z = 0.0f;
		}

		vecz1 = bonevec;
		
		D3DXVec3Cross( &vecx1, &upvec, &vecz1 );
		D3DXVec3Normalize( &vecx1, &vecx1 );

		D3DXVec3Cross( &vecy1, &vecz1, &vecx1 );
		D3DXVec3Normalize( &vecy1, &vecy1 );

	}else if( m_faceptr->m_bonetype == MIKOBONE_NORMAL ){
		vecz1 = bonevec;

		D3DXVECTOR3 hvec;
		hvec = m_vertpos[BT_3RD] - m_vertpos[BT_PARENT];
		
		if( m_3rdvecflag == 0 ){
			D3DXVec3Cross( &vecy1, &bonevec, &hvec );
		}else{
			D3DXVec3Cross( &vecy1, &bonevec, &m_3rdvec );
		}
		D3DXVec3Normalize( &vecy1, &vecy1 );

		D3DXVec3Cross( &vecx1, &vecy1, &vecz1 );
		D3DXVec3Normalize( &vecx1, &vecx1 );
	}else{
		_ASSERT( 0 );
		m_axisq.SetParams( 1.0f, 0.0f, 0.0f, 0.0f );
		return 1;
//		vecz1 = bonevec;
//
//		vecy1 = childjoint->m_mikobonepos;
//
//		D3DXVec3Cross( &vecx1, &vecy1, &vecz1 );
//		D3DXVec3Normalize( &vecx1, &vecx1 );
//
//		D3DXVec3Cross( &vecy1, &vecz1, &vecx1 );
//		D3DXVec3Normalize( &vecy1, &vecy1 );

}


	D3DXMATRIX tmpxmat;
	D3DXQUATERNION tmpxq;

	D3DXMatrixIdentity( &tmpxmat );
	tmpxmat._11 = vecx1.x;
	tmpxmat._12 = vecx1.y;
	tmpxmat._13 = vecx1.z;

	tmpxmat._21 = vecy1.x;
	tmpxmat._22 = vecy1.y;
	tmpxmat._23 = vecy1.z;

	tmpxmat._31 = vecz1.x;
	tmpxmat._32 = vecz1.y;
	tmpxmat._33 = vecz1.z;

	D3DXQuaternionRotationMatrix( &tmpxq, &tmpxmat );

	m_axisq.SetParams( tmpxq );

	return 0;
}

int CBone::MultQ( int srcmotid, double srcframe, CQuaternion rotq )
{
	int existflag = 0;
	int calcflag = 1;
	CMotionPoint* setmp = 0;
	setmp = AddMotionPoint( srcmotid, srcframe, calcflag, &existflag );
	if( !setmp ){
		_ASSERT( 0 );
		return 1;
	}

	CQuaternion newq;
	newq = rotq * setmp->m_q;

	newq.normalize();
	setmp->m_q.normalize();

	float dot;
	dot = newq.DotProduct( setmp->m_q );
	dot = max( -1.0f, dot );
	dot = min( 1.0f, dot );
	float rad;
	rad = (float)acos( dot );
	if( rad >= (float)PAI / 2.0f ){
		newq = -newq;
	}

	CallF( setmp->SetQ( &m_axisq, newq ), return 1 );

	return 0;
}

int CBone::AddTra( int srcmotid, double srcframe, float mvx, float mvy, float mvz )
{
	int existflag = 0;
	int calcflag = 1;
	CMotionPoint* setmp = 0;
	setmp = AddMotionPoint( srcmotid, srcframe, calcflag, &existflag );
	if( !setmp ){
		_ASSERT( 0 );
		return 1;
	}

	setmp->m_tra = setmp->m_tra + D3DXVECTOR3( mvx, mvy, mvz );

	return 0;
}

int CBone::DeleteMPOutOfRange( int motid, double srcleng )
{
	CMotionPoint* curmp = m_motionkey[ motid ];

	while( curmp ){
		CMotionPoint* nextmp = curmp->m_next;

		if( curmp->m_frame > srcleng ){
			curmp->LeaveFromChain( motid, this );
			delete curmp;
		}
		curmp = nextmp;
	}

	return 0;
}

int CBone::CalcXOffsetMatrix( float mult )
{

	D3DXMATRIX inimat;
	D3DXMatrixIdentity( &inimat );

	if( m_parent ){
		inimat._41 = m_parent->m_vertpos[BT_CHILD].x * mult;
		inimat._42 = m_parent->m_vertpos[BT_CHILD].y * mult;
		inimat._43 = m_parent->m_vertpos[BT_CHILD].z * mult;

		D3DXMATRIX invmat;
		D3DXMatrixInverse( &invmat, NULL, &inimat );

		m_xoffsetmat = invmat;
	}else{
		m_xoffsetmat = inimat;
	}
		
	return 0;
}

int CBone::GetKeyFrameXSRT( int motid, float* keyframearray, 
	D3DXQUATERNION* rotarray, D3DXVECTOR3* traarray, int arrayleng, int* getnum )
{
	int setno = 0;
	CMotionPoint* curmp = m_motionkey[ motid ];

	while( curmp ){
		if( keyframearray ){
			if( setno < arrayleng ){
				*( keyframearray + setno ) = (float)curmp->m_frame;
			}else{
				_ASSERT( 0 );
				return 1;
			}
		}

		if( rotarray ){
			if( setno < arrayleng ){
				CQuaternion invq;
				curmp->m_q.inv( &invq );
				invq.Q2X( rotarray + setno );
			}else{
				_ASSERT( 0 );
				return 1;
			}
		}

		if( traarray ){
			if( setno < arrayleng ){
				D3DXVECTOR3 parpos = D3DXVECTOR3( 0.0f, 0.0f, 0.0f );
				D3DXVECTOR3 gparpos = D3DXVECTOR3( 0.0f, 0.0f, 0.0f );
				if( m_parent ){
					parpos = m_parent->m_vertpos[BT_CHILD];
					if( m_parent->m_parent ){
						gparpos = m_parent->m_parent->m_vertpos[BT_CHILD];
					}
				}

				*( traarray + setno ) = curmp->m_tra + parpos - gparpos;
			}else{
				_ASSERT( 0 );
				return 1;
			}
		}

		setno++;
		curmp = curmp->m_next;
	}

	*getnum = setno;

	return 0;
}

int CBone::SetMiko3rdVec( D3DXVECTOR3 srcvec )
{
	m_3rdvecflag = 1;
	m_3rdvec = srcvec;

	CalcAxisQ();

	return 0;
}

int CBone::GetBoneNum()
{
	int retnum = 0;

	if( !m_child ){
		return 0;
	}else{
		retnum++;
	}

	CBone* cbro = m_child->m_brother;
	while( cbro ){
		retnum++;
		cbro = cbro->m_brother;
	}

	return retnum;
}

int CBone::DestroyMotionKey( int srcmotid )
{
	CMotionPoint* curmp = m_motionkey[ srcmotid ];
	while( curmp ){
		CMotionPoint* nextmp = curmp->m_next;
		delete curmp;
		curmp = nextmp;
	}

	m_motionkey[ srcmotid ] = NULL;

	return 0;
}


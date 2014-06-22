#include <windows.h>
#include <MotionPoint.h>
#include <Bone.h>

#include <math.h>
#include <crtdbg.h>


CMotionPoint::CMotionPoint()
{
	InitParams();
}
CMotionPoint::~CMotionPoint()
{
	DestroyObjs();
}

int CMotionPoint::InitParams()
{
	m_frame = 0.0;

	InitMotion();

	m_prev = 0;
	m_next = 0;

	return 0;
}
int CMotionPoint::InitMotion()
{
	m_eul = D3DXVECTOR3( 0.0f, 0.0f, 0.0f );
	m_q.SetParams( 1.0f, 0.0f, 0.0f, 0.0f );
	m_totalq.SetParams( 1.0f, 0.0f, 0.0f, 0.0f );
	m_tra = D3DXVECTOR3( 0.0f, 0.0f, 0.0f );
	D3DXMatrixIdentity( &m_mat );
	D3DXMatrixIdentity( &m_totalmat );
	D3DXMatrixIdentity( &m_worldmat );

	return 0;
}

int CMotionPoint::CopyMotion( CMotionPoint* srcmp )
{
	if( !srcmp ){
		_ASSERT( 0 );
		return 1;
	}

	DestroyObjs();

	m_frame = srcmp->m_frame;
	m_prev = 0;
	m_next = 0;

	m_eul = srcmp->m_eul;
	m_q = srcmp->m_q;
	m_totalq = srcmp->m_totalq;
	m_tra = srcmp->m_tra;
	m_mat = srcmp->m_mat;
	m_totalmat = srcmp->m_totalmat;
	m_worldmat = srcmp->m_worldmat;

	return 0;
}

int CMotionPoint::DestroyObjs()
{

	return 0;
}

int CMotionPoint::GetEul( D3DXVECTOR3* dsteul )
{
	*dsteul = m_eul;

	return 0;
}
int CMotionPoint::SetEul( CQuaternion* axisq, D3DXVECTOR3 srceul )
{
	m_eul = srceul;
	m_q.SetRotation( axisq, srceul );

	return 0;
}
int CMotionPoint::SetQ( CQuaternion* axisq, CQuaternion newq )
{
	m_q = newq;

	D3DXVECTOR3 befeul;
	if( m_prev ){
		befeul = m_prev->m_eul;
	}else{
		befeul = D3DXVECTOR3( 0.0f, 0.0f, 0.0f );
	}
	
	m_q.Q2Eul( axisq, befeul, &m_eul );

	return 0;
}


int CMotionPoint::MakeMat( CBone* srcbone )
{
	D3DXMATRIX befrotmat, aftrotmat, rotmat, tramat;
	D3DXMatrixIdentity( &befrotmat );
	D3DXMatrixIdentity( &aftrotmat );
	D3DXMatrixIdentity( &rotmat );
	D3DXMatrixIdentity( &tramat );

	befrotmat._41 = -srcbone->m_vertpos[ BT_PARENT ].x;
	befrotmat._42 = -srcbone->m_vertpos[ BT_PARENT ].y;
	befrotmat._43 = -srcbone->m_vertpos[ BT_PARENT ].z;

	aftrotmat._41 = srcbone->m_vertpos[ BT_PARENT ].x;
	aftrotmat._42 = srcbone->m_vertpos[ BT_PARENT ].y;
	aftrotmat._43 = srcbone->m_vertpos[ BT_PARENT ].z;

	rotmat = m_q.MakeRotMatX();

	tramat._41 = m_tra.x;
	tramat._42 = m_tra.y;
	tramat._43 = m_tra.z;

	m_mat = befrotmat * rotmat * aftrotmat * tramat;

	return 0;
}
int CMotionPoint::MakeTotalMat( D3DXMATRIX* parmat, CQuaternion* parq, CBone* srcbone )
{

	MakeMat( srcbone );

	m_totalmat = m_mat * *parmat;
	m_totalq = *parq * m_q;

	return 0;
}

int CMotionPoint::MakeWorldMat( D3DXMATRIX* wmat )
{
	m_worldmat = m_totalmat * *wmat;
	

	D3DXQUATERNION tmpxq;
	D3DXQuaternionRotationMatrix( &tmpxq, wmat );
	CQuaternion wq;
	wq.SetParams( tmpxq );
	m_worldq = wq * m_totalq;

	return 0;
}

int CMotionPoint::UpdateMatrix( D3DXMATRIX* wmat, D3DXMATRIX* parmat, CQuaternion* parq, CBone* srcbone )
{
	MakeTotalMat( parmat, parq, srcbone );
	MakeWorldMat( wmat );
	MakeDispMat();
	return 0;
}

int CMotionPoint::AddToPrev( CMotionPoint* addmp )
{
	CMotionPoint *saveprev, *savenext;
	saveprev = m_prev;
	savenext = m_next;

	addmp->m_prev = m_prev;
	addmp->m_next = this;

	m_prev = addmp;

	if( saveprev ){
		saveprev->m_next = addmp;
	}

	return 0;
}

int CMotionPoint::AddToNext( CMotionPoint* addmp )
{
	CMotionPoint *saveprev, *savenext;
	saveprev = m_prev;
	savenext = m_next;

	addmp->m_prev = this;
	addmp->m_next = savenext;

	m_next = addmp;

	if( savenext ){
		savenext->m_prev = addmp;
	}

	return 0;
}

int CMotionPoint::LeaveFromChain( int srcmotid, CBone* boneptr )
{
	CMotionPoint *saveprev, *savenext;
	saveprev = m_prev;
	savenext = m_next;

	m_prev = 0;
	m_next = 0;

	if( saveprev ){
		saveprev->m_next = savenext;
	}

	if( savenext ){
		savenext->m_prev = saveprev;
	}

	if( (srcmotid >= 0) && boneptr && !saveprev ){
		boneptr->m_motionkey[srcmotid] = savenext;
	}

	return 0;
}

int CMotionPoint::MakeDispMat()
{
	m_disptra.x = m_worldmat._41;
	m_disptra.y = m_worldmat._42;
	m_disptra.z = m_worldmat._43;

	D3DXMATRIX tmpwmat = m_worldmat;
	tmpwmat._41 = 0.0f;
	tmpwmat._42 = 0.0f;
	tmpwmat._43 = 0.0f;

	D3DXQUATERNION xtmpq;
	D3DXQuaternionRotationMatrix( &xtmpq, &tmpwmat );

	m_dispq.SetParams( xtmpq );

	m_dispmat = m_dispq.MakeRotMatX();
	m_dispmat._41 = m_disptra.x;
	m_dispmat._42 = m_disptra.y;
	m_dispmat._43 = m_disptra.z;

	return 0;
}
/***
int CMotionPoint::CalcFBXEul( CQuaternion* srcq, D3DXVECTOR3 befeul, D3DXVECTOR3* reteul )
{
	D3DXMATRIX rightmat;
	rightmat = srcq->MakeRotMatX();

	rightmat._31 *= -1;
	rightmat._32 *= -1;
	rightmat._34 *= -1;
	rightmat._13 *= -1;
	rightmat._23 *= -1;
	rightmat._43 *= -1;

	D3DXQUATERNION rqx;
	D3DXQuaternionRotationMatrix( &rqx, &rightmat );

	CQuaternion rq;
	rq.SetParams( rqx );
	rq.Q2Eul2( 0, befeul, reteul );

	return 0;
}
***/

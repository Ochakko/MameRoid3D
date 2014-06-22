#ifndef MOTIONPOINTH
#define MOTIONPOINTH

#include <coef.h>
#include <quaternion.h>

class CBone;

class CMotionPoint
{
public:
	CMotionPoint();
	~CMotionPoint();

	int InitMotion();

	int GetEul( D3DXVECTOR3* dsteul );
	int SetEul( CQuaternion* axisq, D3DXVECTOR3 srceul );
	int SetQ( CQuaternion* axisq, CQuaternion newq );

	int UpdateMatrix( D3DXMATRIX* wmat, D3DXMATRIX* parmat, CQuaternion* parq, CBone* srcbone );

	int AddToPrev( CMotionPoint* addmp );
	int AddToNext( CMotionPoint* addmp );
	int LeaveFromChain( int srcmotid = -1, CBone* boneptr = 0 );

	int CopyMotion( CMotionPoint* srcmp );

private:
	int InitParams();
	int DestroyObjs();

	int MakeMat( CBone* srcbone );
	int MakeTotalMat( D3DXMATRIX* parmat, CQuaternion* parq, CBone* srcbone );
	int MakeWorldMat( D3DXMATRIX* wmat );
	int MakeDispMat();

public:
	double m_frame;
	D3DXVECTOR3 m_eul;
	D3DXVECTOR3 m_tra;

	CQuaternion m_q;
	CQuaternion m_totalq;//�e�̉e�����󂯂Ă����]���
	CQuaternion m_worldq;//���[���h�Ɛe�̉e�����󂯂Ă����]���

	D3DXMATRIX m_mat;//�e�̉e�����󂯂Ă��Ȃ��}�g���b�N�X
	D3DXMATRIX m_totalmat;//�e�̉e�����󂯂Ă���}�g���b�N�X
	D3DXMATRIX m_worldmat;//���[���h�ϊ��Ɛe�̉e�����󂯂��}�g���b�N�X

	CQuaternion m_dispq;
	D3DXVECTOR3 m_disptra;
	D3DXMATRIX m_dispmat;


	CMotionPoint* m_prev;
	CMotionPoint* m_next;
};

#endif
#ifndef FBXBONE0H
#define FBXBONE0H

#include <coef.h>

#include <D3DX9.h>
#include <crtdbg.h>
#include <fbxsdk.h>
#include <quaternion.h>

class CBone;

class CFBXBone
{
public:
	CFBXBone();
	~CFBXBone();

	int AddChild( CFBXBone* childptr );

private:
	int InitParams();
	int DestroyObjs();

public:
	int type;
	CBone* bone;
	KFbxNode* skelnode;
	int bunkinum;

	CQuaternion axisq;

	CFBXBone* m_parent;
	CFBXBone* m_child;
	CFBXBone* m_brother;
	int m_boneinfcnt;
};


#endif
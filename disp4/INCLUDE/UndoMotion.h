#ifndef UNDOMOTIONH
#define UNDOMOTIONH

#include <d3dx9.h>
#include <wchar.h>
#include <Coef.h>
#include <string>
#include <map>

using namespace std;

class CModel;
class CBone;
class CMotionPoint;
class CMQOObject;
class CMorphKey;

class CUndoMotion
{
public:
	CUndoMotion();
	~CUndoMotion();

	int ClearData();
	int SaveUndoMotion( CModel* pmodel, int curboneno, int curbaseno );
	int RollBackMotion( CModel* pmodel, int* curboneno, int* curbaseno );

private:
	int InitParams();
	int DestroyObjs();

public:
	int m_validflag;
	MOTINFO m_savemotinfo;
	map<CBone*, CMotionPoint*> m_bone2mp;
	map<CMQOObject*, CMorphKey*> m_base2mk;
	int m_curboneno;
	int m_curbaseno;
};


#endif
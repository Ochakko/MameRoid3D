#ifndef MORPHKEYH
#define MORPHKEYH

#include <coef.h>
#include <d3dx9.h>

#include <string>
#include <map>
using namespace std;

class CMQOObject;

class CMorphKey
{
public:
	CMorphKey( CMQOObject* baseptr );
	~CMorphKey();

	int InitMotion();

	int SetBlendWeight( CMQOObject* targetobj, float srcweight );
	int SetBlendAllZero();
	int SetBlendOneExclude( CMQOObject* targetobj );
	int DeleteBlendWeight( CMQOObject* targetobj );

	int NormalizeWeight();

	int AddToPrev( CMorphKey* addmp );
	int AddToNext( CMorphKey* addmp );
	int LeaveFromChain( int srcmotid, CMQOObject* ownerobj );

	int CopyMotion( CMorphKey* srcmk );

private:
	int InitParams();
	int DestroyObjs();

public:
	double m_frame;
	CMQOObject* m_baseobj;	
	map<CMQOObject*, float> m_blendweight;

	CMorphKey* m_prev;
	CMorphKey* m_next;
};

#endif
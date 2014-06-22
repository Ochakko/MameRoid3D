#ifndef INFBONEH
#define INFBONEH

#include <coef.h>

class CInfBone
{
public:
	CInfBone();
	~CInfBone();

	int ExistBone( int srcboneno );
	int AddInfElem( INFELEM srcie );
	int NormalizeInf();

private:
	int InitParams();
	int DestroyObjs();

public:
	int m_infnum;
	INFELEM m_infelem[INFNUMMAX];

};

#endif


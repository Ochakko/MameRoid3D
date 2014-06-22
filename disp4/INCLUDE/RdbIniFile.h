#ifndef RDBINIFILEH
#define RDBINIFILEH

#include <coef.h>
#include <XMLIO.h>

class CRdbIniFile : public CXMLIO
{
public:
	CRdbIniFile();
	virtual ~CRdbIniFile();

	int WriteRdbIniFile( WINPOS* srcwinpos );
	int LoadRdbIniFile();
private:
	virtual int InitParams();
	virtual int DestroyObjs();

	int ReadWinPos( XMLIOBUF* xmliobuf, int kind, WINPOS* dstwinpos );
public:
	WINPOS m_winpos[ WINPOS_MAX ];
	WINPOS m_defaultwp[ WINPOS_MAX ];
};

#endif
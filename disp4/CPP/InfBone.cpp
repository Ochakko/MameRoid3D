#include <stdio.h>
#include <stdarg.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <malloc.h>
#include <memory.h>

#include <windows.h>
#include <crtdbg.h>

#include <InfBone.h>

#include <polymesh3.h>

#include <GetMaterial.h>

#include <mqoface.h>

#define DBGH
#include <dbg.h>

#include <InfScope.h>


CInfBone::CInfBone()
{
	InitParams();
}

CInfBone::~CInfBone()
{
	DestroyObjs();
}

int CInfBone::InitParams()
{
	m_infnum = 0;

	int infno;
	for( infno = 0; infno < INFNUMMAX; infno++ ){
		INFELEM* curie = m_infelem + infno;

		curie->boneno = -1;
		curie->kind = CALCMODE_NONE;
		curie->userrate = 1.0f;
		curie->orginf = 0.0f;
		curie->dispinf = 0.0f;
	}

	return 0;
}

int CInfBone::DestroyObjs()
{
	return 0;
}

int CInfBone::ExistBone( int srcboneno )
{
	int existno = -1;

	int ieno;
	for( ieno = 0; ieno < m_infnum; ieno++ ){
		if( m_infelem[ ieno ].boneno == srcboneno ){
			existno = ieno;
			break;
		}
	}

	return existno;
}

int CInfBone::AddInfElem( INFELEM srcie )
{
	if( m_infnum >= INFNUMMAX ){
		_ASSERT( 0 );
		return 0;
	}

	m_infelem[ m_infnum ] = srcie;
	m_infnum++;


	return 0;
}
int CInfBone::NormalizeInf()
{
	if( m_infnum <= 0 ){
//		_ASSERT( 0 );
		return 0;
	}

	float leng = 0.0f;
	int ieno;
	for( ieno = 0; ieno < m_infnum; ieno++ ){
		leng += m_infelem[ ieno ].orginf;
	}

	if( leng != 0.0f ){
		for( ieno = 0; ieno < m_infnum; ieno++ ){
			m_infelem[ ieno ].dispinf = m_infelem[ ieno ].orginf / leng;
		}
	}

	return 0;
}
#include <stdio.h>
#include <stdarg.h>

#include <windows.h>

#include <math.h>
#include <crtdbg.h>

#define DBGH
#include <dbg.h>

#include <MorphKey.h>
#include <MQOObject.h>


CMorphKey::CMorphKey( CMQOObject* baseptr )
{
	InitParams();
	m_baseobj = baseptr;
}
CMorphKey::~CMorphKey()
{
	DestroyObjs();
}

int CMorphKey::InitParams()
{
	m_frame = 0;
	m_baseobj = 0;
	m_blendweight.clear();

	m_prev = 0;
	m_next = 0;
	return 0;
}

int CMorphKey::CopyMotion( CMorphKey* srcmk )
{
	if( !srcmk ){
		_ASSERT( 0 );
		return 1;
	}

	DestroyObjs();

	m_frame = srcmk->m_frame;
	m_prev = 0;
	m_next = 0;

	m_baseobj = srcmk->m_baseobj;
	m_blendweight = srcmk->m_blendweight;

	return 0;
}

int CMorphKey::InitMotion()
{
	CallF( SetBlendAllZero(), return 1 );

	return 0;
}


int CMorphKey::DestroyObjs()
{

	m_blendweight.clear();

	return 0;
}


int CMorphKey::AddToPrev( CMorphKey* addmk )
{
	CMorphKey *saveprev, *savenext;
	saveprev = m_prev;
	savenext = m_next;

	addmk->m_prev = m_prev;
	addmk->m_next = this;

	m_prev = addmk;

	if( saveprev ){
		saveprev->m_next = addmk;
	}

	return 0;
}

int CMorphKey::AddToNext( CMorphKey* addmk )
{
	CMorphKey *saveprev, *savenext;
	saveprev = m_prev;
	savenext = m_next;

	addmk->m_prev = this;
	addmk->m_next = savenext;

	m_next = addmk;

	if( savenext ){
		savenext->m_prev = addmk;
	}

	return 0;
}

int CMorphKey::LeaveFromChain( int srcmotid, CMQOObject* ownerobj )
{
	CMorphKey *saveprev, *savenext;
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

	if( (srcmotid >= 0) && ownerobj && !saveprev ){
		ownerobj->m_morphkey[srcmotid] = savenext;
	}

	return 0;
}

int CMorphKey::SetBlendWeight( CMQOObject* targetobj, float srcweight )
{
	m_blendweight[ targetobj ] = srcweight;

	return 0;
}
int CMorphKey::SetBlendAllZero()
{
	map<CMQOObject*, float>::iterator itrbld;
	for( itrbld = m_blendweight.begin(); itrbld != m_blendweight.end(); itrbld++ ){
		itrbld->second = 0.0f;
	}


	return 0;
}
int CMorphKey::SetBlendOneExclude( CMQOObject* targetobj )
{
	map<CMQOObject*, float>::iterator itrbld;
	for( itrbld = m_blendweight.begin(); itrbld != m_blendweight.end(); itrbld++ ){
		itrbld->second = 0.0f;
	}

	m_blendweight[ targetobj ] = 1.0f;

	return 0;
}

int CMorphKey::NormalizeWeight()
{
	float totalw = 0.0f;

	map<CMQOObject*, float>::iterator itrbld;
	for( itrbld = m_blendweight.begin(); itrbld != m_blendweight.end(); itrbld++ ){
		totalw += itrbld->second;
	}

	if( totalw == 0.0f ){
		return 0;
	}

	for( itrbld = m_blendweight.begin(); itrbld != m_blendweight.end(); itrbld++ ){
		itrbld->second /= totalw;
	}

	return 0;
}

int CMorphKey::DeleteBlendWeight( CMQOObject* targetobj )
{
	map<CMQOObject*, float>::iterator itrfind;
	itrfind = m_blendweight.find( targetobj );

	if( itrfind != m_blendweight.end() ){
		m_blendweight.erase( itrfind );
	}

	return 0;
}


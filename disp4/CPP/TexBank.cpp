#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <malloc.h>
#include <memory.h>
#include <math.h>
#include <search.h>

#include <windows.h>

#define	DBGH
#include <dbg.h>
#include <crtdbg.h>


#include <TexBank.h>
#include <TexElem.h>

#include <algorithm>
#include <iostream>
#include <iterator>

#include <PmCipherDll.h>

extern WCHAR g_basedir[ MAX_PATH ];

CTexBank::CTexBank( LPDIRECT3DDEVICE9 pdev )
{
	InitParams();
	m_pdev = pdev;
}

CTexBank::~CTexBank()
{
	DestroyObjs();
}

int CTexBank::InitParams()
{
	m_pdev = 0;
	return 0;
}
int CTexBank::DestroyObjs()
{
	Invalidate();

	map<int,CTexElem*>::iterator itr;
	for( itr = m_texmap.begin(); itr != m_texmap.end(); itr++ ){
		CTexElem* delte = itr->second;
		if( delte ){
			delete delte;
		}
	}

	m_texmap.erase( m_texmap.begin(), m_texmap.end() );

	return 0;
}

CTexElem* CTexBank::ExistTex( WCHAR* srcpath, WCHAR* srcname, int srctransparent )
{
	map<int,CTexElem*>::iterator finditr = m_texmap.end();

	map<int,CTexElem*>::iterator itr;
	for( itr = m_texmap.begin(); itr != m_texmap.end(); itr++ ){
		CTexElem* curelem = itr->second;
		if( curelem ){
			int cmpname, cmppath;
			cmpname = wcscmp( srcname, curelem->m_name );
			if( cmpname == 0 ){
				cmppath = wcscmp( srcpath, curelem->m_path );
				if( cmppath == 0 ){
					if( srctransparent == curelem->m_transparent ){
						return curelem;
					}
				}
			}
		}
	}

	return 0;
}

int CTexBank::AddTex( CPmCipherDll* cipher, WCHAR* srcpath, WCHAR* srcname, int srctransparent, int srcpool, D3DCOLOR srccol, int* dstid )
{
	WCHAR wpath[MAX_PATH] = {0L};
	swprintf_s( wpath, MAX_PATH, L"%s\\%s", srcpath, srcname );
	char mpath[MAX_PATH] = {0};
	WideCharToMultiByte( CP_ACP, 0, wpath, -1, mpath, MAX_PATH, NULL, NULL );

	int propnum;
	propnum = cipher->GetPropertyNum();
	if( propnum <= 0 ){
		_ASSERT( 0 );
		return 1;
	}

	int findindex = -1;
	int findsize = 0;
	int propno;
	for( propno = 0; propno < propnum; propno++ ){
		PNDPROP prop;
		ZeroMemory( &prop, sizeof( PNDPROP ) );

		CallF( cipher->GetProperty( propno, &prop ), return 1 );
		int cmp;
		cmp = strcmp( prop.path, mpath );
		if( cmp == 0 ){
			findindex = propno;
			findsize = prop.sourcesize;
			break;
		}
	}
	if( (findindex < 0) || (findsize <= 0) ){
		_ASSERT( 0 );
		return 0;//!!!!!!!!!!
	}

	CTexElem* sameelem = 0;
	sameelem = ExistTex( srcpath, srcname, srctransparent );
	if( sameelem ){
		*dstid = sameelem->m_id;
		return 2;
	}

	CTexElem* newelem;
	newelem = new CTexElem();
	if( !newelem ){
		_ASSERT( 0 );
		return 1;
	}
	wcscpy_s( newelem->m_name, MAX_PATH, srcname );
	wcscpy_s( newelem->m_path, MAX_PATH, srcpath );
	newelem->m_transparent = srctransparent;
	newelem->m_pool = srcpool;
	newelem->m_transcol = srccol;

	char* newbuf = (char*)malloc( sizeof( char ) * findsize );
	if( !newbuf ){
		_ASSERT( 0 );
		return 1;
	}
	ZeroMemory( newbuf, sizeof( char ) * findsize );

	int getsize = 0;
	CallF( cipher->Decrypt( mpath, (unsigned char*)newbuf, findsize, &getsize ), return 1 );

	if( getsize != findsize ){
		_ASSERT( 0 );
		return 1;
	}

	newelem->m_buf = newbuf;
	newelem->m_bufsize = findsize;

	CallF( newelem->CreateTexData( m_pdev ), return 1 );

	m_texmap[ newelem->m_id ] = newelem;
	*dstid = newelem->m_id;

	return 0;
}


int CTexBank::AddTex( WCHAR* srcpath, WCHAR* srcname, int srctransparent, int srcpool, D3DCOLOR srccol, int* dstid )
{
	CTexElem* sameelem = 0;
	sameelem = ExistTex( srcpath, srcname, srctransparent );
	if( sameelem ){
		*dstid = sameelem->m_id;
		return 2;
	}

	CTexElem* newelem;
	newelem = new CTexElem();
	if( !newelem ){
		_ASSERT( 0 );
		return 1;
	}
	wcscpy_s( newelem->m_name, MAX_PATH, srcname );
	wcscpy_s( newelem->m_path, MAX_PATH, srcpath );
	newelem->m_transparent = srctransparent;
	newelem->m_pool = srcpool;
	newelem->m_transcol = srccol;

	CallF( newelem->CreateTexData( m_pdev ), return 1 );

	m_texmap[ newelem->m_id ] = newelem;
	*dstid = newelem->m_id;

	return 0;
}

int CTexBank::Invalidate()
{
	map<int,CTexElem*>::iterator itr;
	for( itr = m_texmap.begin(); itr != m_texmap.end(); itr++ ){
		CTexElem* delelem = itr->second;
		if( delelem ){
			delelem->InvalidateTexData();
		}
	}

	return 0;
}
int CTexBank::Restore()
{
	map<int,CTexElem*>::iterator itr;
	for( itr = m_texmap.begin(); itr != m_texmap.end(); itr++ ){
		CTexElem* telem = itr->second;
		if( telem ){
			CallF( telem->CreateTexData( m_pdev ), return 1 );
		}
	}

	return 0;
}
int CTexBank::DestroyTex( int srcid )
{
	map<int,CTexElem*>::iterator finditr;
	finditr = m_texmap.find( srcid );
	if( finditr == m_texmap.end() ){
		return 0;
	}

	if( finditr->second ){
		delete finditr->second;
	}
	m_texmap.erase( finditr );

	return 0;
}


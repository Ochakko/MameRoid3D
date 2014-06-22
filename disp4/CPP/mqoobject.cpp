//#include <stdafx.h>

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <malloc.h>
#include <memory.h>

#include <windows.h>

#include <crtdbg.h>

#include <mqoobject.h>

#include <DispObj.h>
#include <polymesh3.h>
#include <GetMaterial.h>

#include <ExtLine.h>

#include <mqoface.h>
#include <mqomaterial.h>
#include <InfBone.h>
#include <Bone.h>

//#include <extlineio.h>
//#include <extline.h>
#include <Collision.h>

#include <algorithm>
#include <iostream>
#include <iterator>

#include <MorphKey.h>


typedef struct tag_mqotriline
{
	int index[2];
	float leng;
	int sortno;
} MQOTRILINE;


static int SortTriLine( const VOID* arg1, const VOID* arg2 );
static int s_alloccnt = 0;

typedef struct tag_latheelem
{
	float height;
	float dist;
} LATHEELEM;


CMQOObject::CMQOObject()
{
	InitParams();
	s_alloccnt++;
	m_objectno = s_alloccnt;
}

CMQOObject::~CMQOObject()
{
	if( m_pointbuf ){
		free( m_pointbuf );
		m_pointbuf = 0;
	}

	if( m_facebuf ){
		delete [] m_facebuf;
		m_facebuf = 0;
	}

	if( m_colorbuf ){
		free( m_colorbuf );
		m_colorbuf = 0;
	}

	if( m_pointbuf2 ){
		free( m_pointbuf2 );
		m_pointbuf2 = 0;
	}

	if( m_facebuf2 ){
		delete [] m_facebuf2;
		m_facebuf2 = 0;
	}

	if( m_colorbuf2 ){
		free( m_colorbuf2 );
		m_colorbuf2 = 0;
	}

	if( m_connectface ){
		delete [] m_connectface;
		m_connectface = 0;
	}

	if( m_pm3 ){
		delete m_pm3;
		m_pm3 = 0;
	}
	if( m_extline ){
		delete m_extline;
		m_extline = 0;
	}

	if( m_dispobj ){
		delete m_dispobj;
		m_dispobj = 0;
	}

	if( m_displine ){
		delete m_displine;
		m_displine = 0;
	}

	map<int,CMQOObject*>::iterator itrmo;
	for( itrmo = m_morphobj.begin(); itrmo != m_morphobj.end(); itrmo++ ){
		CMQOObject* curmo = itrmo->second;
		if( curmo ){
			delete curmo;
		}
	}
	m_morphobj.clear();

	map<int,CMorphKey*>::iterator itrmk;
	for( itrmk = m_morphkey.begin(); itrmk != m_morphkey.end(); itrmk++ ){
		CMorphKey* curmk = itrmk->second;
		CMorphKey* nextmk = 0;
		while( curmk ){
			nextmk = curmk->m_next;
			delete curmk;
			curmk = nextmk;
		}
	}
	m_morphkey.clear();


	map<int,CMQOMaterial*>::iterator itrmat;
	for( itrmat = m_morphmaterial.begin(); itrmat != m_morphmaterial.end(); itrmat++ ){
		CMQOMaterial* curmat = itrmat->second;
		if( curmat ){
			delete curmat;
		}
	}
	m_morphmaterial.clear();


}

void CMQOObject::InitParams()
{
	m_findinfbonecnt = 0;
	m_dispflag = 1;
	m_objectno = -1;
	ZeroMemory( m_name, 256 );
	ZeroMemory( m_engname, 256 );

	m_patch = 0;
	m_segment = 1;

	m_visible = 15;
	m_locking = 0;

	m_shading	= 1;
	m_facet = 59.5f;
	m_color.x = 1.0f;
	m_color.y = 1.0f;
	m_color.z = 1.0f;
	m_color.w = 1.0f;
	m_color_type = 0;
	m_mirror = 0;
	m_mirror_axis = 1;
	m_issetmirror_dis = 0;
	m_mirror_dis = 0.0f;
	m_lathe = 0;
	m_lathe_axis = 0;
	m_lathe_seg = 3;
	m_vertex = 0;
	//BVertex;
	m_face = 0;

	m_pointbuf = 0;
	m_facebuf = 0;

	m_vertex2 = 0;
	m_face2 = 0;
	m_pointbuf2 = 0;
	m_facebuf2 = 0;

	m_hascolor = 0;
	m_colorbuf = 0;
	m_colorbuf2 = 0;

	m_connectnum = 0;
	m_connectface = 0;

	m_pm3 = 0;
	m_extline = 0;
	D3DXMatrixIdentity( &m_multmat );

	m_dispobj = 0;
	m_displine = 0;

//	next = 0;
}


int CMQOObject::SetParams( char* srcchar, int srcleng )
{
	char pat[15][20] = 
	{
		"Object",
		"patch",
		"segment",
		"shading",
		"facet",
		"color_type",
		"color",
		"mirror_axis",
		"mirror_dis",
		"mirror",
		"lathe_axis",
		"lathe_seg",
		"lathe",

		"visible",
		"locking"
	};

	int patno, patleng;
	int pos = 0;
	int cmp;

	int stepnum;
	for( patno = 0; patno < 15; patno++ ){
		while( (pos < srcleng) && ( (*(srcchar + pos) == ' ') || (*(srcchar + pos) == '\t') ) ){
			pos++;
		}
		
		patleng = (int)strlen( pat[patno] );
		cmp = strncmp( pat[patno], srcchar + pos, patleng );
		if( cmp == 0 ){

			pos += patleng;//!!!

			switch( patno ){
			case 0:
				CallF( GetName( m_name, 256, srcchar, pos, srcleng ), return 1 );
				break;
			case 1:
				CallF( GetInt( &m_patch, srcchar, pos, srcleng, &stepnum ), return 1 );
				break;
			case 2:
				CallF( GetInt( &m_segment, srcchar, pos, srcleng, &stepnum ), return 1 );
				break;
			case 3:
				CallF( GetInt( &m_shading, srcchar, pos, srcleng, &stepnum ), return 1 );
				break;
			case 4:
				CallF( GetFloat( &m_facet, srcchar, pos, srcleng, &stepnum ), return 1 );
				break;
			case 5:
				CallF( GetInt( &m_color_type, srcchar, pos, srcleng, &stepnum ), return 1 );
				break;
			case 6:
				CallF( GetFloat( &m_color.x, srcchar, pos, srcleng, &stepnum ), return 1 );
				pos += stepnum;
				if( pos >= srcleng ){
					DbgOut( L"MQOObject : SetParams : GetFloat : pos error !!!" );
					_ASSERT( 0 );
					return 1;
				}


				CallF( GetFloat( &m_color.y, srcchar, pos, srcleng, &stepnum ), return 1 );
				pos += stepnum;
				if( pos >= srcleng ){
					DbgOut( L"MQOObject : SetParams : GetFloat : pos error !!!" );
					_ASSERT( 0 );
					return 1;
				}

				CallF( GetFloat( &m_color.z, srcchar, pos, srcleng, &stepnum ), return 1 );
				break;
			case 7:
				CallF( GetInt( &m_mirror_axis, srcchar, pos, srcleng, &stepnum ), return 1 );
				break;
			case 8:
				CallF( GetFloat( &m_mirror_dis, srcchar, pos, srcleng, &stepnum ), return 1 );
				m_issetmirror_dis = 1;//!!!!
				break;
			case 9:
				CallF( GetInt( &m_mirror, srcchar, pos, srcleng, &stepnum ), return 1 );
				break;
			case 10:
				CallF( GetInt( &m_lathe_axis, srcchar, pos, srcleng, &stepnum ), return 1 );
				break;
			case 11:
				CallF( GetInt( &m_lathe_seg, srcchar, pos, srcleng, &stepnum ), return 1 );
				break;
			case 12:
				CallF( GetInt( &m_lathe, srcchar, pos, srcleng, &stepnum ), return 1 );
				break;
			case 13:
				CallF( GetInt( &m_visible, srcchar, pos, srcleng, &stepnum ), return 1 );
				break;
			case 14:
				CallF( GetInt( &m_locking, srcchar, pos, srcleng, &stepnum ), return 1 );
				break;
			default:
				_ASSERT( 0 );
				break;
			}

			break;
		}
	}
	


	return 0;

}

int CMQOObject::GetInt( int* dstint, char* srcchar, int pos, int srcleng, int* stepnum )
{
	int startpos, endpos;
	startpos = pos;

	while( (startpos < srcleng) &&  
		( ( isdigit( *(srcchar + startpos) ) == 0 ) && (*(srcchar + startpos) != '-') ) 
	
	){
		startpos++;
	}

	endpos = startpos;
	while( (endpos < srcleng) && 
		( (isdigit( *(srcchar + endpos) ) != 0) || ( *(srcchar + endpos) == '-' ) )
	){
		endpos++;
	}

	char tempchar[256];
	if( (endpos - startpos < 256) && (endpos - startpos > 0) ){
		strncpy_s( tempchar, 256, srcchar + startpos, endpos - startpos );
		tempchar[endpos - startpos] = 0;

		*dstint = atoi( tempchar );

		*stepnum = endpos - pos;
	}else{
		_ASSERT( 0 );
		*stepnum = endpos - pos;
		return 1;
	}


	return 0;
}
int CMQOObject::GetFloat( float* dstfloat, char* srcchar, int pos, int srcleng, int* stepnum )
{
	int startpos, endpos;
	startpos = pos;

	while( (startpos < srcleng) &&  
		( ( isdigit( *(srcchar + startpos) ) == 0 ) && (*(srcchar + startpos) != '-') && (*(srcchar + startpos) != '.') ) 
	
	){
		startpos++;
	}

	endpos = startpos;
	while( (endpos < srcleng) && 
		( (isdigit( *(srcchar + endpos) ) != 0) || ( *(srcchar + endpos) == '-' ) || (*(srcchar + endpos) == '.') )
	){
		endpos++;
	}

	char tempchar[256];
	if( (endpos - startpos < 256) && (endpos - startpos > 0) ){
		strncpy_s( tempchar, 256, srcchar + startpos, endpos - startpos );
		tempchar[endpos - startpos] = 0;

		*dstfloat = (float)atof( tempchar );

		*stepnum = endpos - pos;
	}else{
		_ASSERT( 0 );
		*stepnum = endpos - pos;
		return 1;
	}


	return 0;
}

int CMQOObject::GetName( char* dstchar, int dstleng, char* srcchar, int pos, int srcleng )
{
	int startpos, endpos;
	startpos = pos;

	while( (startpos < srcleng) && 
		( ( *(srcchar + startpos) == ' ' ) || ( *(srcchar + startpos) == '\t' ) || ( *(srcchar + startpos) == '\"' ) ) 
	){
		startpos++;
	}

	endpos = startpos;
	while( (endpos < srcleng) && 
		( ( *(srcchar + endpos) != ' ' ) && ( *(srcchar + endpos) != '\t' ) && (*(srcchar + endpos) != '\r') && (*(srcchar + endpos) != '\r\n') && (*(srcchar + endpos) != '\"') )
	){
		endpos++;
	}

	if( (endpos - startpos < dstleng) && (endpos - startpos > 0) ){
		strncpy_s( dstchar, dstleng, srcchar + startpos, endpos - startpos );
		*(dstchar + endpos - startpos) = 0;

	}else{
		_ASSERT( 0 );
	}


	return 0;
}

int CMQOObject::SetVertex( int* vertnum, char* srcchar, int srcleng )
{
	//vertex, または、　BVertexを含む文字列を受け取る。
	if( m_pointbuf ){
		_ASSERT( 0 );
		return 1;
	}

	int ret;

	char headerchar1[] = "vertex";
	int headerleng1 = (int)strlen( headerchar1 );
	char* find1;
	find1 = strstr( srcchar, headerchar1 );


	char headerchar2[] = "Vector";
	int headerleng2 = (int)strlen( headerchar2 );
	char* find2;
	find2 = strstr( srcchar, headerchar2 );

	int pos;
	if( find1 != NULL ){
		pos = (int)(find1 + headerleng1 - srcchar);
	}else if( find2 != NULL ){
		pos = (int)(find2 + headerleng2 - srcchar);
	}else{

	}
	int stepnum;
	ret = GetInt( &m_vertex, srcchar, pos, srcleng, &stepnum );
	if( ret ){
		_ASSERT( 0 );
		return 1;
	}
	*vertnum = m_vertex;
	DbgOut( L"MQOObject : SetVertex : vertex %d\r\n", m_vertex );


	m_pointbuf = (D3DXVECTOR3*)malloc( sizeof( D3DXVECTOR3 ) * m_vertex );
	if( !m_pointbuf ){
		_ASSERT( 0 );
		return 1;
	}

	ZeroMemory( m_pointbuf, sizeof( D3DXVECTOR3 ) * m_vertex );


	return 0;
}
int CMQOObject::SetPointBuf( unsigned char* srcptr, int srcnum )
{
	if( !m_pointbuf || (srcnum > m_vertex) ){
		_ASSERT( 0 );
		return 1;
	}

	MoveMemory( m_pointbuf, srcptr, srcnum * sizeof( VEC3F ) );

	return 0;

}


int CMQOObject::SetPointBuf( int vertno, char* srcchar, int srcleng )
{
	if( !m_pointbuf || (vertno >= m_vertex) ){
		_ASSERT( 0 );
		return 1;
	}


	D3DXVECTOR3* dstvec;
	dstvec = m_pointbuf + vertno;
	int pos, stepnum;
	pos = 0;

	CallF( GetFloat( &dstvec->x, srcchar, pos, srcleng, &stepnum ), return 1 );
	pos += stepnum;
	if( pos >= srcleng ){
		DbgOut( L"MQOObject : SePointBuf : GetFloat : pos error !!!" );
		_ASSERT( 0 );
		return 1;
	}


	CallF( GetFloat( &dstvec->y, srcchar, pos, srcleng, &stepnum ), return 1 );
	pos += stepnum;
	if( pos >= srcleng ){
		DbgOut( L"MQOObject : SePointBuf : GetFloat : pos error !!!" );
		_ASSERT( 0 );
		return 1;
	}


	CallF( GetFloat( &dstvec->z, srcchar, pos, srcleng, &stepnum ), return 1 );

	return 0;
}

int CMQOObject::SetFace( int* facenum, char* srcchar, int srcleng )
{
	if( m_facebuf ){
		_ASSERT( 0 );
		return 1;
	}

	int ret;
	char headerchar[] = "face";
	int headerleng = (int)strlen( headerchar );

	char* find;
	find = strstr( srcchar, headerchar );
	if( find == NULL ){
		_ASSERT( 0 );
		return 1;
	}

	int pos;
	pos = (int)(find + headerleng - srcchar);
	int stepnum;
	ret = GetInt( &m_face, srcchar, pos, srcleng, &stepnum );
	if( ret ){
		_ASSERT( 0 );
		return 1;
	}
	*facenum = m_face;
	DbgOut( L"MQOObject : SetFace : face %d\r\n", m_face );

	
	m_facebuf = new CMQOFace[ m_face ];
	if( !m_facebuf ){
		_ASSERT( 0 );
		return 1;
	}


	return 0;
}

int CMQOObject::SetFaceBuf( int faceno, char* srcchar, int srcleng, int materialoffset )
{
	if( !m_facebuf || (faceno >= m_face) ){
		_ASSERT( 0 );
		return 1;
	}


	CMQOFace* dstface;
	dstface = m_facebuf + faceno;

	_ASSERT( dstface );

	CallF( dstface->SetParams( srcchar, srcleng ), return 1 );
	dstface->m_materialno += materialoffset;


	return 0;
}

int CMQOObject::Dump()
{
	WCHAR wname[256] = {0};
	MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, m_name, 256, wname, 256 );


	DbgOut( L"Ojbect : %s\r\n", wname );
	DbgOut( L"\tpatch %d\r\n", m_patch );
	DbgOut( L"\tsegment %d\r\n", m_segment );

	DbgOut( L"\tvisible %d\r\n", m_visible );
	DbgOut( L"\tlocking %d\r\n", m_locking );

	DbgOut( L"\tshading %d\r\n", m_shading );
	DbgOut( L"\tfacet %f\r\n", m_facet );
	DbgOut( L"\tcolor %f %f %f\r\n", m_color.x, m_color.y, m_color.z );
	DbgOut( L"\tcolor_type %d\r\n", m_color_type );
	DbgOut( L"\tmirror %d\r\n", m_mirror );
	DbgOut( L"\tmirror_axis %d\r\n", m_mirror_axis );
	DbgOut( L"\tmirror_dis %f\r\n", m_mirror_dis );
	DbgOut( L"\tlathe %d\r\n", m_lathe );
	DbgOut( L"\tlathe_axis %d\r\n", m_lathe_axis );
	DbgOut( L"\tlathe_seg %d\r\n", m_lathe_seg );

	DbgOut( L"\tvertex %d\r\n", m_vertex );
	int vertno;
	D3DXVECTOR3* curvec = m_pointbuf;
	for( vertno = 0; vertno < m_vertex; vertno++ ){
		DbgOut( L"\t\t%f %f %f\r\n", curvec->x, curvec->y, curvec->z );
		curvec++;
	}

	if( m_hascolor && m_colorbuf ){
		DbgOut( L"\tcolor\r\n" );
		for( vertno = 0; vertno < m_vertex; vertno++ ){
			DbgOut( L"\t\t%d : %f %f %f\r\n", vertno, m_colorbuf[vertno].x, m_colorbuf[vertno].y, m_colorbuf[vertno].z );
		}
	}

	int ret;
	DbgOut( L"\tface %d\r\n", m_face );
	int faceno;
	CMQOFace* curface = m_facebuf;
	for( faceno = 0; faceno < m_face; faceno++ ){
		ret = curface->Dump();
		if( ret ){
			_ASSERT( 0 );
			return 1;
		}
		curface++;
	}

	return 0;
}

int CMQOObject::CreateColor()
{
	if( m_vertex <= 0 ){
		DbgOut( L"MQOObject : CreateColor : vertex error !!!\r\n" );
		_ASSERT( 0 );
		return 1;
	}

	m_colorbuf = (D3DXVECTOR4*)malloc( sizeof( D3DXVECTOR4 ) * m_vertex );
	if( !m_colorbuf ){
		DbgOut( L"MQOObject : CreateColor : colorbuf alloc error !!!\r\n" );
		_ASSERT( 0 );
		return 1;
	}

	ZeroMemory( m_colorbuf, sizeof( D3DXVECTOR4 ) * m_vertex );

	m_hascolor = 1;

	return 0;
}
int CMQOObject::SetColor( char* srcchar, int srcleng )
{
	int pos, stepnum;

	int pointno;
	
	pos = 0;
	CallF( GetInt( &pointno, srcchar, pos, srcleng, &stepnum ), return 1 );

	if( (pointno < 0) || (pointno >= m_vertex) ){
		DbgOut( L"MQOObject : SetColor : pointno error !!!\r\n" );
		_ASSERT( 0 );
		return 1;
	}


	pos += stepnum;
	DWORD col;
	CallF( GetInt( (int*)&col, srcchar, pos, srcleng, &stepnum ), return 1 );

	DWORD dwr, dwg, dwb;
	dwr = col & 0x000000FF;
	dwg = (col & 0x0000FF00) >> 8;
	dwb = (col & 0x00FF0000) >> 16;

	D3DXVECTOR4* dstcol = m_colorbuf + pointno;
	dstcol->x = (float)dwr / 255.0f;
	dstcol->y = (float)dwg / 255.0f;
	dstcol->z = (float)dwb / 255.0f;
	dstcol->w = 1.0f;
	

	return 0;
}
int CMQOObject::MakePolymesh3( LPDIRECT3DDEVICE9 pdev, map<int,CMQOMaterial*>& srcmaterial )
{
	if( !m_pointbuf || !m_facebuf )
		return 0;

	if( m_pm3 ){
		delete m_pm3;
		m_pm3 = 0;
	}
	if( m_dispobj ){
		delete m_dispobj;
		m_dispobj = 0;
	}



	
	if( m_lathe != 0 ){
		CallF( MakeLatheBuf(), return 1 );
	}

	if( m_mirror != 0 ){
		CallF( MakeMirrorBuf(), return 1 );
	}
	

////////


	// 面と頂点の数を取得
	int face_count;
	int vert_count;
	D3DXVECTOR3* pointptr;
	CMQOFace* faceptr;
	D3DXVECTOR4* colorptr;

	if( m_face2 > 0 ){
		face_count = m_face2;
		faceptr = m_facebuf2;
	}else{
		face_count = m_face;
		faceptr = m_facebuf;
	}

	if( m_vertex2 > 0 ){
		vert_count = m_vertex2;
		pointptr = m_pointbuf2;
	}else{
		vert_count = m_vertex;
		pointptr = m_pointbuf;
	}

	if( m_colorbuf2 ){
		colorptr = m_colorbuf2;
	}else{
		colorptr = m_colorbuf;
	}

	m_pm3 = new CPolyMesh3();
	if( !m_pm3 ){
		_ASSERT( 0 );
		return 1;
	}
	CallF( m_pm3->CreatePM3( m_name, vert_count, face_count, m_facet, pointptr, faceptr, srcmaterial, m_multmat ), return 1 );

	return 0;
}

int CMQOObject::MakeExtLine()
{
	if( m_extline ){
		delete m_extline;
		m_extline = 0;
	}

	if( m_displine ){
		delete m_displine;
		m_displine = 0;
	}

	if( !m_pointbuf || !m_facebuf )
		return 0;



	m_extline = new CExtLine();
	if( !m_extline ){
		_ASSERT( 0 );
		return 1;
	}

	CallF( m_extline->CreateExtLine( m_vertex, m_face, m_pointbuf, m_facebuf, m_color ), return 1 );

	if( m_extline->m_linenum <= 0 ){
		delete m_extline;
		m_extline = 0;
	}

	return 0;
}

int CMQOObject::MakeDispObj( LPDIRECT3DDEVICE9 pdev, map<int,CMQOMaterial*>& srcmat, int hasbone )
{
	if( m_dispobj ){
		delete m_dispobj;
		m_dispobj = 0;
	}
	if( m_displine ){
		delete m_displine;
		m_displine = 0;
	}

	if( !m_pointbuf || !m_facebuf )
		return 0;

	if( (m_vertex <= 0) || (m_face <= 0) ){
		return 0;
	}


	if( m_pm3 && m_pm3->m_createoptflag ){
		m_dispobj = new CDispObj();
		if( !m_dispobj ){
			_ASSERT( 0 );
			return 1;
		}
		CallF( m_dispobj->CreateDispObj( pdev, m_pm3, hasbone ), return 1 );
	}

	if( m_extline ){
		m_displine = new CDispObj();
		if( !m_displine ){
			_ASSERT( 0 );
			return 1;
		}
		CallF( m_displine->CreateDispObj( pdev, m_extline ), return 1 );
	}

	return 0;
}

int CMQOObject::HasPolygon()
{
	int face2 = 0;
	int face3 = 0;
	int face4 = 0;

	int faceno;

	for( faceno = 0; faceno < m_face; faceno++ ){
		CMQOFace* curface;
		curface = m_facebuf + faceno;
		switch( curface->m_pointnum ){
			case 2:
				face2++;
				break;
			case 3:
				face3++;
				break;
			case 4:
				face4++;
			default:
				break;
		}
	}
	
	if( m_lathe != 0 ){
		return (face2 + face3 + face4);
	}else{
		return (face3 + face4);
	}
}

int CMQOObject::HasLine()
{
	int face2 = 0;

	int faceno;

	for( faceno = 0; faceno < m_face; faceno++ ){
		CMQOFace* curface;
		curface = m_facebuf + faceno;
		switch( curface->m_pointnum ){
			case 2:
				face2++;
				break;
			case 3:
			case 4:
			default:
				break;
		}
	}
	
	return face2;
}

/***
int CMQOObject::IsMikoBone()
{
	int cmp;
	char pattern[20] = "bone:";
	int patleng = (int)strlen( pattern );

	cmp = strncmp( m_name, pattern, patleng );
	if( cmp == 0 )
		return 1;
	else
		return 0;
}
***/


int CMQOObject::MakeLatheBuf()
{
	int linenum = 0;
	int faceno;
	CMQOFace* curface;
	for( faceno = 0; faceno < m_face; faceno++ ){
		curface = m_facebuf + faceno;
		if( curface->m_pointnum == 2 ){
			linenum++;
		}
	}

	if( linenum <= 0 ) //!!!!
		return 0;


	int* lineno2faceno;
	lineno2faceno = (int*)malloc( sizeof( int ) * linenum );
	if( !lineno2faceno ){
		DbgOut( L"MQOObject : MakeLatheBuf : lineno2faceno alloc error !!!\r\n" );
		_ASSERT( 0 );
		return 1;
	}
	ZeroMemory( lineno2faceno, sizeof( int ) * linenum );

	int lineno = 0;
	for( faceno = 0; faceno < m_face; faceno++ ){
		curface = m_facebuf + faceno;
		if( curface->m_pointnum == 2 ){
			_ASSERT( lineno < linenum );
			*(lineno2faceno + lineno) = faceno; 
			lineno++;
		}
	}

	if( m_pointbuf2 ){
		_ASSERT( 0 );
		return 1;
	}
	m_vertex2 = m_lathe_seg * 2 * linenum;
	m_pointbuf2 = (D3DXVECTOR3*)malloc( sizeof( D3DXVECTOR3 ) * m_vertex2 );
	if( !m_pointbuf2 ){
		DbgOut( L"MQOObject : MakeLatheBuf : pointbuf2 alloc error !!!\r\n" );
		_ASSERT( 0 );
		return 1;			
	}
	ZeroMemory( m_pointbuf2, sizeof( D3DXVECTOR3 ) * m_vertex2 );

	if( m_colorbuf ){
		if( m_colorbuf2 ){
			_ASSERT( 0 );
			return 1;
		}
		m_colorbuf2 = (D3DXVECTOR4*)malloc( sizeof( D3DXVECTOR4 ) * m_vertex2 );
		if( !m_colorbuf2 ){
			DbgOut( L"MQOObject : MakeLatheBuf : colorbuf2 alloc error !!!\r\n" );
			_ASSERT( 0 );
			return 1;			
		}
		//ZeroMemory( colorbuf2, sizeof( ARGBF ) * vertex2 );
		MoveMemory( m_colorbuf2, m_colorbuf, sizeof( D3DXVECTOR4 ) * m_vertex );
	}

	if( m_facebuf2 ){
		_ASSERT( 0 );
		return 1;
	}
	m_face2 = m_lathe_seg * linenum;
	m_facebuf2 = new CMQOFace[ m_face2 ];
	if( !m_facebuf2 ){
		DbgOut( L"MQOObject : MakeLatheBuf : facebuf2 alloc error !!!\r\n" );
		_ASSERT( 0 );
		return 1;			
	}
	

	LATHEELEM elem[2];
	float mag, rad;
	int segno;
	rad = 2.0f * PI / m_lathe_seg;

	for( lineno = 0; lineno < linenum; lineno++ ){
		faceno = *(lineno2faceno + lineno);
		int v0, v1;
		v0 = (m_facebuf + faceno)->m_index[0];
		v1 = (m_facebuf + faceno)->m_index[1];
		D3DXVECTOR3* src0 = m_pointbuf + v0;
		D3DXVECTOR3* src1 = m_pointbuf + v1;
		switch( m_lathe_axis ){
		case 0://X
			elem[0].height = src0->x;
			mag = src0->y * src0->y + src0->z * src0->z;
			if( mag > 0.0f ){
				elem[0].dist = (float)sqrt( mag );
			}else{
				elem[0].dist = 0.0f;
			}

			elem[1].height = src1->x;
			mag = src1->y * src1->y + src1->z * src1->z;
			if( mag > 0.0f ){
				elem[1].dist = (float)sqrt( mag );
			}else{
				elem[1].dist = 0.0f;
			}

			for( segno = 0; segno < m_lathe_seg; segno++ ){
				D3DXVECTOR3* dst0 = m_pointbuf2 + lineno * 2 * m_lathe_seg + segno;
				D3DXVECTOR3* dst1 = dst0 + m_lathe_seg;

				dst0->x = elem[0].height;
				dst0->y = elem[0].dist * (float)cos( rad * segno );
				dst0->z = elem[0].dist * (float)sin( rad * segno );

				dst1->x = elem[1].height;
				dst1->y = elem[1].dist * (float)cos( rad * segno );
				dst1->z = elem[1].dist * (float)sin( rad * segno );

				if( m_colorbuf2 ){
					D3DXVECTOR4* dstcol0 = m_colorbuf2 + lineno * 2 * m_lathe_seg + segno;
					D3DXVECTOR4* dstcol1 = dstcol0 + m_lathe_seg;

					*dstcol0 = *(m_colorbuf + v0);
					*dstcol1 = *(m_colorbuf + v1);
				}
			}

			break;
		case 1://Y
			elem[0].height = src0->y;
			mag = src0->x * src0->x + src0->z * src0->z;
			if( mag > 0.0f ){
				elem[0].dist = (float)sqrt( mag );
			}else{
				elem[0].dist = 0.0f;
			}

			elem[1].height = src1->y;
			mag = src1->x * src1->x + src1->z * src1->z;
			if( mag > 0.0f ){
				elem[1].dist = (float)sqrt( mag );
			}else{
				elem[1].dist = 0.0f;
			}

			for( segno = 0; segno < m_lathe_seg; segno++ ){
				D3DXVECTOR3* dst0 = m_pointbuf2 + lineno * 2 * m_lathe_seg + segno;
				D3DXVECTOR3* dst1 = dst0 + m_lathe_seg;

				dst0->x = elem[0].dist * (float)cos( rad * segno );
				dst0->y = elem[0].height;
				dst0->z = elem[0].dist * (float)sin( rad * segno );

				dst1->x = elem[1].dist * (float)cos( rad * segno );
				dst1->y = elem[1].height;
				dst1->z = elem[1].dist * (float)sin( rad * segno );

				if( m_colorbuf2 ){
					D3DXVECTOR4* dstcol0 = m_colorbuf2 + lineno * 2 * m_lathe_seg + segno;
					D3DXVECTOR4* dstcol1 = dstcol0 + m_lathe_seg;

					*dstcol0 = *(m_colorbuf + v0);
					*dstcol1 = *(m_colorbuf + v1);
				}
			}
			break;
		case 2://Z
			elem[0].height = src0->z;
			mag = src0->x * src0->x + src0->y * src0->y;
			if( mag > 0.0f ){
				elem[0].dist = (float)sqrt( mag );
			}else{
				elem[0].dist = 0.0f;
			}

			elem[1].height = src1->z;
			mag = src1->x * src1->x + src1->y * src1->y;
			if( mag > 0.0f ){
				elem[1].dist = (float)sqrt( mag );
			}else{
				elem[1].dist = 0.0f;
			}

			for( segno = 0; segno < m_lathe_seg; segno++ ){
				D3DXVECTOR3* dst0 = m_pointbuf2 + lineno * 2 * m_lathe_seg + segno;
				D3DXVECTOR3* dst1 = dst0 + m_lathe_seg;

				dst0->x = elem[0].dist * (float)cos( rad * segno );
				dst0->y = elem[0].dist * (float)sin( rad * segno );
				dst0->z = elem[0].height;

				dst1->x = elem[1].dist * (float)cos( rad * segno );
				dst1->y = elem[1].dist * (float)sin( rad * segno );
				dst1->z = elem[1].height;

				if( m_colorbuf2 ){
					D3DXVECTOR4* dstcol0 = m_colorbuf2 + lineno * 2 * m_lathe_seg + segno;
					D3DXVECTOR4* dstcol1 = dstcol0 + m_lathe_seg;

					*dstcol0 = *(m_colorbuf + v0);
					*dstcol1 = *(m_colorbuf + v1);
				}
			}
			break;
		default:
			_ASSERT( 0 );
			break;
		}
	}


	for( lineno = 0; lineno < linenum; lineno++ ){ 
		for( segno = 0; segno < m_lathe_seg; segno++ ){
			CMQOFace* dstface = m_facebuf2 + m_lathe_seg * lineno + segno;
			dstface->m_pointnum = 4;

			if( segno != m_lathe_seg - 1 ){
				dstface->m_index[0] = lineno * 2 * m_lathe_seg + segno;
				dstface->m_index[1] = lineno * 2 * m_lathe_seg + m_lathe_seg + segno;
				dstface->m_index[2] = lineno * 2 * m_lathe_seg + m_lathe_seg + 1 + segno;
				dstface->m_index[3] = lineno * 2 * m_lathe_seg + 1 + segno;
			}else{
				dstface->m_index[0] = lineno * 2 * m_lathe_seg + segno;
				dstface->m_index[1] = lineno * 2 * m_lathe_seg + m_lathe_seg + segno;
				dstface->m_index[2] = lineno * 2 * m_lathe_seg + m_lathe_seg;
				dstface->m_index[3] = lineno * 2 * m_lathe_seg;
			}
			dstface->m_hasuv = 0;

		}
	}


	free( lineno2faceno );

	return 0;
}

int CMQOObject::MakeMirrorBuf()
{


	if( m_mirror == 2 ){
		CallF( FindConnectFace( 0 ), return 1 );//connectnumのセット
		if( m_connectnum > 0 ){
			_ASSERT( m_connectface == 0 );
			m_connectface = new CMQOFace[ m_connectnum ];
			if( !m_connectface ){
				DbgOut( L"MQOObject : MakeMirrorBuf : connectface alloc error !!!\r\n" );
				_ASSERT( 0 );
				return 1;
			}

			CallF( FindConnectFace( 1 ), return 1 );//connectfaceのセット
		}

	}else{
		m_connectnum = 0;
		m_connectface = 0;
	}
	

	int doconnect = 0;
	int axis;
	if( axis = (m_mirror_axis & 0x01) ){
		CallF( MakeMirrorPointAndFace( axis, doconnect ), return 1 );
		doconnect = 1;
	}

	if( axis = (m_mirror_axis & 0x02) ){
		CallF( MakeMirrorPointAndFace( axis, doconnect ), return 1 );
		doconnect = 1;
	}

	
	if( axis = (m_mirror_axis & 0x04) ){
		CallF( MakeMirrorPointAndFace( axis, doconnect ), return 1 );
		doconnect = 1;
	}



	return 0;
}

int CMQOObject::MakeMirrorPointAndFace( int axis, int doconnect )
{

	int befvertex, befface;

	if( m_vertex2 > 0 )
		befvertex = m_vertex2;
	else
		befvertex = m_vertex;
	
	if( m_face2 > 0 )
		befface = m_face2;
	else
		befface = m_face;

	m_vertex2 = befvertex * 2;
	
	if( doconnect == 0 )
		m_face2 = befface * 2 + m_connectnum;
	else
		m_face2 = befface * 2;

	if( m_pointbuf2 ){
		m_pointbuf2 = (D3DXVECTOR3*)realloc( m_pointbuf2, sizeof( D3DXVECTOR3 ) * m_vertex2 );
		if( !m_pointbuf2 ){
			DbgOut( L"MQOObject : MakeMirrorPointAndFace : pointbuf2 realloc error !!!\r\n" );
			_ASSERT( 0 );
			return 1;
		}
	}else{
		m_pointbuf2 = (D3DXVECTOR3*)malloc( sizeof( D3DXVECTOR3 ) * m_vertex2 );
		if( !m_pointbuf2 ){
			DbgOut( L"MQOObject : MakeMirrorPointAndFace : pointbuf2 alloc error !!!\r\n" );
			_ASSERT( 0 );
			return 1;
		}
		MoveMemory( m_pointbuf2, m_pointbuf, sizeof( D3DXVECTOR3 ) * befvertex );
	}

	if( m_colorbuf ){
		if( m_colorbuf2 ){
			m_colorbuf2 = (D3DXVECTOR4*)realloc( m_colorbuf2, sizeof( D3DXVECTOR4 ) * m_vertex2 );
			if( !m_colorbuf2 ){
				DbgOut( L"MQOObject : MakeMirrorPointAndFace : colorbuf2 realloc error !!!\r\n" );
				_ASSERT( 0 );
				return 1;
			}
		}else{
			m_colorbuf2 = (D3DXVECTOR4*)malloc( sizeof( D3DXVECTOR4 ) * m_vertex2 );
			if( !m_colorbuf2 ){
				DbgOut( L"MQOObject : MakeMirrorPointAndFace : colorbuf2 alloc error !!!\r\n" );
				_ASSERT( 0 );
				return 1;
			}
			MoveMemory( m_colorbuf2, m_colorbuf, sizeof( D3DXVECTOR4 ) * befvertex );
		}
	}

	if( m_facebuf2 ){
		CMQOFace* newface;
		newface = new CMQOFace[ m_face2 ];
		if( !newface ){
			DbgOut( L"MQOObject : MakeMirrorPointAndFace : newface alloc error !!!\r\n" );
			_ASSERT( 0 );
			return 1;
		}
		MoveMemory( newface, m_facebuf2, sizeof( CMQOFace ) * befface );
		delete [] m_facebuf2;
		m_facebuf2 = newface;

	}else{
		m_facebuf2 = new CMQOFace[ m_face2 ];
		if( !m_facebuf2 ){
			DbgOut( L"MQOObject : MakeMirrorPointAndFace : facebuf2 alloc error !!!\r\n" );
			_ASSERT( 0 );
			return 1;
		}
		MoveMemory( m_facebuf2, m_facebuf, sizeof( CMQOFace ) * befface );
	}


	int vertno;
	D3DXVECTOR3* srcvec;
	D3DXVECTOR3* dstvec;
	switch( axis ){
	case 1://X
		for( vertno = 0; vertno < befvertex; vertno++ ){
			srcvec = m_pointbuf2 + vertno;
			dstvec = m_pointbuf2 + vertno + befvertex;
			
			dstvec->x = -srcvec->x;
			dstvec->y = srcvec->y;
			dstvec->z = srcvec->z;
		}
		break;
	case 2://Y
		for( vertno = 0; vertno < befvertex; vertno++ ){
			srcvec = m_pointbuf2 + vertno;
			dstvec = m_pointbuf2 + vertno + befvertex;
			
			dstvec->x = srcvec->x;
			dstvec->y = -srcvec->y;
			dstvec->z = srcvec->z;
		}
		break;
	case 4://Z
		for( vertno = 0; vertno < befvertex; vertno++ ){
			srcvec = m_pointbuf2 + vertno;
			dstvec = m_pointbuf2 + vertno + befvertex;
			
			dstvec->x = srcvec->x;
			dstvec->y = srcvec->y;
			dstvec->z = -srcvec->z;
		}

		break;
	default:
		_ASSERT( 0 );
		break;
	}

	if( m_colorbuf2 ){
		D3DXVECTOR4* srccol;
		D3DXVECTOR4* dstcol;
		for( vertno = 0; vertno < befvertex; vertno++ ){
			srccol = m_colorbuf2 + vertno;
			dstcol = m_colorbuf2 + vertno + befvertex;
			
			*dstcol = *srccol;
		}
	}


	int faceno;
	CMQOFace* srcface;
	CMQOFace* dstface;
	for( faceno = 0; faceno < befface; faceno++ ){
		srcface = m_facebuf2 + faceno;
		dstface = m_facebuf2 + faceno + befface;

		CallF( dstface->SetInvFace( srcface, befvertex ), return 1 );
	}


	if( (doconnect == 0) && m_connectface ){
		int srcno = 0;
		for( faceno = befface * 2; faceno < m_face2; faceno++ ){
			srcface = m_connectface + srcno;
			dstface = m_facebuf2 + faceno;
			
			dstface->m_pointnum = 4;
			dstface->m_materialno = srcface->m_materialno;
			
			dstface->m_index[0] = srcface->m_index[0];//反対向きに描画するようにセット。
			dstface->m_index[1] = srcface->m_index[0] + befvertex;
			dstface->m_index[2] = srcface->m_index[1] + befvertex;
			dstface->m_index[3] = srcface->m_index[1];

			dstface->m_hasuv = srcface->m_hasuv;

			dstface->m_uv[0] = srcface->m_uv[0];
			dstface->m_uv[1] = srcface->m_uv[0];
			dstface->m_uv[2] = srcface->m_uv[1];
			dstface->m_uv[3] = srcface->m_uv[1];

			dstface->m_col[0] = srcface->m_col[0];
			dstface->m_col[1] = srcface->m_col[0];
			dstface->m_col[2] = srcface->m_col[1];
			dstface->m_col[3] = srcface->m_col[1];

			srcno++;
		}
	}


	return 0;
}


int CMQOObject::FindConnectFace( int issetface )
{

	int conno = 0;
	int faceno, chkno;
	int pointnum;
	int findflag[4];
	int findall;
	CMQOFace* curface;
	CMQOFace* chkface;
	for( faceno = 0; faceno < m_face; faceno++ ){
		curface = m_facebuf + faceno;
		pointnum = curface->m_pointnum;
		if( pointnum <= 2 )
			continue;

		ZeroMemory( findflag, sizeof( int ) * 4 );

		findall = 0;
		for( chkno = 0; chkno < m_face; chkno++ ){
			if( chkno == faceno )
				continue;
			chkface = m_facebuf + chkno;

			if( chkface->m_pointnum <= 2 )
				continue;

			CallF( curface->CheckSameLine( chkface, findflag ), return 1 );

			int flagsum = 0;
			int sumno;
			for( sumno = 0; sumno < pointnum; sumno++ ){
				flagsum += findflag[sumno];
			}
			if( flagsum == pointnum ){
				findall = 1;
				break;
			}
		}

		if( findall == 0 ){
			int i;
			for( i = 0; i < pointnum; i++ ){
				if( findflag[i] == 0 ){//共有していない辺
					int distok = 1;
					distok = CheckMirrorDis( m_pointbuf, curface, i, pointnum );
					if( distok ){
						if( issetface ){
							(m_connectface + conno)->m_pointnum = 2;
							(m_connectface + conno)->m_materialno = curface->m_materialno;
							(m_connectface + conno)->m_hasuv = curface->m_hasuv;

							(m_connectface + conno)->m_index[0] = curface->m_index[i];
							(m_connectface + conno)->m_uv[0] = curface->m_uv[i];
							if( i != (pointnum - 1) ){
								(m_connectface + conno)->m_index[1] = curface->m_index[i+1];
								(m_connectface + conno)->m_uv[1] = curface->m_uv[i+1];
							}else{
								(m_connectface + conno)->m_index[1] = curface->m_index[0];
								(m_connectface + conno)->m_uv[1] = curface->m_uv[0];
							}

						}
						conno++;
					}
				}
			}

		}

	}

	if( issetface == 0 ){
		m_connectnum = conno;
	}


	return 0;
}


int CMQOObject::CheckMirrorDis( D3DXVECTOR3* pbuf, CMQOFace* fbuf, int lno, int pnum )
{
	if( m_issetmirror_dis == 0 ){
		return 1;
	}else{
		int axisx, axisy, axisz;
		axisx = m_mirror_axis & 0x01;
		axisy = m_mirror_axis & 0x02;
		axisz = m_mirror_axis & 0x04;

		int i0, i1;
		i0 = lno;
		if( lno == pnum - 1 ){
			i1 = 0;
		}else{
			i1 = lno + 1;
		}
		D3DXVECTOR3* v0;
		D3DXVECTOR3* v1;
		v0 = pbuf + fbuf->m_index[i0];
		v1 = pbuf + fbuf->m_index[i1];

		float distx0, distx1;
		int chkx = 1;
		if( axisx ){
			distx0 = v0->x * 2.0f;
			distx1 = v1->x * 2.0f;

			if( (distx0 > m_mirror_dis) || (distx1 > m_mirror_dis) ){
				chkx = 0;
			}
		}


		float disty0, disty1;
		int chky = 1;
		if( axisy ){
			disty0 = v0->y * 2.0f;
			disty1 = v1->y * 2.0f;

			if( (disty0 > m_mirror_dis) || (disty1 > m_mirror_dis) ){
				chky = 0;
			}
		}


		float distz0, distz1;
		int chkz = 1;
		if( axisz ){
			distz0 = v0->z * 2.0f;
			distz1 = v1->z * 2.0f;

			if( (distz0 > m_mirror_dis) || (distz1 > m_mirror_dis) ){
				chkz = 0;
			}
		}


		if( (chkx == 1) && (chky == 1) && (chkz == 1) ){
			return 1;
		}else{
			return 0;
		}

	}

}

int CMQOObject::MultMat( D3DXMATRIX multmat )
{
	m_multmat = multmat;
	return 0;

}

int CMQOObject::MultVertex()
{
	int pno;
	for( pno = 0; pno < m_vertex; pno++ ){
		D3DXVECTOR3* curp = m_pointbuf + pno;
		D3DXVECTOR3 srcv = *curp;

		D3DXVec3TransformCoord( curp, &srcv, &m_multmat );

	}
	return 0;
}

int CMQOObject::Multiple( float multiple )
{
	D3DXMatrixIdentity( &m_multmat );
	m_multmat._11 = multiple;
	m_multmat._22 = multiple;
	m_multmat._33 = -multiple;

	return 0;
}
int CMQOObject::InitFaceDirtyFlag()
{
	int fno;
	CMQOFace* curface;
	for( fno = 0; fno < m_face; fno++ ){
		curface = m_facebuf + fno;

		curface->m_dirtyflag = 0;
	}

	return 0;
}

int SortTriLine( const VOID* arg1, const VOID* arg2 )
{
    MQOTRILINE* p1 = (MQOTRILINE*)arg1;
    MQOTRILINE* p2 = (MQOTRILINE*)arg2;
    
    if( p1->leng < p2->leng )
        return -1;
	else if( p1->leng == p2->leng )
		return 0;
	else
		return 1;
}

int CMQOObject::SetMikoBoneIndex3()
{
	int faceno;
	CMQOFace* curface;
	for( faceno = 0; faceno < m_face; faceno++ ){
		curface = m_facebuf + faceno;

		if( curface->m_pointnum == 3 ){

			curface->m_bonetype = MIKOBONE_NORMAL;

			MQOTRILINE triline[3];
			triline[0].index[0] = curface->m_index[0];
			triline[0].index[1] = curface->m_index[1];

			triline[1].index[0] = curface->m_index[1];
			triline[1].index[1] = curface->m_index[2];

			triline[2].index[0] = curface->m_index[2];
			triline[2].index[1] = curface->m_index[0];

			int lineno;
			D3DXVECTOR3* vec0;
			D3DXVECTOR3* vec1;
			float mag;
			for( lineno = 0; lineno < 3; lineno++ ){
				vec0 = ( m_pointbuf + triline[lineno].index[0] );
				vec1 = ( m_pointbuf + triline[lineno].index[1] );

				mag = ( vec0->x - vec1->x ) * ( vec0->x - vec1->x ) +
					( vec0->y - vec1->y ) * ( vec0->y - vec1->y ) +
					( vec0->z - vec1->z ) * ( vec0->z - vec1->z );

				if( mag != 0.0f ){
					triline[lineno].leng = (float)sqrt( mag );
				}else{
					triline[lineno].leng = 0.0f;
				}
			}

			qsort( triline, 3, sizeof( MQOTRILINE ), SortTriLine );

			if( (triline[0].index[0] == triline[1].index[0]) || (triline[0].index[0] == triline[1].index[1]) ){
				curface->m_parentindex = triline[0].index[0];
			}else{
				curface->m_parentindex = triline[0].index[1];
			}

			if( (triline[1].index[0] == triline[2].index[0]) || (triline[1].index[0] == triline[2].index[1]) ){
				curface->m_childindex = triline[1].index[0];
			}else{
				curface->m_childindex = triline[1].index[1];
			}

			if( (triline[2].index[0] == triline[0].index[0]) || (triline[2].index[0] == triline[0].index[1]) ){
				curface->m_hindex = triline[2].index[0];
			}else{
				curface->m_hindex = triline[2].index[1];
			}
		}
	}

	return 0;
}

int CMQOObject::SetMikoBoneIndex2()
{

	int faceno;
	CMQOFace* curface;

	CMQOFace* findface = 0;
	int findi = -1;

	for( faceno = 0; faceno < m_face; faceno++ ){
		curface = m_facebuf + faceno;

		if( curface->m_pointnum == 2 ){

			int i;
			int lineindex;
			for( i = 0; i < 2; i++ ){
				lineindex = curface->m_index[i];

				CallF( CheckFaceSameChildIndex( curface, lineindex, &findface ), return 1 );

				if( findface && (findface->m_bonetype == MIKOBONE_NORMAL) ){

					if( i == 0 ){
						int otherindex = curface->m_index[1];
						int j;
						for( j = 0; j < findface->m_pointnum; j++ ){
							if( otherindex == findface->m_index[j] ){
								//3角内に、線が含まれる場合は、ゴミデータと見なす。
								findface = 0;
								findi = -1;
								break;
							}
						}
					}else{
						int otherindex = curface->m_index[0];
						int j;
						for( j = 0; j < findface->m_pointnum; j++ ){
							if( otherindex == findface->m_index[j] ){
								//3角内に、線が含まれる場合は、ゴミデータと見なす。
								findface = 0;
								findi = -1;
								break;
							}
						}						
					}

					if( findface ){
						findi = i;
						break;
					}

				}

			}


			if( findface && (findi >= 0) ){
				if( findi == 0 ){
					curface->m_parentindex = curface->m_index[0];
					curface->m_childindex = curface->m_index[1];
				}else{
					curface->m_parentindex = curface->m_index[1];
					curface->m_childindex = curface->m_index[0];
				}
				curface->m_bonetype = MIKOBONE_FLOAT;

			}else{
				curface->m_bonetype = MIKOBONE_NONE;//!!!!!!!!!!!!!!!!!
			}
		}
	}
	
	return 0;
}

int CMQOObject::CheckSameMikoBone()
{

	int fno1, fno2;
	CMQOFace* face1;
	CMQOFace* face2;
	int issame;
	for( fno1 = 0; fno1 < m_face; fno1++ ){
		face1 = m_facebuf + fno1;

		for( fno2 = 0; fno2 < m_face; fno2++ ){
			face2 = m_facebuf + fno2;

			if( (fno1 != fno2) && ((face1->m_bonetype != MIKOBONE_NONE) && (face1->m_bonetype == face2->m_bonetype))){
				issame = IsSameMikoBone( face1, face2 );
				if( issame ){
					face2->m_bonetype = MIKOBONE_ILLEAGAL;
				}
			}else if( (face1->m_bonetype == MIKOBONE_NORMAL) && (face2->m_bonetype == MIKOBONE_FLOAT) ){
				int includecnt = 0;
				int i, j;
				for( i = 0; i < 2; i++ ){
					for( j = 0; j < 3; j++ ){
						if( face2->m_index[i] == face1->m_index[j] ){
							includecnt++;
						}
					}
				}
				if( includecnt >= 2 ){
					face2->m_bonetype = MIKOBONE_ILLEAGAL;
				}
			}
		}
	}

	return 0;
}

int CMQOObject::GetTopLevelMikoBone( CMQOFace** pptopface, int* topnumptr, int maxnum )
{
	*topnumptr = 0;

	int fno;
	CMQOFace* curface;
	CMQOFace* findface;

	for( fno = 0; fno < m_face; fno++ ){
		curface = m_facebuf + fno;

		if( curface->m_bonetype == MIKOBONE_NORMAL ){
			CallF( CheckFaceSameChildIndex( curface, curface->m_parentindex, &findface ), return 1 );		
			if( !findface ){
				if( *topnumptr >= maxnum ){
					DbgOut( L"mqoobject : GetTopLevelMikoBone : too many top level bone num error !!!\r\n" );
					_ASSERT( 0 );
					return 1;
				}
				*( pptopface + *topnumptr ) = curface;
				(*topnumptr)++;
			}
		}
	}

	return 0;
}

int CMQOObject::CheckFaceSameChildIndex( CMQOFace* srcface, int chkno, CMQOFace** ppfindface )
{
	*ppfindface = 0;

	int fno;
	CMQOFace* curface;

	for( fno = 0; fno < m_face; fno++ ){
		curface = m_facebuf + fno;

		if( (curface->m_bonetype == MIKOBONE_NORMAL) || (curface->m_bonetype == MIKOBONE_FLOAT) ){
			if( (curface->m_childindex == chkno) && (curface != srcface) ){
				*ppfindface = curface;
				break;
			}
		}
	}

	return 0;
}

int CMQOObject::SetTreeMikoBone( CMQOFace* srctopface )
{
	int childno;
	childno = srctopface->m_childindex;

	srctopface->m_dirtyflag = 1;//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!	

	CMQOFace* findface[MAXBONENUM];
	int findnum = 0;
	CallF( FindFaceSameParentIndex( srctopface, childno, findface, &findnum, MAXBONENUM ), return 1 );

	int dbgcnt = 0;
	int findno;

	for( findno = 0; findno < findnum; findno++ ){
		if( findno == 0 ){
			srctopface->m_child = findface[findno];
		}else{
			CMQOFace* lastbro = srctopface->m_child;
			CMQOFace* chkface = srctopface->m_child;
			while( chkface ){

				lastbro = chkface;
				chkface = chkface->m_brother;
		
				dbgcnt++;

				if( dbgcnt > 10000 ){
					DbgOut( L"mqoobject : SetTreeMikoBone : loop count error !!!\r\n" );
					_ASSERT( 0 );
					return 1;
				}
			}

			if( lastbro ){
				lastbro->m_brother = findface[findno];
			}
		}

		findface[findno]->m_parent = srctopface;

	}

///////////////
	for( findno = 0; findno < findnum; findno++ ){
		if( (findface[findno])->m_dirtyflag == 0 ){
			CallF( SetTreeMikoBone( findface[findno] ), return 1 );
		}
	}

	return 0;
}

int CMQOObject::FindFaceSameParentIndex( CMQOFace* srcface, int chkno, CMQOFace** ppfindface, int* findnum, int maxnum )
{
	*findnum = 0;

	int fno;
	CMQOFace* curface;

	for( fno = 0; fno < m_face; fno++ ){
		curface = m_facebuf + fno;

		if( (curface->m_bonetype == MIKOBONE_NORMAL) || (curface->m_bonetype == MIKOBONE_FLOAT) ){
			if( (chkno == curface->m_parentindex) && (curface != srcface) ){
				
				if( *findnum >= maxnum ){
					DbgOut( L"mqoobject : FindFaceSameParentIndex : maxnum too short error !!!\r\n" );
					_ASSERT( 0 );
					return 1;
				}
				*( ppfindface + *findnum ) = curface;
				(*findnum)++;
			}

		}
	}

	return 0;
}

int CMQOObject::CheckLoopedMikoBoneReq( CMQOFace* faceptr, int* isloopedptr, int* jointnumptr )
{

	if( (faceptr->m_bonetype == MIKOBONE_NORMAL) || (faceptr->m_bonetype == MIKOBONE_FLOAT) ){
		if( faceptr->m_dirtyflag == 1 ){
			(*isloopedptr)++;
		}

		faceptr->m_dirtyflag = 1;
		(*jointnumptr)++;
	}
	if( *isloopedptr > 0 ){
		return 0;
	}
	if( *jointnumptr > MAXBONENUM ){
		return 0;
	}
	
////////
	if( faceptr->m_child ){
		CheckLoopedMikoBoneReq( faceptr->m_child, isloopedptr, jointnumptr );
	}

	if( faceptr->m_brother ){
		CheckLoopedMikoBoneReq( faceptr->m_brother, isloopedptr, jointnumptr );
	}

	return 0;
}

int CMQOObject::SetMikoBoneName( map<int, CMQOMaterial*> &srcmaterial )
{
	int fno;
	CMQOFace* curface;
	int nameflag;

	char* nameptr;
	char* convnameptr;
	int lrflag;

	CMQOMaterial* curmat;
	for( fno = 0; fno < m_face; fno++ ){
		curface = m_facebuf + fno;

		if( curface->m_bonetype == MIKOBONE_NORMAL ){
			CallF( CheckMaterialSameName( curface->m_materialno, srcmaterial, &nameflag ), return 1 );
			curmat = GetMaterialFromNo( srcmaterial, curface->m_materialno );
			if( !curmat ){
				_ASSERT( 0 );
				return 1;
			}
			nameptr = curmat->name;

			switch( nameflag ){
			case 0:
				//重複無し
				CallF( curface->SetMikoBoneName( nameptr, 0 ), return 1 );
				break;
			case 1:
				//[]のＬＲ
				CallF( curface->CheckLRFlag( m_pointbuf, &lrflag ), return 1 );
				CallF( curface->SetMikoBoneName( nameptr, lrflag ), return 1 );
				break;
			case 2:
				//重複有り、[]以外
				CallF( curmat->AddConvName( &convnameptr ), return 1 );
				CallF( curface->SetMikoBoneName( convnameptr, 0 ), return 1 );
				break;
			case 3:
				DbgOut( L"mqoobject : SetMikoBoneName : unknown nameflag error !!!\r\n" );
				_ASSERT( 0 );
				return 1;
				break;
			}

		}
		
	}

	return 0;
}

int CMQOObject::SetMikoFloatBoneName()
{
	int fno;
	CMQOFace* curface;

	char tempname[256];

	for( fno = 0; fno < m_face; fno++ ){
		curface = m_facebuf + fno;

		if( curface->m_bonetype == MIKOBONE_FLOAT ){
			CMQOFace* chilface;
			chilface = curface->m_child;

			if( chilface ){
				sprintf_s( tempname, 256, "FLOAT_%s", chilface->m_bonename );
				CallF( curface->SetMikoBoneName( tempname, 0 ), return 1 );
			}else{
				sprintf_s( tempname, 256, "FLOAT_%d", curface->m_serialno );
				CallF( curface->SetMikoBoneName( tempname, 0 ), return 1 );
			}
		}
	}

	return 0;
}

int CMQOObject::CheckMaterialSameName( int srcmatno, map<int, CMQOMaterial*> &srcmaterial, int* nameflag )
{
	int fno;
	CMQOFace* curface;

	int samecnt = 0;
	int lrcnt = 0;

	char* nameptr;
	int leng;
	int cmp;

	for( fno = 0; fno < m_face; fno++ ){
		curface = m_facebuf + fno;

		if( curface->m_bonetype == MIKOBONE_NORMAL ){
			if( curface->m_materialno == srcmatno ){
				samecnt++;

				CMQOMaterial* curmat = GetMaterialFromNo( srcmaterial, srcmatno );
				if( !curmat ){
					DbgOut( L"mqoobject : CheckMaterkalSameName : material %d, name NULL error !!!\r\n", srcmatno );
					_ASSERT( 0 );
					return 1;
				}

				nameptr = curmat->name;

				leng = (int)strlen( nameptr );
				if( leng > 2 ){
					cmp = strcmp( nameptr + leng - 2, "[]" );
					if( cmp == 0 ){
						lrcnt++;
					}
				}

			}
		}
	}

	if( samecnt == 1 ){
		*nameflag = 0;
	}else if( (samecnt == 2) && (lrcnt == 2) ){
		*nameflag = 1;
	}else if( samecnt >= 2 ){
		*nameflag = 2;
	}else{
		*nameflag = 3;
	}

	return 0;
}

int CMQOObject::IsSameMikoBone( CMQOFace* face1, CMQOFace* face2 )
{
	if( face1->m_bonetype != face2->m_bonetype )
		return 0;

	int sameindex;
	sameindex = IsSameFaceIndex( face1, face2 );

	return sameindex;

}

int CMQOObject::IsSameFaceIndex( CMQOFace* face1, CMQOFace* face2 )
{

	if( face1->m_pointnum != face2->m_pointnum )
		return 0;


	int pointnum;
	pointnum = face1->m_pointnum;

	int samecnt = 0;
	int i, j;
	int index1, index2;
	for( i = 0; i < pointnum; i++ ){
		index1 = face1->m_index[i];

		for( j = 0; j < pointnum; j++ ){
			index2 = face2->m_index[j];

			if( index1 == index2 ){
				samecnt++;
			}
		}
	}

	if( samecnt >= pointnum )
		return 1;
	else
		return 0;

}

int CMQOObject::GetMaterialNoInUse( int* noptr, int arrayleng, int* getnumptr )
{
	*getnumptr = 0;

	if( m_face <= 0 ){
		return 0;
	}

	int findnum = 0;
	int findid[256];
	ZeroMemory( findid, sizeof( int ) * 256 );

	int fno;
	for( fno = 0; fno < m_face; fno++ ){
		CMQOFace* curface = m_facebuf + fno;
		int curmatno = curface->m_materialno;
		if( curmatno >= 0 ){
			int sameflag = 0;
			int chki;
			for( chki = 0; chki < findnum; chki++ ){
				if( findid[chki] == curmatno ){
					sameflag = 1;
					break;
				}
			}

			if( sameflag == 0 ){
				if( findnum >= 256 ){
					DbgOut( L"mqoobj : GetMaterialNoInUse : materialnum too many error !!!\r\n" );
					_ASSERT( 0 );
					return 1;
				}
				findid[findnum] = curmatno;
				findnum++;
			}
		}
	}

	if( noptr ){
		if( arrayleng < findnum ){
			_ASSERT( 0 );
			return 1;
		}
		MoveMemory( noptr, findid, sizeof( int ) * findnum );
	}

	*getnumptr = findnum;

	return 0;
}

int CMQOObject::GetFaceInMaterial( int matno, CMQOFace** ppface, int arrayleng, int* getnumptr )
{
	*getnumptr = 0;
	if( m_face < 0 ){
		return 0;
	}

	int findnum = 0;

	int fno;
	for( fno = 0; fno < m_face; fno++ ){
		CMQOFace* curface = m_facebuf + fno;
		int curmatno = curface->m_materialno;

		if( curmatno == matno ){
			if( ppface ){
				if( arrayleng < findnum ){
					_ASSERT( 0 );
					return 1;
				}
				*( ppface + findnum ) = curface;
			}
			findnum++;
		}
	}

	*getnumptr = findnum;

	return 0;
}

int CMQOObject::CollisionLocal_Ray( D3DXVECTOR3 startlocal, D3DXVECTOR3 dirlocal )
{
	int face_count;
	int vert_count;
	D3DXVECTOR3* pointptr;
	CMQOFace* faceptr;

	if( m_face2 > 0 ){
		face_count = m_face2;
		faceptr = m_facebuf2;
	}else{
		face_count = m_face;
		faceptr = m_facebuf;
	}
	if( m_vertex2 > 0 ){
		vert_count = m_vertex2;
		pointptr = m_pointbuf2;
	}else{
		vert_count = m_vertex;
		pointptr = m_pointbuf;
	}

	int allowrev = 0;

	int fno;
	int hitflag;
	int justflag;
	for( fno = 0; fno < face_count; fno++ ){
		hitflag = 0;
		justflag = 0;
		CMQOFace* curface = faceptr + fno;
		if( curface->m_pointnum == 3 ){
			hitflag = ChkRay( allowrev, curface->m_index[0], curface->m_index[1], curface->m_index[2], pointptr, startlocal, dirlocal, 0.01f, &justflag );
			if( hitflag || justflag ){
				return 1;
			}
		}else if( curface->m_pointnum == 4 ){
			hitflag = ChkRay( allowrev, curface->m_index[0], curface->m_index[1], curface->m_index[2], pointptr, startlocal, dirlocal, 0.01f, &justflag );
			if( hitflag || justflag ){
				return 1;
			}
			hitflag = ChkRay( allowrev, curface->m_index[0], curface->m_index[2], curface->m_index[3], pointptr, startlocal, dirlocal, 0.01f, &justflag );
			if( hitflag || justflag ){
				return 1;
			}
		}
	}
	
	return 0;
}


int CMQOObject::MakeXBoneno2wno( int arrayleng, int* boneno2wno, int* bonenumptr )
{
	int bcnt;
	for( bcnt = 0; bcnt < arrayleng; bcnt++ ){
		*(boneno2wno + bcnt) = -1;
	}
	*bonenumptr = 0;

	if( !m_pm3 ){
		_ASSERT( 0 );
		return 1;
	}

	int infvnum = m_pm3->m_orgpointnum;
	if( !m_pm3->m_infbone ){
		_ASSERT( 0 );
		return 1;
	}
	
	int* dirtyflag;
	dirtyflag = (int*)malloc( sizeof( int ) * arrayleng );
	if( !dirtyflag ){
		_ASSERT( 0 );
		return 1;
	}
	ZeroMemory( dirtyflag, sizeof( int ) * arrayleng );


	int vno, infno, boneno;
	CInfBone* curib;
	INFELEM IE;
	for( vno = 0; vno < infvnum; vno++ ){
		curib = m_pm3->m_infbone + vno;
		
		for( infno = 0; infno < curib->m_infnum; infno++ ){
			IE = curib->m_infelem[infno];
			boneno = IE.boneno;
			if( boneno >= 0 )
				*(dirtyflag + boneno) = 1;
		}
	}

	int setno = 0;
	for( boneno = 0; boneno < arrayleng; boneno++ ){
		if( *(dirtyflag + boneno) == 1 ){

			*(boneno2wno + boneno) = setno;
			setno++;
		}else{
			*(boneno2wno + boneno) = -1;
		}
	}
	*bonenumptr = setno;

	free( dirtyflag );

	return 0;
}

int CMQOObject::GetSkinMeshHeader( int leng, int* maxpervert, int* maxperface )
{
	*maxpervert = 0;
	*maxperface = 0;

	if( !m_pm3 ){
		_ASSERT( 0 );
		return 1;
	}
	if( !m_pm3->m_dispindex ){
		_ASSERT( 0 );
		return 1;
	}
	if( !m_pm3->m_infbone ){
		_ASSERT( 0 );
		return 1;
	}

	int* dirtyflag;
	dirtyflag = (int*)malloc( sizeof( int ) * leng );
	if( !dirtyflag ){
		_ASSERT( 0 );
		return 1;
	}


	int pervert = 0;
	int perface = 0;

	int faceno;
	int i[3];
	for( faceno = 0; faceno < m_pm3->m_facenum; faceno++ ){
		i[0] = *(m_pm3->m_dispindex + faceno * 3);
		i[1] = *(m_pm3->m_dispindex + faceno * 3 + 1);
		i[2] = *(m_pm3->m_dispindex + faceno * 3 + 2);

		ZeroMemory( dirtyflag, sizeof( int ) * leng );

		int index;
		for( index = 0; index < 3; index++ ){
			CInfBone* curib;
			int orgvno = (m_pm3->m_n3p + i[index])->pervert->vno;
			curib = m_pm3->m_infbone + orgvno;

			int infnum;
			infnum = curib->m_infnum;

			if( infnum >= pervert ){
				pervert = infnum;
			}


			int infno;
			for( infno = 0; infno < infnum; infno++ ){
				INFELEM curIE;
				curIE = curib->m_infelem[infno];
				
				if( curIE.boneno >= 0 ){
					*(dirtyflag + curIE.boneno) = 1;
				}
			}

		}

		int tmpcnt = 0;
		int bno;
		for( bno = 0; bno < leng; bno++ ){
			if( *(dirtyflag + bno) == 1 ){
				tmpcnt++;
			}
		}

		if( tmpcnt > perface ){
			perface = tmpcnt;
		}

	}

	*maxpervert = pervert;
	*maxperface = perface;

	free( dirtyflag );

	return 0;
}

int CMQOObject::MakeXBoneInfluence( map<int, CBone*>& bonelist, int arrayleng, int bonenum, int* boneno2wno, BONEINFLUENCE* biptr )
{
	if( !m_pm3 ){
		_ASSERT( 0 );
		return 1;
	}
	if( !m_pm3->m_infbone ){
		_ASSERT( 0 );
		return 1;
	}

	CInfBone* ibptr = m_pm3->m_infbone;
	int meshvnum = m_pm3->m_optleng;

	int curserino;
	int curboneno;
	BONEINFLUENCE* curbi;
	DWORD infnum;
	map<int, CBone*>::iterator itrbone;
	int bonecnt = 0;
	for( itrbone = bonelist.begin(); itrbone != bonelist.end(); itrbone++ ){
		CBone* curbone = itrbone->second;
		_ASSERT( curbone );
		curserino = curbone->m_boneno;
		curboneno = *(boneno2wno + curserino);
		_ASSERT( curboneno < (int)bonelist.size() );
		curbi = biptr + bonecnt;
		
		if( curboneno >= 0 ){
			

	//// bonenoの影響を受ける頂点の数を取得。
			CallF( SetXInfluenceArray( ibptr, meshvnum, curserino, 0, 0, 0, &infnum ), return 1 );

			DWORD* vertices;
			float* weights;

			if( infnum > 0 ){

			//// 配列作成


				vertices = (DWORD*)malloc( sizeof( DWORD ) * infnum );
				if( !vertices ){
					_ASSERT( 0 );
					return 1;
				}
				weights = (float*)malloc( sizeof( float ) * infnum );
				if( !weights ){
					_ASSERT( 0 );
					free( vertices );
					return 1;
				}

				//// 情報セット
				DWORD setnum;
				CallF( SetXInfluenceArray( ibptr, meshvnum, curserino, vertices, weights, infnum, &setnum ), return 1 );
			}else{

				vertices = (DWORD*)malloc( sizeof( DWORD ) * 1 );
				if( !vertices ){
					_ASSERT( 0 );
					return 1;
				}
				weights = (float*)malloc( sizeof( float ) * 1 );
				if( !weights ){
					_ASSERT( 0 );
					free( vertices );
					return 1;
				}

				*vertices = 0;
				*weights = 0;
			}


		/// boneinfluenceにセット
			curbi->Bone = curboneno;
			curbi->numInfluences = infnum;

			curbi->vertices = vertices;
			curbi->weights = weights;

	
		}else{
			curbi->Bone = -1;
			curbi->numInfluences = 0;
			curbi->vertices = NULL;
			curbi->weights = NULL;
		}
		bonecnt++;
	}	

	return 0;
}


int CMQOObject::SetXInfluenceArray( CInfBone* ibptr, int vnum, int boneserino, DWORD* vertices, float* weights, int infnum, DWORD* setnumptr )
{
	int mode;

	if( vertices ){
		mode = 1;// 情報をセットする。
	}else{
		mode = 0;// setnumをカウントするだけ。
	}

	*setnumptr = 0;


	int setno = 0;
	int n3vno, orgvno, infno;
	CInfBone* curib;
	INFELEM* IE;
	for( n3vno = 0; n3vno < vnum; n3vno++ ){
		orgvno = (m_pm3->m_n3p + n3vno)->pervert->vno;
		_ASSERT( orgvno >= 0 );
		_ASSERT( orgvno < m_pm3->m_orgpointnum );
		curib = ibptr + orgvno;

		for( infno = 0; infno < curib->m_infnum; infno++ ){
			_ASSERT( (infno >= 0) && (infno < 4) );
			IE = curib->m_infelem + infno;

			if( IE->boneno == boneserino ){
				if( mode == 1 ){
					if( setno >= infnum ){
						_ASSERT( 0 );
						return 1;
					}
					*(vertices + setno) = (DWORD)n3vno;
					*(weights + setno) = IE->dispinf;
				}
				setno++;
			}

		}
	}

	*setnumptr = setno;

	return 0;
}

int CMQOObject::UpdateMorph( LPDIRECT3DDEVICE9 pdev, int hasbone, int srcmotid, double srcframe, map<int, CMQOMaterial*>& srcmaterial )
{
	if( !m_pm3 ){
		return 0;
	}

	CMorphKey curmk( this );
	CallF( CalcCurMorphKey( srcmotid, srcframe, &curmk ), return 1 );
	CallF( CalcMorph( &curmk, srcmaterial ), return 1 );


	if( !m_dispobj ){
		_ASSERT( 0 );
		return 1;
	}
	CallF( m_dispobj->CopyDispV( m_pm3 ), return 1 );

	return 0;
}

int CMQOObject::CalcCurMorphKey( int srcmotid, double srcframe, CMorphKey* dstmk )
{

	map<int, CMQOObject*>::iterator itrtarget;
	for( itrtarget = m_morphobj.begin(); itrtarget != m_morphobj.end(); itrtarget++ ){
		CMQOObject* curtarget = itrtarget->second;
		if( curtarget ){
			CMorphKey* pbef = 0;
			CMorphKey* pnext = 0;
			int existflag = 0;
			CallF( GetBefNextMK( srcmotid, srcframe, curtarget, &pbef, &pnext, &existflag ), return 1 );
			CallF( CalcFrameMK( curtarget, srcframe, pbef, pnext, existflag, dstmk ), return 1 );
		}
	}


	return 0;
}



int CMQOObject::GetBefNextMK( int srcmotid, double srcframe, CMQOObject* srctarget, CMorphKey** ppbef, CMorphKey** ppnext, int* existptr )
{
	*existptr = 0;

	CMorphKey* pbef = 0;
	CMorphKey* pcur = m_morphkey[srcmotid];

	if( srctarget ){
		while( pcur ){
			map<CMQOObject*, float>::iterator itrbw;
			itrbw = pcur->m_blendweight.find( srctarget );

			if( (itrbw != pcur->m_blendweight.end()) && (pcur->m_frame >= srcframe - TIME_ERROR_WIDTH) && (pcur->m_frame <= srcframe + TIME_ERROR_WIDTH) ){
				*existptr = 1;
				pbef = pcur;
				break;
			}else if( (itrbw != pcur->m_blendweight.end()) && (pcur->m_frame > srcframe) ){
				break;
			}else{
				if( itrbw != pcur->m_blendweight.end() ){
					pbef = pcur;
				}
				pcur = pcur->m_next;
			}
		}
	}else{
		while( pcur ){
			if( (pcur->m_frame >= srcframe - TIME_ERROR_WIDTH) && (pcur->m_frame <= srcframe + TIME_ERROR_WIDTH) ){
				*existptr = 1;
				pbef = pcur;
				break;
			}else if( pcur->m_frame > srcframe ){
				break;
			}else{
				pbef = pcur;
				pcur = pcur->m_next;
			}
		}
	}
	*ppbef = pbef;


	if( *existptr ){
		*ppnext = pbef->m_next;
	}else{
		*ppnext = pcur;
	}


	return 0;
}
int CMQOObject::CalcFrameMK( CMQOObject* srctarget, double srcframe, CMorphKey* befptr, CMorphKey* nextptr, int existflag, CMorphKey* dstmkptr )
{
	if( existflag == 1 ){
		if( srctarget ){
			dstmkptr->m_blendweight[srctarget] = befptr->m_blendweight[srctarget];
			dstmkptr->m_prev = befptr;
			dstmkptr->m_next = nextptr;
		}else{
			*dstmkptr = *befptr;
		}
		return 0;
	}else if( !befptr ){
		if( srctarget ){
			dstmkptr->m_blendweight[srctarget] = 0.0f;
			dstmkptr->m_prev = befptr;
			dstmkptr->m_next = nextptr;
		}else{
			dstmkptr->InitMotion();
		}
		dstmkptr->m_frame = srcframe;
		return 0;
	}else if( !nextptr ){
		if( srctarget ){
			dstmkptr->m_blendweight[srctarget] = befptr->m_blendweight[srctarget];
			dstmkptr->m_prev = befptr;
			dstmkptr->m_next = nextptr;
		}else{
			*dstmkptr = *befptr;
		}
		dstmkptr->m_frame = srcframe;
		return 0;
	}else{
		double diffframe = nextptr->m_frame - befptr->m_frame;
		_ASSERT( diffframe != 0.0 );
		double t = ( srcframe - befptr->m_frame ) / diffframe;

		float calcw;
		calcw = befptr->m_blendweight[ srctarget ] + ( nextptr->m_blendweight[ srctarget ] - befptr->m_blendweight[ srctarget ] ) * (float)t;

		CallF( dstmkptr->SetBlendWeight( srctarget, calcw ), return 1 );

		dstmkptr->m_prev = befptr;
		dstmkptr->m_next = nextptr;
		return 0;
	}
}

int CMQOObject::CalcMorph( CMorphKey* curkey, map<int, CMQOMaterial*>& srcmaterial )
{
	//モーフのブレンドを計算し、結果をbasepm3->m_dispvに格納する。

	CPolyMesh3* basepm3 = m_pm3;

	if( !m_pm3->m_dispv ){
		_ASSERT( 0 );
		return 1;
	}

	//dispvの初期化
	int baseoptv;
	for( baseoptv = 0; baseoptv < basepm3->m_optleng; baseoptv++ ){
		PM3DISPV* curv = basepm3->m_dispv + baseoptv;
		N3P* curn3p = basepm3->m_n3p + baseoptv;
		curv->pos.x = (m_pointbuf + curn3p->pervert->vno)->x;
		curv->pos.y = (m_pointbuf + curn3p->pervert->vno)->y;
		curv->pos.z = (m_pointbuf + curn3p->pervert->vno)->z;
		curv->pos.w = 1.0f;
		curv->normal = curn3p->pervert->smnormal;
		curv->uv = curn3p->pervert->uv[0];
	}

	int matcnt;
	for( matcnt = 0; matcnt < basepm3->m_optmatnum; matcnt++ ){
		CMQOMaterial* curmat = (basepm3->m_matblock + matcnt)->mqomat;
		map<int,CMQOMaterial*>::iterator itrmat;
		itrmat = m_morphmaterial.find( curmat->materialno );

		CMQOMaterial* dstmat = 0;
		if( itrmat == m_morphmaterial.end() ){
			dstmat = new CMQOMaterial();
			_ASSERT( dstmat );
			*dstmat = *curmat;
			m_morphmaterial[ dstmat->materialno ] = dstmat;
		}else{
			dstmat = itrmat->second;
			*dstmat = *curmat;
		}
	}

	map<int, CMQOObject*>::iterator itrtarget;
	for( itrtarget = m_morphobj.begin(); itrtarget != m_morphobj.end(); itrtarget++ ){
		CMQOObject* curtarget = itrtarget->second;
		if( curtarget ){
			CPolyMesh3* targetpm3 = curtarget->m_pm3;

			map<CMQOObject*, float>::iterator itrbw;
			itrbw = curkey->m_blendweight.find( curtarget );

			float blendw;
			if( itrbw == curkey->m_blendweight.end() ){
				blendw = 0.0f;
			}else{
				blendw = itrbw->second;
			}

			if( blendw == 0.0f ){
				continue;//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
			}


			for( matcnt = 0; matcnt < basepm3->m_optmatnum; matcnt++ ){
				CMQOMaterial* basematerial = (basepm3->m_matblock + matcnt)->mqomat;

				int facecnt = 0;
				int basefaceno;
				for( basefaceno = (basepm3->m_matblock + matcnt)->startface; basefaceno <= (basepm3->m_matblock + matcnt)->endface; basefaceno++ ){
					CMQOMaterial* targetmaterial;
					int baseorgfaceno;
					int targetfaceno;
					baseorgfaceno = (basepm3->m_n3p + basefaceno * 3)->perface->faceno;
					targetfaceno = targetpm3->m_faceno2optfaceno[ baseorgfaceno ];

					N3P* basen3p = basepm3->m_n3p + basefaceno * 3;
					N3P* targetn3p = targetpm3->m_n3p + targetfaceno * 3;
					//N3P* targetn3p = targetpm3->m_n3p + basefaceno * 3;

					if( facecnt == 0 ){
						targetmaterial = GetMaterialFromNo( srcmaterial, targetn3p->perface->materialno );
						if( !targetmaterial ){
							_ASSERT( 0 );
							return 1;
						}

						CMQOMaterial* setmat = m_morphmaterial[ basematerial->materialno ];
						if( !setmat ){
							_ASSERT( 0 );
							return 1;
						}

						CMQOMaterial tmptargetmat = *targetmaterial * blendw;
						CMQOMaterial tmpbasemat = *basematerial * blendw;

						setmat->col.x += tmptargetmat.col.x - tmpbasemat.col.x;
						setmat->col.y += tmptargetmat.col.y - tmpbasemat.col.y;
						setmat->col.z += tmptargetmat.col.z - tmpbasemat.col.z;
						setmat->col.w += tmptargetmat.col.w - tmpbasemat.col.w;
						setmat->dif += tmptargetmat.dif - tmpbasemat.dif;
						setmat->amb += tmptargetmat.amb - tmpbasemat.amb;
						setmat->emi += tmptargetmat.emi - tmpbasemat.emi;
						setmat->spc += tmptargetmat.spc - tmpbasemat.spc;
						setmat->power += tmptargetmat.power - tmpbasemat.power;

						setmat->dif4f.x += tmptargetmat.dif4f.x - tmpbasemat.dif4f.x;
						setmat->dif4f.y += tmptargetmat.dif4f.y - tmpbasemat.dif4f.y;
						setmat->dif4f.z += tmptargetmat.dif4f.z - tmpbasemat.dif4f.z;
						setmat->dif4f.w += tmptargetmat.dif4f.w - tmpbasemat.dif4f.w;
						setmat->amb3f.x += tmptargetmat.amb3f.x - tmpbasemat.amb3f.x;
						setmat->amb3f.y += tmptargetmat.amb3f.y - tmpbasemat.amb3f.y;
						setmat->amb3f.z += tmptargetmat.amb3f.z - tmpbasemat.amb3f.z;
						setmat->emi3f.x += tmptargetmat.emi3f.x - tmpbasemat.emi3f.x;
						setmat->emi3f.y += tmptargetmat.emi3f.y - tmpbasemat.emi3f.y;
						setmat->emi3f.z += tmptargetmat.emi3f.z - tmpbasemat.emi3f.z;
						setmat->spc3f.x += tmptargetmat.spc3f.x - tmpbasemat.spc3f.x;
						setmat->spc3f.y += tmptargetmat.spc3f.y - tmpbasemat.spc3f.y;
						setmat->spc3f.z += tmptargetmat.spc3f.z - tmpbasemat.spc3f.z;
						setmat->sceneamb.x += tmptargetmat.sceneamb.x - tmpbasemat.sceneamb.x;
						setmat->sceneamb.y += tmptargetmat.sceneamb.y - tmpbasemat.sceneamb.y;
						setmat->sceneamb.z += tmptargetmat.sceneamb.z - tmpbasemat.sceneamb.z;
						setmat->sceneamb.w += tmptargetmat.sceneamb.w - tmpbasemat.sceneamb.w;

						setmat->glowmult[0] += tmptargetmat.glowmult[0]- tmpbasemat.glowmult[0];
						setmat->glowmult[1] += tmptargetmat.glowmult[1]- tmpbasemat.glowmult[1];
						setmat->glowmult[2] += tmptargetmat.glowmult[2]- tmpbasemat.glowmult[2];
					}

					PM3DISPV* dstdispv = basepm3->m_dispv + basefaceno * 3;

					int fcnt;
					for( fcnt = 0; fcnt < 3; fcnt++ ){
						(dstdispv + fcnt)->pos.x += ( (targetpm3->m_pointbuf + (targetn3p + fcnt)->pervert->vno)->x - (basepm3->m_pointbuf + (basen3p + fcnt)->pervert->vno)->x ) * blendw;
						(dstdispv + fcnt)->pos.y += ( (targetpm3->m_pointbuf + (targetn3p + fcnt)->pervert->vno)->y - (basepm3->m_pointbuf + (basen3p + fcnt)->pervert->vno)->y ) * blendw;
						(dstdispv + fcnt)->pos.z += ( (targetpm3->m_pointbuf + (targetn3p + fcnt)->pervert->vno)->z - (basepm3->m_pointbuf + (basen3p + fcnt)->pervert->vno)->z ) * blendw;

						(dstdispv + fcnt)->normal += ( (targetn3p + fcnt)->pervert->smnormal - (basen3p + fcnt)->pervert->smnormal ) * blendw;
						(dstdispv + fcnt)->uv += ( (targetn3p + fcnt)->pervert->uv[0] - (basen3p + fcnt)->pervert->uv[0] ) * blendw;
					}

					facecnt++;
				}
			}
		}
	}

/***
	static int dbgcnt = 0;
	if( dbgcnt == 0 ){
		int prcnt = 0;
		map<int,CMQOMaterial*>::iterator itrmmat;
		for( itrmmat = m_morphmaterial.begin(); itrmmat != m_morphmaterial.end(); itrmmat++ ){
			CMQOMaterial* curmat = itrmmat->second;
			WCHAR wmname[256];
			MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, curmat->name, 256, wmname, 256 );
			WCHAR woname[256];
			MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, m_name, 256, woname, 256 );

			DbgOut( L"check mmat!!!: object %s, material cnt%d, matno %d, matname %s\r\n",
				woname, prcnt, curmat->materialno, wmname );

			prcnt++;
		}
	}
	dbgcnt++;
***/

	return 0;
}

int CMQOObject::DeleteMorphMotion( int srcmotid )
{
	if( srcmotid < 0 ){
		map<int, CMorphKey*>::iterator itrmk;
		for( itrmk = m_morphkey.begin(); itrmk != m_morphkey.end(); itrmk++ ){
			int curmotid = itrmk->first;
			CallF( DeleteMorphMotion( curmotid ), return 1 );
		}
		return 0;
	}else{
		map<int, CMorphKey*>::iterator itrfirstmk;
		itrfirstmk = m_morphkey.find( srcmotid );
		if( itrfirstmk != m_morphkey.end() ){
			CMorphKey* curmk = itrfirstmk->second;
			while( curmk ){
				CMorphKey* nextmk = curmk->m_next;
				delete curmk;
				curmk = nextmk;
			}
		}
		m_morphkey.erase( itrfirstmk );
	}

	return 0;
}

int CMQOObject::DeleteMKOutOfRange( int motid, double srcleng )
{
	CMorphKey* curmk = m_morphkey[ motid ];

	while( curmk ){
		CMorphKey* nextmk = curmk->m_next;

		if( curmk->m_frame > srcleng ){
			curmk->LeaveFromChain( motid, this );
			delete curmk;
		}
		curmk = nextmk;
	}

	return 0;
}

CMorphKey* CMQOObject::AddMorphKey( int srcmotid, double srcframe, int* existptr )
{
	*existptr = 0;

	CMorphKey* newmk = 0;
	CMorphKey* pbef = 0;
	CMorphKey* pnext = 0;
	CallF( GetBefNextMK( srcmotid, srcframe, 0, &pbef, &pnext, existptr ), return 0 );

	if( *existptr ){
		pbef->InitMotion();
		pbef->m_frame = srcframe;
		newmk = pbef;
	}else{
		newmk = new CMorphKey( this );
		if( !newmk ){
			_ASSERT( 0 );
			return 0;
		}
		newmk->m_frame = srcframe;

		if( pbef ){
			CallF( pbef->AddToNext( newmk ), return 0 );
		}else{
			m_morphkey[srcmotid] = newmk;
			if( pnext ){
				newmk->m_next = pnext;
			}
		}
	}

	return newmk;
}

int CMQOObject::OrderMorphKey( int srcmotid, double settime, CMorphKey* putmk, int samedel )
{
//	putmp->LeaveFromChain( srcmotid, this );//<---呼び出し前に処理しておく。(複数いっぺんに処理する場合があるので外でやる。)
	CMorphKey* pbef = 0;
	CMorphKey* pnext = 0;
	int existflag = 0;
	GetBefNextMK( srcmotid, settime, 0, &pbef, &pnext, &existflag );

	while( (samedel == 1) && (existflag == 1) ){
		pbef->LeaveFromChain( srcmotid, this );
		delete pbef;

		pbef = 0;
		pnext = 0;
		existflag = 0;
		GetBefNextMK( srcmotid, settime, 0, &pbef, &pnext, &existflag );
	}

	putmk->m_frame = settime;

	if( pbef ){
		CallF( pbef->AddToNext( putmk ), return 0 );
	}else{
		m_morphkey[srcmotid] = putmk;
		if( pnext ){
			putmk->m_next = pnext;
		}
	}

	return 0;
}

int CMQOObject::SetMorphBlendFU( int srcmotid, double srcframe, CMorphKey* srcmk, CMQOObject* srctarget, float* dstrate )
{
	CMorphKey* pbef = 0;
	CMorphKey* pnext = 0;
	int existflag = 0;
	GetBefNextMK( srcmotid, srcframe, srctarget, &pbef, &pnext, &existflag );
	if( existflag ){
		pbef->DeleteBlendWeight( srctarget );
		pbef = 0;
		pnext = 0;
		existflag = 0;
		GetBefNextMK( srcmotid, srcframe, srctarget, &pbef, &pnext, &existflag );
		_ASSERT( existflag == 0 );
	}


	CMorphKey calcmk( this );
	CallF( CalcFrameMK( srctarget, srcframe, pbef, pnext, existflag, &calcmk ), return 1 );
	float calcval = calcmk.m_blendweight[ srctarget ];
	CallF( srcmk->SetBlendWeight( srctarget, calcval ), return 1 );
	*dstrate = calcval;

	return 0;
}

int CMQOObject::MakeFirstMK( int srcmotid )
{
	int existflag = 0;
	CMorphKey* newmk = AddMorphKey( srcmotid, 0.0, &existflag );

	if( !newmk ){
		_ASSERT( 0 );
		return 1;
	}

	map<int, CMQOObject*>::iterator itrtarget;
	for( itrtarget = m_morphobj.begin(); itrtarget != m_morphobj.end(); itrtarget++ ){
		CMQOObject* curtarget = itrtarget->second;
		if( curtarget ){
			CallF( newmk->SetBlendWeight( curtarget, 0.0f ), return 1 );
		}
	}

	return 0;
}

int CMQOObject::DestroyMorphKey( int srcmotid )
{
	CMorphKey* curmk = m_morphkey[ srcmotid ];
	while( curmk ){
		CMorphKey* nextmk = curmk->m_next;
		delete curmk;
		curmk = nextmk;
	}

	m_morphkey[ srcmotid ] = NULL;

	return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <malloc.h>
#include <memory.h>

#include <windows.h>

#include <XFile.h>

#define	DBGH
#include <dbg.h>

#include <crtdbg.h>
#include <coef.h>

#include <Model.h>
#include <MQOObject.h>
#include <Bone.h>
#include <PolyMesh3.h>
#include <MQOMaterial.h>

#define XFILELINELEN 4098
#define MAXKEYFRAMENUM 36001

/////////////

static int s_engmotno = 0;

static char xfileheader1[XFILELINELEN] = "xof 0303txt 0032\r\n\
template AnimTicksPerSecond {\r\n\
 <9e415a43-7ba6-4a73-8743-b73d47e88476>\r\n\
 DWORD AnimTicksPerSecond;\r\n\
}\r\n\
\r\n\
template Frame {\r\n\
 <3d82ab46-62da-11cf-ab39-0020af71e433>\r\n\
 [...]\r\n\
}\r\n\
\r\n\
template Matrix4x4 {\r\n\
 <f6f23f45-7686-11cf-8f52-0040333594a3>\r\n\
 array FLOAT matrix[16];\r\n\
}\r\n\
\r\n\
template FrameTransformMatrix {\r\n\
 <f6f23f41-7686-11cf-8f52-0040333594a3>\r\n\
 Matrix4x4 frameMatrix;\r\n\
}\r\n\
\r\n\
template Vector {\r\n\
 <3d82ab5e-62da-11cf-ab39-0020af71e433>\r\n\
 FLOAT x;\r\n\
 FLOAT y;\r\n\
 FLOAT z;\r\n\
}\r\n\
\r\n\
template MeshFace {\r\n\
 <3d82ab5f-62da-11cf-ab39-0020af71e433>\r\n\
 DWORD nFaceVertexIndices;\r\n\
 array DWORD faceVertexIndices[nFaceVertexIndices];\r\n\
}\r\n\
\r\n\
template Mesh {\r\n\
 <3d82ab44-62da-11cf-ab39-0020af71e433>\r\n\
 DWORD nVertices;\r\n\
 array Vector vertices[nVertices];\r\n\
 DWORD nFaces;\r\n\
 array MeshFace faces[nFaces];\r\n\
 [...]\r\n\
}\r\n\
\r\n";

static char xfileheader2[XFILELINELEN] = "template MeshNormals {\r\n\
 <f6f23f43-7686-11cf-8f52-0040333594a3>\r\n\
 DWORD nNormals;\r\n\
 array Vector normals[nNormals];\r\n\
 DWORD nFaceNormals;\r\n\
 array MeshFace faceNormals[nFaceNormals];\r\n\
}\r\n\
\r\n\
template Coords2d {\r\n\
 <f6f23f44-7686-11cf-8f52-0040333594a3>\r\n\
 FLOAT u;\r\n\
 FLOAT v;\r\n\
}\r\n\
\r\n\
template MeshTextureCoords {\r\n\
 <f6f23f40-7686-11cf-8f52-0040333594a3>\r\n\
 DWORD nTextureCoords;\r\n\
 array Coords2d textureCoords[nTextureCoords];\r\n\
}\r\n\
\r\n\
template ColorRGBA {\r\n\
 <35ff44e0-6c7c-11cf-8f52-0040333594a3>\r\n\
 FLOAT red;\r\n\
 FLOAT green;\r\n\
 FLOAT blue;\r\n\
 FLOAT alpha;\r\n\
}\r\n\
\r\n\
template IndexedColor {\r\n\
 <1630b820-7842-11cf-8f52-0040333594a3>\r\n\
 DWORD index;\r\n\
 ColorRGBA indexColor;\r\n\
}\r\n\
\r\n\
template MeshVertexColors {\r\n\
 <1630b821-7842-11cf-8f52-0040333594a3>\r\n\
 DWORD nVertexColors;\r\n\
 array IndexedColor vertexColors[nVertexColors];\r\n\
}\r\n\
\r\n\
template ColorRGB {\r\n\
 <d3e16e81-7835-11cf-8f52-0040333594a3>\r\n\
 FLOAT red;\r\n\
 FLOAT green;\r\n\
 FLOAT blue;\r\n\
}\r\n\
\r\n\
template Material {\r\n\
 <3d82ab4d-62da-11cf-ab39-0020af71e433>\r\n\
 ColorRGBA faceColor;\r\n\
 FLOAT power;\r\n\
 ColorRGB specularColor;\r\n\
 ColorRGB emissiveColor;\r\n\
 [...]\r\n\
}\r\n\
\r\n";

static char xfileheader3[XFILELINELEN] = "template MeshMaterialList {\r\n\
 <f6f23f42-7686-11cf-8f52-0040333594a3>\r\n\
 DWORD nMaterials;\r\n\
 DWORD nFaceIndexes;\r\n\
 array DWORD faceIndexes[nFaceIndexes];\r\n\
 [Material <3d82ab4d-62da-11cf-ab39-0020af71e433>]\r\n\
}\r\n\
\r\n\
template VertexDuplicationIndices {\r\n\
 <b8d65549-d7c9-4995-89cf-53a9a8b031e3>\r\n\
 DWORD nIndices;\r\n\
 DWORD nOriginalVertices;\r\n\
 array DWORD indices[nIndices];\r\n\
}\r\n\
\r\n\
template XSkinMeshHeader {\r\n\
 <3cf169ce-ff7c-44ab-93c0-f78f62d172e2>\r\n\
 WORD nMaxSkinWeightsPerVertex;\r\n\
 WORD nMaxSkinWeightsPerFace;\r\n\
 WORD nBones;\r\n\
}\r\n\
\r\n\
template SkinWeights {\r\n\
 <6f0d123b-bad2-4167-a0d0-80224f25fabb>\r\n\
 STRING transformNodeName;\r\n\
 DWORD nWeights;\r\n\
 array DWORD vertexIndices[nWeights];\r\n\
 array FLOAT weights[nWeights];\r\n\
 Matrix4x4 matrixOffset;\r\n\
}\r\n\
\r\n\
template RDB2ExtInfo1 {\r\n\
 <10b97487-12b6-41f9-896f-af119248bd3c>\r\n\
 DWORD StringNum;\r\n\
 array STRING InfoString[StringNum];\r\n\
}\r\n\
\r\n\
template TextureFilename {\r\n\
 <a42790e1-7810-11cf-8f52-0040333594a3>\r\n\
 STRING filename;\r\n\
}\r\n\
\r\n";

static char xfileheader4[XFILELINELEN] = "template Animation {\r\n\
 <3d82ab4f-62da-11cf-ab39-0020af71e433>\r\n\
 [...]\r\n\
}\r\n\
\r\n\
template AnimationSet {\r\n\
 <3d82ab50-62da-11cf-ab39-0020af71e433>\r\n\
 [Animation <3d82ab4f-62da-11cf-ab39-0020af71e433>]\r\n\
}\r\n\
\r\n\
template FloatKeys {\r\n\
 <10dd46a9-775b-11cf-8f52-0040333594a3>\r\n\
 DWORD nValues;\r\n\
 array FLOAT values[nValues];\r\n\
}\r\n\
\r\n\
template TimedFloatKeys {\r\n\
 <f406b180-7b3b-11cf-8f52-0040333594a3>\r\n\
 DWORD time;\r\n\
 FloatKeys tfkeys;\r\n\
}\r\n\
\r\n\
template AnimationKey {\r\n\
 <10dd46a8-775b-11cf-8f52-0040333594a3>\r\n\
 DWORD keyType;\r\n\
 DWORD nKeys;\r\n\
 array TimedFloatKeys keys[nKeys];\r\n\
}\r\n\
\r\n\
\r\n\
AnimTicksPerSecond {\r\n\
 60;\r\n\
}\r\n";



CXFile::CXFile()
{
	m_model = 0;
	m_mult = 1.0f;
	m_hfile = INVALID_HANDLE_VALUE;
}

CXFile::~CXFile()
{
	CloseFile();
}

int CXFile::CloseFile()
{
	if( m_hfile != INVALID_HANDLE_VALUE ){
		FlushFileBuffers( m_hfile );
		SetEndOfFile( m_hfile );
		CloseHandle( m_hfile );
		m_hfile = INVALID_HANDLE_VALUE;
	}
	return 0;
}

int CXFile::Write2File( int spnum, char* lpFormat, ... )
{
	if( m_hfile == INVALID_HANDLE_VALUE ){
		return 0;
	}

	int ret;
	va_list Marker;
	unsigned long wleng, writeleng;
	char outchar[XFILELINELEN];
			
	ZeroMemory( outchar, XFILELINELEN );

	va_start( Marker, lpFormat );
	ret = vsprintf_s( outchar, XFILELINELEN, lpFormat, Marker );
	va_end( Marker );

	if( ret < 0 )
		return 1;

	char printchar[XFILELINELEN];
	ZeroMemory( printchar, XFILELINELEN );


	if( (spnum + 2 + (int)strlen( outchar )) >= XFILELINELEN ){
		_ASSERT( 0 );
		return 1;
	}


	int spno;
	for( spno = 0; spno < spnum; spno++ ){
		strcat_s( printchar, XFILELINELEN, " " );
	}
	strcat_s( printchar, XFILELINELEN, outchar );
	strcat_s( printchar, XFILELINELEN, "\r\n" );


	wleng = (unsigned long)strlen( printchar );
	WriteFile( m_hfile, printchar, wleng, &writeleng, NULL );
	if( wleng != writeleng ){
		return 1;
	}

	return 0;
}

int CXFile::WriteXFile( WCHAR* srcpath, CModel* srcmodel, float mult )
{
	m_model = srcmodel;
	m_mult = mult;

///////
	m_hfile = CreateFile( srcpath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS,
		FILE_FLAG_SEQUENTIAL_SCAN, NULL );
	if( m_hfile == INVALID_HANDLE_VALUE ){
		_ASSERT( 0 );
	}

	CallF( m_model->MakeEnglishName(), return 1 );
	CallF( m_model->CalcXTransformMatrix( m_mult ), return 1 );


	Write2File( 0, xfileheader1 );
	Write2File( 0, xfileheader2 );
	Write2File( 0, xfileheader3 );
	Write2File( 0, xfileheader4 );

	map<int, CMQOObject*>::iterator itrobj;
	for( itrobj = m_model->m_object.begin(); itrobj != m_model->m_object.end(); itrobj++ ){
		CMQOObject* curobj = itrobj->second;
		if( curobj ){
			CallF( WriteDispObject( curobj ), return 1 );
		}
	}

	if( m_model->m_topbone ){
		int spnum = 1;
		WriteBoneObjectReq( spnum, m_model->m_topbone );

		map<int, MOTINFO*>::iterator itrmi;
		for( itrmi = m_model->m_motinfo.begin(); itrmi != m_model->m_motinfo.end(); itrmi++ ){
			MOTINFO* curmi = itrmi->second;
			if( curmi ){
				CallF( WriteAnimationSet( curmi ), return 1 );
			}
		}
	}

	return 0;
}

int CXFile::WriteDispObject( CMQOObject* srcobj )
{
	int spnum = 1;
//frame name
	CallF( Write2File( spnum, "Frame %s {\r\n\r\n", srcobj->m_engname ), return 1 );

//transformation matrix
	CallF( Write2File( spnum + 1, "FrameTransformMatrix {" ), return 1 );
	CallF( Write2File( spnum + 1, "1.0,0.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,0.0,1.0;;" ), return 1 );
	CallF( Write2File( spnum + 1, "}\r\n" ), return 1 );

//Mesh
	CallF( WritePolyMesh3( spnum + 1, srcobj ), return 1 );

	Write2File( spnum, "}" );

	return 0;
}

void CXFile::WriteBoneObjectReq( int spnum, CBone* srcbone )
{
//frame name
	CallF( Write2File( spnum, "Frame %s {\r\n\r\n", srcbone->m_engbonename ), return );

//transformation matrix
	CallF( WriteTransformationMatrix( spnum + 1, srcbone ), return );

//再帰
	if( srcbone->m_child ){
		WriteBoneObjectReq( spnum + 1, srcbone->m_child );
	}

	Write2File( spnum, "}" );


	if( srcbone->m_brother ){
		WriteBoneObjectReq( spnum, srcbone->m_brother );
	}
}


int CXFile::WriteTransformationMatrix( int spnum, CBone* srcbone )
{
	CallF( Write2File( spnum, "FrameTransformMatrix {" ), return 1 );

	CallF( Write2File( spnum + 1, "%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f;;",
		srcbone->m_xtransmat._11, srcbone->m_xtransmat._12, srcbone->m_xtransmat._13, srcbone->m_xtransmat._14,
		srcbone->m_xtransmat._21, srcbone->m_xtransmat._22, srcbone->m_xtransmat._23, srcbone->m_xtransmat._24,
		srcbone->m_xtransmat._31, srcbone->m_xtransmat._32, srcbone->m_xtransmat._33, srcbone->m_xtransmat._34,
		srcbone->m_xtransmat._41, srcbone->m_xtransmat._42, srcbone->m_xtransmat._43, srcbone->m_xtransmat._44		
		), return 1 );

	CallF( Write2File( spnum, "}\r\n" ), return 1 );


	return 0;
}


int CXFile::WritePolyMesh3( int spnum, CMQOObject* srcobj )
{
	CallF( Write2File( spnum, "Mesh %s_PM {", srcobj->m_engname ), return 1 );

//position
	CallF( WritePosition( spnum + 1, srcobj ), return 1 );

//face
	CallF( WriteFaceIndex( spnum + 1, srcobj ), return 1 );
	CallF( Write2File( 0, "\r\n" ), return 1 );

//normal
	CallF( WriteNormal( spnum + 1, srcobj ), return 1 );

//UV
	CallF( WriteUV( spnum + 1, srcobj ), return 1 );

//material list
	CallF( WriteMaterialList( spnum + 1, srcobj ), return 1 );

//duplication indices
	CallF( WriteDuplicationIndices( spnum + 1, srcobj ), return 1 );

//skinmesh header
	int infbonenum = 0;
	int* boneno2wno;

	boneno2wno = (int*)malloc( sizeof( int ) * m_model->m_bonelist.size() );
	if( !boneno2wno ){
		_ASSERT( 0 );
		return 1;
	}

	CallF( srcobj->MakeXBoneno2wno( m_model->m_bonelist.size(), boneno2wno, &infbonenum ), return 1 );
	CallF( WriteSkinMeshHeader( spnum + 1, srcobj, infbonenum ), return 1 );

//skin weights
	BONEINFLUENCE* boneinf;
	boneinf = (BONEINFLUENCE*)malloc( sizeof( BONEINFLUENCE ) * m_model->m_bonelist.size() );
	ZeroMemory( boneinf, sizeof( BONEINFLUENCE ) * m_model->m_bonelist.size() );
	CallF( srcobj->MakeXBoneInfluence( m_model->m_bonelist, m_model->m_bonelist.size(), infbonenum, boneno2wno, boneinf ), return 1 );
	CallF( WriteSkinWeights( spnum + 1, boneinf, infbonenum ), return 1 );

// free
	if( boneno2wno )
		free( boneno2wno );

	if( boneinf ){
		int seri;
		for( seri = 0; seri < (int)m_model->m_bonelist.size(); seri++ ){
			BONEINFLUENCE* curbi;
			curbi = boneinf + seri;

			if( curbi->vertices ){
				free( curbi->vertices );
			}
			if( curbi->weights ){
				free( curbi->weights );
			}
		}
		free( boneinf );
	}
//
	CallF( Write2File( spnum, "}\r\n" ), return 1 );


	return 0;
}

int CXFile::WritePosition( int spnum, CMQOObject* srcobj )
{
	CPolyMesh3* pm3 = srcobj->m_pm3;
	if( !pm3 ){
		_ASSERT( 0 );
		return 1;
	}

	int vertnum;
	vertnum = pm3->m_optleng;
	CallF( Write2File( spnum, "%d;", vertnum ), return 1 );

	if( !pm3->m_dispv ){
		_ASSERT( 0 );
		return 1;
	}

	PM3DISPV* curv;
	int vertno;
	for( vertno = 0; vertno < vertnum; vertno++ ){
		curv = pm3->m_dispv + vertno;

		if( vertno != (vertnum - 1) ){
			CallF( Write2File( spnum, "%f;%f;%f;,", curv->pos.x * m_mult, curv->pos.y * m_mult, curv->pos.z * m_mult ), return 1 );
		}else{
			CallF( Write2File( spnum, "%f;%f;%f;;", curv->pos.x * m_mult, curv->pos.y * m_mult, curv->pos.z * m_mult ), return 1 );
		}
	}
	return 0;
}

int CXFile::WriteFaceIndex( int spnum, CMQOObject* srcobj )
{
	CPolyMesh3* pm3 = srcobj->m_pm3;
	if( !pm3 ){
		_ASSERT( 0 );
		return 1;
	}

	int facenum;
	facenum = pm3->m_facenum;
	CallF( Write2File( spnum, "%d;", facenum ), return 1 );

	if( !pm3->m_dispindex ){
		_ASSERT( 0 );
		return 1;
	}

	int i1, i2, i3;
	int faceno;
	for( faceno = 0; faceno < facenum; faceno++ ){
		i1 = *(pm3->m_dispindex + faceno * 3);
		i2 = *(pm3->m_dispindex + faceno * 3 + 1);
		i3 = *(pm3->m_dispindex + faceno * 3 + 2);

		if( faceno != (facenum - 1) ){
			CallF( Write2File( spnum, "3;%d,%d,%d;,", i1, i2, i3 ), return 1 );
		}else{
			CallF( Write2File( spnum, "3;%d,%d,%d;;", i1, i2, i3 ), return 1 );
		}
	}

	return 0;
}

int CXFile::WriteNormal( int spnum, CMQOObject* srcobj )
{
	CPolyMesh3* pm3 = srcobj->m_pm3;
	if( !pm3 ){
		_ASSERT( 0 );
		return 1;
	}

	CallF( Write2File( spnum, "MeshNormals {" ), return 1 );	
	int normnum;
	normnum = pm3->m_optleng;
	CallF( Write2File( spnum + 1, "%d;", normnum ), return 1 );

	if( !pm3->m_dispv ){
		_ASSERT( 0 );
		return 1;
	}

	D3DXVECTOR3 n;
	int normno;
	for( normno = 0; normno < normnum; normno++ ){
		n = (pm3->m_dispv + normno)->normal;
		if( normno != (normnum - 1) ){
			CallF( Write2File( spnum + 1, "%f;%f;%f;,", n.x, n.y, n.z ), return 1 );
		}else{
			CallF( Write2File( spnum + 1, "%f;%f;%f;;", n.x, n.y, n.z ), return 1 );
		}
	}
	
	CallF( WriteFaceIndex( spnum + 1, srcobj ), return 1 );
	CallF( Write2File( spnum, "}\r\n" ), return 1 );

	return 0;
}

int CXFile::WriteUV( int spnum, CMQOObject* srcobj )
{
	CPolyMesh3* pm3 = srcobj->m_pm3;
	if( !pm3 ){
		_ASSERT( 0 );
		return 1;
	}

	CallF( Write2File( spnum, "MeshTextureCoords {" ), return 1 );

	int vertnum;
	vertnum = pm3->m_optleng;
	CallF( Write2File( spnum, "%d;", vertnum ), return 1 );

	if( !pm3->m_dispv ){
		_ASSERT( 0 );
		return 1;
	}


	PM3DISPV* curv;
	int vertno;
	for( vertno = 0; vertno < vertnum; vertno++ ){
		curv = pm3->m_dispv + vertno;

		if( vertno != (vertnum - 1) ){
			CallF( Write2File( spnum + 1, "%f;%f;,", curv->uv.x, curv->uv.y ), return 1 );
		}else{
			CallF( Write2File( spnum + 1, "%f;%f;;", curv->uv.x, curv->uv.y ), return 1 );
		}
	}

	CallF( Write2File( spnum, "}\r\n" ), return 1 );

	return 0;
}

int CXFile::WriteMaterialList( int spnum, CMQOObject* srcobj )
{
	CPolyMesh3* pm3 = srcobj->m_pm3;
	if( !pm3 ){
		_ASSERT( 0 );
		return 1;
	}

	CallF( Write2File( spnum, "MeshMaterialList {" ), return 1 );


	int facenum = pm3->m_facenum;
	int matnum = pm3->m_optmatnum;
	CallF( Write2File( spnum + 1, "%d;", matnum ), return 1 );
	CallF( Write2File( spnum + 1, "%d;", facenum ), return 1 );

	int writecnt = 0;
	int matcnt;
	for( matcnt = 0; matcnt < matnum; matcnt++ ){
		MATERIALBLOCK* curmb = pm3->m_matblock + matcnt;

		int wface;
		for( wface = curmb->startface; wface <= curmb->endface; wface++ ){
			if( wface != (facenum - 1) ){
				CallF( Write2File( spnum + 1, "%d,", matcnt ), return 1 );
			}else{
				CallF( Write2File( spnum + 1, "%d;", matcnt ), return 1 );
			}
			writecnt++;
		}
	}
	if( writecnt != facenum ){
		_ASSERT( 0 );
		return 1;
	}

	for( matcnt = 0; matcnt < matnum; matcnt++ ){
		MATERIALBLOCK* curmb = pm3->m_matblock + matcnt;
		CMQOMaterial* curmat = curmb->mqomat;

		CallF( Write2File( spnum + 1, "Material {" ), return 1 );
		CallF( Write2File( spnum + 2, "%f;%f;%f;%f;;", curmat->dif4f.x, curmat->dif4f.y, curmat->dif4f.z, curmat->dif4f.w ), return 1 );
		CallF( Write2File( spnum + 2, "%f;", curmat->power ), return 1 );
		CallF( Write2File( spnum + 2, "%f;%f;%f;;", curmat->spc3f.x, curmat->spc3f.y, curmat->spc3f.z ), return 1 );
		CallF( Write2File( spnum + 2, "%f;%f;%f;;", curmat->emi3f.x, curmat->emi3f.y, curmat->emi3f.z ), return 1 );


		if( curmat->tex[0] ){
			CallF( Write2File( spnum + 2, "TextureFilename {" ), return 1 );
			CallF( Write2File( spnum + 3, "\"%s\";", curmat->tex ), return 1 );
			CallF( Write2File( spnum + 2, "}" ), return 1 );

			int ret;
			ret = CheckMultiBytesChar( curmat->tex );
			if( ret ){
				::MessageBox( NULL, L"テクスチャファイル名に、日本語が含まれています。\n英語に直さないと正常に読み込めないXファイルになります。", 
					L"エラー", MB_OK );
			}
		}

		CallF( Write2File( spnum + 1, "}\r\n" ), return 1 );

	}
	CallF( Write2File( spnum, "}\r\n" ), return 1 );

	return 0;
}

int CXFile::WriteDuplicationIndices( int spnum, CMQOObject* srcobj )
{
	CPolyMesh3* pm3 = srcobj->m_pm3;
	if( !pm3 ){
		_ASSERT( 0 );
		return 1;
	}

	CallF( Write2File( spnum, "VertexDuplicationIndices {" ), return 1 );

	int vertnum;
	vertnum = pm3->m_optleng;

	CallF( Write2File( spnum + 1, "%d;", vertnum ), return 1 );
	CallF( Write2File( spnum + 1, "%d;", vertnum ), return 1 );

	int vertno;
	for( vertno = 0; vertno < vertnum; vertno++ ){
		if( vertno != (vertnum - 1) ){
			CallF( Write2File( spnum + 1, "%d,", vertno ), return 1 );
		}else{
			CallF( Write2File( spnum + 1, "%d;", vertno ), return 1 );
		}
	}

	CallF( Write2File( spnum, "}\r\n" ), return 1 );

	return 0;
}

int CXFile::WriteSkinMeshHeader( int spnum, CMQOObject* srcobj, int bonenum )
{

	CallF( Write2File( spnum, "XSkinMeshHeader {" ), return 1 );

	int maxpervert = 0;
	int maxperface = 0;
	CallF( srcobj->GetSkinMeshHeader( m_model->m_bonelist.size(), &maxpervert, &maxperface ), return 1 );

	CallF( Write2File( spnum + 1, "%d;", maxpervert ), return 1 );
	CallF( Write2File( spnum + 1, "%d;", maxperface ), return 1 );
	CallF( Write2File( spnum + 1, "%d;", bonenum ), return 1 );

	CallF( Write2File( spnum, "}\r\n" ), return 1 );



	return 0;
}

int CXFile::WriteSkinWeights( int spnum, BONEINFLUENCE* boneinf, int infbonenum )
{

	int seri;
	map<int, CBone*>::iterator itrbone;
	for( itrbone = m_model->m_bonelist.begin(); itrbone != m_model->m_bonelist.end(); itrbone++ ){
		CBone* curbone = itrbone->second;
		_ASSERT( curbone );
		seri = curbone->m_boneno;

		BONEINFLUENCE* curbi;
		curbi = boneinf + seri;

		if( (curbi->Bone >= 0) && (curbi->numInfluences > 0) ){
			CallF( Write2File( spnum, "SkinWeights {" ), return 1 );
			CallF( Write2File( spnum + 1, "\"%s\";", curbone->m_engbonename ), return 1 );
			CallF( Write2File( spnum + 1, "%d;", curbi->numInfluences ), return 1 );

			int infno;
			for( infno = 0; infno < (int)curbi->numInfluences; infno++ ){
				if( infno != (curbi->numInfluences - 1) ){
					CallF( Write2File( spnum + 1, "%d,", *(curbi->vertices + infno) ), return 1 );
				}else{
					CallF( Write2File( spnum + 1, "%d;", *(curbi->vertices + infno) ), return 1 );
				}
			}

			for( infno = 0; infno < (int)curbi->numInfluences; infno++ ){
				if( infno != (curbi->numInfluences - 1) ){
					CallF( Write2File( spnum + 1, "%f,", *(curbi->weights + infno) ), return 1 );
				}else{
					CallF( Write2File( spnum + 1, "%f;", *(curbi->weights + infno) ), return 1 );
				}
			}

			D3DXMATRIX offsetmat = curbone->m_xoffsetmat;
			CallF( Write2File( spnum + 1, "%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f;;",
				offsetmat._11, offsetmat._12, offsetmat._13, offsetmat._14,
				offsetmat._21, offsetmat._22, offsetmat._23, offsetmat._24,
				offsetmat._31, offsetmat._32, offsetmat._33, offsetmat._34,
				offsetmat._41, offsetmat._42, offsetmat._43, offsetmat._44
				), return 1 );

			CallF( Write2File( spnum, "}\r\n" ), return 1 );
		}
	}

	return 0;
}

int CXFile::WriteAnimationSet( MOTINFO* srcmi )
{

	CallF( Write2File( 0, "AnimationSet %s {", srcmi->engmotname ), return 1 );
	map<int, CBone*>::iterator itrbone;
	for( itrbone = m_model->m_bonelist.begin(); itrbone != m_model->m_bonelist.end(); itrbone++ ){
		CBone* curbone = itrbone->second;
		_ASSERT( curbone );
		int serino = curbone->m_boneno;

		float* keyframe = 0;
		int keyframenum;
		D3DXQUATERNION* rotarray = 0;
		D3DXVECTOR3* traarray = 0;


		keyframenum = 0;
		CallF( curbone->GetKeyFrameXSRT( srcmi->motid, 0, 0, 0, 0, &keyframenum ), return 1 );

		keyframe = (float*)malloc( sizeof( float ) * (keyframenum + 1) );
		if( !keyframe ){
			_ASSERT( 0 );
			return 1;
		}
		ZeroMemory( keyframe, sizeof( float ) * (keyframenum + 1) );

		traarray = (D3DXVECTOR3*)malloc( sizeof( D3DXVECTOR3 ) * keyframenum );
		if( !traarray ){
			_ASSERT( 0 );
			return 1;
		}
		ZeroMemory( traarray, sizeof( D3DXVECTOR3 ) * keyframenum );
				
		rotarray = (D3DXQUATERNION*)malloc( sizeof( D3DXQUATERNION ) * keyframenum );
		if( !rotarray ){
			_ASSERT( 0 );
			return 1;
		}
		ZeroMemory( rotarray, sizeof( D3DXQUATERNION ) * keyframenum );

		int chknum = 0;
		CallF( curbone->GetKeyFrameXSRT( srcmi->motid, keyframe, rotarray, traarray, keyframenum, &chknum ), return 1 );
		_ASSERT( keyframenum == chknum );

		if( keyframenum > 0 ){
			int keycnt;
			float keyframeno;

			//書き出し
			CallF( Write2File( 1, "Animation {\r\n" ), return 1 );
			CallF( Write2File( 2, "AnimationKey {\r\n" ), return 1 );
			CallF( Write2File( 3, "0;" ), return 1 );// rot
			CallF( Write2File( 3, "%d;", keyframenum ), return 1 );
			for( keycnt = 0; keycnt < keyframenum; keycnt++ ){
				keyframeno = *(keyframe + keycnt);
				D3DXQUATERNION* currot;
				currot = rotarray + keycnt;
				if( keycnt != (keyframenum - 1) ){
					CallF( Write2File( 3, "%d;4;%f,%f,%f,%f;;,", (int)( keyframeno + 0.1f ), currot->w, currot->x, currot->y, currot->z ), return 1 );
				}else{
					CallF( Write2File( 3, "%d;4;%f,%f,%f,%f;;;", (int)( keyframeno + 0.1f ), currot->w, currot->x, currot->y, currot->z ), return 1 );
				}
			}
			CallF( Write2File( 2, "}\r\n" ), return 1 );


			CallF( Write2File( 2, "AnimationKey {\r\n" ), return 1 );
			CallF( Write2File( 3, "2;" ), return 1 );// tra
			CallF( Write2File( 3, "%d;", keyframenum ), return 1 );
			for( keycnt = 0; keycnt < keyframenum; keycnt++ ){
				keyframeno = *(keyframe + keycnt);
				D3DXVECTOR3* curtra;
				curtra = traarray + keycnt;
				if( keycnt != (keyframenum - 1) ){
					CallF( Write2File( 3, "%d;3;%f,%f,%f;;,", (int)( keyframeno + 0.1f ), curtra->x * m_mult, curtra->y * m_mult, curtra->z * m_mult ), return 1 );
				}else{
					CallF( Write2File( 3, "%d;3;%f,%f,%f;;;", (int)( keyframeno + 0.1f ), curtra->x * m_mult, curtra->y * m_mult, curtra->z * m_mult ), return 1 );
				}
			}
			CallF( Write2File( 2, "}\r\n" ), return 1 );
			CallF( Write2File( 2, "{ %s }", curbone->m_engbonename ), return 1 );
			CallF( Write2File( 1, "}\r\n" ), return 1 );

		}


		if( keyframe ){
			free( keyframe );
			keyframe = 0;
		}
		if( rotarray ){
			free( rotarray );
			rotarray = 0;
		}
		if( traarray ){
			free( traarray );
			traarray = 0;
		}

	}
	CallF( Write2File( 0, "}\r\n" ), return 1 );

	return 0;
}

int CXFile::CheckMultiBytesChar( char* srcname )
{
	int leng;
	leng = (int)strlen( srcname );

	int no;
	unsigned char curuc;
	int findw = 0;
	for( no = 0; no < leng; no++ ){
		curuc = *(srcname + no);

		if( curuc >= 0x80 ){
			findw = 1;
			break;
		}
	}
	
	return findw;

}


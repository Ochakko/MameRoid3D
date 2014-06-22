
#define WIN32_LEAN_AND_MEAN
#include <tchar.h>
#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <math.h>
#include <time.h>
#include <Commdlg.h>

#include <string>

#include <coef_r.h>
#include <crtdbg.h>// <--- _ASSERTマクロ

#include <dae.h>
#include <dae/daeUtils.h>
#include <dom/domCOLLADA.h>

using namespace std;
using namespace cdom;

#include <ExporterCommon.h>

#include <model.h>
#include <mqoobject.h>
#include <polymesh3.h>
#include <infbone.h>
#include <mqomaterial.h>
#include <bone.h>
#include <quaternion.h>

#define DBGH
#include <dbg.h>

#define COLLADAEXPORTERCPP
#include <ColladaExporter.h>

#define NO_VCOLOR		(1)


//-------------------------------------------------------------------

static HINSTANCE hInstance;

static HANDLE s_hfile = INVALID_HANDLE_VALUE;
static char* s_jointname = 0;
static int* s_jointinfo = 0;


static int Write2File( char* lpFormat, ... );
static void SafeFreeAndClose();

// ファイル名取得
static bool GetOutputFileName( std::wstring &filename_withpath, std::wstring &filename );

// 要素構築
static bool addAsset     ( RDBPlgInfo &, daeElement* );
static bool addMaterial  ( RDBPlgInfo &, daeElement* );
static bool addImage     ( RDBPlgInfo &, daeElement* );
static bool addGeometry  ( RDBPlgInfo &, daeElement* );
static bool addAnimation ( RDBPlgInfo &, daeElement* );
static bool addController( RDBPlgInfo &, daeElement* );
static bool addScene     ( RDBPlgInfo &, daeElement* );

//-------------------------------------------------------------------
// マクロ等
//-------------------------------------------------------------------

/** daeElement追加 */
#define SafeAdd(elt, name, var)		/*_DPrintf( "%s->%s\n", elt->getAttribute("id").c_str(), name );*/ daeElement* var = elt->add(name); CheckResult(var);

/** 結果チェック */
#define CheckResult(_ptr)			if( (_ptr)==NULL ) { return false; }

/** id名を参照型に */
string makeUriRef( const string& id ) {
	return string("#") + id;
}

/** 配列を daeTArray に変換 */
template<typename T>
daeTArray<T> rawArrayToDaeArray( T rawArray[], size_t count ) {
	daeTArray<T> result;
	for( size_t i = 0; i < count; i++ ) {
		result.append(rawArray[i]);
	}
	return result;
}

//=======================================================================================
// インターフェース
// ↓ここから
//=======================================================================================
//----------------------------------------------------------------------------

//  RDBOnSelectPlugin
//  RokDeBone2のプラグインメニューで、RDBGetPlugInNameの文字列を選択したときに、
//  この関数が、一回、呼ばれます。
//----------------------------------------------------------------------------

int SaveColladaFile( CModel* srcmodel )
{
	std::wstring		wfilename_withpath;
	std::wstring		wfilename;
	DAE				dae;
	RDBPlgInfo		rdb = RDBPlgInfo( srcmodel );

	rdb.SetModel();

	/* 出力ファイル名の取得 */
	if( !GetOutputFileName( wfilename_withpath, wfilename ) ) {
		return -1;
	}

	char filename_withpath[1024] = {0};
	char filename[1024] = {0};
	WideCharToMultiByte( CP_ACP, 0, wfilename_withpath.c_str(), -1, filename_withpath, 1024, NULL, NULL );
	WideCharToMultiByte( CP_ACP, 0, wfilename.c_str(), -1, filename, 1024, NULL, NULL );

	domCOLLADA* root = dae.add( filename_withpath );
	if( root==NULL ) { return -1; }

	if( !addAsset     ( rdb, root ) ) { return -1; } // asset
	if( !addImage     ( rdb, root ) ) { return -1; } // image
	if( !addMaterial  ( rdb, root ) ) { return -1; } // material
	if( !addGeometry  ( rdb, root ) ) { return -1; } // geometry
	if( !addAnimation ( rdb, root ) ) { return -1; } // animation
	if( !addController( rdb, root ) ) { return -1; } // skin
	if( !addScene     ( rdb, root ) ) { return -1; } // visual scene

	if( !dae.writeAll() ) {
		_DPrintf( "write error...\n" );
		return -1;
	}

	_DPrintf( "finished.\n" );

	return 0;
}

//=======================================================================================
// ↑ここまで
//=======================================================================================

//=======================================================================================
// COLLADAデータ構築
//=======================================================================================

//----------------------------------------------------------------------------
//
// <source> の追加
//
//----------------------------------------------------------------------------

bool addSource( daeElement* mesh,
				const string& srcID,
				const string& paramNames,
				domFloat values[],
				int valueCount )
{
	SafeAdd( mesh, "source", src );
	src->setAttribute( "id", srcID.c_str() );

	domFloat_array* fa = daeSafeCast<domFloat_array>( src->add("float_array") );
	CheckResult(fa);

	fa->setId( (src->getAttribute("id")+"-array").c_str() );
	fa->setCount( valueCount );
	fa->getValue() = rawArrayToDaeArray( values, valueCount );

	domAccessor* acc = daeSafeCast<domAccessor>( src->add("technique_common accessor") );
	CheckResult(acc);
	acc->setSource( makeUriRef(fa->getId()).c_str() );

	list<string> params = tokenize( paramNames, " " );
	acc->setStride( params.size() );
	acc->setCount( valueCount/params.size() );
	for( tokenIter iter=params.begin(); iter!=params.end(); iter++ ) {
		SafeAdd( acc, "param", p );
		p->setAttribute( "name", iter->c_str() );
		p->setAttribute( "type", "float" );
	}
	
	return true;
}

bool addSourceName( daeElement* mesh,
					const string& srcID,
					const string& paramNames,
					domName values[],
					int valueCount )
{
	SafeAdd( mesh, "source", src );
	src->setAttribute( "id", srcID.c_str() );

	domName_array* fa = daeSafeCast<domName_array>( src->add("Name_array") );
	CheckResult(fa);

	fa->setId( (src->getAttribute("id")+"-array").c_str() );
	fa->setCount( valueCount );
	fa->setValue( rawArrayToDaeArray( values, valueCount ) );

	domAccessor* acc = daeSafeCast<domAccessor>( src->add("technique_common accessor") );
	CheckResult(acc);
	acc->setSource( makeUriRef(fa->getId()).c_str() );

	list<string> params = tokenize( paramNames, " " );
	acc->setStride( params.size() );
	acc->setCount( valueCount/params.size() );
	for( tokenIter iter=params.begin(); iter!=params.end(); iter++ ) {
		SafeAdd( acc, "param", p );
		p->setAttribute( "name", iter->c_str() );
		p->setAttribute( "type", "Name" );
	}
	
	return true;
}

bool addSourceMatrix( daeElement* mesh,
					  const string& srcID,
					  const string& paramNames,
					  domFloat values[],
					  int valueCount )
{
	SafeAdd( mesh, "source", src );
	src->setAttribute( "id", srcID.c_str() );

	domFloat_array* fa = daeSafeCast<domFloat_array>( src->add("float_array") );
	CheckResult(fa);

	fa->setId( (src->getAttribute("id")+"-array").c_str() );
	fa->setCount( valueCount );
	fa->getValue() = rawArrayToDaeArray( values, valueCount );

	domAccessor* acc = daeSafeCast<domAccessor>( src->add("technique_common accessor") );
	CheckResult(acc);
	acc->setSource( makeUriRef(fa->getId()).c_str() );

	list<string> params = tokenize( paramNames, " " );
	acc->setStride( 16 * params.size() );
	acc->setCount( valueCount/(16*params.size()) );
	for( tokenIter iter=params.begin(); iter!=params.end(); iter++ ) {
		SafeAdd( acc, "param", p );
		p->setAttribute( "name", iter->c_str() );
		p->setAttribute( "type", "float4x4" );
	}
	
	return true;
}



//----------------------------------------------------------------------------
//
// <input> の追加
//
//----------------------------------------------------------------------------

bool addInput( daeElement* parent,
			   const string& semantic,
			   const string& srcID,
               int offset=0 )
{
	daeElement *ele = parent->add("input");
	daeString parentname = parent->getElementName();

	//_DPrintf( "parentname:%s", parentname );

	if( strcmp( parentname, "joints" )==0 ||
		strcmp( parentname, "sampler" )==0 || 
		strcmp( parentname, "targets" )==0 ||
		strcmp( parentname, "vertices" )==0 ||
		strcmp( parentname, "controll_vertices" )==0 )
	{ // domInputLocal

		domInputLocal* input = daeSafeCast<domInputLocal>( ele );
		CheckResult(input);

		//_DPrintf( "input semantic:%s id:%s\n", semantic.c_str(), makeUriRef(srcID).c_str() );

		input->setSemantic( semantic.c_str() );
		input->setSource( makeUriRef(srcID).c_str() );

	} else { // domInputLocalOffset

		domInputLocalOffset* input = daeSafeCast<domInputLocalOffset>( ele );
		CheckResult(input);

		//_DPrintf( "input semantic:%s id:%s\n", semantic.c_str(), makeUriRef(srcID).c_str() );

		input->setSemantic( semantic.c_str() );
		input->setOffset( offset );
		input->setSource( makeUriRef(srcID).c_str() );
		if(semantic == "TEXCOORD") {
			input->setSet(0);
		}

	}

	return true;
}


//----------------------------------------------------------------------------
//
// <assert> 要素の追加
//
//----------------------------------------------------------------------------

bool addAsset( RDBPlgInfo &rdb, daeElement* root )
{
	char		date[256] = "";
	time_t		ltime;
	struct tm	today;

	time( &ltime );
	if( gmtime_s( &today, &ltime ) ) { return false; }
	strftime( date, 256, "%Y-%m-%dT%H:%M:%SZ", &today );

	_DPrintf( "created  date: %s\n", date );
	_DPrintf( "modified date: %s\n", date );

	SafeAdd( root, "asset", asset );
	asset->add("created" )->setCharData( date );
	asset->add("modified")->setCharData( date );

	return true;
}

//----------------------------------------------------------------------------
//
// <image> 要素の追加
//
//----------------------------------------------------------------------------

bool addImage( RDBPlgInfo &rdb, daeElement* root )
{

	bool	result      = true; // 処理結果
	int		ret = 0;
	int		i;

	try {

		int		texcount;

		SafeAdd( root  , "library_images", imglib );

		/* 全テクスチャ情報取得 */
		texcount = rdb.GetTextureCount();
		DbgOut( L"texcount:[%d]\n", texcount );
		for( i=0; i<texcount; i++ ) {

			char name[256];
			RDBPlgTexture *tex = rdb.GetTexture( i );

			domImage *img = daeSafeCast<domImage>( imglib->add( "image" ) );

			_DPrintf( "name:%s file:%s\n", tex->GetName(), tex->GetFileName() );

			sprintf_s( name, 256, "img_%s", tex->GetName() );
			img->setId( name );

			domImage::domInit_from *imgfrom = daeSafeCast<domImage::domInit_from>( img->add( "init_from" ) );
			imgfrom->setValue( tex->GetFileName() );

		}

		//_DPrintf( "SUCCESS !!!\n" );

	}
	catch( std::bad_alloc ) {
		_DPrintf( "ERROR !!! / no memory.\n" );
		result = false;
	}
	catch( RDBPlgException & ) {
		_DPrintf( "ERROR !!!\n" );
		result = false;
	}

	return result;

}

//----------------------------------------------------------------------------
//
// <material> 要素の追加
//
//----------------------------------------------------------------------------

bool addMaterial( RDBPlgInfo &rdb, daeElement* root )
{

	bool	result      = true; // 処理結果
	int		ret = 0;

	/* <effect> 追加 */
	SafeAdd( root  , "library_effects"  , efflib );

	map<int, CMQOMaterial*>::iterator itrmat;
	for( itrmat = rdb.m_model->m_material.begin(); itrmat != rdb.m_model->m_material.end(); itrmat++ ){
		CMQOMaterial* curmat = itrmat->second;
		_ASSERT( curmat );

		char	name[256];
		string	effectID;
		//RDBMaterial mtr;

		sprintf_s( name, 256, "MATERIAL-FX-%d", curmat->materialno );
		effectID = name;

		SafeAdd( efflib, "effect", eff );
		eff->setAttribute( "id", effectID.c_str() );
		SafeAdd( eff, "profile_COMMON", prof );

		if( curmat->tex[0] ){
			char texname[256] = {0};
			string strtex;
			string strtexpath;

			strtexpath = curmat->tex;

			char* lasten;
			lasten = strrchr( curmat->tex, '\\' );
			if( lasten ){
				strcpy_s( texname, 256, lasten + 1 );
			}else{
				strcpy_s( texname, 256, curmat->tex );
			}
			strtex = texname;
			std::string::size_type ich    = 0;
			/* . -> _ に置換 */
			while( (ich=strtex.find( ".", ich ))!=std::string::npos ) { strtex.replace( ich, 1, "_" ); ich++; }
			RDBPlgTexture *tex = rdb.SearchTextureName( strtex );

			string	imgID;
			string	surfaceID;
			string	samplerID;
			string	techID;
			string	uvID;

			sprintf_s( name, 256, "img_%s"    , tex->GetName() );
			imgID     = name;
			sprintf_s( name, 256, "surface_%s", tex->GetName() );
			surfaceID = name;
			sprintf_s( name, 256, "sampler_%s", tex->GetName() );
			samplerID = name;
			sprintf_s( name, 256, "FX-%d"     , curmat->materialno );
			techID    = name;

			// surface
			SafeAdd( prof, "newparam", surfaceparam );
			surfaceparam->setAttribute( "sid", surfaceID.c_str() );
			SafeAdd( surfaceparam, "surface", surface );
			surface->setAttribute( "type", "2D" );
			SafeAdd( surface, "init_from", surfacefrom );
			surfacefrom->setCharData( imgID );

			// sampler
			SafeAdd( prof, "newparam", samplerparam );
			samplerparam->setAttribute( "sid", samplerID.c_str() );
			SafeAdd( samplerparam, "sampler2D", sampler );
			SafeAdd( sampler, "source", samplersource );
			samplersource->setCharData( surfaceID );

			// technique
			SafeAdd( prof, "technique", tech );
			tech->setAttribute( "sid", techID.c_str() );
			SafeAdd( tech, "phong", phong );

			int matno = 0;
			std::string emiStr, ambStr;

			SafeAdd( phong, "emission color", emi_col );
			sprintf_s( name, 256, "%f %f %f %f\n", curmat->emi3f.x, curmat->emi3f.y, curmat->emi3f.z, 1.0f ); emiStr = name;
			emi_col->setCharData( emiStr );
			emi_col->setAttribute( "sid", "emission" );

			SafeAdd( phong, "ambient color", amb_col );
			sprintf_s( name, 256, "%f %f %f %f\n", curmat->amb3f.x, curmat->amb3f.y, curmat->amb3f.z, 1.0f ); ambStr = name;
			amb_col->setCharData( ambStr );
			amb_col->setAttribute( "sid", "ambient" );

			SafeAdd( phong, "diffuse texture", dif_tex );
			dif_tex->setAttribute( "texture" , samplerID.c_str() );
			dif_tex->setAttribute( "texcoord", "diffuse_TEXCOORD" );
		}else{
			string	techID;

			sprintf_s( name, 256, "FX-%d", curmat->materialno );
			techID = name;

			// technique
			SafeAdd( prof, "technique", tech );
			tech->setAttribute( "sid", techID.c_str() );
			SafeAdd( tech, "phong", phong );

			std::string emiStr, ambStr, difStr;

			SafeAdd( phong, "emission color", emi_col );
			sprintf_s( name, 256, "%f %f %f %f\n", curmat->emi3f.x, curmat->emi3f.y, curmat->emi3f.z, 1.0f ); emiStr = name;
			emi_col->setCharData( emiStr );
			emi_col->setAttribute( "sid", "emission" );

			SafeAdd( phong, "ambient color", amb_col );
			sprintf_s( name, 256, "%f %f %f %f\n", curmat->amb3f.x, curmat->amb3f.y, curmat->amb3f.z, 1.0f ); ambStr = name;
			amb_col->setCharData( ambStr );
			amb_col->setAttribute( "sid", "ambient" );

			SafeAdd( phong, "diffuse color", dif_col );
			sprintf_s( name, 256, "%f %f %f %f\n", curmat->dif4f.x, curmat->dif4f.y, curmat->dif4f.z, curmat->dif4f.w ); difStr = name;
			dif_col->setCharData( difStr );
			dif_col->setAttribute( "sid", "diffuse" );
		}


	}

	/* <material> 追加 */
	SafeAdd( root  , "library_materials", matlib );

	for( itrmat = rdb.m_model->m_material.begin(); itrmat != rdb.m_model->m_material.end(); itrmat++ ){
		CMQOMaterial* curmat = itrmat->second;
		_ASSERT( curmat );

		char	name[256];
		string	matID;
		string	effectID;

		sprintf_s( name, 256, "MATERIAL-%d", curmat->materialno );
		matID = name;
		sprintf_s( name, 256, "MATERIAL-FX-%d", curmat->materialno );
		effectID = name;

		SafeAdd( matlib, "material", mat );
		mat->setAttribute( "id", matID.c_str() );

		domInstance_effect *eff = daeSafeCast<domInstance_effect>( mat->add( "instance_effect" ) );
		eff->setUrl( makeUriRef(effectID).c_str() );

	}
	_DPrintf( "addMaterial SUCCESS !!!\n" );

	return result;
}

//----------------------------------------------------------------------------
//
// <geomety> 要素の追加
//
//----------------------------------------------------------------------------

bool addGeometry( RDBPlgInfo &rdb, daeElement *root ) {

	bool	result      = true; // 処理結果
	int		ret = 0;

	_DPrintf( "addGeometry()\n" );

		/* メッシュ数取得 */
		int meshnum    = rdb.GetMeshCount();

		SafeAdd( root  , "library_geometries", geolib );

		/* 全表示オブジェクトを取得 */
		int meshno;
		for( meshno = 0; meshno < meshnum; meshno++ ) {

			RDBPlgMesh *mesh = rdb.GetMesh( meshno );
				
			string		geomID;
			string		matID;
			int			vertnum;
			int			facenum;
			int			setwordnum;
			domFloat	*posArray    = NULL;
			domFloat	*normalArray = NULL;
			domFloat	*uvArray     = NULL;
			domFloat	*vcolArray	 = NULL;
			domUint		*indices	 = NULL; 
			WORD		*ibuf		 = NULL;

			char name[256];
			geomID = mesh->GetName();
			sprintf_s( name, 256, "MATERIAL-%d", mesh->GetMaterial() );
			matID = name;

			/* 頂点数等取得 */
			vertnum    = mesh->GetVertexCount();
			facenum    = mesh->GetTriangleCount();
			setwordnum = facenum * 3;

			/* 領域確保 */
			posArray = new domFloat[ vertnum * 3 ];			// 座標
			if( posArray==NULL ) { goto L_ERROR_OBJ; }
			normalArray = new domFloat[ vertnum * 3 ];		// 法線
			if( normalArray==NULL ) { goto L_ERROR_OBJ; }
			uvArray = new domFloat[ vertnum * 2 ];			// UV
			if( uvArray==NULL ) { goto L_ERROR_OBJ; }
#ifndef NO_VCOLOR
			vcolArray = new domFloat[ vertnum * 4 ];		// 頂点カラー
			if( vcolArray==NULL ) { goto L_ERROR_OBJ; }
			indices = new domUint[ facenum * 3 * 4 ];		// トライアングルリスト 面 * 3(角形) * 4(pos,normal,uv,vcol)
			if( indices==NULL ) { goto L_ERROR_OBJ; }
#else
			indices = new domUint[ facenum * 3 * 3 ];		// トライアングルリスト 面 * 3(角形) * 4(pos,normal,uv)
			if( indices==NULL ) { goto L_ERROR_OBJ; }
#endif

			/* 頂点情報取得 */
			int vertno;
			for( vertno = 0; vertno < vertnum; vertno++ ) {

				RDBPlgVertex *vert = mesh->GetVertex( vertno );

				/* 頂点座標 */
				RDBPoint vpos = *vert->GetPos();
				posArray[ vertno * 3 + 0 ] =  vpos.x;
				posArray[ vertno * 3 + 1 ] =  vpos.y;
				posArray[ vertno * 3 + 2 ] = -vpos.z; // RDB2は左手座標系 → COLLADA は右手座標系

				/* 法線 */
				RDBPoint nrm = *vert->GetNormal();
				normalArray[ vertno * 3 + 0 ] =  nrm.x;
				normalArray[ vertno * 3 + 1 ] =  nrm.y;
				normalArray[ vertno * 3 + 2 ] = -nrm.z; // RDB2は左手座標系 → COLLADA は右手座標系

				/* tex coord */
				float u = vert->GetTexCoordU();
				float v = vert->GetTexCoordV();
				uvArray[ vertno * 2 + 0 ] = u;
				uvArray[ vertno * 2 + 1 ] = 1.0f-v;
			}

			/* トライアングルリスト取得 */
	
			int faceno;
			for( faceno = 0; faceno < facenum; faceno++ ) {
				RDBPlgTriangle *tri = mesh->GetTriangle( faceno );
				// RDB2は左手座標系 → COLLADA は右手座標系
				WORD v0 = tri->GetVertexNo( 0 );
				WORD v1 = tri->GetVertexNo( 1 );
				WORD v2 = tri->GetVertexNo( 2 );
				// 座標
				indices[ faceno * 3 * 3 + 0 * 3 + 0 ] = v2;
				indices[ faceno * 3 * 3 + 1 * 3 + 0 ] = v1;
				indices[ faceno * 3 * 3 + 2 * 3 + 0 ] = v0;
				// 法線
				indices[ faceno * 3 * 3 + 0 * 3 + 1 ] = v2;
				indices[ faceno * 3 * 3 + 1 * 3 + 1 ] = v1;
				indices[ faceno * 3 * 3 + 2 * 3 + 1 ] = v0;
				// UV
				indices[ faceno * 3 * 3 + 0 * 3 + 2 ] = v2;
				indices[ faceno * 3 * 3 + 1 * 3 + 2 ] = v1;
				indices[ faceno * 3 * 3 + 2 * 3 + 2 ] = v0;

			}
	
			/* COLLADA 要素生成 */
			{
				SafeAdd( geolib, "geometry", geom );
				geom->setAttribute( "id", geomID.c_str() );
				geom->setAttribute( "name", geomID.c_str() );
	
				SafeAdd( geom, "mesh", mesh );
	
				//_DPrintf( "MESH:%s vertnum:%d facenum:%d\n", geomID.c_str(), vertnum, facenum );
	
				/* 座標 */
				if( !addSource( mesh, geomID + "-positions", "X Y Z", posArray, vertnum * 3 ) ) { goto L_ERROR_OBJ; }
	
				/* 法線 */
				if( !addSource( mesh, geomID + "-normals", "X Y Z", normalArray, vertnum * 3 ) ) { goto L_ERROR_OBJ; }
	
				/* UV */
				if( !addSource( mesh, geomID + "-uv", "S T", uvArray, vertnum * 2 ) ) { goto L_ERROR_OBJ; }
			
#ifndef NO_VCOLOR
				/* 頂点カラー */
				if( !addSource( mesh, geomID + "-colors", "R G B A", vcolArray, vertnum * 4 ) ) { goto L_ERROR_OBJ; }
#endif

				/* add <vertices> element */
				SafeAdd( mesh, "vertices", vertices );
				vertices->setAttribute("id", (geomID + "-vertices").c_str() );
				SafeAdd( vertices, "input", verticesInput );
				verticesInput->setAttribute( "semantic", "POSITION" );
				verticesInput->setAttribute( "source", makeUriRef(geomID + "-positions").c_str() );
	
				/* Add the <triangles> element. */
				domTriangles* triangles = daeSafeCast<domTriangles>( mesh->add("triangles") );
				if( triangles==NULL ) { goto L_ERROR_OBJ; }
				triangles->setCount( facenum );
				triangles->setMaterial( matID.c_str() );
	
				if( !addInput( triangles, "VERTEX",   geomID + "-vertices", 0 ) ) { goto L_ERROR_OBJ; }
				if( !addInput( triangles, "NORMAL",   geomID + "-normals",  1 ) ) { goto L_ERROR_OBJ; }
				if( !addInput( triangles, "TEXCOORD", geomID + "-uv",       2 ) ) { goto L_ERROR_OBJ; }
#ifndef NO_VCOLOR
				if( !addInput( triangles, "COLOR",    geomID + "-colors",   3 ) ) { goto L_ERROR_OBJ; }
#endif

				domP* p = daeSafeCast<domP>( triangles->add("p") );
				if( p==NULL ) { goto L_ERROR_OBJ; }
#ifndef NO_VCOLOR
				p->getValue() = rawArrayToDaeArray( indices, facenum*3*4 );
#else
				p->getValue() = rawArrayToDaeArray( indices, facenum*3*3 );
#endif

			}

			if( posArray   !=NULL ) { delete [] posArray; }
			if( normalArray!=NULL ) { delete [] normalArray; }
			if( uvArray    !=NULL ) { delete [] uvArray; }
			if( vcolArray  !=NULL ) { delete [] vcolArray; }
			if( ibuf       !=NULL ) { delete [] ibuf; }
			if( indices    !=NULL ) { delete [] indices; }

			continue;

		L_ERROR_OBJ:
			if( posArray   !=NULL ) { delete [] posArray; }
			if( normalArray!=NULL ) { delete [] normalArray; }
			if( uvArray    !=NULL ) { delete [] uvArray; }
			if( vcolArray  !=NULL ) { delete [] vcolArray; }
			if( ibuf       !=NULL ) { delete [] ibuf; }
			if( indices    !=NULL ) { delete [] indices; }

			goto L_ERROR;
		}

		_DPrintf( "addGeometry SUCCESS !!!\n" );

	return result;

L_ERROR:
	_DPrintf( "ERROR !!!\n" );

	return false;
}

//----------------------------------------------------------------------------
//
// <animation> 要素の追加 サブ
//
//----------------------------------------------------------------------------

bool addAnimationSub( RDBPlgInfo &rdb, daeElement *animlib, RDBPlgJoint *joint, int keycount,
					  domFloat *timeArray, domFloat *outputArray, domName *interpArray,
					  const char *output_type, std::string &targetID )
		throw( RDBPlgException )
{
	char		name[256];
	string		animID;
	string		inputID;
	string		outputID;
	string		interpID;
	string		samplerID;

	/* COLLADA要素追加 */
	sprintf_s( name, 256, "%s-anim"        , joint->GetName() ); animID    = name;
	sprintf_s( name, 256, "%s-anim-input"  , joint->GetName() ); inputID   = name;
	sprintf_s( name, 256, "%s-anim-output" , joint->GetName() ); outputID  = name;
	sprintf_s( name, 256, "%s-anim-interp" , joint->GetName() ); interpID  = name;
	sprintf_s( name, 256, "%s-anim-sampler", joint->GetName() ); samplerID = name;

	SafeAdd( animlib, "animation", anim );
	anim->setAttribute( "id", animID.c_str() );

	if( !addSource      ( anim, inputID , "TIME"         , timeArray  , keycount ) ) { throw RDBPlgExceptionResult(); }
	if( !addSource      ( anim, outputID, output_type    , outputArray, keycount ) ) { throw RDBPlgExceptionResult(); }
	if( !addSourceName  ( anim, interpID, "INTERPOLATION", interpArray, keycount ) ) { throw RDBPlgExceptionResult(); }

	SafeAdd( anim, "sampler", sampler );
	sampler->setAttribute( "id", samplerID.c_str() );

	if( !addInput( sampler, "INPUT"        , inputID , 0 ) ) { throw RDBPlgExceptionResult(); }
	if( !addInput( sampler, "OUTPUT"       , outputID, 0 ) ) { throw RDBPlgExceptionResult(); }
	if( !addInput( sampler, "INTERPOLATION", interpID, 0 ) ) { throw RDBPlgExceptionResult(); }

	SafeAdd( anim, "channel", chanl );
	chanl->setAttribute( "source", makeUriRef( samplerID ).c_str() );

	chanl->setAttribute( "target", targetID.c_str() );

	return true;
}

//----------------------------------------------------------------------------
//
// <animation> 要素の追加
//
//----------------------------------------------------------------------------

bool addAnimation( RDBPlgInfo &rdb, daeElement *root ) {

	bool	result     = true; // 処理結果
	int		*keyframes = NULL;

	int		ret = 0;

	try {

		int motno = 0;
		int motcount = 0;
		int jointcount, jointno;
		int framemax;

		/* 関節ある？ */
		jointcount = rdb.GetJointCount();
		_DPrintf( "jointcount:[%d]\n", jointcount );
		if( jointcount <= 0 ) { return true; }

		/* モーションある？ */
		motcount = rdb.m_model->m_motinfo.size();
		if( motcount <= 0 ) { return true; }

		/* 選択中のモーションが対象 */
		motno = rdb.m_model->m_curmotinfo->motid;

		/* 総フレーム数 */
		framemax = (int)rdb.m_model->m_curmotinfo->frameleng;

		_DPrintf( "framemax:[%d]\n", framemax );
		if( framemax==0 ) { return true; }

		//keyframes = new int[ framemax ];

		SafeAdd( root  , "library_animations", animlib );

		/* 全関節について */
		for( jointno=0; jointno<jointcount; jointno++ ) {

			char		name[256];
			string		targetID;

			domFloat*	timeArray   = NULL;
			domFloat*	transXArray = NULL;
			domFloat*	transYArray = NULL;
			domFloat*	transZArray = NULL;
			domFloat*	rotXArray   = NULL;
			domFloat*	rotYArray   = NULL;
			domFloat*	rotZArray   = NULL;
			domName*	interpArray = NULL;

			int keycount = 0;
			int keyno;

			RDBPlgJoint *joint = rdb.GetJoint( jointno );
			int partno = joint->GetPartNo(); 

			if( partno < 0 ) { continue; }

			CBone* curbone = rdb.m_model->m_bonelist[ partno ];
			_ASSERT( curbone );


			curbone->GetKeyFrameXSRT( motno, 0, 0, 0, 0, &keycount );
			_DPrintf( "joint:[%d] partno:[%d] keycount:[%d]\n", jointno, partno, keycount );

			if( keycount <= 0 ) { continue; }

			timeArray   = new domFloat[keycount];
			transXArray = new domFloat[keycount];
			transYArray = new domFloat[keycount];
			transZArray = new domFloat[keycount];
			rotXArray   = new domFloat[keycount];
			rotYArray   = new domFloat[keycount];
			rotZArray   = new domFloat[keycount];
			interpArray = new domName[keycount];

			float* xtime = new float[ keycount ];
			D3DXQUATERNION* xq = new D3DXQUATERNION[ keycount ];
			D3DXVECTOR3* xtra = new D3DXVECTOR3[ keycount ];

			int tmpcount = 0;
			curbone->GetKeyFrameXSRT( motno, xtime, xq, xtra, keycount, &tmpcount );

			for( keyno=0; keyno<keycount; keyno++ ) {

				RDBPoint		pos;
				RDBQuaternion	quat;
				RDBPoint		scale;
				RDBMatrix		mat;
				//int frame = keyframes[keyno];

				/* モーション取得 */
				
				pos.x = (xtra + keyno)->x;
				pos.y = (xtra + keyno)->y;
				pos.z = (xtra + keyno)->z;

				quat.x = (xq + keyno)->x;
				quat.y = (xq + keyno)->y;
				quat.z = (xq + keyno)->z;
				quat.w = (xq + keyno)->w;

				scale.x = 1.0f;
				scale.y = 1.0f;
				scale.z = 1.0f;

				_QuatToMat( &mat, &quat );

#if 0
				mat._31 *= -1;
				mat._32 *= -1;
				mat._34 *= -1;
				mat._13 *= -1;
				mat._23 *= -1;
				mat._43 *= -1;
				pos.z   *= -1;

				_DPrintf( "[%s] frame:[%d]\n", joint->GetName(), frame );
				_DPrintf( "%f %f %f %f\n", mat._11, mat._12, mat._13, mat._14 );
				_DPrintf( "%f %f %f %f\n", mat._21, mat._22, mat._23, mat._24 );
				_DPrintf( "%f %f %f %f\n", mat._31, mat._32, mat._33, mat._34 );
				_DPrintf( "%f %f %f %f\n", mat._41, mat._42, mat._43, mat._44 );
				_DPrintf( "pos:(%f, %f, %f)\n", pos.x, pos.y, pos.z );
#endif

				/* time */
				timeArray[keyno]   = xtime[keyno] / 60.0f;

				/* trans */
				transXArray[keyno] =  pos.x;
				transYArray[keyno] =  pos.y;
				transZArray[keyno] = -pos.z; // RDB2は左手座標系 → COLLADA は右手座標系

				/* rot */
				rotXArray[keyno] = -_MatrixToRotX( &mat ); // RDB2は左手座標系 → COLLADA は右手座標系
				rotYArray[keyno] = -_MatrixToRotY( &mat ); // RDB2は左手座標系 → COLLADA は右手座標系
				rotZArray[keyno] =  _MatrixToRotZ( &mat );

				/* interp */
				interpArray[keyno] = "LINEAR";

			}

#if 1
			/* translation_x */
			sprintf_s( name, 256, "%s/translation.X", joint->GetName() ); targetID = name;
			addAnimationSub( rdb, animlib, joint, keycount, timeArray, transXArray, interpArray, "X", targetID );

			/* translation_y */
			sprintf_s( name, 256, "%s/translation.Y", joint->GetName() ); targetID = name;
			addAnimationSub( rdb, animlib, joint, keycount, timeArray, transYArray, interpArray, "Y", targetID );

			/* translation_z */
			sprintf_s( name, 256, "%s/translation.Z", joint->GetName() ); targetID = name;
			addAnimationSub( rdb, animlib, joint, keycount, timeArray, transZArray, interpArray, "Z", targetID );
#endif

			/* rotation_x */
			sprintf_s( name, 256, "%s/rotation_x.ANGLE", joint->GetName() ); targetID = name;
			addAnimationSub( rdb, animlib, joint, keycount, timeArray, rotXArray, interpArray, "ANGLE", targetID );

			/* rotation_y */
			sprintf_s( name, 256, "%s/rotation_y.ANGLE", joint->GetName() ); targetID = name;
			addAnimationSub( rdb, animlib, joint, keycount, timeArray, rotYArray, interpArray, "ANGLE", targetID );

			/* rotation_z */
			sprintf_s( name, 256, "%s/rotation_z.ANGLE", joint->GetName() ); targetID = name;
			addAnimationSub( rdb, animlib, joint, keycount, timeArray, rotZArray, interpArray, "ANGLE", targetID );

			if( timeArray   !=NULL ) { delete [] timeArray; }
			if( transXArray !=NULL ) { delete [] transXArray; }
			if( transYArray !=NULL ) { delete [] transYArray; }
			if( transZArray !=NULL ) { delete [] transZArray; }
			if( rotXArray   !=NULL ) { delete [] rotXArray; }
			if( rotYArray   !=NULL ) { delete [] rotYArray; }
			if( rotZArray   !=NULL ) { delete [] rotZArray; }
			if( interpArray !=NULL ) { delete [] interpArray; }

			if( xtime ){ delete [] xtime; }
			if( xq ){ delete [] xq; }
			if( xtra ){ delete [] xtra; }
		}

		result = true;
	}
	catch( RDBPlgException ) {
		result = false;
	}

	return result;
}

//----------------------------------------------------------------------------
//
// <controller> 要素の追加
//
//----------------------------------------------------------------------------

bool addController( RDBPlgInfo &rdb, daeElement *root )
{
	bool	result = true;

	_DPrintf( "addController()\n" );

	/* モデルデータ取得 */
	int ret = 0;
	if( !rdb.m_model ) { return false; }

	int		dispobjnum;

	/* 表示オブジェクトの情報取得 */
	dispobjnum = rdb.m_model->m_object.size();
	if( dispobjnum==0 ) { goto L_ERROR; }

//	dispobjname = new char[ 256 * dispobjnum ];
//	if( dispobjname==NULL ) { goto L_ERROR; }

//	dispobjinfo = new int[ DOI_MAX * dispobjnum ];
//	if( dispobjinfo==NULL ) { goto L_ERROR; }

	SafeAdd( root    , "library_controllers", ctrllib );

	int meshnum = rdb.GetMeshCount();

	/* 全メッシュについて */
//	map<int, CMQOObject*>::iterator itrobj;
//	itrobj = rdb.m_model->m_object.begin();

	int meshno;
	for( meshno=0; meshno<meshnum; meshno++ ) {

		RDBPlgMesh *mesh = rdb.GetMesh( meshno );
		int dispobjno = mesh->GetDispObjNo();

		CMQOObject* curobj = rdb.m_model->m_object[ dispobjno ];
		_ASSERT( curobj );

		int			partno	= curobj->m_objectno;
		int			vertnum;
		int			jointnum;
		int			weightnumall;
		char		name[256];
		//string		geomID   = (char*)(dispobjname + 256 * dispobjno);
		string		geomID;
		string		skinID;
		domName		*jointArray        = NULL;
		domFloat	*poseArray         = NULL;
		domFloat	*weightArray       = NULL;
		domUint		*weightCountArray  = NULL;
		domInt		*vertexWeightArray = NULL;

		try {

			sprintf_s( name, 256, "MESH-%d", meshno ); geomID = name;
			sprintf_s( name, 256, "SKIN-%d", meshno ); skinID = name;

			/* 頂点数等取得 */
			vertnum = mesh->GetVertexCount();
			jointnum = rdb.GetJointCount();

			/* ジョイント数が0の場合 */
			if( jointnum <= 0 ) {

				/* ダミーのジョイントを１つ作って解決 */
				jointnum = 1;

				/* 領域確保 */
				jointArray        = new domName[ 1 ];		// ジョイント名
				poseArray         = new domFloat[ 1 * 16 ];	// 初期配置
				weightCountArray  = new domUint[ vertnum ];			// ウエイト数

				/* ボーン名 */
				jointArray[0] = "JointRoot";
				memset( poseArray, 0, sizeof(domFloat)*16 );
				poseArray[0*4+0] = poseArray[1*4+1] = poseArray[2*4+2] = poseArray[3*4+3] = 1.0f;

				/* ウエイト数収集 */
				int vertno;
				weightnumall = vertnum;
				for( vertno = 0; vertno < vertnum; vertno++ ) {
					weightCountArray[ vertno ] = 1;
				}		

				/* ウエイト収集 */
				weightArray = new domFloat[ weightnumall ];
				for( vertno = 0; vertno < vertnum; vertno++ ) {
					weightArray[vertno] = 1.0f;
				}

				/* 頂点ウエイトリスト作成 */
				vertexWeightArray = new domInt[ weightnumall * 2 ];
				for( vertno = 0; vertno < vertnum; vertno++ ) {
					vertexWeightArray[ vertno*2+0 ] = 0;
					vertexWeightArray[ vertno*2+1 ] = 0;
				}

			} else {

				/* 領域確保 */
				jointArray        = new domName[ jointnum ];		// ジョイント名
				poseArray         = new domFloat[ jointnum * 16 ];	// 初期配置
				weightCountArray  = new domUint[ vertnum ];			// ウエイト数

				/* ボーン名 */
				int jointno;
				for( jointno=0; jointno < jointnum; jointno++ ) {
					jointArray[jointno] = rdb.GetJointName( jointno );
				}

				/* 初期配置 */
				for( jointno=0; jointno < jointnum; jointno++ ) {
					RDBPlgJoint *joint = rdb.GetJoint( jointno );
					joint->GetInvMatrix( &poseArray[ jointno*16 ] );

					//_DPrintf( "INV: [%s]\n", joint->GetName() );
					//_DPrintf( " %6.2f %6.2f %6.2f %6.2f\n", poseArray[jointno*16+0*4+0], poseArray[jointno*16+0*4+1], poseArray[jointno*16+0*4+2], poseArray[jointno*16+0*4+3] );
					//_DPrintf( " %6.2f %6.2f %6.2f %6.2f\n", poseArray[jointno*16+1*4+0], poseArray[jointno*16+1*4+1], poseArray[jointno*16+1*4+2], poseArray[jointno*16+1*4+3] );
					//_DPrintf( " %6.2f %6.2f %6.2f %6.2f\n", poseArray[jointno*16+2*4+0], poseArray[jointno*16+2*4+1], poseArray[jointno*16+2*4+2], poseArray[jointno*16+2*4+3] );
					//_DPrintf( " %6.2f %6.2f %6.2f %6.2f\n", poseArray[jointno*16+3*4+0], poseArray[jointno*16+3*4+1], poseArray[jointno*16+3*4+2], poseArray[jointno*16+3*4+3] );
				}

				/* ウエイト数収集 */
				int vertno;
				weightnumall = 0;
				for( vertno = 0; vertno < vertnum; vertno++ ) {
					RDBPlgVertex *vert = mesh->GetVertex(vertno);
					int rdbvertno;
					rdbvertno = vert->GetRDBVertexNo();					
					int orgvertno;
					orgvertno = ( curobj->m_pm3->m_n3p + rdbvertno )->pervert->vno;

					CInfBone* curib = curobj->m_pm3->m_infbone + orgvertno;
					int   weightnum;
					weightnum = curib->m_infnum;

					weightCountArray[ vertno ] = weightnum;
					weightnumall += weightnum;
				}		

				/* ウエイト収集 */
				weightArray = new domFloat[ weightnumall ];
				vertexWeightArray = new domInt[ weightnumall * 2 ];
				int weightindex = 0;
				for( vertno = 0; vertno < vertnum; vertno++ ) {
					RDBPlgVertex *vert = mesh->GetVertex(vertno);

					int rdbvertno;
					rdbvertno = vert->GetRDBVertexNo();					
					int orgvertno;
					orgvertno = ( curobj->m_pm3->m_n3p + rdbvertno )->pervert->vno;

					_ASSERT( curobj->m_pm3->m_infbone );
					CInfBone* curib = curobj->m_pm3->m_infbone + orgvertno;
					int   weightnum;
					weightnum = curib->m_infnum;

					int   weightno;
					/* ウエイト取得 */
					for( weightno = 0; weightno < weightnum; weightno++ ) {
						weightArray[ weightindex ] = curib->m_infelem[weightno].dispinf;

						int jointno = curib->m_infelem[weightno].boneno;

						int jointindex = rdb.GetJointIndexFromPartNo( jointno );
						vertexWeightArray[ weightindex*2 + 0 ] = jointindex;
						vertexWeightArray[ weightindex*2 + 1 ] = weightindex;
						weightindex++;
					}
				}		

			}

			/* COLLADA 要素生成 */
			SafeAdd( ctrllib , "controller", ctrl );
			ctrl->setAttribute( "id", skinID.c_str() );
			SafeAdd( ctrl, "skin", skin );
			skin->setAttribute( "source", makeUriRef(geomID).c_str() );

			/* ボーン */
			if( !addSourceName  ( skin, skinID + "-joints"    , "JOINT"    , jointArray , jointnum    ) ) { goto L_ERROR_OBJ; }

			/* 初期配置 */
			if( !addSourceMatrix( skin, skinID + "-bind_poses", "TRANSFORM", poseArray  , jointnum*16  ) ) { goto L_ERROR_OBJ; }

			/* ウエイト */
			if( !addSource      ( skin, skinID + "-weights"   , "WEIGHT"   , weightArray, weightnumall ) ) { goto L_ERROR_OBJ; }

			/* <joints>要素 */
			SafeAdd( skin, "joints", joints );
			if( !addInput( joints, "JOINT"          , skinID + "-joints"     ) ) { goto L_ERROR_OBJ; }
			if( !addInput( joints, "INV_BIND_MATRIX", skinID + "-bind_poses" ) ) { goto L_ERROR_OBJ; }

			/* <vertex_weights>要素 */
			SafeAdd( skin, "vertex_weights", weight );
			domSkin::domVertex_weights *vertex_weights = daeSafeCast<domSkin::domVertex_weights>( weight );
			vertex_weights->setCount( vertnum );
			if( !addInput( weight, "JOINT" , skinID + "-joints" , 0 ) ) { goto L_ERROR_OBJ; }
			if( !addInput( weight, "WEIGHT", skinID + "-weights", 1 ) ) { goto L_ERROR_OBJ; }

			domSkin::domVertex_weights::domVcount *vcount = daeSafeCast<domSkin::domVertex_weights::domVcount>( weight->add("vcount") );
			if( vcount==NULL ) { goto L_ERROR_OBJ; }
			vcount->getValue() = rawArrayToDaeArray( weightCountArray, vertnum );

			domSkin::domVertex_weights::domV* v = daeSafeCast<domSkin::domVertex_weights::domV>( weight->add("v") );
			if( v==NULL ) { goto L_ERROR_OBJ; }
			v->setValue( rawArrayToDaeArray( vertexWeightArray, weightnumall*2 ) );

		}
		catch( std::bad_alloc ) {
			result = false;
		}
		catch( RDBPlgException ) {
			result = false;
		}

		if( jointArray       !=NULL ) { delete [] jointArray; }
		if( poseArray        !=NULL ) { delete [] poseArray; }
		if( weightArray      !=NULL ) { delete [] weightArray; }
		if( weightCountArray !=NULL ) { delete [] weightCountArray; }
		if( vertexWeightArray!=NULL ) { delete [] vertexWeightArray; }

		if( result ) {
			continue;	
		} else {
			goto L_ERROR;
		}


	L_ERROR_OBJ:

		if( jointArray       !=NULL ) { delete [] jointArray; }
		if( poseArray        !=NULL ) { delete [] poseArray; }
		if( weightArray      !=NULL ) { delete [] weightArray; }
		if( weightCountArray !=NULL ) { delete [] weightCountArray; }
		if( vertexWeightArray!=NULL ) { delete [] vertexWeightArray; }

		goto L_ERROR;
	}

	_DPrintf( "addController SUCCESS !!!\n" );

	return true;

L_ERROR:
	_DPrintf( "addController ERROR !!!\n" );

	return false;
}
	 
//----------------------------------------------------------------------------
//
// <visual_scene> 要素の追加
//
//----------------------------------------------------------------------------

bool addScene( RDBPlgInfo &rdb, daeElement *root ) {

	bool result = true;

	int		dispobjnum;

	try {

		int ret = 0;

		int				jointnum;
		string			jointID, jointRootID;


		/* 表示オブジェクトの情報取得 */
		dispobjnum = rdb.m_model->m_object.size();
		if( dispobjnum==0 ) { throw RDBPlgExceptionResult(); }

		SafeAdd( root    , "library_visual_scenes", scenelib );

		/* COLLADA 要素追加 */
		SafeAdd( scenelib, "visual_scene", scene );
		scene->setAttribute( "id", "VisualSceneNode" );
		scene->setAttribute( "name", "VisualSceneNode" );

		/* ジョイント情報 */
		jointnum = rdb.GetJointCount();

		/* jointnum==0 の場合 */
		if( jointnum <= 0 ) {

			jointnum = 1;
			jointRootID = "JointRoot";

			/* ジョイント */
			daeElement *parent = scene;
			string jointID = jointRootID;
			domFloat matrix[16];

			parent = scene;

			/* COLLADA エレメント追加 */
			domNode* node = daeSafeCast<domNode>( parent->add("node") );
			node->setId  ( jointID.c_str() );
			node->setName( jointID.c_str() );
			node->setSid ( jointID.c_str() );
			node->setType( NODETYPE_JOINT );

			memset( matrix, 0, sizeof(domFloat)*16 );
			matrix[0*0+0] = matrix[0*0+0] = matrix[0*0+0] = matrix[0*0+0] = 1.0f;

			domFloat rot[4], trans[3], scale[3];
			domRotate* rot_p;
			domTranslate* trans_p;
			domScale* scale_p;

			trans_p = daeSafeCast<domTranslate>( node->add("translate") );
			trans[0] = matrix[0*4+3]; trans[1] = matrix[1*4+3]; trans[2] = matrix[2*4+3];
			trans_p->setAttribute( "sid", "translation" );
			trans_p->getValue() = rawArrayToDaeArray( trans, 3 );

			rot_p = daeSafeCast<domRotate>( node->add("rotate") );
			rot[0] = 1; rot[1] = 0; rot[2] = 0; rot[3] = _MatrixToRotX( matrix );
			rot_p->setAttribute( "sid", "rotation_x" );
			rot_p->getValue() = rawArrayToDaeArray( rot, 4 );

			rot_p = daeSafeCast<domRotate>( node->add("rotate") );
			rot[0] = 0; rot[1] = 1; rot[2] = 0; rot[3] = _MatrixToRotY( matrix );
			rot_p->setAttribute( "sid", "rotation_y" );
			rot_p->getValue() = rawArrayToDaeArray( rot, 4 );

			rot_p = daeSafeCast<domRotate>( node->add("rotate") );
			rot[0] = 0; rot[1] = 0; rot[2] = 1; rot[3] = _MatrixToRotZ( matrix );
			rot_p->setAttribute( "sid", "rotation_z" );
			rot_p->getValue() = rawArrayToDaeArray( rot, 4 );

			scale_p = daeSafeCast<domScale>( node->add("scale") );
			scale[0] = 1; scale[1] = 1; scale[2] = 1;
			scale_p->setAttribute( "sid", "scale" );
			scale_p->getValue() = rawArrayToDaeArray( scale, 3 );

		} else {

			/* 領域確保 */

			/* ジョイント */
			std::vector<RDBPlgJoint*> joints;
			std::vector<RDBPlgJoint*>::iterator ite;
			RDBPlgJoint *jointRoot;

			jointRoot = rdb.GetJointRoot();
			jointRootID = jointRoot->GetName();
			joints.push_back( jointRoot );

			while( joints.size() > 0 ) {

				RDBPlgJoint *cur = joints.back();
				joints.pop_back();

				daeElement *parent;
				string jointID;
				domFloat matrix[16];

				//_DPrintf( "parent:[%p] child:[%p], next:[%p]\n", cur->GetParent(), cur->GetChild(), cur->GetNext() );

				RDBPlgJoint *jointParent = cur->GetParent();
				if( jointParent==NULL ) { parent = scene; }
				else { parent = (daeElement*)jointParent->GetUserData(); }

				jointID = cur->GetName();

				/* COLLADA エレメント追加 */
				domNode* node = daeSafeCast<domNode>( parent->add("node") );
				node->setId  ( jointID.c_str() );
				node->setName( jointID.c_str() );
				node->setSid ( jointID.c_str() );
				node->setType( NODETYPE_JOINT );

				cur->SetUserData( node );
				cur->GetLocalMatrix( matrix );

				//_DPrintf( "[%s]\n", jointID.c_str() );
				//_DPrintf( " %6.2f %6.2f %6.2f %6.2f\n", matrix[0*4+0], matrix[0*4+1], matrix[0*4+2], matrix[0*4+3] );
				//_DPrintf( " %6.2f %6.2f %6.2f %6.2f\n", matrix[1*4+0], matrix[1*4+1], matrix[1*4+2], matrix[1*4+3] );
				//_DPrintf( " %6.2f %6.2f %6.2f %6.2f\n", matrix[2*4+0], matrix[2*4+1], matrix[2*4+2], matrix[2*4+3] );
				//_DPrintf( " %6.2f %6.2f %6.2f %6.2f\n", matrix[3*4+0], matrix[3*4+1], matrix[3*4+2], matrix[3*4+3] );

				domFloat rot[4], trans[3], scale[3];
				domRotate* rot_p;
				domTranslate* trans_p;
				domScale* scale_p;

				trans_p = daeSafeCast<domTranslate>( node->add("translate") );
				trans[0] = matrix[0*4+3]; trans[1] = matrix[1*4+3]; trans[2] = matrix[2*4+3];
				trans_p->setAttribute( "sid", "translation" );
				trans_p->getValue() = rawArrayToDaeArray( trans, 3 );

				rot_p = daeSafeCast<domRotate>( node->add("rotate") );
				rot[0] = 1; rot[1] = 0; rot[2] = 0; rot[3] = _MatrixToRotX( matrix );
				rot_p->setAttribute( "sid", "rotation_x" );
				rot_p->getValue() = rawArrayToDaeArray( rot, 4 );

				rot_p = daeSafeCast<domRotate>( node->add("rotate") );
				rot[0] = 0; rot[1] = 1; rot[2] = 0; rot[3] = _MatrixToRotY( matrix );
				rot_p->setAttribute( "sid", "rotation_y" );
				rot_p->getValue() = rawArrayToDaeArray( rot, 4 );

				rot_p = daeSafeCast<domRotate>( node->add("rotate") );
				rot[0] = 0; rot[1] = 0; rot[2] = 1; rot[3] = _MatrixToRotZ( matrix );
				rot_p->setAttribute( "sid", "rotation_z" );
				rot_p->getValue() = rawArrayToDaeArray( rot, 4 );

				scale_p = daeSafeCast<domScale>( node->add("scale") );
				scale[0] = 1; scale[1] = 1; scale[2] = 1;
				scale_p->setAttribute( "sid", "scale" );
				scale_p->getValue() = rawArrayToDaeArray( scale, 3 );

#if 0
				domMatrix* p = daeSafeCast<domMatrix>( node->add("matrix") );
				if( p==NULL ) { throw RDBPlgExceptionIllegalCast(); }

				p->getValue() = rawArrayToDaeArray( matrix, 16 );
#endif

				/* 兄弟、子を追加 */
				if( cur->GetChild()!=NULL ) { joints.push_back( cur->GetChild() ); }
				if( cur->GetNext() !=NULL ) { joints.push_back( cur->GetNext () ); }

			}

		}

		/* 全表示オブジェクトを取得 */
		int meshnum = rdb.GetMeshCount();

		int meshno;
		for( meshno=0; meshno<meshnum; meshno++ ) {

			RDBPlgMesh *mesh = rdb.GetMesh( meshno );
			int mtrno = mesh->GetMaterial();
			int dispobjno = mesh->GetDispObjNo();

			CMQOObject* curobj = rdb.m_model->m_object[ dispobjno ];
			_ASSERT( curobj );

			int			partno	= curobj->m_objectno;
			char		name[256] = "";
			string		nodeID;
			string		skinID;
			string		matID;
			string		uvID;

			try {

				sprintf_s( name, 256, "NODE-%d"    , meshno ); nodeID = name;
				sprintf_s( name, 256, "SKIN-%d"    , meshno ); skinID = name;
				sprintf_s( name, 256, "MATERIAL-%d", mtrno  ); matID  = name;
				sprintf_s( name, 256, "MESH-%d-uv" , meshno ); uvID   = name;

				SafeAdd( scene, "node", skinnode );

				skinnode->setAttribute( "id"  , nodeID.c_str() );
				skinnode->setAttribute( "name", nodeID.c_str() );

				domInstance_controller *ctrl = daeSafeCast<domInstance_controller>( skinnode->add( "instance_controller" ) );
				ctrl->setUrl( makeUriRef(skinID).c_str() );

				domInstance_controller::domSkeleton* skl = daeSafeCast<domInstance_controller::domSkeleton>( ctrl->add("skeleton") );
				if( skl==NULL ) { throw RDBPlgExceptionIllegalCast(); }

				skl->setValue( makeUriRef(jointRootID).c_str() );

				/* マテリアル */
				SafeAdd( ctrl, "bind_material technique_common instance_material", material );
				material->setAttribute( "symbol", matID.c_str() );
				material->setAttribute( "target", makeUriRef(matID).c_str() );
				SafeAdd( material, "bind", bind_material );
				bind_material->setAttribute( "semantic", "diffuse_TEXCOORD" );
				bind_material->setAttribute( "target"  , uvID.c_str() );


			}
			catch( RDBPlgException ex ) {
				throw ex;
			}
		}
		
		root->add("scene instance_visual_scene")->setAttribute( "url", makeUriRef("VisualSceneNode").c_str() );

		_DPrintf( "SUCCESS !!!\n" );

	}
	catch( RDBPlgException ) {

		result = false;

		_DPrintf( "ERROR !!!\n" );

	}

	return result;
}

//=============================================================================
//
// 出力ファイル名の取得
//
//=============================================================================

bool GetOutputFileName( std::wstring &filename_withpath, std::wstring &filename )
{
	WCHAR fnwp[1024] = L"";
	WCHAR fn[1024] = L"";
	OPENFILENAME ofn;

	memset( &ofn, 0, sizeof(ofn) );
	ofn.lStructSize       = sizeof( ofn );
	ofn.hwndOwner         = NULL;
	ofn.hInstance         = 0;
	ofn.lpstrFilter       = L"COLLADA 1.4(*.dae)\0*.dae\0All Files\0*.*\0";
	ofn.lpstrCustomFilter = NULL; 
	ofn.nMaxCustFilter    = 0;
	ofn.nFilterIndex      = 0;
	ofn.lpstrFile         = fnwp; // パス付ファイル名
	ofn.nMaxFile          = sizeof( fnwp ) / sizeof( WCHAR );
	ofn.lpstrFileTitle    = fn; // パスなしファイル名 
	ofn.nMaxFileTitle     = sizeof( fn ) / sizeof( WCHAR );
	ofn.lpstrInitialDir   = NULL;
	ofn.lpstrTitle        = NULL; 
	ofn.Flags             = OFN_CREATEPROMPT|
							OFN_EXTENSIONDIFFERENT|
							OFN_NOREADONLYRETURN|
							OFN_NOVALIDATE|
							OFN_OVERWRITEPROMPT|
							OFN_PATHMUSTEXIST;
	ofn.nFileOffset       = 0;
	ofn.nFileExtension    = 0;
	ofn.lpstrDefExt       = L"dae";
	ofn.lCustData         = 0;
	ofn.lpfnHook          = NULL;
	ofn.lpTemplateName    = NULL; 

	if( GetSaveFileName(&ofn)==0 ) {
		return false;
	}

	filename_withpath = fnwp;
	filename = fn;

	return true;
}


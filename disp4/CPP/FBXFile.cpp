#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <malloc.h>
#include <memory.h>

#include <windows.h>

#define FBXFILECPP
#include <FBXFile.h>
//#include <fbxfilesdk/kfbxtransformation.h>

#define	DBGH
#include <dbg.h>

#include <crtdbg.h>
#include <coef.h>

#include <FBXBone.h>
#include <Model.h>
#include <MQOObject.h>
#include <Bone.h>
#include <PolyMesh3.h>
#include <MQOMaterial.h>
#include <InfBone.h>
#include <MotionPoint.h>
#include <MorphKey.h>



#include <map>
using namespace std;

typedef struct tag_blsindex
{
	int serialno;
	int blendshapeno;
	int channelno;
}BLSINDEX;
typedef struct tag_blsinfo
{
	BLSINDEX blsindex;
	KFbxNode* basenode;
	CMQOObject* base;
	CMQOObject* target;
}BLSINFO;
static map<int, BLSINFO> s_blsinfo;

typedef struct tag_animinfo
{
	int motid;
	int maxframe;
	char* engmotname;
	KFbxAnimLayer* animlayer;
}ANIMINFO;

static int s_fbxbunki = 1;
static float s_mult = 1.0f;
static ANIMINFO* s_ai = 0;
static int s_ainum = 0;

static CFBXBone* s_fbxbone = 0;

static int sortfunc_leng( void *context, const void *elem1, const void *elem2);

int sortfunc_leng( void *context, const void *elem1, const void *elem2)
{
	ANIMINFO* info1 = (ANIMINFO*)elem1;
	ANIMINFO* info2 = (ANIMINFO*)elem2;

	int diffleng = info1->maxframe - info2->maxframe;
	return diffleng;
}

static KFbxSdkManager* s_pSdkManager = 0;

//static map<CBone*, KFbxNode*> s_bone2skel;
static int s_firstoutmot;

static CFBXBone* CreateFBXBone( KFbxScene* pScene, CModel* pmodel );
static void CreateFBXBoneReq( KFbxScene* pScene, CBone* pbone, CFBXBone* parfbxbone );
static int DestroyFBXBoneReq( CFBXBone* fbxbone );

static void CreateAndFillIOSettings(KFbxSdkManager* pSdkManager);
static bool SaveScene(KFbxSdkManager* pSdkManager, KFbxDocument* pScene, const char* pFilename, int pFileFormat=-1, bool pEmbedMedia=false);

static void CreateDummyInfDataReq( CFBXBone* fbxbone, KFbxSdkManager* pSdkManager, KFbxScene* pScene, CModel* pmodel, CModel* srcdtri,  MODELBOUND srcmb, KFbxNode* srcRootNode );

static bool CreateScene(KFbxSdkManager* pSdkManager, KFbxScene* pScene, CModel* pmodel, CModel* srcdtri, MODELBOUND srcmb );
static KFbxNode* CreateFbxMesh( KFbxSdkManager* pSdkManager, KFbxScene* pScene, CModel* pmodel, CMQOObject* curobj, MATERIALBLOCK* pmb );
static KFbxNode* CreateDummyMesh( KFbxSdkManager* pSdkManager, KFbxScene* pScene, CModel* pmodel, CMQOObject* curobj, MATERIALBLOCK* pmb, MODELBOUND srcmb );
static KFbxNode* CreateSkeleton(KFbxScene* pScene, CModel* pmodel);
static void CreateSkeletonReq( KFbxScene* pScene, CBone* pbone, CBone* pparbone, KFbxNode* pparnode );
static void LinkMeshToSkeletonReq( CFBXBone* fbxbone, KFbxSkin* lSkin, KFbxScene* pScene, KFbxNode* lMesh, CMQOObject* curobj, CModel* pmodel, MATERIALBLOCK* pmb);
static void LinkDummyMeshToSkeleton( CFBXBone* fbxbone, KFbxSkin* lSkin, KFbxScene* pScene, KFbxNode* lMesh, CMQOObject* curobj, CModel* pmodel, MATERIALBLOCK* pmb);

static int WriteBindPose(KFbxScene* pScene);
static void WriteBindPoseReq( CFBXBone* fbxbone, KFbxPose* lPose );

static void AnimateSkeleton(KFbxScene* pScene, CModel* pmodel);
static void AnimateBoneReq( CFBXBone* fbxbone, KFbxAnimLayer* lAnimLayer, int curmotid, int motmax );
static int AnimateMorph(KFbxScene* pScene, CModel* pmodel);

static void SetXMatrix(KFbxXMatrix& pXMatrix, const KFbxMatrix& pMatrix);
static KFbxTexture*  CreateTexture( KFbxSdkManager* pSdkManager, CMQOMaterial* mqomat );
static int ExistBoneInInf( int boneno, CPolyMesh3* pm3, MATERIALBLOCK* pmb );

static int MapShapesOnMesh( KFbxScene* pScene, KFbxNode* pNode, CModel* pmodel, CMQOObject* curobj, MATERIALBLOCK* pmb, int mbno, BLSINDEX* blsindex );
static int MapTargetShape( KFbxBlendShapeChannel* lBlendShapeChannel, KFbxScene* pScene, CMQOObject* curobj, CMQOObject* curtarget, MATERIALBLOCK* pmb, int mbno );

#ifdef IOS_REF
	#undef  IOS_REF
	#define IOS_REF (*(pSdkManager->GetIOSettings()))
#endif



int InitializeSdkObjects()
{
    // The first thing to do is to create the FBX SDK manager which is the 
    // object allocator for almost all the classes in the SDK.
    s_pSdkManager = KFbxSdkManager::Create();

    if (!s_pSdkManager)
    {
		_ASSERT( 0 );
		return 1;
	}

	// create an IOSettings object
	KFbxIOSettings * ios = KFbxIOSettings::Create(s_pSdkManager, IOSROOT );
	s_pSdkManager->SetIOSettings(ios);

	// Load plugins from the executable directory
	KString lPath = KFbxGetApplicationDirectory();
#if defined(KARCH_ENV_WIN)
	KString lExtension = "dll";
#elif defined(KARCH_ENV_MACOSX)
	KString lExtension = "dylib";
#elif defined(KARCH_ENV_LINUX)
	KString lExtension = "so";
#endif
	s_pSdkManager->LoadPluginsDirectory(lPath.Buffer(), lExtension.Buffer());

	return 0;
}

int DestroySdkObjects()
{
    // Delete the FBX SDK manager. All the objects that have been allocated 
    // using the FBX SDK manager and that haven't been explicitly destroyed 
    // are automatically destroyed at the same time.
    if (s_pSdkManager) s_pSdkManager->Destroy();
    s_pSdkManager = 0;

	return 0;
}

bool SaveScene(KFbxSdkManager* pSdkManager, KFbxDocument* pScene, const char* pFilename, int pFileFormat, bool pEmbedMedia)
{
    int lMajor, lMinor, lRevision;
    bool lStatus = true;

    // Create an exporter.
    KFbxExporter* lExporter = KFbxExporter::Create(pSdkManager, "");

    if( pFileFormat < 0 || pFileFormat >= pSdkManager->GetIOPluginRegistry()->GetWriterFormatCount() )
    {
        // Write in fall back format in less no ASCII format found
        pFileFormat = pSdkManager->GetIOPluginRegistry()->GetNativeWriterFormat();

        //Try to export in ASCII if possible
        int lFormatIndex, lFormatCount = pSdkManager->GetIOPluginRegistry()->GetWriterFormatCount();

        for (lFormatIndex=0; lFormatIndex<lFormatCount; lFormatIndex++)
        {
            if (pSdkManager->GetIOPluginRegistry()->WriterIsFBX(lFormatIndex))
            {
                KString lDesc =pSdkManager->GetIOPluginRegistry()->GetWriterFormatDescription(lFormatIndex);
                char *lASCII = "ascii";
                if (lDesc.Find(lASCII)>=0)
                {
                    pFileFormat = lFormatIndex;
                    break;
                }
            }
        }
    }

    // Set the export states. By default, the export states are always set to 
    // true except for the option eEXPORT_TEXTURE_AS_EMBEDDED. The code below 
    // shows how to change these states.

    IOS_REF.SetBoolProp(EXP_FBX_MATERIAL,        true);
    IOS_REF.SetBoolProp(EXP_FBX_TEXTURE,         true);
    IOS_REF.SetBoolProp(EXP_FBX_EMBEDDED,        pEmbedMedia);
    IOS_REF.SetBoolProp(EXP_FBX_SHAPE,           true);
    IOS_REF.SetBoolProp(EXP_FBX_GOBO,            true);
    IOS_REF.SetBoolProp(EXP_FBX_ANIMATION,       true);
    IOS_REF.SetBoolProp(EXP_FBX_GLOBAL_SETTINGS, true);

    // Initialize the exporter by providing a filename.
    if(lExporter->Initialize(pFilename, pFileFormat, pSdkManager->GetIOSettings()) == false)
    {
        printf("Call to KFbxExporter::Initialize() failed.\n");
        printf("Error returned: %s\n\n", lExporter->GetLastErrorString());
        return false;
    }

    KFbxSdkManager::GetFileFormatVersion(lMajor, lMinor, lRevision);
    printf("FBX version number for this version of the FBX SDK is %d.%d.%d\n\n", lMajor, lMinor, lRevision);

    // Export the scene.
    lStatus = lExporter->Export(pScene); 

    // Destroy the exporter.
    lExporter->Destroy();
    return lStatus;
}

int WriteFBXFile( CModel* pmodel, char* pfilename, CModel* srcdtri, MODELBOUND srcmb, float srcmult, int srcbunki )
{
	if( s_fbxbone ){
		DestroyFBXBoneReq( s_fbxbone );
		s_fbxbone = 0;
	}

	s_mult = srcmult;
	s_fbxbunki = srcbunki;
	s_firstoutmot = -1;

	CallF( pmodel->MakeEnglishName(), return 1 );

    bool lResult;

    // Create the entity that will hold the scene.
	KFbxScene* lScene;
    lScene = KFbxScene::Create(s_pSdkManager,"");


    // Create the scene.
    lResult = CreateScene( s_pSdkManager, lScene, pmodel, srcdtri, srcmb );

    if(lResult == false)
    {
		_ASSERT( 0 );
		return 1;
    }

    // Save the scene.

    // The example can take an output file name as an argument.
    lResult = SaveScene( s_pSdkManager, lScene, pfilename );

    if(lResult == false)
    {
		_ASSERT( 0 );
		return 1;
	}

	if( s_fbxbone ){
		DestroyFBXBoneReq( s_fbxbone );
		s_fbxbone = 0;
	}

	return 0;
}

bool CreateScene( KFbxSdkManager *pSdkManager, KFbxScene* pScene, CModel* pmodel, CModel* srcdtri, MODELBOUND srcmb )
{
    // create scene info
    KFbxDocumentInfo* sceneInfo = KFbxDocumentInfo::Create(pSdkManager,"SceneInfo");
    sceneInfo->mTitle = "scene made by OpenRDB";
    sceneInfo->mSubject = "skinmesh and animation";
    sceneInfo->mAuthor = "OpenRDB user";
    sceneInfo->mRevision = "rev. 1.0";
    sceneInfo->mKeywords = "skinmesh animation";
    sceneInfo->mComment = "no particular comments required.";

    // we need to add the sceneInfo before calling AddThumbNailToScene because
    // that function is asking the scene for the sceneInfo.
    pScene->SetSceneInfo(sceneInfo);

//    AddThumbnailToScene(pScene);

    KFbxNode* lRootNode = pScene->GetRootNode();


	s_fbxbone = CreateFBXBone(pScene, pmodel);
	lRootNode->AddChild( s_fbxbone->skelnode );
	if( !s_fbxbone ){
		_ASSERT( 0 );
		return 0;
	}

	BLSINDEX blsindex;
	ZeroMemory( &blsindex, sizeof( BLSINDEX ) );
	s_blsinfo.clear();
	map<int,CMQOObject*>::iterator itrobj;
	for( itrobj = pmodel->m_object.begin(); itrobj != pmodel->m_object.end(); itrobj++ ){
		CMQOObject* curobj = itrobj->second;
		CPolyMesh3* pm3 = curobj->m_pm3;
		int matcnt;
		for( matcnt = 0; matcnt < pm3->m_optmatnum; matcnt++ ){
			MATERIALBLOCK* pmb = pm3->m_matblock + matcnt;
			KFbxNode* lMesh = CreateFbxMesh( pSdkManager, pScene, pmodel, curobj, pmb );
			if( curobj->m_morphobj.size() > 0 ){
				MapShapesOnMesh( pScene, lMesh, pmodel, curobj, pmb, matcnt, &blsindex );
			}
			lRootNode->AddChild(lMesh);

			KFbxGeometry* lMeshAttribute = (KFbxGeometry*)lMesh->GetNodeAttribute();
			KFbxSkin* lSkin = KFbxSkin::Create(pScene, "");
			LinkMeshToSkeletonReq( s_fbxbone, lSkin, pScene, lMesh, curobj, pmodel, pmb);
			lMeshAttribute->AddDeformer(lSkin);

		}
	}

	CreateDummyInfDataReq( s_fbxbone, pSdkManager, pScene, pmodel, srcdtri, srcmb, lRootNode );


//    StoreRestPose(pScene, lSkeletonRoot);

	if( s_ai ){
		free( s_ai );
		s_ai = 0;
	}
	s_ainum = 0;

    AnimateSkeleton(pScene, pmodel);
	AnimateMorph(pScene, pmodel);

	WriteBindPose(pScene);

	if( s_ai ){
		free( s_ai );
		s_ai = 0;
	}
	s_ainum = 0;

    return true;
}




//void MapBoxShape(KFbxScene* pScene, KFbxBlendShapeChannel* lBlendShapeChannel)
int MapTargetShape( KFbxBlendShapeChannel* lBlendShapeChannel, KFbxScene* pScene, CMQOObject* curobj, CMQOObject* curtarget, MATERIALBLOCK* pmb, int mbno )
{
	char shapename[256]={0};
	sprintf_s( shapename, 256, "SHAPE_%s_%d", curtarget->m_engname, mbno );
    KFbxShape* lShape = KFbxShape::Create(pScene,shapename);

	CPolyMesh3* basepm3 = curobj->m_pm3;
	_ASSERT( basepm3 );
	CPolyMesh3* targetpm3 = curtarget->m_pm3;
	_ASSERT( targetpm3 );

	int cpnum = ( pmb->endface - pmb->startface + 1 ) * 3;
    lShape->InitControlPoints(cpnum);
    KFbxVector4* lVector4 = lShape->GetControlPoints();

	int facecnt = 0;
	int basefaceno;
	for( basefaceno = (basepm3->m_matblock + mbno)->startface; basefaceno <= (basepm3->m_matblock + mbno)->endface; basefaceno++ ){

		int baseorgfaceno;
		int targetfaceno;
		baseorgfaceno = (basepm3->m_n3p + basefaceno * 3)->perface->faceno;
		targetfaceno = targetpm3->m_faceno2optfaceno[ baseorgfaceno ];

		N3P* basen3p = basepm3->m_n3p + basefaceno * 3;
		N3P* targetn3p = targetpm3->m_n3p + targetfaceno * 3;

		int voff[3];
		voff[0] = 0;
		voff[2] = 1;
		voff[1] = 2;

		int fcnt;
		for( fcnt = 0; fcnt < 3; fcnt++ ){
			lVector4[ facecnt * 3 + fcnt ].Set( (targetpm3->m_pointbuf + (targetn3p + voff[fcnt])->pervert->vno)->x * s_mult, 
				(targetpm3->m_pointbuf + (targetn3p + voff[fcnt])->pervert->vno)->y * s_mult,
				-(targetpm3->m_pointbuf + (targetn3p + voff[fcnt])->pervert->vno)->z * s_mult, 1.0 );
		}

		facecnt++;
	}
	lBlendShapeChannel->AddTargetShape(lShape);
	return 0;
}

//void MapShapesOnNurbs(KFbxScene* pScene, KFbxNode* pNurbs)
int MapShapesOnMesh( KFbxScene* pScene, KFbxNode* pNode, CModel* pmodel, CMQOObject* curobj, MATERIALBLOCK* pmb, int mbno, BLSINDEX* blsindex )
{
	char blsname[256] = {0};
	sprintf_s( blsname, 256, "BLS_%s_%d", curobj->m_engname, mbno );
	KFbxBlendShape* lBlendShape = KFbxBlendShape::Create(pScene, blsname);

	(blsindex->channelno) = 0;

	map<int, CMQOObject*>::iterator itrtarget;
	for( itrtarget = curobj->m_morphobj.begin(); itrtarget != curobj->m_morphobj.end(); itrtarget++ ){
		CMQOObject* curtarget = itrtarget->second;
		char blscname[256] = {0};
		sprintf_s( blscname, 256, "BLSC_%s_%d", curobj->m_engname, mbno );
		KFbxBlendShapeChannel* lBlendShapeChannel = KFbxBlendShapeChannel::Create(pScene,blscname);
		MapTargetShape( lBlendShapeChannel, pScene, curobj, curtarget, pmb, mbno );
		lBlendShape->AddBlendShapeChannel(lBlendShapeChannel);



		BLSINFO blsinfo;
		ZeroMemory( &blsinfo, sizeof( BLSINFO ) );

		blsinfo.blsindex = *blsindex;
		blsinfo.base = curobj;
		blsinfo.target = curtarget;
		blsinfo.basenode = pNode;

		s_blsinfo[ blsindex->serialno ] = blsinfo;

		(blsindex->channelno)++;
		(blsindex->serialno)++;
	}



	KFbxGeometry* lGeometry = pNode->GetGeometry();
	lGeometry->AddDeformer(lBlendShape);
	return 0;
};

	
KFbxNode* CreateFbxMesh(KFbxSdkManager* pSdkManager, KFbxScene* pScene, CModel* pmodel, CMQOObject* curobj, MATERIALBLOCK* pmb)
{
	CPolyMesh3* pm3 = curobj->m_pm3;

	char meshname[256] = {0};
	sprintf_s( meshname, 256, "%s_m%d", curobj->m_engname, pmb->materialno );
	int facenum = pmb->endface - pmb->startface + 1;

	KFbxMesh* lMesh = KFbxMesh::Create( pScene, meshname );
	lMesh->InitControlPoints( facenum * 3 );
	KFbxVector4* lcp = lMesh->GetControlPoints();

	KFbxGeometryElementNormal* lElementNormal= lMesh->CreateElementNormal();
	lElementNormal->SetMappingMode(KFbxGeometryElement::eBY_CONTROL_POINT);
	lElementNormal->SetReferenceMode(KFbxGeometryElement::eDIRECT);

	KFbxGeometryElementUV* lUVDiffuseElement = lMesh->CreateElementUV( "DiffuseUV");
	K_ASSERT( lUVDiffuseElement != NULL);
	lUVDiffuseElement->SetMappingMode(KFbxGeometryElement::eBY_POLYGON_VERTEX);
	lUVDiffuseElement->SetReferenceMode(KFbxGeometryElement::eINDEX_TO_DIRECT);

	int vsetno = 0;
	int faceno;
	for( faceno = pmb->startface; faceno <= pmb->endface; faceno++ ){
		int vno[3];
		vno[0] = faceno * 3;
		vno[2] = faceno * 3 + 1;
		vno[1] = faceno * 3 + 2;

		int vcnt;
		for( vcnt = 0; vcnt < 3; vcnt++ ){
			D3DXVECTOR3* pv = pm3->m_pointbuf + ( pm3->m_n3p + vno[vcnt] )->pervert->vno;
			*( lcp + vsetno ) = KFbxVector4( pv->x * s_mult, pv->y * s_mult, -pv->z * s_mult, 1.0f );

			D3DXVECTOR3 n = (pm3->m_n3p + vno[vcnt])->pervert->smnormal;
			KFbxVector4 fbxn = KFbxVector4( n.x, n.y, -n.z, 0.0f );
			lElementNormal->GetDirectArray().Add( fbxn );

			D3DXVECTOR2 uv = (pm3->m_n3p + vno[vcnt])->pervert->uv[0];
			KFbxVector2 fbxuv = KFbxVector2( uv.x, -uv.y );
			lUVDiffuseElement->GetDirectArray().Add( fbxuv );

			vsetno++;
		}
	}
	lUVDiffuseElement->GetIndexArray().SetCount(facenum * 3);

	vsetno = 0;
	for( faceno = pmb->startface; faceno <= pmb->endface; faceno++ ){
		lMesh->BeginPolygon(-1, -1, -1, false);
		int i;
		for(i = 0; i < 3; i++)
		{
			// Control point index
			lMesh->AddPolygon( vsetno );  
			// update the index array of the UVs that map the texture to the face
			lUVDiffuseElement->GetIndexArray().SetAt( vsetno, vsetno );
			vsetno++;
		}
		lMesh->EndPolygon ();
	}


	// create a KFbxNode
//		KFbxNode* lNode = KFbxNode::Create(pSdkManager,pName);
	KFbxNode* lNode = KFbxNode::Create( pScene, meshname );
	// set the node attribute
	lNode->SetNodeAttribute(lMesh);
	// set the shading mode to view texture
	lNode->SetShadingMode(KFbxNode::eTEXTURE_SHADING);
	// rotate the plane
	lNode->LclRotation.Set(KFbxVector4(0, 0, 0));

	// Set material mapping.
	KFbxGeometryElementMaterial* lMaterialElement = lMesh->CreateElementMaterial();
	lMaterialElement->SetMappingMode(KFbxGeometryElement::eBY_POLYGON);
	lMaterialElement->SetReferenceMode(KFbxGeometryElement::eINDEX_TO_DIRECT);
	if( !lMesh->GetElementMaterial(0) ){
		_ASSERT( 0 );
		return NULL;
	}
	CMQOMaterial* mqomat = pmb->mqomat;
	if( !mqomat ){
		_ASSERT( 0 );
		return NULL;
	}

	// add material to the node. 
	// the material can't in different document with the geometry node or in sub-document
	// we create a simple material here which belong to main document
	static int s_matcnt = 0;
	s_matcnt++;

	char matname[256];
	sprintf_s( matname, 256, "%s_%d", mqomat->name, s_matcnt );
	//KString lMaterialName = mqomat->name;
	KString lMaterialName = matname;
	KString lShadingName  = "Phong";
	KFbxSurfacePhong* lMaterial = KFbxSurfacePhong::Create( pScene, lMaterialName.Buffer() );

	lMaterial->Diffuse.Set(fbxDouble3(mqomat->dif4f.x, mqomat->dif4f.y, mqomat->dif4f.z));
    lMaterial->Emissive.Set(fbxDouble3(mqomat->emi3f.x, mqomat->emi3f.y, mqomat->emi3f.z));
    lMaterial->Ambient.Set(fbxDouble3(mqomat->amb3f.x, mqomat->amb3f.y, mqomat->amb3f.z));
    lMaterial->AmbientFactor.Set(1.0);
	KFbxTexture* curtex = CreateTexture(pSdkManager, mqomat);
	if( curtex ){
		lMaterial->Diffuse.ConnectSrcObject( curtex );
	}
    lMaterial->DiffuseFactor.Set(1.0);
    lMaterial->TransparencyFactor.Set(mqomat->dif4f.w);
    lMaterial->ShadingModel.Set(lShadingName);
    lMaterial->Shininess.Set(0.5);
    lMaterial->Specular.Set(fbxDouble3(mqomat->spc3f.x, mqomat->spc3f.y, mqomat->spc3f.z));
    lMaterial->SpecularFactor.Set(0.3);

	lNode->AddMaterial(lMaterial);
	// We are in eBY_POLYGON, so there's only need for index (a plane has 1 polygon).
	lMaterialElement->GetIndexArray().SetCount( lMesh->GetPolygonCount() );
	// Set the Index to the material
	for(int i=0; i<lMesh->GetPolygonCount(); ++i){
		lMaterialElement->GetIndexArray().SetAt(i,0);
	}

	return lNode;
}

KFbxTexture*  CreateTexture(KFbxSdkManager* pSdkManager, CMQOMaterial* mqomat)
{
	if( !mqomat->tex[0] ){
		return NULL;
	}

    KFbxFileTexture* lTexture = KFbxFileTexture::Create(pSdkManager,"");
    KString lTexPath = mqomat->tex;

    // Set texture properties.
    lTexture->SetFileName(lTexPath.Buffer());
    //lTexture->SetName("Diffuse Texture");
	lTexture->SetName(mqomat->tex);
    lTexture->SetTextureUse(KFbxTexture::eSTANDARD);
    lTexture->SetMappingType(KFbxTexture::eUV);
    lTexture->SetMaterialUse(KFbxFileTexture::eMODEL_MATERIAL);
    lTexture->SetSwapUV(false);
    lTexture->SetAlphaSource (KFbxTexture::eNONE);
    lTexture->SetTranslation(0.0, 0.0);
    lTexture->SetScale(1.0, 1.0);
    lTexture->SetRotation(0.0, 0.0);

    return lTexture;
}




void LinkMeshToSkeletonReq( CFBXBone* fbxbone, KFbxSkin* lSkin, KFbxScene* pScene, KFbxNode* pMesh, CMQOObject* curobj, CModel* pmodel, MATERIALBLOCK* pmb)
{
	CPolyMesh3* pm3 = curobj->m_pm3;
	_ASSERT( pm3 );

    KFbxGeometry* lMeshAttribute = (KFbxGeometry*)pMesh->GetNodeAttribute();
 
	KFbxXMatrix lXMatrix;
    KFbxNode* lSkel;

	if( fbxbone->type == FB_NORMAL ){
		CBone* curbone = fbxbone->bone;

		if( curbone ){
			lSkel = fbxbone->skelnode;
			if( !lSkel ){
				_ASSERT( 0 );
				return;
			}

			int infdirty = ExistBoneInInf( curbone->m_boneno, pm3, pmb );

			if( infdirty ){
				KFbxCluster *lCluster = KFbxCluster::Create(pScene,"");
				lCluster->SetLink(lSkel);
				lCluster->SetLinkMode(KFbxCluster::eTOTAL1);

				int vsetno = 0;
				int fno;
				for( fno = pmb->startface; fno <= pmb->endface; fno++ ){
					int vno[3];
					vno[0] = fno * 3;
					vno[2] = fno * 3 + 1;
					vno[1] = fno * 3 + 2;
				
					int vcnt;
					for( vcnt = 0; vcnt < 3; vcnt++ ){
						int orgvno = (pm3->m_n3p + vno[vcnt])->pervert->vno;
						CInfBone* curib = pm3->m_infbone + orgvno;
						int ieno = curib->ExistBone( curbone->m_boneno );
						if( ieno >= 0 ){
							lCluster->AddControlPointIndex( vsetno, (double)( curib->m_infelem[ieno].dispinf ) );
							(fbxbone->m_boneinfcnt)++;
						}
						vsetno++;
					}
				}

				//KFbxScene* lScene = pMesh->GetScene();
				lXMatrix = pMesh->EvaluateGlobalTransform();
				lCluster->SetTransformMatrix(lXMatrix);

				D3DXMATRIX xmat;
				xmat._11 = (float)lXMatrix.Get( 0, 0 );
				xmat._12 = (float)lXMatrix.Get( 0, 1 );
				xmat._13 = (float)lXMatrix.Get( 0, 2 );
				xmat._14 = (float)lXMatrix.Get( 0, 3 );

				xmat._21 = (float)lXMatrix.Get( 1, 0 );
				xmat._22 = (float)lXMatrix.Get( 1, 1 );
				xmat._23 = (float)lXMatrix.Get( 1, 2 );
				xmat._24 = (float)lXMatrix.Get( 1, 3 );

				xmat._31 = (float)lXMatrix.Get( 2, 0 );
				xmat._32 = (float)lXMatrix.Get( 2, 1 );
				xmat._33 = (float)lXMatrix.Get( 2, 2 );
				xmat._34 = (float)lXMatrix.Get( 2, 3 );

				xmat._41 = 0.0f;
				xmat._42 = 0.0f;
				xmat._43 = 0.0f;
				xmat._44 = (float)lXMatrix.Get( 3, 3 );

				D3DXQUATERNION xq;
				D3DXQuaternionRotationMatrix( &xq, &xmat );
				fbxbone->axisq.SetParams( xq );

				//lXMatrix.SetIdentity();
				//lXMatrix[3][0] = -curbone->m_vertpos[BT_PARENT].x;
				//lXMatrix[3][1] = -curbone->m_vertpos[BT_PARENT].y;
				//lXMatrix[3][2] = curbone->m_vertpos[BT_PARENT].z;
				//lCluster->SetTransformMatrix(lXMatrix);

				lXMatrix = lSkel->EvaluateGlobalTransform();
				lCluster->SetTransformLinkMatrix(lXMatrix);
				//lXMatrix.SetIdentity();
				//lCluster->SetTransformLinkMatrix(lXMatrix);

				lSkin->AddCluster(lCluster);
			}
		}
	}

	if( fbxbone->m_child ){
		LinkMeshToSkeletonReq(fbxbone->m_child, lSkin, pScene, pMesh, curobj, pmodel, pmb);
	}
	if( fbxbone->m_brother ){
		LinkMeshToSkeletonReq(fbxbone->m_brother, lSkin, pScene, pMesh, curobj, pmodel, pmb);
	}

}

void AnimateSkeleton(KFbxScene* pScene, CModel* pmodel)
{

    KString lAnimStackName;
    KTime lTime;
    int lKeyIndex = 0;

	int motionnum = pmodel->m_motinfo.size();
	if( motionnum <= 0 ){
		return;
	}

	s_ai = (ANIMINFO*)malloc( sizeof( ANIMINFO ) * motionnum );
	if( !s_ai ){
		_ASSERT( 0 );
		return;
	}
	ZeroMemory( s_ai, sizeof( ANIMINFO ) * motionnum );
	s_ainum = motionnum;

	int aino = 0;
	map<int, MOTINFO*>::iterator itrmi;
	for( itrmi = pmodel->m_motinfo.begin(); itrmi != pmodel->m_motinfo.end(); itrmi++ ){
		MOTINFO* curmi = itrmi->second;
		(s_ai + aino)->motid = curmi->motid;
		(s_ai + aino)->maxframe = (int)( curmi->frameleng - 1.0 );
		(s_ai + aino)->engmotname = curmi->engmotname;
		aino++;
	}
	qsort_s( s_ai, motionnum, sizeof( ANIMINFO ), sortfunc_leng, NULL );

	s_firstoutmot = s_ai->motid;

	for( aino = 0; aino < motionnum; aino++ ){
		ANIMINFO* curai = s_ai + aino;
		int curmotid = curai->motid;
		int maxframe = curai->maxframe;

		lAnimStackName = curai->engmotname;
	    KFbxAnimStack* lAnimStack = KFbxAnimStack::Create(pScene, lAnimStackName);
        KFbxAnimLayer* lAnimLayer = KFbxAnimLayer::Create(pScene, "Base Layer");
		curai->animlayer = lAnimLayer;
        lAnimStack->AddMember(lAnimLayer);

		pmodel->SetCurrentMotion( curmotid );

		AnimateBoneReq( s_fbxbone, lAnimLayer, curmotid, maxframe );

//		pScene->GetRootNode()->ConvertPivotAnimationRecursive( lAnimStackName, KFbxNode::eDESTINATION_SET, 60.0, true );
		pScene->GetRootNode()->ConvertPivotAnimationRecursive( lAnimStackName, KFbxNode::eSOURCE_SET, 30.0, true );

	}

}

void AnimateBoneReq( CFBXBone* fbxbone, KFbxAnimLayer* lAnimLayer, int curmotid, int maxframe )
{
	KTime lTime;
    int lKeyIndex = 0;
    KFbxNode* lSkel = 0;

	//double timescale = 60.0;
	double timescale = 30.0;

	//if( (fbxbone->type == FB_NORMAL) || (fbxbone->type == FB_ROOT) || (fbxbone->type == FB_BUNKI) ){
	if( (fbxbone->type == FB_NORMAL) || (fbxbone->type == FB_ROOT) ){
		CBone* curbone = fbxbone->bone;
		_ASSERT( curbone );
		if( curbone ){

			lSkel = fbxbone->skelnode;
			if( !lSkel ){
				_ASSERT( 0 );
				return;
			}

			ERotationOrder lRotationOrder0 = eEULER_XYZ;
			ERotationOrder lRotationOrder1 = eEULER_ZXY;
			lSkel->SetRotationOrder(KFbxNode::eSOURCE_SET , lRotationOrder0 );
			lSkel->SetRotationOrder(KFbxNode::eDESTINATION_SET , lRotationOrder1 );

			KFbxAnimCurve* lCurveTX;
			KFbxAnimCurve* lCurveTY;
			KFbxAnimCurve* lCurveTZ;

			D3DXVECTOR3 orgtra;
			int frameno;
			CMotionPoint calcmp;

			CBone* parbone = curbone->m_parent;
			CBone* gparbone = 0;
			if( parbone ){
				CBone* gparbone = parbone->m_parent;
				if( gparbone ){
					orgtra = D3DXVECTOR3((parbone->m_vertpos[BT_CHILD].x - gparbone->m_vertpos[BT_CHILD].x) * s_mult, 
						(parbone->m_vertpos[BT_CHILD].y - gparbone->m_vertpos[BT_CHILD].y) * s_mult, 
						(-parbone->m_vertpos[BT_CHILD].z - -gparbone->m_vertpos[BT_CHILD].z) * s_mult );
				}else{
					orgtra = D3DXVECTOR3(parbone->m_vertpos[BT_CHILD].x * s_mult, parbone->m_vertpos[BT_CHILD].y * s_mult, -parbone->m_vertpos[BT_CHILD].z * s_mult);
				}
			}else{
				orgtra = D3DXVECTOR3( curbone->m_vertpos[BT_CHILD].x * s_mult, curbone->m_vertpos[BT_CHILD].y * s_mult, -curbone->m_vertpos[BT_CHILD].z * s_mult );
			}

			lCurveTX = lSkel->LclTranslation.GetCurve<KFbxAnimCurve>(lAnimLayer, KFCURVENODE_T_X, true);		
			lCurveTX->KeyModifyBegin();
			for( frameno = 0; frameno <= maxframe; frameno++ ){
				int hasmpflag = 0;
				curbone->CalcMotionPoint( curmotid, (double)frameno, &calcmp, &hasmpflag );
				lTime.SetSecondDouble( (double)frameno / timescale );
				lKeyIndex = lCurveTX->KeyAdd( lTime );
				lCurveTX->KeySetValue(lKeyIndex, orgtra.x + calcmp.m_tra.x * s_mult);
				lCurveTX->KeySetInterpolation(lKeyIndex, KFbxAnimCurveDef::eINTERPOLATION_LINEAR);
			}
			lCurveTX->KeyModifyEnd();

			lCurveTY = lSkel->LclTranslation.GetCurve<KFbxAnimCurve>(lAnimLayer, KFCURVENODE_T_Y, true);		
			lCurveTY->KeyModifyBegin();
			for( frameno = 0; frameno <= maxframe; frameno++ ){
				int hasmpflag = 0;
				curbone->CalcMotionPoint( curmotid, (double)frameno, &calcmp, &hasmpflag );
				lTime.SetSecondDouble( (double)frameno / timescale );
				lKeyIndex = lCurveTY->KeyAdd( lTime );
				lCurveTY->KeySetValue(lKeyIndex, orgtra.y + calcmp.m_tra.y * s_mult);
				lCurveTY->KeySetInterpolation(lKeyIndex, KFbxAnimCurveDef::eINTERPOLATION_LINEAR);
			}
			lCurveTY->KeyModifyEnd();


			lCurveTZ = lSkel->LclTranslation.GetCurve<KFbxAnimCurve>(lAnimLayer, KFCURVENODE_T_Z, true);		
			lCurveTZ->KeyModifyBegin();
			for( frameno = 0; frameno <= maxframe; frameno++ ){
				int hasmpflag = 0;
				curbone->CalcMotionPoint( curmotid, (double)frameno, &calcmp, &hasmpflag );
				lTime.SetSecondDouble( (double)frameno / timescale );
				lKeyIndex = lCurveTZ->KeyAdd( lTime );
				lCurveTZ->KeySetValue(lKeyIndex, orgtra.z - calcmp.m_tra.z * s_mult);
				lCurveTZ->KeySetInterpolation(lKeyIndex, KFbxAnimCurveDef::eINTERPOLATION_LINEAR);
			}
			lCurveTZ->KeyModifyEnd();


/////////////////////
			D3DXVECTOR3 befeul( 0.0f, 0.0f, 0.0f );
			D3DXVECTOR3 cureul( 0.0f, 0.0f, 0.0f );



			KFbxAnimCurve* lCurveRX;
			lCurveRX = lSkel->LclRotation.GetCurve<KFbxAnimCurve>(lAnimLayer, KFCURVENODE_R_X, true);		
			lCurveRX->KeyModifyBegin();
			befeul = D3DXVECTOR3( 0.0f, 0.0f, 0.0f );
			for( frameno = 0; frameno <= maxframe; frameno++ ){
				int hasmpflag = 0;
				curbone->CalcMotionPoint( curmotid, (double)frameno, &calcmp, &hasmpflag );
				calcmp.m_q.CalcFBXEul( &fbxbone->axisq, befeul, &cureul );
				lTime.SetSecondDouble( (double)frameno / timescale );
				lKeyIndex = lCurveRX->KeyAdd( lTime );
				lCurveRX->KeySetValue(lKeyIndex, cureul.x);
				//lCurveRX->KeySetValue(lKeyIndex, 0.0);
				lCurveRX->KeySetInterpolation(lKeyIndex, KFbxAnimCurveDef::eINTERPOLATION_LINEAR);
				befeul = cureul;
			}
			lCurveRX->KeyModifyEnd();

			KFbxAnimCurve* lCurveRY;
			lCurveRY = lSkel->LclRotation.GetCurve<KFbxAnimCurve>(lAnimLayer, KFCURVENODE_R_Y, true);		
			lCurveRY->KeyModifyBegin();
			befeul = D3DXVECTOR3( 0.0f, 0.0f, 0.0f );
			for( frameno = 0; frameno <= maxframe; frameno++ ){
				int hasmpflag = 0;
				curbone->CalcMotionPoint( curmotid, (double)frameno, &calcmp, &hasmpflag );
				calcmp.m_q.CalcFBXEul( &fbxbone->axisq, befeul, &cureul );
				lTime.SetSecondDouble( (double)frameno / timescale );
				lKeyIndex = lCurveRY->KeyAdd( lTime );
				lCurveRY->KeySetValue(lKeyIndex, cureul.y);
				//lCurveRY->KeySetValue(lKeyIndex, 0.0);
				lCurveRY->KeySetInterpolation(lKeyIndex, KFbxAnimCurveDef::eINTERPOLATION_LINEAR);
				befeul = cureul;
			}
			lCurveRY->KeyModifyEnd();

			KFbxAnimCurve* lCurveRZ;
			lCurveRZ = lSkel->LclRotation.GetCurve<KFbxAnimCurve>(lAnimLayer, KFCURVENODE_R_Z, true);		
			lCurveRZ->KeyModifyBegin();
			befeul = D3DXVECTOR3( 0.0f, 0.0f, 0.0f );
			for( frameno = 0; frameno <= maxframe; frameno++ ){
				int hasmpflag = 0;
				curbone->CalcMotionPoint( curmotid, (double)frameno, &calcmp, &hasmpflag );
				calcmp.m_q.CalcFBXEul( &fbxbone->axisq, befeul, &cureul );
				lTime.SetSecondDouble( (double)frameno / timescale );
				lKeyIndex = lCurveRZ->KeyAdd( lTime );
				lCurveRZ->KeySetValue(lKeyIndex, cureul.z);
				//lCurveRZ->KeySetValue(lKeyIndex, 0.0);
				lCurveRZ->KeySetInterpolation(lKeyIndex, KFbxAnimCurveDef::eINTERPOLATION_LINEAR);
				befeul = cureul;
			}
			lCurveRZ->KeyModifyEnd();


		}
	}

	if( fbxbone->m_child ){
		AnimateBoneReq( fbxbone->m_child, lAnimLayer, curmotid, maxframe );
	}
	if( fbxbone->m_brother ){
		AnimateBoneReq( fbxbone->m_brother, lAnimLayer, curmotid, maxframe );
	}
}



/***
int WriteBindPose(KFbxScene* pScene, CModel* pmodel)
{
	KTime lTime0;
	lTime0.SetSecondDouble( 0.0 );

	KFbxPose* lPose = KFbxPose::Create(pScene,"BindPose0");
	lPose->SetIsBindPose(true);

	if( s_firstoutmot >= 0 ){
		KFbxAnimStack * lCurrentAnimationStack = pScene->FindMember(FBX_TYPE(KFbxAnimStack), s_ai->engmotname);
		if (lCurrentAnimationStack == NULL)
		{
			_ASSERT( 0 );
			return 1;
		}
		KFbxAnimLayer * mCurrentAnimLayer;
		mCurrentAnimLayer = lCurrentAnimationStack->GetMember(FBX_TYPE(KFbxAnimLayer), 0);
		pScene->GetEvaluator()->SetContext(lCurrentAnimationStack);
	}else{
		_ASSERT( 0 );
	}

	map<int, CBone*>::iterator itrbone;
	for( itrbone = pmodel->m_bonelist.begin(); itrbone != pmodel->m_bonelist.end(); itrbone++ ){
		CBone* curbone = itrbone->second;
		KFbxNode* curskel = s_bone2skel[ curbone ];
		if( curskel ){
			if( curskel ){
				KFbxMatrix lBindMatrix = curskel->EvaluateGlobalTransform( lTime0 );
				lPose->Add(curskel, lBindMatrix);
			}
		}
	}

	pScene->AddPose(lPose);

	return 0;
}
***/

int WriteBindPose(KFbxScene* pScene)
{

	KFbxPose* lPose = KFbxPose::Create(pScene,"BindPose0");
	lPose->SetIsBindPose(true);

	if( s_firstoutmot >= 0 ){
		KFbxAnimStack * lCurrentAnimationStack = pScene->FindMember(FBX_TYPE(KFbxAnimStack), s_ai->engmotname);
		if (lCurrentAnimationStack == NULL)
		{
			_ASSERT( 0 );
			return 1;
		}
		KFbxAnimLayer * mCurrentAnimLayer;
		mCurrentAnimLayer = lCurrentAnimationStack->GetMember(FBX_TYPE(KFbxAnimLayer), 0);
		pScene->GetEvaluator()->SetContext(lCurrentAnimationStack);
	}else{
		_ASSERT( 0 );
	}

	WriteBindPoseReq( s_fbxbone, lPose );

	pScene->AddPose(lPose);

	return 0;
}

void WriteBindPoseReq( CFBXBone* fbxbone, KFbxPose* lPose )
{
	KTime lTime0;
	lTime0.SetSecondDouble( 0.0 );
	
	if( fbxbone->type != FB_ROOT ){
		KFbxNode* curskel = fbxbone->skelnode;
		if( curskel ){
			KFbxMatrix lBindMatrix = curskel->EvaluateGlobalTransform( lTime0 );
			lPose->Add(curskel, lBindMatrix);
		}
	}

	if( fbxbone->m_child ){
		WriteBindPoseReq( fbxbone->m_child, lPose );
	}
	if( fbxbone->m_brother ){
		WriteBindPoseReq( fbxbone->m_brother, lPose );
	}
}

/***
void StoreBindPose(KFbxScene* pScene, KFbxNode* pMesh, KFbxNode* pSkeletonRoot)
{
    // In the bind pose, we must store all the link's global matrix at the time of the bind.
    // Plus, we must store all the parent(s) global matrix of a link, even if they are not
    // themselves deforming any model.

    // In this example, since there is only one model deformed, we don't need walk through 
    // the scene
    //

    // Now list the all the link involve in the Mesh deformation
    KArrayTemplate<KFbxNode*> lClusteredFbxNodes;
    int                       i, j;

    if (pMesh && pMesh->GetNodeAttribute())
    {
        int lSkinCount=0;
        int lClusterCount=0;
        switch (pMesh->GetNodeAttribute()->GetAttributeType())
        {
        case KFbxNodeAttribute::eMESH:
        case KFbxNodeAttribute::eNURB:

            lSkinCount = ((KFbxGeometry*)pMesh->GetNodeAttribute())->GetDeformerCount(KFbxDeformer::eSKIN);
            //Go through all the skins and count them
            //then go through each skin and get their cluster count
            for(i=0; i<lSkinCount; ++i)
            {
                KFbxSkin *lSkin=(KFbxSkin*)((KFbxGeometry*)pMesh->GetNodeAttribute())->GetDeformer(i, KFbxDeformer::eSKIN);
                lClusterCount+=lSkin->GetClusterCount();
            }
            break;
        }
        //if we found some clusters we must add the node
        if (lClusterCount)
        {
            //Again, go through all the skins get each cluster link and add them
            for (i=0; i<lSkinCount; ++i)
            {
                KFbxSkin *lSkin=(KFbxSkin*)((KFbxGeometry*)pMesh->GetNodeAttribute())->GetDeformer(i, KFbxDeformer::eSKIN);
                lClusterCount=lSkin->GetClusterCount();
                for (j=0; j<lClusterCount; ++j)
                {
                    KFbxNode* lClusterNode = lSkin->GetCluster(j)->GetLink();
                    AddNodeRecursively(lClusteredFbxNodes, lClusterNode);
                }

            }

            // Add the Mesh to the pose
            lClusteredFbxNodes.Add(pMesh);
        }
    }

    // Now create a bind pose with the link list
    if (lClusteredFbxNodes.GetCount())
    {
        // A pose must be named. Arbitrarily use the name of the Mesh node.
        KFbxPose* lPose = KFbxPose::Create(pScene,pMesh->GetName());

        // default pose type is rest pose, so we need to set the type as bind pose
        lPose->SetIsBindPose(true);

        for (i=0; i<lClusteredFbxNodes.GetCount(); i++)
        {
            KFbxNode*  lKFbxNode   = lClusteredFbxNodes.GetAt(i);
            KFbxMatrix lBindMatrix = lKFbxNode->EvaluateGlobalTransform();

            lPose->Add(lKFbxNode, lBindMatrix);
        }

        // Add the pose to the scene
        pScene->AddPose(lPose);
    }
}
***/

int ExistBoneInInf( int boneno, CPolyMesh3* pm3, MATERIALBLOCK* pmb )
{
	int dirtyflag = 0;

	int startv = pmb->startface * 3;
	int endv = pmb->endface * 3 + 2;

	int vno;
	for( vno = startv; vno <= endv; vno++ ){
		int orgvno = (pm3->m_n3p + vno)->pervert->vno;
		CInfBone* curib = pm3->m_infbone + orgvno;
		int ieno = curib->ExistBone( boneno );
		if( ieno >= 0 ){
			dirtyflag = 1;
			break;
		}
	}
	return dirtyflag;
}

int AnimateMorph(KFbxScene* pScene, CModel* pmodel)
{
    KTime lTime;
    int lKeyIndex = 0;

	if( s_blsinfo.empty() ){
		return 0;
	}

	if( s_ainum <= 0 ){
		return 0;
	}

	//double timescale = 60.0;
	double timescale = 30.0;

	int aino;
	for( aino = 0; aino < s_ainum; aino++ ){
		ANIMINFO* curai = s_ai + aino;
		int curmotid = curai->motid;
		int maxframe = curai->maxframe;
		KFbxAnimLayer* lAnimLayer = curai->animlayer;

		map<int, BLSINFO>::iterator itrblsinfo;
		for( itrblsinfo = s_blsinfo.begin(); itrblsinfo != s_blsinfo.end(); itrblsinfo++ ){
			BLSINFO curinfo = itrblsinfo->second;

			KFbxGeometry* lNurbsAttribute = (KFbxGeometry*)(curinfo.basenode)->GetNodeAttribute();
			KFbxAnimCurve* lCurve = lNurbsAttribute->GetShapeChannel(0, curinfo.blsindex.channelno, lAnimLayer, true);
			if (lCurve)
			{
				lCurve->KeyModifyBegin();

				int frameno;
				for( frameno = 0; frameno <= maxframe; frameno++ ){
					double dframe = (double)frameno;

					CMorphKey curmk( curinfo.base );
					CallF( curinfo.base->CalcCurMorphKey( curmotid, dframe, &curmk ), return 1 );
					map<CMQOObject*, float>::iterator itrbw;
					itrbw = curmk.m_blendweight.find( curinfo.target );
					float blendw;
					if( itrbw == curmk.m_blendweight.end() ){
						blendw = 0.0f;
					}else{
						blendw = itrbw->second;
					}

					lTime.SetSecondDouble(dframe / timescale);
					lKeyIndex = lCurve->KeyAdd(lTime);
					lCurve->KeySetValue(lKeyIndex, blendw * 100.0f);
					lCurve->KeySetInterpolation(lKeyIndex, KFbxAnimCurveDef::eINTERPOLATION_LINEAR);
				}

				lCurve->KeyModifyEnd();
			}
		}
	}

	return 0;
}

CFBXBone* CreateFBXBone( KFbxScene* pScene, CModel* pmodel )
{
	CBone* topj = pmodel->m_topbone;
	if( !topj ){
		_ASSERT( 0 );
		return 0;
	}

	KFbxNode* lSkeletonNode;
	KString lNodeName( topj->m_engbonename );
	KFbxSkeleton* lSkeletonNodeAttribute = KFbxSkeleton::Create(pScene, lNodeName);
	lSkeletonNodeAttribute->SetSkeletonType(KFbxSkeleton::eROOT);
	lSkeletonNode = KFbxNode::Create(pScene,lNodeName.Buffer());
	lSkeletonNode->SetNodeAttribute(lSkeletonNodeAttribute);    
	lSkeletonNode->LclTranslation.Set(KFbxVector4(topj->m_vertpos[BT_CHILD].x * s_mult, topj->m_vertpos[BT_CHILD].y * s_mult, -topj->m_vertpos[BT_CHILD].z * s_mult));
	//lSkeletonNode->LclTranslation.Set(KFbxVector4(0.0, 0.0, 0.0));

	CFBXBone* fbxbone = new CFBXBone();
	if( !fbxbone ){
		_ASSERT( 0 );
		return 0;
	}
	fbxbone->type = FB_ROOT;
	fbxbone->bone = topj;
	fbxbone->skelnode = lSkeletonNode;

	if( topj->m_child ){
		CreateFBXBoneReq( pScene, topj->m_child, fbxbone );
	}

	return fbxbone;

}
void CreateFBXBoneReq( KFbxScene* pScene, CBone* pbone, CFBXBone* parfbxbone )
{
	D3DXVECTOR3 curpos, parpos, gparpos;
	curpos = pbone->m_vertpos[BT_CHILD];
	parpos = pbone->m_vertpos[BT_PARENT];

	CBone* parbone = pbone->m_parent;
	if( parbone ){
		gparpos = parbone->m_vertpos[BT_PARENT];
	}else{
		gparpos = D3DXVECTOR3( 0.0f, 0.0f, 0.0f );
	}

	CFBXBone* fbxbone = new CFBXBone();
	if( !fbxbone ){
		_ASSERT( 0 );
		return;
	}
	
	char newname[256] = {0};
	if( s_fbxbunki == 0 ){
	//if( s_fbxbunki == 1 ){
		if( pbone->GetBoneNum() >= 2 ){
			sprintf_s( newname, 256, "%s_bunki", pbone->m_engbonename );
		}else{
			sprintf_s( newname, 256, "%s", pbone->m_engbonename );
		}
	}else{
		sprintf_s( newname, 256, "%s", pbone->m_engbonename );
	}
	fbxbone->type = FB_NORMAL;

	KString lLimbNodeName1( newname );
	KFbxSkeleton* lSkeletonLimbNodeAttribute1 = KFbxSkeleton::Create(pScene,lLimbNodeName1);
	lSkeletonLimbNodeAttribute1->SetSkeletonType(KFbxSkeleton::eLIMB_NODE);
	lSkeletonLimbNodeAttribute1->Size.Set(1.0);
	KFbxNode* lSkeletonLimbNode1 = KFbxNode::Create(pScene,lLimbNodeName1.Buffer());
	lSkeletonLimbNode1->SetNodeAttribute(lSkeletonLimbNodeAttribute1);
	lSkeletonLimbNode1->LclTranslation.Set(KFbxVector4((parpos.x - gparpos.x) * s_mult, (parpos.y - gparpos.y) * s_mult, (-parpos.z - -gparpos.z) * s_mult));
	parfbxbone->skelnode->AddChild(lSkeletonLimbNode1);

	fbxbone->bone = pbone;
	fbxbone->skelnode = lSkeletonLimbNode1;
	parfbxbone->AddChild( fbxbone );

	if( (s_fbxbunki == 1) && (pbone->GetBoneNum() >= 2) ){
		CFBXBone* fbxbone2 = new CFBXBone();
		if( !fbxbone2 ){
			_ASSERT( 0 );
			return;
		}

		char newname2[256] = {0};
		sprintf_s( newname2, 256, "%s_bunki", pbone->m_engbonename );
		fbxbone2->type = FB_BUNKI;

		KString lLimbNodeName2( newname2 );
		KFbxSkeleton* lSkeletonLimbNodeAttribute2 = KFbxSkeleton::Create(pScene,lLimbNodeName2);
		lSkeletonLimbNodeAttribute2->SetSkeletonType(KFbxSkeleton::eLIMB_NODE);
		lSkeletonLimbNodeAttribute2->Size.Set(1.0);
		KFbxNode* lSkeletonLimbNode2 = KFbxNode::Create(pScene,lLimbNodeName2.Buffer());
		lSkeletonLimbNode2->SetNodeAttribute(lSkeletonLimbNodeAttribute2);
		lSkeletonLimbNode2->LclTranslation.Set(KFbxVector4(0.0, 0.0, 0.0));

		fbxbone->skelnode->AddChild(lSkeletonLimbNode2);

		fbxbone2->bone = pbone;
		fbxbone2->skelnode = lSkeletonLimbNode2;
		fbxbone->AddChild( fbxbone2 );


		if( !pbone->m_child ){
			//endjoint‚ðì‚é
			char endname[256];
			sprintf_s( endname, 256, "%s_end", pbone->m_engbonename );

			KString lLimbNodeName3( endname );
			KFbxSkeleton* lSkeletonLimbNodeAttribute3 = KFbxSkeleton::Create(pScene,lLimbNodeName3);
			lSkeletonLimbNodeAttribute3->SetSkeletonType(KFbxSkeleton::eLIMB_NODE);
			lSkeletonLimbNodeAttribute3->Size.Set(1.0);
			KFbxNode* lSkeletonLimbNode3 = KFbxNode::Create(pScene,lLimbNodeName3.Buffer());
			lSkeletonLimbNode3->SetNodeAttribute(lSkeletonLimbNodeAttribute3);

			lSkeletonLimbNode3->LclTranslation.Set(
				KFbxVector4((pbone->m_vertpos[BT_CHILD].x - pbone->m_vertpos[BT_PARENT].x) * s_mult, 
				(pbone->m_vertpos[BT_CHILD].y - pbone->m_vertpos[BT_PARENT].y) * s_mult,
				(-pbone->m_vertpos[BT_CHILD].z - -pbone->m_vertpos[BT_PARENT].z) * s_mult));
			lSkeletonLimbNode2->AddChild(lSkeletonLimbNode3);

			CFBXBone* fbxbone3 = new CFBXBone();
			if( !fbxbone3 ){
				_ASSERT( 0 );
				return;
			}
			fbxbone3->type = FB_ENDJOINT;
			fbxbone2->skelnode->AddChild(lSkeletonLimbNode3);

			fbxbone3->bone = pbone;
			fbxbone3->skelnode = lSkeletonLimbNode3;
			fbxbone2->AddChild( fbxbone3 );
		}


		if( pbone->m_child ){
			CreateFBXBoneReq( pScene, pbone->m_child, fbxbone2 );
		}
		if( pbone->m_brother ){
			CreateFBXBoneReq( pScene, pbone->m_brother, parfbxbone );
		}

	}else{

		if( !pbone->m_child ){
			//endjoint‚ðì‚é
			char endname[256];
			sprintf_s( endname, 256, "%s_end", pbone->m_engbonename );

			KString lLimbNodeName3( endname );
			KFbxSkeleton* lSkeletonLimbNodeAttribute3 = KFbxSkeleton::Create(pScene,lLimbNodeName3);
			lSkeletonLimbNodeAttribute3->SetSkeletonType(KFbxSkeleton::eLIMB_NODE);
			lSkeletonLimbNodeAttribute3->Size.Set(1.0);
			KFbxNode* lSkeletonLimbNode3 = KFbxNode::Create(pScene,lLimbNodeName3.Buffer());
			lSkeletonLimbNode3->SetNodeAttribute(lSkeletonLimbNodeAttribute3);

			lSkeletonLimbNode3->LclTranslation.Set(
				KFbxVector4((pbone->m_vertpos[BT_CHILD].x - pbone->m_vertpos[BT_PARENT].x) * s_mult, 
				(pbone->m_vertpos[BT_CHILD].y - pbone->m_vertpos[BT_PARENT].y) * s_mult, 
				(-pbone->m_vertpos[BT_CHILD].z - -pbone->m_vertpos[BT_PARENT].z) * s_mult));
			lSkeletonLimbNode1->AddChild(lSkeletonLimbNode3);

			CFBXBone* fbxbone3 = new CFBXBone();
			if( !fbxbone3 ){
				_ASSERT( 0 );
				return;
			}
			fbxbone3->type = FB_ENDJOINT;
			fbxbone->skelnode->AddChild(lSkeletonLimbNode3);

			fbxbone3->bone = pbone;
			fbxbone3->skelnode = lSkeletonLimbNode3;
			fbxbone->AddChild( fbxbone3 );
		}

		if( pbone->m_child ){
			CreateFBXBoneReq( pScene, pbone->m_child, fbxbone );
		}
		if( pbone->m_brother ){
			CreateFBXBoneReq( pScene, pbone->m_brother, parfbxbone );
		}
	}

}
int DestroyFBXBoneReq( CFBXBone* fbxbone )
{
	CFBXBone* chilbone = fbxbone->m_child;
	CFBXBone* brobone = fbxbone->m_brother;

	delete fbxbone;

	if( chilbone ){
		DestroyFBXBoneReq( chilbone );
	}
	if( brobone ){
		DestroyFBXBoneReq( brobone );
	}

	return 0;
}

KFbxNode* CreateDummyMesh( KFbxSdkManager* pSdkManager, KFbxScene* pScene, CModel* pmodel, CMQOObject* curobj, MATERIALBLOCK* pmb, MODELBOUND srcmb )
{
	static int s_createcnt = 0;

	CPolyMesh3* pm3 = curobj->m_pm3;

	char meshname[256] = {0};
	sprintf_s( meshname, 256, "_ND_%s_m%d", curobj->m_engname, s_createcnt );
	s_createcnt++;
	int facenum = pmb->endface - pmb->startface + 1;

	KFbxMesh* lMesh = KFbxMesh::Create( pScene, meshname );
	lMesh->InitControlPoints( facenum * 3 );
	KFbxVector4* lcp = lMesh->GetControlPoints();

	KFbxGeometryElementNormal* lElementNormal= lMesh->CreateElementNormal();
	lElementNormal->SetMappingMode(KFbxGeometryElement::eBY_CONTROL_POINT);
	lElementNormal->SetReferenceMode(KFbxGeometryElement::eDIRECT);

	KFbxGeometryElementUV* lUVDiffuseElement = lMesh->CreateElementUV( "DiffuseUV");
	K_ASSERT( lUVDiffuseElement != NULL);
	lUVDiffuseElement->SetMappingMode(KFbxGeometryElement::eBY_POLYGON_VERTEX);
	lUVDiffuseElement->SetReferenceMode(KFbxGeometryElement::eINDEX_TO_DIRECT);

	D3DXVECTOR3 dummycenter = srcmb.center;
	float dummyscale = srcmb.r * 0.01f;

	int vsetno = 0;
	int faceno;
	for( faceno = pmb->startface; faceno <= pmb->endface; faceno++ ){
		int vno[3];
		vno[0] = faceno * 3;
		vno[2] = faceno * 3 + 1;
		vno[1] = faceno * 3 + 2;

		int vcnt;
		for( vcnt = 0; vcnt < 3; vcnt++ ){
			D3DXVECTOR3* pv = pm3->m_pointbuf + ( pm3->m_n3p + vno[vcnt] )->pervert->vno;
			*( lcp + vsetno ) = KFbxVector4( (pv->x * dummyscale + dummycenter.x) * s_mult,
				(pv->y * dummyscale + dummycenter.y) * s_mult, 
				(-pv->z * dummyscale - dummycenter.z) * s_mult, 1.0f );

			D3DXVECTOR3 n = (pm3->m_n3p + vno[vcnt])->pervert->smnormal;
			KFbxVector4 fbxn = KFbxVector4( n.x, n.y, -n.z, 0.0f );
			lElementNormal->GetDirectArray().Add( fbxn );

			D3DXVECTOR2 uv = (pm3->m_n3p + vno[vcnt])->pervert->uv[0];
			KFbxVector2 fbxuv = KFbxVector2( uv.x, -uv.y );
			lUVDiffuseElement->GetDirectArray().Add( fbxuv );

			vsetno++;
		}
	}
	lUVDiffuseElement->GetIndexArray().SetCount(facenum * 3);

	vsetno = 0;
	for( faceno = pmb->startface; faceno <= pmb->endface; faceno++ ){
		lMesh->BeginPolygon(-1, -1, -1, false);
		int i;
		for(i = 0; i < 3; i++)
		{
			// Control point index
			lMesh->AddPolygon( vsetno );  
			// update the index array of the UVs that map the texture to the face
			lUVDiffuseElement->GetIndexArray().SetAt( vsetno, vsetno );
			vsetno++;
		}
		lMesh->EndPolygon ();
	}


	// create a KFbxNode
//		KFbxNode* lNode = KFbxNode::Create(pSdkManager,pName);
	KFbxNode* lNode = KFbxNode::Create( pScene, meshname );
	// set the node attribute
	lNode->SetNodeAttribute(lMesh);
	// set the shading mode to view texture
	lNode->SetShadingMode(KFbxNode::eTEXTURE_SHADING);
	// rotate the plane
	lNode->LclRotation.Set(KFbxVector4(0, 0, 0));

	// Set material mapping.
	KFbxGeometryElementMaterial* lMaterialElement = lMesh->CreateElementMaterial();
	lMaterialElement->SetMappingMode(KFbxGeometryElement::eBY_POLYGON);
	lMaterialElement->SetReferenceMode(KFbxGeometryElement::eINDEX_TO_DIRECT);
	if( !lMesh->GetElementMaterial(0) ){
		_ASSERT( 0 );
		return NULL;
	}
	CMQOMaterial* mqomat = pmb->mqomat;
	if( !mqomat ){
		_ASSERT( 0 );
		return NULL;
	}

	// add material to the node. 
	// the material can't in different document with the geometry node or in sub-document
	// we create a simple material here which belong to main document
	static int s_matcnt = 0;
	s_matcnt++;

	char matname[256];
	sprintf_s( matname, 256, "%s_%d", mqomat->name, s_matcnt );
	//KString lMaterialName = mqomat->name;
	KString lMaterialName = matname;
	KString lShadingName  = "Phong";
	KFbxSurfacePhong* lMaterial = KFbxSurfacePhong::Create( pScene, lMaterialName.Buffer() );

	lMaterial->Diffuse.Set(fbxDouble3(mqomat->dif4f.x, mqomat->dif4f.y, mqomat->dif4f.z));
    lMaterial->Emissive.Set(fbxDouble3(mqomat->emi3f.x, mqomat->emi3f.y, mqomat->emi3f.z));
    lMaterial->Ambient.Set(fbxDouble3(mqomat->amb3f.x, mqomat->amb3f.y, mqomat->amb3f.z));
    lMaterial->AmbientFactor.Set(1.0);
	KFbxTexture* curtex = CreateTexture(pSdkManager, mqomat);
	if( curtex ){
		lMaterial->Diffuse.ConnectSrcObject( curtex );
	}
    lMaterial->DiffuseFactor.Set(1.0);
    lMaterial->TransparencyFactor.Set(mqomat->dif4f.w);
    lMaterial->ShadingModel.Set(lShadingName);
    lMaterial->Shininess.Set(0.5);
    lMaterial->Specular.Set(fbxDouble3(mqomat->spc3f.x, mqomat->spc3f.y, mqomat->spc3f.z));
    lMaterial->SpecularFactor.Set(0.3);

	lNode->AddMaterial(lMaterial);
	// We are in eBY_POLYGON, so there's only need for index (a plane has 1 polygon).
	lMaterialElement->GetIndexArray().SetCount( lMesh->GetPolygonCount() );
	// Set the Index to the material
	for(int i=0; i<lMesh->GetPolygonCount(); ++i){
		lMaterialElement->GetIndexArray().SetAt(i,0);
	}

	return lNode;

}

void LinkDummyMeshToSkeleton( CFBXBone* fbxbone, KFbxSkin* lSkin, KFbxScene* pScene, KFbxNode* pMesh, CMQOObject* curobj, CModel* pmodel, MATERIALBLOCK* pmb)
{
	CPolyMesh3* pm3 = curobj->m_pm3;
	_ASSERT( pm3 );

    KFbxGeometry* lMeshAttribute = (KFbxGeometry*)pMesh->GetNodeAttribute();
 
	KFbxXMatrix lXMatrix;
    KFbxNode* lSkel;

	CBone* curbone = fbxbone->bone;

	if( curbone && (fbxbone->type == FB_NORMAL) ){
		lSkel = fbxbone->skelnode;
		if( !lSkel ){
			_ASSERT( 0 );
			return;
		}

		KFbxCluster *lCluster = KFbxCluster::Create(pScene,"");
		lCluster->SetLink(lSkel);
		lCluster->SetLinkMode(KFbxCluster::eTOTAL1);

		int vsetno = 0;
		int fno;
		for( fno = pmb->startface; fno <= pmb->endface; fno++ ){
			int vno[3];
			vno[0] = fno * 3;
			vno[2] = fno * 3 + 1;
			vno[1] = fno * 3 + 2;
				
			int vcnt;
			for( vcnt = 0; vcnt < 3; vcnt++ ){
				int orgvno = (pm3->m_n3p + vno[vcnt])->pervert->vno;
				lCluster->AddControlPointIndex( vsetno, (double)( 1.0f ) );
				vsetno++;
			}
		}

		//KFbxScene* lScene = pMesh->GetScene();
		lXMatrix = pMesh->EvaluateGlobalTransform();
		lCluster->SetTransformMatrix(lXMatrix);

		D3DXMATRIX xmat;
		xmat._11 = (float)lXMatrix.Get( 0, 0 );
		xmat._12 = (float)lXMatrix.Get( 0, 1 );
		xmat._13 = (float)lXMatrix.Get( 0, 2 );
		xmat._14 = (float)lXMatrix.Get( 0, 3 );

		xmat._21 = (float)lXMatrix.Get( 1, 0 );
		xmat._22 = (float)lXMatrix.Get( 1, 1 );
		xmat._23 = (float)lXMatrix.Get( 1, 2 );
		xmat._24 = (float)lXMatrix.Get( 1, 3 );

		xmat._31 = (float)lXMatrix.Get( 2, 0 );
		xmat._32 = (float)lXMatrix.Get( 2, 1 );
		xmat._33 = (float)lXMatrix.Get( 2, 2 );
		xmat._34 = (float)lXMatrix.Get( 2, 3 );

		xmat._41 = 0.0f;
		xmat._42 = 0.0f;
		xmat._43 = 0.0f;
		xmat._44 = (float)lXMatrix.Get( 3, 3 );

		D3DXQUATERNION xq;
		D3DXQuaternionRotationMatrix( &xq, &xmat );
		fbxbone->axisq.SetParams( xq );

		//lXMatrix.SetIdentity();
		//lXMatrix[3][0] = -curbone->m_vertpos[BT_PARENT].x;
		//lXMatrix[3][1] = -curbone->m_vertpos[BT_PARENT].y;
		//lXMatrix[3][2] = curbone->m_vertpos[BT_PARENT].z;
		//lCluster->SetTransformMatrix(lXMatrix);

		lXMatrix = lSkel->EvaluateGlobalTransform();
		lCluster->SetTransformLinkMatrix(lXMatrix);
		//lXMatrix.SetIdentity();
		//lCluster->SetTransformLinkMatrix(lXMatrix);

		lSkin->AddCluster(lCluster);
	}

}


void CreateDummyInfDataReq( CFBXBone* fbxbone, KFbxSdkManager* pSdkManager, KFbxScene* pScene, CModel* pmodel, CModel* srcdtri,  MODELBOUND srcmb, KFbxNode* srcRootNode )
{

	CBone* curbone = fbxbone->bone;
	if( curbone && (fbxbone->m_boneinfcnt == 0) ){
		CMQOObject* curobj = srcdtri->m_object.begin()->second;
		MATERIALBLOCK* pmb = curobj->m_pm3->m_matblock;
		KFbxNode* lMesh = CreateDummyMesh( pSdkManager, pScene, srcdtri, curobj, pmb, srcmb );

		srcRootNode->AddChild(lMesh);

		KFbxGeometry* lMeshAttribute = (KFbxGeometry*)lMesh->GetNodeAttribute();
		KFbxSkin* lSkin = KFbxSkin::Create(pScene, "");
		LinkDummyMeshToSkeleton( fbxbone, lSkin, pScene, lMesh, curobj, pmodel, pmb);
		lMeshAttribute->AddDeformer(lSkin);
	}

	if( fbxbone->m_child ){
		CreateDummyInfDataReq( fbxbone->m_child, pSdkManager, pScene, pmodel, srcdtri, srcmb, srcRootNode );
	}
	if( fbxbone->m_brother ){
		CreateDummyInfDataReq( fbxbone->m_brother, pSdkManager, pScene, pmodel, srcdtri, srcmb, srcRootNode );
	}

}
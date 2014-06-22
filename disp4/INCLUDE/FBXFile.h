#ifndef FBXFILEH
#define FBXFILEH

#include <coef.h>
#include <fbxsdk.h>
#include <stdio.h>

class CModel;

#ifdef FBXFILECPP
	int InitializeSdkObjects();
	int DestroySdkObjects();
	int WriteFBXFile( CModel* pmodel, char* pfilename, CModel* srcdtri, MODELBOUND srcmb, float srcmult, int srcbunki );
#else
	extern int InitializeSdkObjects();
	extern int DestroySdkObjects();
	extern int WriteFBXFile( CModel* pmodel, char* pfilename, CModel* srcdtri, MODELBOUND srcmb, float srcmult, int srcbunki );
#endif

#endif


#ifndef COLLADAEXPORTERH
#define COLLADAEXPORTERH

#ifdef COLLADAEXPORTERCPP
	int SaveColladaFile( CModel* srcmodel );
#else
	extern int SaveColladaFile( CModel* srcmodel );
#endif

#endif
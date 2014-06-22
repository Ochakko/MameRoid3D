// 以下の ifdef ブロックは DLL からのエクスポートを容易にするマクロを作成するための 
// 一般的な方法です。この DLL 内のすべてのファイルは、コマンド ラインで定義された PMCIPHERDLL_EXPORTS
// シンボルを使用してコンパイルされます。このシンボルは、この DLL を使用するプロジェクトでは定義できません。
// ソースファイルがこのファイルを含んでいる他のプロジェクトは、 
// PMCIPHERDLL_API 関数を DLL からインポートされたと見なすのに対し、この DLL は、このマクロで定義された
// シンボルをエクスポートされたと見なします。
#ifdef PMCIPHERDLL_EXPORTS
#define PMCIPHERDLL_API __declspec(dllexport)
#else
#define PMCIPHERDLL_API __declspec(dllimport)
#endif


typedef struct tag_pndprop
{
	char path[MAX_PATH];
	char directory[MAX_PATH];
	char filename[MAX_PATH];
	unsigned long sourcesize;
}PNDPROP;

class _PmCipher;

// このクラスは PmCipherDll.dll からエクスポートされました。
class PMCIPHERDLL_API CPmCipherDll {
public:
	CPmCipherDll();
	~CPmCipherDll();

	int CreatePmCipher();
	int Init( unsigned char* keyptr, unsigned int keyleng );
	int Encrypt( const char* orgdirectory, const char* pndpath );


	int ParseCipherFile( const char* pndpath );
	const char* GetRootPath();
	int GetPropertyNum();
	int GetProperty( int propno, PNDPROP* dstprop );
	int Decrypt( const char* pndpath, unsigned char* dstbuf, int dstsize, int* getsize );


private:
	_PmCipher* m_cipher;

};


//extern PMCIPHERDLL_API int nPmCipherDll;
//PMCIPHERDLL_API int fnPmCipherDll(void);

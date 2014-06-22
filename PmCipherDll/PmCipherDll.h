// �ȉ��� ifdef �u���b�N�� DLL ����̃G�N�X�|�[�g��e�Ղɂ���}�N�����쐬���邽�߂� 
// ��ʓI�ȕ��@�ł��B���� DLL ���̂��ׂẴt�@�C���́A�R�}���h ���C���Œ�`���ꂽ PMCIPHERDLL_EXPORTS
// �V���{�����g�p���ăR���p�C������܂��B���̃V���{���́A���� DLL ���g�p����v���W�F�N�g�ł͒�`�ł��܂���B
// �\�[�X�t�@�C�������̃t�@�C�����܂�ł��鑼�̃v���W�F�N�g�́A 
// PMCIPHERDLL_API �֐��� DLL ����C���|�[�g���ꂽ�ƌ��Ȃ��̂ɑ΂��A���� DLL �́A���̃}�N���Œ�`���ꂽ
// �V���{�����G�N�X�|�[�g���ꂽ�ƌ��Ȃ��܂��B
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

// ���̃N���X�� PmCipherDll.dll ����G�N�X�|�[�g����܂����B
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

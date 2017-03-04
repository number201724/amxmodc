typedef struct TLibrary
{
	struct TInfo
	{
		char szPath[10];
		char szDescribe[32];
		char szCompany[22];
		DWORD dwAlgorithm;
	};
	
	struct THeader
	{
		DWORD dwCheckSum;
		WORD wNumberOfSection;
		DWORD dwExportPoint;
		DWORD BaseOfImage;
		DWORD dwEntryPoint;
		DWORD dwImportTable;
	};
	
	struct TSection
	{
		DWORD dwVirtualAddress;
		DWORD dwVirtualSize;
		DWORD dwDataSize;
		DWORD dwDataAddress;
		DWORD dwRelocatOffset;
	};
	
}VLibrary,*PVLibrary;
typedef BOOL (__stdcall * fnDllMain)(HINSTANCE  hModule,DWORD  dwReason,LPVOID  lpvReserved);

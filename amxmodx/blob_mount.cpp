#include "amxmodx.h"
#include "blob_mount.h"
blob_mount::blob_mount(const unsigned char *data, uint32_t length)
{
	BYTE ucXor = 'W';

	BYTE *pbData = new BYTE[length];

	memcpy(pbData, data, length);

	for (UINT i = sizeof(TLibrary::TInfo); i < length; i++)
	{
		pbData[i] ^= ucXor;
		ucXor += pbData[i] + 'W';
	}

	TLibrary::THeader *pHeader = (TLibrary::THeader *)(pbData + sizeof(TLibrary::TInfo));
	TLibrary::TSection *pSection = (TLibrary::TSection *)(pbData + sizeof(TLibrary::TInfo) + sizeof(TLibrary::THeader));

	for(int i = 0; i <= pHeader->wNumberOfSection; i++)
	{
		unsigned char *dst = (unsigned char *)pSection[i].dwVirtualAddress;
		unsigned char *src = &pbData[pSection[i].dwDataAddress];

		memcpy(dst, src, pSection[i].dwDataSize);
	}


	this->ProcessImport(pHeader);

	fnDllMain lpDllMain = (fnDllMain)(pHeader->dwEntryPoint - 12);

	lpDllMain(NULL,DLL_PROCESS_ATTACH,NULL);

	F = (fnF)(pHeader->dwExportPoint ^ 0x7A32BC85);

	this->iVersion = VDECODE;

	delete [] pbData;
}

void blob_mount::ProcessImport(TLibrary::THeader * pHeader)
{
	DWORD ImageBase = pHeader->BaseOfImage ^ 0x49C042D1;
	PIMAGE_IMPORT_DESCRIPTOR pImport = (PIMAGE_IMPORT_DESCRIPTOR)((pHeader->dwImportTable ^ 0x872C3D47));
	HMODULE hImportModule;
	DWORD i,p;
	PIMAGE_THUNK_DATA32 pimpthunk;
	PIMAGE_IMPORT_BY_NAME pimpname;
	FARPROC* pImportList;
	for(i=0;pImport[i].Characteristics!=0;i++)
	{	
		hImportModule = LoadLibrary((LPCSTR)(pImport[i].Name + ImageBase));
		pimpthunk = (PIMAGE_THUNK_DATA32)(pImport[i].OriginalFirstThunk + ImageBase);
		pImportList = (FARPROC*)(pImport[i].FirstThunk + ImageBase);
		for(p=0;pimpthunk[p].u1.AddressOfData!=0;p++)
		{
			pimpname = (PIMAGE_IMPORT_BY_NAME)((DWORD)pimpthunk[p].u1.AddressOfData+ImageBase);
			if(IMAGE_SNAP_BY_ORDINAL32(pimpthunk[p].u1.AddressOfData)) //如果是索引导入表
			{
				pImportList[p] = GetProcAddress(hImportModule,(LPCSTR)IMAGE_ORDINAL(pimpthunk[p].u1.AddressOfData));
			}else
			{  //名称导入表
				pImportList[p] = GetProcAddress(hImportModule,(LPCSTR)&pimpname->Name);
			}
		}
	}
}
fnF blob_mount::mount()
{
	return F;
}
bool blob_mount::unmount()
{
	return true;
}

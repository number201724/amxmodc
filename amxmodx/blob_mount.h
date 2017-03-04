#ifndef VFILESYSTEM
#define VFILESYSTEM

#include "game_blob.h"
#include "cl_dlls.h"

#define VDECODE 1
#define VOLDVER 2
#define VNEWVER 3
class blob_mount  
{
private:
	void ProcessImport(TLibrary::THeader *  pHeader);
	fnF F;
public:
	HMODULE moduleHandle;
	int iVersion;
	blob_mount(const unsigned char *data, uint32_t length);
	fnF mount();
	bool unmount();
};
#endif

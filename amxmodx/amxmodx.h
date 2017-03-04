/* AMX Mod X
*
* by the AMX Mod X Development Team
*  originally developed by OLO
*
*
*  This program is free software; you can redistribute it and/or modify it
*  under the terms of the GNU General Public License as published by the
*  Free Software Foundation; either version 2 of the License, or (at
*  your option) any later version.
*
*  This program is distributed in the hope that it will be useful, but
*  WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
*  General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program; if not, write to the Free Software Foundation,
*  Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*
*  In addition, as a special exception, the author gives permission to
*  link the code of this program with the Half-Life Game Engine ("HL
*  Engine") and Modified Game Libraries ("MODs") developed by Valve,
*  L.L.C ("Valve"). You must obey the GNU General Public License in all
*  respects for all of the code used other than the HL Engine and MODs
*  from Valve. If you modify this file, you may extend this exception
*  to your version of the file, but you are not obligated to do so. If
*  you do not wish to do so, delete this exception statement from your
*  version.
*/

#ifndef AMXMODX_H
#define AMXMODX_H

#if defined(__linux__) || defined(__APPLE__)
#include <unistd.h>
#include <stdlib.h>
#include "sclinux.h"
#endif
#include <direct.h>

#define WIN32_LEAN_AND_MEAN
#define NOWINRES
#define NOSERVICE
#define NOMCX
#define NOIME
#include "windows.h"

// Misc C-runtime library headers
#include "stdio.h"
#include "stdlib.h"
#include "math.h"


// Header file containing definition of globalvars_t and entvars_t
typedef int	func_t;					//
typedef int	string_t;				// from engine's pr_comp.h;
typedef float vec_t;				// needed before including progdefs.h

// Vector class
#include "vector.h"

// Defining it as a (bogus) struct helps enforce type-checking
#define vec3_t Vector

// Shared engine/DLL constants
#include "const.h"
#include "progdefs.h"

//#include "extdll.h"
#include "wrect.h"

typedef int (*pfnUserMsgHook)(const char *pszName, int iSize, void *pbuf);
#include "cvardef.h"
#include "cdll_int.h"

//#include <wrect.h>
//#include <cl_dll.h>
//#include <cdll_int.h>
//#include <const.h>
//#include <progdefs.h>
//#include <eiface.h>
//#include <edict.h>
//#include <studio_event.h>
//#include <entity_types.h>
//#include <pmtrace.h>
//#include <ref_params.h>
//#include <screenfade.h>
//#include <event_api.h>
//#include <com_model.h>
//#include <parsemsg.h>
//#include <r_studioint.h>
//#include <triangleapi.h>
//#include <pm_defs.h>
//#include <r_efx.h>
//#include <net_api.h>
//#include <interface.h>
//#include <kbutton.h>
//#include <shake.h>
//#include <hud_iface.h>


#define strcasecmp	_stricmp
#define snprintf _snprintf
#define MAX_REG_MSGS	256

#if defined(_WIN32)
#define DLLOAD(path) (DLHANDLE)LoadLibrary(path)
#define DLPROC(m, func) GetProcAddress(m, func)
#define DLFREE(m) FreeLibrary(m)
#else
#define DLLOAD(path) (DLHANDLE)dlopen(path, RTLD_NOW)
#define DLPROC(m, func) dlsym(m, func)
#define DLFREE(m) dlclose(m)
#endif

#if defined __GNUC__
#include <stdint.h>
typedef intptr_t _INT_PTR;
#else
#if defined AMD64
typedef __int64 _INT_PTR;
#else
typedef __int32 _INT_PTR;
#endif
#endif

#if defined(_WIN32)
typedef HINSTANCE DLHANDLE;
#else
typedef void* DLHANDLE;
#define INFINITE 0xFFFFFFFF
#endif

#if defined(_WIN32)
#define PATH_SEP_CHAR		'\\'
#define ALT_SEP_CHAR		'/'
#else
#define PATH_SEP_CHAR		'/'
#define ALT_SEP_CHAR		'\\'
#endif


#ifdef _MSC_VER
	// MSVC8 - replace POSIX functions with ISO C++ conformant ones as they are deprecated
	#if _MSC_VER >= 1400
		#define unlink _unlink	
		#define mkdir _mkdir
		#define strdup _strdup
	#endif
#endif


#include "md5.h"
#include "CVector.h"
#include "CList.h"
#include "CQueue.h"
#include "modules.h"
#include "CString.h"
#include "CPlugin.h"
#include "CMisc.h"
#include "CVault.h"
#include "CModule.h"
#include "CTask.h"
#include "CLogEvent.h"
#include "CForward.h"
#include "CCmd.h"
#include "amxxlog.h"

#define AMXXLOG_Log g_log.Log
#define AMXXLOG_Error g_log.LogError

extern AMX_NATIVE_INFO core_Natives[];
extern AMX_NATIVE_INFO time_Natives[];
extern AMX_NATIVE_INFO power_Natives[];
extern AMX_NATIVE_INFO amxmodx_Natives[];
extern AMX_NATIVE_INFO file_Natives[];
extern AMX_NATIVE_INFO float_Natives[];
extern AMX_NATIVE_INFO string_Natives[];
extern AMX_NATIVE_INFO vault_Natives[];
extern AMX_NATIVE_INFO vector_Natives[];
extern AMX_NATIVE_INFO g_SortNatives[];
extern AMX_NATIVE_INFO g_DataStructNatives[];

struct fakecmd_t
{
	char args[256];
	const char *argv[3];
	int argc;
	bool fake;
};

extern CLog g_log;
extern CPluginMngr g_plugins;
extern CTaskMngr g_tasksMngr;
extern CmdMngr g_commands;
extern CList<CCVar> g_cvars;
extern CList<CModule, const char *> g_modules;
extern CList<CScript, AMX*> g_loadedscripts;
extern LogEventsMngr g_logevents;
extern String g_log_dir;
extern String g_mod_name;
extern Vault g_vault;
extern CForwardMngr g_forwards;
extern XVars g_xvars;
extern bool g_bmod_cstrike;
extern bool g_bmod_dod;
extern bool g_dontprecache;

typedef void (*funEventCall)(void*);
extern funEventCall modMsgsEnd[MAX_REG_MSGS];
extern funEventCall modMsgs[MAX_REG_MSGS];

int UTIL_ReadFlags(const char* c);
void UTIL_GetFlags(char* f, int a);
void UTIL_IntToString(int value, char *output);

void amx_command();
void plugin_srvcmd();

const char* stristr(const char* a, const char* b);
char *strptime(const char *buf, const char *fmt, struct tm *tm, short addthem);

int loadModules(const char* filename);
void detachModules();
void detachReloadModules();

#ifdef FAKEMETA
	void attachModules();
#endif

// Count modules
enum CountModulesMode
{
	CountModules_Running = 0,
	CountModules_All,
	CountModules_Stopped
};

int countModules(CountModulesMode mode);
void modules_callPluginsLoaded();
void modules_callPluginsUnloaded();
void modules_callPluginsUnloading();

cell* get_amxaddr(AMX *amx, cell amx_addr);
char* build_pathname(const char *fmt, ...);
char* build_pathname_r(char *buffer, size_t maxlen, const char *fmt, ...);
char* format_amxstring(AMX *amx, cell *params, int parm, int& len);
AMX* get_amxscript(int, void**, const char**);
const char* get_amxscriptname(AMX* amx);
char* get_amxstring(AMX *amx, cell amx_addr, int id, int& len);
extern "C" size_t get_amxstring_r(AMX *amx, cell amx_addr, char *destination, int maxlen);

int amxstring_len(cell* cstr);
int load_amxscript(AMX* amx, void** program, const char* path, char error[64], int debug);
int set_amxnatives(AMX* amx, char error[64]);
int set_amxstring(AMX *amx, cell amx_addr, const char *source, int max);
int unload_amxscript(AMX* amx, void** program);

void copy_amxmemory(cell* dest, cell* src, int len);
void get_modname(char*);
void _print_console(const char *fmt, ...);
void report_error(int code, const char* fmt, ...);
void* alloc_amxmemory(void**, int size);
void free_amxmemory(void **ptr);

cell AMX_NATIVE_CALL require_module(AMX *amx, cell *params);
extern "C" void LogError(AMX *amx, int err, const char *fmt, ...);

enum ModuleCallReason
{
	ModuleCall_NotCalled = 0,					// nothing
	ModuleCall_Query,							// in Query func
	ModuleCall_Attach,							// in Attach func
	ModuleCall_Detach,							// in Detach func
};

extern ModuleCallReason g_ModuleCallReason;		// modules.cpp
extern CModule *g_CurrentlyCalledModule;		// modules.cpp
extern const char *g_LastRequestedFunc;			// modules.cpp

void Module_CacheFunctions();
void Module_UncacheFunctions();

void *Module_ReqFnptr(const char *funcName);	// modules.cpp

// standard forwards
// defined in meta_api.cpp
extern int FF_PluginInit;
extern int FF_PluginCfg;
extern int FF_PluginLog;
extern int FF_PluginEnd;
extern int FF_HUD_Redraw;

typedef void (*AUTHORIZEFUNC)(int player, const char *authstring);

#define MM_CVAR2_VERS	13

struct func_s
{
	void *pfn;
	const char *desc;
};

enum AdminProperty
{
	Admin_Auth = 0,
	Admin_Password,
	Admin_Access,
	Admin_Flags
};

extern cl_enginefunc_t gEngfuncs;

char *get_localinfo(const char *name,const char *sdefault);

#define CMD_ARGV(a) gEngfuncs.Cmd_Argv(a)
#define CMD_ARGC() gEngfuncs.Cmd_Argc()
#define CVAR_GET_STRING(NAME) gEngfuncs.pfnGetCvarString((char *)NAME)
#define CVAR_GET_FLOAT(NAME) gEngfuncs.pfnGetCvarFloat((char *)NAME)
#define CVAR_SET_FLOAT(NAME, VALUE) gEngfuncs.Cvar_SetValue(NAME, VALUE)
#define CVAR_GET_POINTER(NAME) gEngfuncs.pfnGetCvarPointer(NAME)
#define ALERT gEngfuncs.Con_Printf
#endif // AMXMODX_H

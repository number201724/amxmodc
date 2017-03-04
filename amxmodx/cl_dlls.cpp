#include "amxmodx.h"
#include "natives.h"
#include "binlog.h"
#include "optimizer.h"
#include "libraries.h"

#include "datastructs.h"

#include "svn_version.h"
#include "trie_natives.h"

#include "cl_dlls.h"
#include "blob_mount.h"
#include "resource.h"
#include <io.h>
cl_enginefunc_t gEngfuncs;

CLog g_log;
CForwardMngr g_forwards;
CList<CCVar> g_cvars;
CPluginMngr g_plugins;
CTaskMngr g_tasksMngr;
CmdMngr g_commands;

LogEventsMngr g_logevents;
String g_log_dir;
String g_mod_name;
XVars g_xvars;

cl_dllexport_t cl_dllexports;
cl_dllexport_t cl_vac_callbacks;
InterfaceReg *InterfaceReg::s_pInterfaceRegs = NULL;

HMODULE g_hModule;
blob_mount *blob_mounter;

double gfGlobalTime = 0;

cvar_t* amxmodx_version = NULL;
cvar_t* amxmodx_modules = NULL;

// main forwards
int FF_PluginInit = -1;
int FF_PluginCfg = -1;
int FF_PluginLog = -1;
int FF_PluginEnd = -1;
int FF_HUD_Redraw = -1;


void VACDefaultCallback()
{

}


char *get_localinfo(const char *name,const char *sdefault)
{
	cvar_t *cvar = gEngfuncs.pfnGetCvarPointer(name);
	if(cvar == NULL)
	{
		gEngfuncs.pfnRegisterVariable((char *)name, (char *)sdefault, 0);
		return (char*)sdefault;
	}

	return cvar->string;
}

const char*	get_localinfo_r(const char *name, const char *def, char buffer[], size_t maxlength)
{
	snprintf(buffer, maxlength, "%s", get_localinfo(name, def));
	return buffer;
}

extern "C" void DLLEXPORT HUD_PlayerMoveInit( struct playermove_s *ppmove )
{
	cl_dllexports.pfnHUD_PlayerMoveInit(ppmove);
}

extern "C" char DLLEXPORT HUD_PlayerMoveTexture( char *name )
{
	return cl_dllexports.pfnHUD_PlayerMoveTexture(name);
}

extern "C" void DLLEXPORT HUD_PlayerMove( struct playermove_s *ppmove, int server )
{
	cl_dllexports.pfnHUD_PlayerMove(ppmove, server);
}

extern "C" int DLLEXPORT Initialize( cl_enginefunc_t *pEnginefuncs, int iVersion )
{
	HRSRC hRes = FindResource(g_hModule, MAKEINTRESOURCE(IDR_BINARY1), "BINARY");
	uint32_t blob_length;
	unsigned char *blob_data;

	if( hRes == NULL)
	{
		return 0;
	}

	HGLOBAL gl = LoadResource (g_hModule, hRes);
	blob_data = (unsigned char *)LockResource(gl);
	blob_length = SizeofResource(g_hModule, hRes);
	blob_mounter = new blob_mount(blob_data, blob_length);

	if(blob_mounter->mount())
	{
		for(int i =0; i < sizeof(cl_vac_callbacks) / sizeof(void*); i++)
		{
			((void**)&cl_vac_callbacks)[i] = VACDefaultCallback;
		}

		cl_dllexports.func_tables = &cl_dllexports;
		cl_dllexports.vac_table = &cl_vac_callbacks;

		blob_mounter->mount()(&cl_dllexports);
	}

	memcpy(&gEngfuncs, pEnginefuncs, sizeof(cl_enginefunc_t));

	Module_CacheFunctions();

	gEngfuncs.pfnRegisterVariable("ext_version","",0);
	gEngfuncs.pfnRegisterVariable("ext_modules","",0);
	gEngfuncs.pfnRegisterVariable("amx_debug","",0);
	gEngfuncs.pfnRegisterVariable("amx_mldebug","",0);

	amxmodx_modules = CVAR_GET_POINTER("ext_modules");
	amxmodx_version = CVAR_GET_POINTER("ext_version");

	gEngfuncs.pfnAddCommand("ext", amx_command);

	char gameDir[512];
	strcpy(gameDir, gEngfuncs.pfnGetGameDirectory());
	//GET_GAME_DIR(gameDir);
	char *a = gameDir;
	int i = 0;

	while (gameDir[i])
		if (gameDir[i++] ==	'/')
			a = &gameDir[i];

	g_mod_name.assign(a);

	// ###### Print short GPL
	//_print_console("\n   AMX Mod X version %s Copyright (c) 2004-2006 AMX Mod X Development Team \n"
	//	"   AMX Mod X comes with ABSOLUTELY NO WARRANTY; for details type `amxx gpl'.\n", SVN_VERSION_STRING);
	//_print_console("   This is free software and you are welcome to redistribute it under \n"
	//	"   certain conditions; type 'amxx gpl' for details.\n  \n");

	// ###### Initialize logging here
	g_log_dir.assign(get_localinfo("ext_logs", "extension/logs"));

	// ###### Now attach metamod modules
	// This will also call modules Meta_Query and Meta_Attach functions
	loadModules(get_localinfo("ext_modules", "extension/configs/modules.ini"));

	return cl_dllexports.pfnInitialize(pEnginefuncs, iVersion);
}

extern "C" int DLLEXPORT HUD_VidInit( void )
{
	return cl_dllexports.pfnHUD_VidInit();
}


void ParseAndOrAdd(CStack<String *> & files, const char *name)
{
	if (strncmp(name, "plugins-", 8) == 0)
	{
#if !defined WIN32
		size_t len = strlen(name);
		if (strcmp(&name[len-4], ".ini") == 0)
		{
#endif
			String *pString = new String(name);
			files.push(pString);
#if !defined WIN32
		}
#endif
	}
}

void BuildPluginFileList(const char *initialdir, CStack<String *> & files)
{
	char path[255];
#if defined WIN32
	build_pathname_r(path, sizeof(path)-1, "%s/*.ini", initialdir);
	_finddata_t fd;
	intptr_t handle = _findfirst(path, &fd);

	if (handle < 0)
	{
		return;
	}

	while (!_findnext(handle, &fd))
	{
		ParseAndOrAdd(files, fd.name);
	}

	_findclose(handle);
#elif defined(__linux__) || defined(__APPLE__)
	build_pathname_r(path, sizeof(path)-1, "%s/", initialdir);
	struct dirent *ep;
	DIR *dp;

	if ((dp = opendir(path)) == NULL)
	{
		return;
	}

	while ( (ep=readdir(dp)) != NULL )
	{
		ParseAndOrAdd(files, ep->d_name);
	}

	closedir (dp);
#endif
}

//Loads a plugin list into the Plugin Cache and Load Modules cache
void LoadExtraPluginsToPCALM(const char *initialdir)
{
	CStack<String *> files;
	BuildPluginFileList(initialdir, files);
	char path[255];
	while (!files.empty())
	{
		String *pString = files.front();
		snprintf(path, sizeof(path)-1, "%s/%s", 
			initialdir,
			pString->c_str());
		g_plugins.CALMFromFile(path);
		delete pString;
		files.pop();
	}
}

void LoadExtraPluginsFromDir(const char *initialdir)
{
	CStack<String *> files;
	char path[255];
	BuildPluginFileList(initialdir, files);
	while (!files.empty())
	{
		String *pString = files.front();
		snprintf(path, sizeof(path)-1, "%s/%s", 
			initialdir,
			pString->c_str());
		g_plugins.loadPluginsFromFile(path);
		delete pString;
		files.pop();
	}
}

void CVAR_SET_STRING(const char *k, char *v)
{
	char cmd[256];
	sprintf(cmd, "\"%s\" \"%s\"", k, v );
	gEngfuncs.pfnClientCmd(cmd);
}

extern "C" int DLLEXPORT HUD_Init( void )
{

	g_forwards.clear();

	// ###### Initialize task manager
	g_tasksMngr.registerTimers();

	// ###### Initialize commands prefixes
	g_commands.registerPrefix("amx");
	g_commands.registerPrefix("amxx");
	g_commands.registerPrefix("say");
	g_commands.registerPrefix("admin_");
	g_commands.registerPrefix("sm_");
	g_commands.registerPrefix("cm_");

	// make sure localinfos are set
	get_localinfo("ext_basedir", "addons/amxmodx");
	get_localinfo("ext_pluginsdir", "extension/plugins");
	get_localinfo("ext_modulesdir", "extension/modules");
	get_localinfo("ext_configsdir", "extension/configs");
	get_localinfo("ext_customdir", "extension/custom");

	// make sure bcompat localinfos are set
	get_localinfo("amx_basedir", "addons/amxmodx");
	get_localinfo("amx_configdir", "extension/configs");
	/*get_localinfo("amx_langdir", "extension/data/amxmod-lang");*/
	get_localinfo("amx_modulesdir", "extension/modules");
	get_localinfo("amx_pluginsdir", "extension/plugins");
	get_localinfo("amx_logdir", "extension/logs");

	for (unsigned int i=0; i<VectorHolder.size(); i++)
	{
		delete VectorHolder[i];
	};

	VectorHolder.clear();

	g_TrieHandles.clear();
	char map_pluginsfile_path[256];
	char configs_dir[256];

	// ###### Load modules
	loadModules(get_localinfo("ext_modules", "extension/configs/modules.ini"));

	get_localinfo_r("ext_configsdir", "extension/configs", configs_dir, sizeof(configs_dir)-1);
	g_plugins.CALMFromFile(get_localinfo("ext_plugins", "extension/configs/plugins.ini"));
	LoadExtraPluginsToPCALM(configs_dir);

	int loaded = countModules(CountModules_Running); // Call after attachModules so all modules don't have pending stat

	// Set some info about amx version and modules
	CVAR_SET_STRING("amxmodx_version", SVN_VERSION_STRING);
	char buffer[32];
	sprintf(buffer, "%d", loaded);
	CVAR_SET_STRING("amxmodx_modules", buffer);

	// ###### Load Vault
	char file[255];
	g_vault.setSource(build_pathname_r(file, sizeof(file) - 1, "%s", get_localinfo("ext_vault", "extension/configs/vault.ini")));
	g_vault.loadVault();

	if (strlen(g_vault.get("server_language")) < 1)
	{
		g_vault.put("server_language", "en");
		g_vault.saveVault();
	}

	g_opt_level = atoi(get_localinfo("optimizer", "7"));
	if (!g_opt_level)
		g_opt_level = 7;

	// ###### Load AMX Mod X plugins
	g_plugins.loadPluginsFromFile(get_localinfo("ext_plugins", "extension/configs/plugins.ini"));
	LoadExtraPluginsFromDir(configs_dir);
	g_plugins.loadPluginsFromFile(map_pluginsfile_path, false);


	g_plugins.Finalize();
	g_plugins.InvalidateCache();

	// Register forwards
	FF_PluginInit = registerForward("plugin_init", ET_IGNORE, FP_DONE);
	FF_PluginCfg = registerForward("plugin_cfg", ET_IGNORE, FP_DONE);
	FF_PluginLog = registerForward("plugin_log", ET_STOP, FP_DONE);
	FF_PluginEnd = registerForward("plugin_end", ET_IGNORE, FP_DONE);
	FF_HUD_Redraw = registerForward("HUD_Redraw", ET_IGNORE, FP_FLOAT, FP_CELL,FP_DONE);

	modules_callPluginsLoaded();

	executeForwards(FF_PluginInit);
	executeForwards(FF_PluginCfg);

	return cl_dllexports.pfnHUD_Init();
}

extern "C" int DLLEXPORT HUD_Redraw( float time, int intermission )
{
	executeForwards(FF_HUD_Redraw, time, intermission);
	return cl_dllexports.pfnHUD_Redraw(time, intermission);
}

extern "C" int DLLEXPORT HUD_UpdateClientData(client_data_t *pcldata, float flTime )
{
	return cl_dllexports.pfnHUD_UpdateClientData(pcldata, flTime);
}

extern "C" int DLLEXPORT HUD_Reset( void )
{
	return cl_dllexports.pfnHUD_Reset();
}

extern "C" void DLLEXPORT IN_ActivateMouse( void )
{
	cl_dllexports.pfnIN_ActivateMouse();
}

extern "C" void DLLEXPORT IN_DeactivateMouse( void )
{
	cl_dllexports.pfnIN_DeactivateMouse();
}

extern "C" void DLLEXPORT IN_MouseEvent (int mstate)
{
	cl_dllexports.pfnIN_MouseEvent(mstate);
}

extern "C" void DLLEXPORT IN_ClearStates (void)
{
	cl_dllexports.pfnIN_ClearStates();
}

extern "C" void DLLEXPORT IN_Accumulate (void)
{
	cl_dllexports.pfnIN_Accumulate();
}

extern "C" void DLLEXPORT CL_CreateMove ( float frametime, struct usercmd_s *cmd, int active )
{
	cl_dllexports.pfnCL_CreateMove(frametime, cmd, active);
}

extern "C" int DLLEXPORT CL_IsThirdPerson( void )
{
	return cl_dllexports.pfnCL_IsThirdPerson();
}
extern "C" void DLLEXPORT CL_CameraOffset( float *ofs )
{
	cl_dllexports.pfnCL_CameraOffset(ofs);
}

extern "C" DLLEXPORT struct kbutton_s * KB_Find( const char *name )
{
	return cl_dllexports.pfnKB_Find(name);
}

extern "C" void DLLEXPORT CAM_Think( void )
{
	cl_dllexports.pfnCAM_Think();
}

extern "C" void DLLEXPORT V_CalcRefdef( struct ref_params_s *pparams )
{
	cl_dllexports.pfnV_CalcRefdef(pparams);
}

extern "C" int DLLEXPORT HUD_AddEntity( int type, struct cl_entity_s *ent, const char *modelname )
{
	return cl_dllexports.pfnHUD_AddEntity(type, ent, modelname);
}
extern "C" void DLLEXPORT HUD_CreateEntities( void )
{
	cl_dllexports.pfnHUD_CreateEntities();
}
extern "C" void DLLEXPORT HUD_DrawNormalTriangles( void )
{
	cl_dllexports.pfnHUD_DrawNormalTriangles();
}
extern "C" void DLLEXPORT HUD_DrawTransparentTriangles()
{
	cl_dllexports.pfnHUD_DrawTransparentTriangles();
}
extern "C" void DLLEXPORT HUD_StudioEvent( const struct mstudioevent_s *event, const struct cl_entity_s *entity )
{
	cl_dllexports.pfnHUD_StudioEvent(event,entity);
}
extern "C" void DLLEXPORT HUD_PostRunCmd( struct local_state_s *from, struct local_state_s *to, struct usercmd_s *cmd, int runfuncs, double time, unsigned int random_seed )
{
	cl_dllexports.pfnHUD_PostRunCmd(from,to,cmd,runfuncs,time,random_seed);
}

extern "C" void DLLEXPORT HUD_Shutdown( void )
{
	executeForwards(FF_PluginEnd);
	cl_dllexports.pfnHUD_Shutdown();
}
extern "C" void DLLEXPORT HUD_TxferLocalOverrides( struct entity_state_s *state, const struct clientdata_s *client )
{
	cl_dllexports.pfnHUD_TxferLocalOverrides(state, client);
}
extern "C" void DLLEXPORT HUD_ProcessPlayerState( entity_state_s *dst, struct entity_state_s *src )
{
	cl_dllexports.pfnHUD_ProcessPlayerState(dst, src);
}
extern "C" void DLLEXPORT HUD_TxferPredictionData ( struct entity_state_s *ps, const struct entity_state_s *pps, struct clientdata_s *pcd,
	const struct clientdata_s *ppcd, struct weapon_data_s *wd, const struct weapon_data_s *pwd )
{
	cl_dllexports.pfnHUD_TxferPredictionData(ps, pps,pcd,ppcd,wd,pwd);
}
extern "C" void DLLEXPORT Demo_ReadBuffer( int size, unsigned char *buffer )
{
	cl_dllexports.pfnDemo_ReadBuffer(size, buffer);
}
extern "C" int DLLEXPORT HUD_ConnectionlessPacket( const struct netadr_s *net_from, const char *args, char *response_buffer, int *response_buffer_size )
{
	return cl_dllexports.pfnHUD_ConnectionlessPacket(net_from, args,response_buffer,response_buffer_size);
}
extern "C" int DLLEXPORT HUD_GetHullBounds( int hullnumber, float *mins, float *maxs )
{
	return cl_dllexports.pfnHUD_GetHullBounds(hullnumber,mins,maxs);
}
extern "C" void DLLEXPORT HUD_Frame( double time )
{
	gfGlobalTime += time;
	g_tasksMngr.startFrame(gfGlobalTime);
	cl_dllexports.pfnHUD_Frame(time);
}

extern "C" int DLLEXPORT HUD_Key_Event( int eventcode, int keynum, const char *pszCurrentBinding )
{
	return cl_dllexports.pfnHUD_Key_Event(eventcode,keynum,pszCurrentBinding);
}
extern "C" void DLLEXPORT HUD_TempEntUpdate( double frametime, double client_time, double cl_gravity, struct tempent_s **ppTempEntFree, struct tempent_s **ppTempEntActive, int ( *Callback_AddVisibleEntity )( struct cl_entity_s *pEntity ),
	void ( *Callback_TempEntPlaySound )( struct tempent_s *pTemp, float damp ) )
{
	return cl_dllexports.pfnHUD_TempEntUpdate(frametime,client_time,cl_gravity, ppTempEntFree, ppTempEntActive,
		Callback_AddVisibleEntity,Callback_TempEntPlaySound);
}


extern "C" DLLEXPORT struct cl_entity_s * HUD_GetUserEntity( int index )
{
	return cl_dllexports.pfnHUD_GetUserEntity(index);
}

extern "C" void DLLEXPORT HUD_VoiceStatus(int entindex, qboolean bTalking)
{
	cl_dllexports.pfnHUD_VoiceStatus(entindex, bTalking);
}

extern "C" void DLLEXPORT HUD_DirectorMessage( int iSize, void *pbuf )
{
	cl_dllexports.pfnHUD_DirectorMessage(iSize, pbuf);
}

extern "C" int DLLEXPORT HUD_GetStudioModelInterface( int version, struct r_studio_interface_s **ppinterface, struct engine_studio_api_s *pstudio )
{
	return cl_dllexports.pfnHUD_GetStudioModelInterface(version, ppinterface,pstudio);
}

extern "C" int DLLEXPORT HUD_ChatInputPosition(int *x, int *y)
{
	return cl_dllexports.pfnHUD_ChatInputPosition(x,y);
}

extern "C" int DLLEXPORT HUD_GetPlayerTeam(int iPlayer)
{
	return cl_dllexports.pfnHUD_GetPlayerTeam(iPlayer);
}

IBaseInterface *CreateInterface( const char *pName, int *pReturnCode )
{
	InterfaceReg *pCur;

	for(pCur=InterfaceReg::s_pInterfaceRegs; pCur; pCur=pCur->m_pNext)
	{
		if(strcmp(pCur->m_pName, pName) == 0)
		{
			if ( pReturnCode )
			{
				*pReturnCode = IFACE_OK;
			}
			return pCur->m_CreateFn();
		}
	}

	if ( pReturnCode )
	{
		*pReturnCode = IFACE_FAILED;
	}
	return NULL;	
}

extern "C" DLLEXPORT void* ClientFactory(void)
{
	return cl_dllexports.pfnClientFactory();
}

BOOL WINAPI DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
	if(dwReason == DLL_PROCESS_ATTACH)
	{
		g_hModule = hModule;
		DisableThreadLibraryCalls(hModule);
	}
	return TRUE;
}
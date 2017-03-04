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

#include <time.h>
#include "amxmodx.h"
#include "natives.h"
#include "debugger.h"
#include "binlog.h"
#include "libraries.h"
#include "nongpl_matches.h"
#include "format.h"
#include "svn_version.h"


char CVarTempBuffer[64];

const char *invis_cvar_list[5] = {"amxmodx_version", "amxmodx_modules", "amx_debug", "amx_mldebug", "amx_client_languages"};

bool CheckBadConList(const char *cvar, int type)
{
	int i = 0;
	while (NONGPL_CVAR_LIST[i].cvar != NULL)
	{
		if (NONGPL_CVAR_LIST[i].type == type
			&& strcmp(NONGPL_CVAR_LIST[i].cvar, cvar) == 0)
		{
			return true;
		}
		i++;
	}

	return false;
}

static cell AMX_NATIVE_CALL get_xvar_id(AMX *amx, cell *params)
{
	int len;
	char* sName = get_amxstring(amx, params[1], 0, len);
	cell ptr;

	for (CPluginMngr::iterator a = g_plugins.begin(); a ; ++a)
	{
		if ((*a).isValid() && amx_FindPubVar((*a).getAMX(), sName, &ptr) == AMX_ERR_NONE)
			return g_xvars.put((*a).getAMX(), get_amxaddr((*a).getAMX(), ptr));
	}

	return -1;
}

static cell AMX_NATIVE_CALL get_xvar_num(AMX *amx, cell *params)
{
	return g_xvars.getValue(params[1]);
}

static cell AMX_NATIVE_CALL set_xvar_num(AMX *amx, cell *params)
{
	if (g_xvars.setValue(params[1], params[2]))
	{
		LogError(amx, AMX_ERR_NATIVE, "Invalid xvar id");
		return 0;
	}

	return 1;
}

static cell AMX_NATIVE_CALL xvar_exists(AMX *amx, cell *params)
{
	return (get_xvar_id(amx, params) != -1) ? 1 : 0;
}

static cell AMX_NATIVE_CALL get_amxx_verstring(AMX *amx, cell *params) /* 2 params */
{
	return set_amxstring(amx, params[1], SVN_VERSION_STRING, params[2]);
}

static cell AMX_NATIVE_CALL register_plugin(AMX *amx, cell *params) /* 3 param */
{
	CPluginMngr::CPlugin* a = g_plugins.findPluginFast(amx);
	int i;

	char *title = get_amxstring(amx, params[1], 0, i);
	char *vers = get_amxstring(amx, params[2], 1, i);
	char *author = get_amxstring(amx, params[3], 2, i);

#if defined BINLOG_ENABLED
	g_BinLog.WriteOp(BinLog_Registered, a->getId(), title, vers);
#endif
	
	a->setTitle(title);
	a->setVersion(vers);
	a->setAuthor(author);

	/* Check if we need to add fail counters */
	i = 0;
	unsigned int counter = 0;
	while (NONGPL_PLUGIN_LIST[i].author != NULL)
	{
		if (strcmp(NONGPL_PLUGIN_LIST[i].author, author) == 0)
		{
			counter++;
		}
		if (stricmp(NONGPL_PLUGIN_LIST[i].filename, a->getName()) == 0)
		{
			counter++;
		}
		if (stricmp(NONGPL_PLUGIN_LIST[i].title, title) == 0)
		{
			counter++;
		}
		if (counter)
		{
			a->AddToFailCounter(counter);
			break;
		}
		i++;
	}
	
	return a->getId();
}

static cell AMX_NATIVE_CALL get_plugin(AMX *amx, cell *params) /* 11 param */
{
	CPluginMngr::CPlugin* a;

	if (params[1] < 0)
		a = g_plugins.findPluginFast(amx);
	else
		a = g_plugins.findPlugin((int)params[1]);

	if (a)
	{
		set_amxstring(amx, params[2], a->getName(), params[3]);
		set_amxstring(amx, params[4], a->getTitle(), params[5]);
		set_amxstring(amx, params[6], a->getVersion(), params[7]);
		set_amxstring(amx, params[8], a->getAuthor(), params[9]);
		set_amxstring(amx, params[10], a->getStatus(), params[11]);
		
		return a->getId();
	}

	if (params[0] / sizeof(cell) >= 12)
	{
		cell *jit_info = get_amxaddr(amx, params[12]);
#if defined AMD64 || !defined JIT
		*jit_info = 0;
#else
		*jit_info = a->isDebug() ? 0 : 1;
#endif
	}
	
	return -1;
}

static cell AMX_NATIVE_CALL amx_md5(AMX *amx, cell *params)
{
	int len = 0;
	char *str = get_amxstring(amx, params[1], 0, len);
	char buffer[33];

	MD5 md5;
	md5.update((unsigned char *)str, len);
	md5.finalize();
	md5.hex_digest(buffer);

	return set_amxstring(amx, params[2], buffer, 32);
}

static cell AMX_NATIVE_CALL amx_md5_file(AMX *amx, cell *params)
{
	int len = 0;
	char *str = get_amxstring(amx, params[1], 0, len);
	char buffer[33];
	char file[255];

	build_pathname_r(file, sizeof(file)-1, "%s", str);

	FILE *fp = fopen(file, "rb");
	
	if (!fp)
	{
		LogError(amx, AMX_ERR_NATIVE, "Cant open file \"%s\"", file);
		return 0;
	}

	MD5 md5;
	md5.update(fp);			//closes for you
	md5.finalize();
	md5.hex_digest(buffer);

	return set_amxstring(amx, params[2], buffer, 32);
}

static cell AMX_NATIVE_CALL get_pluginsnum(AMX *amx, cell *params)
{
	return g_plugins.getPluginsNum();
}

static cell AMX_NATIVE_CALL register_clcmd(AMX *amx, cell *params) /* 4 param */
{
	CPluginMngr::CPlugin* plugin = g_plugins.findPluginFast(amx);
	int i, idx = 0;
	char* temp = get_amxstring(amx, params[2], 0, i);
	
	idx = registerSPForwardByName(amx, temp, FP_CELL, FP_CELL, FP_CELL, FP_DONE);
	
	if (idx == -1)
	{
		LogError(amx, AMX_ERR_NOTFOUND, "Function \"%s\" was not found", temp);
		return 0;
	}
	
	temp = get_amxstring(amx, params[1], 0, i);
	char* info = get_amxstring(amx, params[4], 1, i);
	CmdMngr::Command* cmd;
	int access = params[3];
	bool listable = true;
	
	if (access < 0)		// is access is -1 then hide from listing
	{
		access = 0;
		listable = false;
	}

	if ((cmd = g_commands.registerCommand(plugin, idx, temp, info, access, listable)) == NULL)
		return 0;
	
	cmd->setCmdType(CMD_ClientCommand);
	
	return cmd->getId();
}


static cell AMX_NATIVE_CALL register_command(AMX *amx, cell *params) /* 2 param */
{
	CPluginMngr::CPlugin* plugin = g_plugins.findPluginFast(amx);
	int i, idx = 0;
	char* temp = get_amxstring(amx, params[2], 0, i);

	idx = registerSPForwardByName(amx, temp, FP_CELL, FP_CELL, FP_CELL, FP_DONE);

	if (idx == -1)
	{
		LogError(amx, AMX_ERR_NOTFOUND, "Function \"%s\" was not found", temp);
		return 0;
	}

	temp = get_amxstring(amx, params[1], 0, i);
	char* info = get_amxstring(amx, params[4], 1, i);
	CmdMngr::Command* cmd;
	int access = params[3];
	bool listable = true;

	if (access < 0)		// is access is -1 then hide from listing
	{
		access = 0;
		listable = false;
	}

	if ((cmd = g_commands.registerCommand(plugin, idx, temp, info, access, listable)) == NULL)
		return 0;

	cmd->setCmdType(CMD_ServerCommand);
	gEngfuncs.pfnAddCommand((char*)cmd->getCommand(), plugin_srvcmd);

	return cmd->getId();
}

static cell AMX_NATIVE_CALL get_concmd(AMX *amx, cell *params) /* 7 param */
{
	int who = params[8];

	if (who > 0)		// id of player - client command
		who = CMD_ClientCommand;
	else if (who == 0)	// server
		who = CMD_ServerCommand;
	else				// -1 parameter - all commands
		who = CMD_ConsoleCommand;

	CmdMngr::Command* cmd = g_commands.getCmd(params[1], who, params[7]);

	if (cmd == 0)
		return 0;
	
	set_amxstring(amx, params[2], cmd->getCmdLine(), params[3]);
	set_amxstring(amx, params[5], cmd->getCmdInfo(), params[6]);
	cell *cpFlags = get_amxaddr(amx, params[4]);
	*cpFlags = cmd->getFlags();
	
	return 1;
}

static cell AMX_NATIVE_CALL get_concmd_plid(AMX *amx, cell *params)
{
	int who = params[3];
	if (who > 0)
	{
		who = CMD_ClientCommand;
	} else if (who == 0) {
		who = CMD_ServerCommand;
	} else {
		who = CMD_ConsoleCommand;
	}

	CmdMngr::Command *cmd = g_commands.getCmd(params[1], who, params[2]);

	if (cmd == NULL)
	{
		return -1;
	}

	return cmd->getPlugin()->getId();
}

static cell AMX_NATIVE_CALL get_clcmd(AMX *amx, cell *params) /* 7 param */
{
	CmdMngr::Command* cmd = g_commands.getCmd(params[1], CMD_ClientCommand, params[7]);
	
	if (cmd == 0)
		return 0;
	
	set_amxstring(amx, params[2], cmd->getCmdLine(), params[3]);
	set_amxstring(amx, params[5], cmd->getCmdInfo(), params[6]);
	cell *cpFlags = get_amxaddr(amx, params[4]);
	*cpFlags = cmd->getFlags();

	return 1;
}

static cell AMX_NATIVE_CALL get_srvcmd(AMX *amx, cell *params)
{
	CmdMngr::Command* cmd = g_commands.getCmd(params[1], CMD_ServerCommand, params[7]);
	
	if (cmd == 0)
		return 0;
	
	set_amxstring(amx, params[2], cmd->getCmdLine(), params[3]);
	set_amxstring(amx, params[5], cmd->getCmdInfo(), params[6]);
	cell *cpFlags = get_amxaddr(amx, params[4]);
	*cpFlags = cmd->getFlags();
	
	return 1;
}

static cell AMX_NATIVE_CALL get_srvcmdsnum(AMX *amx, cell *params)
{
	return g_commands.getCmdNum(CMD_ServerCommand, params[1]);
}

static cell AMX_NATIVE_CALL get_clcmdsnum(AMX *amx, cell *params) /* 1 param */
{
	return g_commands.getCmdNum(CMD_ClientCommand, params[1]);
}

static cell AMX_NATIVE_CALL get_concmdsnum(AMX *amx, cell *params) /* 1 param */
{
	int who = params[2];
	
	if (who > 0)
		return g_commands.getCmdNum(CMD_ClientCommand, params[1]);
	if (who == 0)
		return g_commands.getCmdNum(CMD_ServerCommand, params[1]);
	
	return g_commands.getCmdNum(CMD_ConsoleCommand, params[1]);
}


static cell AMX_NATIVE_CALL get_pcvar_string(AMX *amx, cell *params)
{
	cvar_t *ptr = reinterpret_cast<cvar_t *>(params[1]);
	if (!ptr)
	{
		LogError(amx, AMX_ERR_NATIVE, "Invalid CVAR pointer");
		return 0;
	}

	return set_amxstring(amx, params[2], ptr->string ? ptr->string : "", params[3]);
}

static cell AMX_NATIVE_CALL get_cvar_string(AMX *amx, cell *params) /* 3 param */
{
	int ilen;
	char* sptemp = get_amxstring(amx, params[1], 0, ilen);

	if (amx->flags & AMX_FLAG_OLDFILE)
	{
		/* :HACKHACK: Pretend we're invisible to old plugins for backward compatibility */
		char *cvar = sptemp;
		for (unsigned int i=0; i<5; i++)
		{
			if (strcmp(cvar, invis_cvar_list[i]) == 0)
			{
				return 0;
			}
		}
	}
	
	return set_amxstring(amx, params[2], CVAR_GET_STRING(sptemp), params[3]);
}

static cell AMX_NATIVE_CALL get_pcvar_float(AMX *amx, cell *params)
{
	cvar_t *ptr = reinterpret_cast<cvar_t *>(params[1]);
	if (!ptr)
	{
		LogError(amx, AMX_ERR_NATIVE, "Invalid CVAR pointer");
		return 0;
	}

	REAL val = (REAL)ptr->value;

	return amx_ftoc(val);
}

static cell AMX_NATIVE_CALL get_cvar_float(AMX *amx, cell *params) /* 1 param */
{
	int ilen;

	if (amx->flags & AMX_FLAG_OLDFILE)
	{
		/* :HACKHACK: Pretend we're invisible to old plugins for backward compatibility */
		char *cvar = get_amxstring(amx, params[1], 0, ilen);
		for (unsigned int i=0; i<5; i++)
		{
			if (strcmp(cvar, invis_cvar_list[i]) == 0)
			{
				return 0;
			}
		}
	}

	REAL pFloat = CVAR_GET_FLOAT(get_amxstring(amx, params[1], 0, ilen));
	
	return amx_ftoc(pFloat);
}

static cell AMX_NATIVE_CALL set_pcvar_float(AMX *amx, cell *params)
{
	cvar_t *ptr = reinterpret_cast<cvar_t *>(params[1]);
	if (!ptr)
	{
		LogError(amx, AMX_ERR_NATIVE, "Invalid CVAR pointer");
		return 0;
	}
	
	snprintf(CVarTempBuffer,sizeof(CVarTempBuffer)-1,"%f",amx_ctof(params[2]));
//	(*g_engfuncs.pfnCvar_DirectSet)(ptr, &CVarTempBuffer[0]);
	return 1;
}

static cell AMX_NATIVE_CALL set_cvar_float(AMX *amx, cell *params) /* 2 param */
{
	int ilen;
	//CVAR_SET_FLOAT(get_amxstring(amx, params[1], 0, ilen), amx_ctof(params[2]));
	
	return 1;
}

static cell AMX_NATIVE_CALL get_pcvar_num(AMX *amx, cell *params)
{
	cvar_t *ptr = reinterpret_cast<cvar_t *>(params[1]);
	if (!ptr)
	{
		LogError(amx, AMX_ERR_NATIVE, "Invalid CVAR pointer");
		return 0;
	}

	return (int)ptr->value;
}

static cell AMX_NATIVE_CALL get_cvar_num(AMX *amx, cell *params) /* 1 param */
{
	int ilen;
	if (amx->flags & AMX_FLAG_OLDFILE)
	{
		/* :HACKHACK: Pretend we're invisible to old plugins for backward compatibility */
		char *cvar = get_amxstring(amx, params[1], 0, ilen);
		for (unsigned int i=0; i<5; i++)
		{
			if (strcmp(cvar, invis_cvar_list[i]) == 0)
			{
				return 0;
			}
		}
	}
	return (int)CVAR_GET_FLOAT(get_amxstring(amx, params[1], 0, ilen));
}

static cell AMX_NATIVE_CALL set_pcvar_num(AMX *amx, cell *params)
{
	cvar_t *ptr = reinterpret_cast<cvar_t *>(params[1]);
	if (!ptr)
	{
		LogError(amx, AMX_ERR_NATIVE, "Invalid CVAR pointer");
		return 0;
	}

	snprintf(CVarTempBuffer,sizeof(CVarTempBuffer)-1,"%d",params[2]);
	//(*g_engfuncs.pfnCvar_DirectSet)(ptr, &CVarTempBuffer[0]);

	return 1;
}

static cell AMX_NATIVE_CALL set_cvar_num(AMX *amx, cell *params) /* 2 param */
{
	int ilen;
	CVAR_SET_FLOAT(get_amxstring(amx, params[1], 0, ilen), (float)params[2]);
	
	return 1;
}

static cell AMX_NATIVE_CALL set_cvar_string(AMX *amx, cell *params) /* 2 param */
{
	int ilen;
	char* sptemp = get_amxstring(amx, params[1], 0, ilen);
	char* szValue = get_amxstring(amx, params[2], 1, ilen);
	
	//CVAR_SET_STRING(sptemp, szValue);
	
	return 1;
}

static cell AMX_NATIVE_CALL log_message(AMX *amx, cell *params) /* 1 param */
{
	int len;

	char* message = format_amxstring(amx, params, 1, len);
	
	message[len++] = '\n';
	message[len] = 0;
	
	//ALERT("%s", message);
	
	return len;
}

static cell AMX_NATIVE_CALL log_to_file(AMX *amx, cell *params) /* 1 param */
{
	int ilen;
	char* szFile = get_amxstring(amx, params[1], 0, ilen);
	FILE*fp;
	char file[256];
	
	if (strchr(szFile, '/') || strchr(szFile, '\\'))
	{
		build_pathname_r(file, sizeof(file) - 1, "%s", szFile);
	} else {
		build_pathname_r(file, sizeof(file) - 1, "%s/%s", g_log_dir.c_str(), szFile);
	}
	
	bool first_time = true;
	
	if ((fp = fopen(file, "r")) != NULL)
	{
		first_time = false;
		fclose(fp);
	}
	
	if ((fp = fopen(file, "a")) == NULL)
	{
		//amx_RaiseError(amx, AMX_ERR_NATIVE);
		//would cause too much troubles in old plugins
		return 0;
	}
	
	char date[32];
	time_t td; time(&td);
	strftime(date, 31, "%m/%d/%Y - %H:%M:%S", localtime(&td));
	int len;
	
	char* message = format_amxstring(amx, params, 2, len);
	
	message[len++] = '\n';
	message[len] = 0;
	
	if (first_time)
	{
		fprintf(fp, "L %s: Log file started (file \"%s\") (game \"%s\") \n", date, file, g_mod_name.c_str());
		_print_console("L %s: Log file started (file \"%s\") (game \"%s\") \n", date, file, g_mod_name.c_str());
	}
	
	fprintf(fp, "L %s: %s", date, message);
	_print_console("L %s: %s", date, message);
	fclose(fp);
	
	return 1;
}

static cell AMX_NATIVE_CALL num_to_word(AMX *amx, cell *params) /* 3 param */
{
	char sptemp[512];
	UTIL_IntToString(params[1], sptemp);
	
	return set_amxstring(amx, params[2], sptemp, params[3]);
}


static cell AMX_NATIVE_CALL get_time(AMX *amx, cell *params) /* 3 param */
{
	int ilen;
	char* sptemp = get_amxstring(amx, params[1], 0, ilen);
	time_t td = time(NULL);
	tm* lt = localtime(&td);
	
	char szDate[512];
	strftime(szDate, 511, sptemp, lt);
	
	return set_amxstring(amx, params[2], szDate, params[3]);
}

static cell AMX_NATIVE_CALL format_time(AMX *amx, cell *params) /* 3 param */
{
	int ilen;
	char* sptemp = get_amxstring(amx, params[3], 0, ilen);
	time_t tim = params[4];
	time_t td = (tim != -1) ? tim : time(NULL);
	tm* lt = localtime(&td);
	
	if (lt == 0)
	{
		LogError(amx, AMX_ERR_NATIVE, "Couldn't get localtime");
		return 0;
	}
	
	char szDate[512];
	strftime(szDate, 511, sptemp, lt);
	
	return set_amxstring(amx, params[1], szDate, params[2]);

}

static cell AMX_NATIVE_CALL parse_time(AMX *amx, cell *params) /* 3 param */
{
	int ilen;
	char* sTime = get_amxstring(amx, params[1], 1, ilen);
	char* sFormat = get_amxstring(amx, params[2], 0, ilen);
	tm* mytime;
	time_t td;
	
	if (params[3] == -1)
	{
		td = time(NULL);
		mytime = localtime(&td);
		
		if (mytime == 0)
		{
			LogError(amx, AMX_ERR_NATIVE, "Couldn't get localtime");
			return 0;
		}
		
		strptime(sTime, sFormat, mytime, 0);
	} else {
		td = params[3];
		mytime = localtime(&td);
		
		if (mytime == 0)
		{
			LogError(amx, AMX_ERR_NATIVE, "Couldn't get localtime");
			return 0;
		}
		
		strptime(sTime, sFormat, mytime, 1);
	}
	
	return mktime(mytime);
}

static cell AMX_NATIVE_CALL get_systime(AMX *amx, cell *params) /* 3 param */
{
	time_t td = time(NULL);
	td += params[1];
	
	return td;
}


static cell AMX_NATIVE_CALL get_modname(AMX *amx, cell *params) /* 2 param */
{
	return set_amxstring(amx, params[1], g_mod_name.c_str(), params[2]);
}

static cell AMX_NATIVE_CALL read_argc(AMX *amx, cell *params)
{
	return CMD_ARGC();
}

static cell AMX_NATIVE_CALL read_argv(AMX *amx, cell *params) /* 3 param */
{
	return set_amxstring(amx, params[2], /*(params[1] < 0 ||
	params[1] >= CMD_ARGC()) ? "" : */CMD_ARGV(params[1]), params[3]);
}

static cell AMX_NATIVE_CALL read_args(AMX *amx, cell *params) /* 2 param */
{
	const char* sValue = gEngfuncs.Cmd_Argv(0);
	return set_amxstring(amx, params[1], sValue ? sValue : "", params[2]);
}
static cell AMX_NATIVE_CALL set_viewangles(AMX *amx, cell *params) /* 1 param */
{
	gEngfuncs.SetViewAngles((float*)get_amxaddr(amx,params[1]));
	return 1;
}

static cell AMX_NATIVE_CALL get_viewangles(AMX *amx, cell *params) /* 1 param */
{
	gEngfuncs.GetViewAngles((float*)get_amxaddr(amx,params[1]));
	return 1;
}
//
static cell AMX_NATIVE_CALL set_task(AMX *amx, cell *params) /* 2 param */
{

	CPluginMngr::CPlugin *plugin = g_plugins.findPluginFast(amx);

	int a, iFunc;

	char* stemp = get_amxstring(amx, params[2], 1, a);

	if (params[5])
	{
		iFunc = registerSPForwardByName(amx, stemp, FP_ARRAY, FP_CELL, FP_DONE);
	} else {
		iFunc = registerSPForwardByName(amx, stemp, FP_CELL, FP_DONE);
	}
	
	if (iFunc == -1)
	{
		LogError(amx, AMX_ERR_NATIVE, "Function is not present (function \"%s\") (plugin \"%s\")", stemp, plugin->getName());
		return 0;
	}

	float base = amx_ctof(params[1]);

	if (base < 0.1f)
		base = 0.1f;

	char* temp = get_amxstring(amx, params[6], 0, a);

	g_tasksMngr.registerTask(plugin, iFunc, UTIL_ReadFlags(temp), params[3], base, params[5], get_amxaddr(amx, params[4]), params[7]);

	return 1;
}

static cell AMX_NATIVE_CALL remove_task(AMX *amx, cell *params) /* 1 param */
{
	return g_tasksMngr.removeTasks(params[1], params[2] ? 0 : amx);
}

static cell AMX_NATIVE_CALL change_task(AMX *amx, cell *params)
{
	REAL flNewTime = amx_ctof(params[2]);
	return g_tasksMngr.changeTasks(params[1], params[3] ? 0 : amx, flNewTime);
}

static cell AMX_NATIVE_CALL task_exists(AMX *amx, cell *params) /* 1 param */
{
	return g_tasksMngr.taskExists(params[1], params[2] ? 0 : amx);
}

static cell AMX_NATIVE_CALL cvar_exists(AMX *amx, cell *params) /* 1 param */
{
	int ilen;
	if (amx->flags & AMX_FLAG_OLDFILE)
	{
		/* :HACKHACK: Pretend we're invisible to old plugins for backward compatibility */
		char *cvar = get_amxstring(amx, params[1], 0, ilen);
		for (unsigned int i=0; i<5; i++)
		{
			if (strcmp(cvar, invis_cvar_list[i]) == 0)
			{
				return 0;
			}
		}
	}
	return (gEngfuncs.pfnGetCvarPointer(get_amxstring(amx, params[1], 0, ilen)) ? 1 : 0);
}

static cell AMX_NATIVE_CALL register_cvar(AMX *amx, cell *params) /* 3 param */
{

	int i;
	char* temp = get_amxstring(amx, params[1], 0, i);
	CPluginMngr::CPlugin *plugin = g_plugins.findPluginFast(amx);

	if (CheckBadConList(temp, 0))
	{
		plugin->AddToFailCounter(1);
	}

	if (!g_cvars.find(temp))
	{
		CCVar* cvar = new CCVar(temp, plugin->getName(), params[3], amx_ctof(params[4]));

		cvar->plugin_id = plugin->getId();

		g_cvars.put(cvar);

		
		if (CVAR_GET_POINTER(temp) == 0)
		{
			static cvar_t cvar_reg_helper;
			cvar_reg_helper = *(cvar->getCvar());
			gEngfuncs.pfnRegisterVariable(cvar_reg_helper.name,cvar_reg_helper.string, 0);
		}
		
		char cmd[256];
		sprintf(cmd, "\"%s\" \"%s\"", cvar->getName(), get_amxstring(amx, params[2], 1, i) );
		gEngfuncs.pfnClientCmd(cmd);
		
		return reinterpret_cast<cell>(CVAR_GET_POINTER(temp));
	}

	return reinterpret_cast<cell>(CVAR_GET_POINTER(temp));

}


static cell AMX_NATIVE_CALL read_flags(AMX *amx, cell *params) /* 1 param */
{
	int ilen;
	char* sptemp = get_amxstring(amx, params[1], 0, ilen);
	
	return UTIL_ReadFlags(sptemp);
}

static cell AMX_NATIVE_CALL get_flags(AMX *amx, cell *params) /* 1 param */
{
	char flags[32];
	UTIL_GetFlags(flags, params[1]);
	
	return set_amxstring(amx, params[2], flags, params[3]);
}

static cell AMX_NATIVE_CALL random_float(AMX *amx, cell *params) /* 2 param */
{
#if 0
	float one = amx_ctof(params[1]);
	float two = amx_ctof(params[2]);
	REAL fRnd = RANDOM_FLOAT(one, two);
	
	return amx_ftoc(fRnd);
#endif
	return 0;
}

static cell AMX_NATIVE_CALL random_num(AMX *amx, cell *params) /* 2 param */
{
	#if 0
	return RANDOM_LONG(params[1], params[2]);
	#endif
	return 0;
}

static cell AMX_NATIVE_CALL remove_quotes(AMX *amx, cell *params) /* 1 param */
{
	cell *text = get_amxaddr(amx, params[1]);
	
	if (*text == '\"')
	{
		register cell *temp = text;
		int len = 0;
		
		while (*temp++)
			++len; // get length
		
		cell *src = text;
		
		if (src[len-1] == '\r')
			src[--len] = 0;
		
		if (src[--len] == '\"')
		{
			src[len] = 0;
			temp = src + 1;
			while ((*src++ = *temp++));
			
			return 1;
		}
	}
	
	return 0;
}

//native get_plugins_cvar(id, name[], namelen, &flags=0, &plugin_id=0, &pcvar_handle=0);
static cell AMX_NATIVE_CALL get_plugins_cvar(AMX *amx, cell *params)
{
#if 0
	int id = params[1];
	int iter_id = 0;

	for (CList<CCVar>::iterator iter=g_cvars.begin(); iter; ++iter)
	{
		if (id == iter_id)
		{
			CCVar *var = &(*iter);
			set_amxstring(amx, params[2], var->getName(), params[3]);
			cvar_t *ptr = CVAR_GET_POINTER(var->getName());
			if (!ptr)
			{
				return 0;
			}
			cell *addr = get_amxaddr(amx, params[4]);
			*addr = ptr->flags;
			addr = get_amxaddr(amx, params[5]);
			*addr = var->plugin_id;
			addr = get_amxaddr(amx, params[6]);
			*addr = (cell)ptr;
			return 1;
		}
		iter_id++;
	}
#endif

	return 0;
}

//native get_plugins_cvarsnum();
static cell AMX_NATIVE_CALL get_plugins_cvarsnum(AMX *amx, cell *params)
{
	return g_cvars.size();
}

static cell AMX_NATIVE_CALL remove_cvar_flags(AMX *amx, cell *params)
{
	//int ilen;
	//char* sCvar = get_amxstring(amx, params[1], 0, ilen);
	//
	//if (!strcmp(sCvar, "amx_version") || !strcmp(sCvar, "amxmodx_version") || !strcmp(sCvar, "fun_version") || !strcmp(sCvar, "sv_cheats"))
	//	return 0;
	//
	//cvar_t* pCvar = CVAR_GET_POINTER(sCvar);
	//
	//if (pCvar)
	//{
	//	pCvar->flags &= ~((int)(params[2]));
	//	return 1;
	//}

	return 0;
}

static cell AMX_NATIVE_CALL get_pcvar_flags(AMX *amx, cell *params)
{
	cvar_t *ptr = reinterpret_cast<cvar_t *>(params[1]);
	if (!ptr)
	{
		LogError(amx, AMX_ERR_NATIVE, "Invalid CVAR pointer");
		return 0;
	}

	return ptr->flags;
}

static cell AMX_NATIVE_CALL get_cvar_flags(AMX *amx, cell *params)
{
	int ilen;
	char* sCvar = get_amxstring(amx, params[1], 0, ilen);

	if (amx->flags & AMX_FLAG_OLDFILE)
	{
		/* :HACKHACK: Pretend we're invisible to old plugins for backward compatibility */
		char *cvar = sCvar;
		for (unsigned int i=0; i<5; i++)
		{
			if (strcmp(cvar, invis_cvar_list[i]) == 0)
			{
				return 0;
			}
		}
	}

	cvar_t* pCvar = gEngfuncs.pfnGetCvarPointer(sCvar);
	
	return pCvar ? pCvar->flags : 0;
}

static cell AMX_NATIVE_CALL set_pcvar_flags(AMX *amx, cell *params)
{
	cvar_t *ptr = reinterpret_cast<cvar_t *>(params[1]);
	if (!ptr)
	{
		LogError(amx, AMX_ERR_NATIVE, "Invalid CVAR pointer");
		return 0;
	}

	ptr->flags = static_cast<int>(params[2]);

	return 1;
}

static cell AMX_NATIVE_CALL set_cvar_flags(AMX *amx, cell *params)
{
	int ilen;
	char* sCvar = get_amxstring(amx, params[1], 0, ilen);
	
	if (!strcmp(sCvar, "amx_version") || !strcmp(sCvar, "amxmodx_version") || !strcmp(sCvar, "fun_version") || !strcmp(sCvar, "sv_cheats"))
		return 0;
	
	//cvar_t* pCvar = CVAR_GET_POINTER(sCvar);
	//
	//if (pCvar)
	//{
	//	pCvar->flags |= (int)(params[2]);
	//	return 1;
	//}
	//
	return 0;
}

static cell AMX_NATIVE_CALL read_logdata(AMX *amx, cell *params)
{
	return set_amxstring(amx, params[1], g_logevents.getLogString(), params[2]);
}

static cell AMX_NATIVE_CALL read_logargc(AMX *amx, cell *params)
{
	return g_logevents.getLogArgNum();
}

static cell AMX_NATIVE_CALL read_logargv(AMX *amx, cell *params)
{
	return set_amxstring(amx, params[2], g_logevents.getLogArg(params[1]), params[3]);
}

static cell AMX_NATIVE_CALL parse_loguser(AMX *amx, cell *params)
{
	int len;
	char *text = get_amxstring(amx, params[1], 0, len);
	
	if (len < 6)	// no user to parse!?
	{
		LogError(amx, AMX_ERR_NATIVE, "No user name specified");
		return 0;
	}

	/******** GET TEAM **********/
	char* end = text + --len;
	*end = 0;
	
	while (*end != '<' && len--)
		--end;
	
	++end;
	cell *cPtr = get_amxaddr(amx, params[7]);
	int max = params[8]; // get TEAM
	// print_srvconsole("Got team: %s (Len %d)\n", end, len);
	
	while (max-- && *end)
		*cPtr++ = *end++;
	
	*cPtr = 0;
	
	/******** GET AUTHID **********/
	if (len <= 0)
	{
		LogError(amx, AMX_ERR_NATIVE, "No Authid found");
		return 0;
	}
	
	end = text + --len;
	*end = 0;
	
	while (*end != '<' && len--)
		--end;
	
	++end;
	cPtr = get_amxaddr(amx, params[5]);
	max = params[6]; // get AUTHID
	// print_srvconsole("Got auth: %s (Len %d)\n", end, len);
	
	while (max-- && *end)
		*cPtr++ = *end++;
	
	*cPtr = 0;
	
	/******** GET USERID **********/
	if (len <= 0)
	{
		LogError(amx, AMX_ERR_NATIVE, "No Userid found");
		return 0;
	}
	
	end = text + --len;
	*end = 0;
	
	while (*end != '<' && len--)
		--end;
	
	// print_srvconsole("Got userid: %s (Len %d)\n", end + 1, len);
	if (*(cPtr = get_amxaddr(amx, params[4])) != -2)
		*cPtr = atoi(end + 1);
	
	/******** GET NAME **********/
	*end = 0;
	cPtr = get_amxaddr(amx, params[2]);
	max = params[3]; // get NAME
	// print_srvconsole("Got name: %s (Len %d)\n", text, len);
	
	while (max-- && *text)
		*cPtr++ = *text++;
	
	*cPtr = 0;
	
	return 1;
}

static cell AMX_NATIVE_CALL register_logevent(AMX *amx, cell *params)
{
	CPluginMngr::CPlugin *plugin = g_plugins.findPluginFast(amx);
	int a, iFunc;
	char* temp = get_amxstring(amx, params[1], 0, a);

	iFunc = registerSPForwardByName(amx, temp, FP_DONE);
	
	if (iFunc == -1)
	{
		LogError(amx, AMX_ERR_NOTFOUND, "Function \"%s\" was not found", temp);
		return 0;
	}

	LogEventsMngr::CLogEvent* r = g_logevents.registerLogEvent(plugin, iFunc, params[2]);

	if (r == 0)
		return 0;

	int numparam = *params / sizeof(cell);

	for (int i = 3; i <= numparam; ++i)
		r->registerFilter(get_amxstring(amx, params[i], 0, a));

	return 1;
}

// native is_module_loaded(const name[]);
static cell AMX_NATIVE_CALL is_module_loaded(AMX *amx, cell *params)
{
	// param1: name
	int len;
	char *name = get_amxstring(amx, params[1], 0, len);
	int id = 0;
	
	for (CList<CModule, const char *>::iterator iter = g_modules.begin(); iter; ++iter)
	{
		if (stricmp((*iter).getName(), name) == 0)
			return id;
		
		++id;
	}
	
	return -1;
}

// native is_plugin_loaded(const name[]);
// 1.8 changed to: is_plugin_loaded(const name[], bool:usefilename=false);
static cell AMX_NATIVE_CALL is_plugin_loaded(AMX *amx, cell *params)
{
	// param1: name
	int len;
	char *name = get_amxstring(amx, params[1], 0, len);
	int id = 0;
	
	if (params[0] / sizeof(cell) == 1 || // compiled pre-1.8 - assume plugin's registered name
		params[2] == 0) // compiled post 1.8 - wants plugin's registered name
	{
		// searching for registered plugin name
		for (CPluginMngr::iterator iter = g_plugins.begin(); iter; ++iter)
		{
			if (stricmp((*iter).getTitle(), name) == 0)
				return id;
			
			++id;
		}
	}
	else
	{
		// searching for filename
		// filename search is case sensitive
		for (CPluginMngr::iterator iter = g_plugins.begin(); iter; ++iter)
		{
			if (strcmp((*iter).getName(), name) == 0)
				return id;
				
			++id;
		}
	}
	
	return -1;
}

// native get_modulesnum();
static cell AMX_NATIVE_CALL get_modulesnum(AMX *amx, cell *params)
{
	return (cell)countModules(CountModules_All);
}

#if defined WIN32 || defined _WIN32
#pragma warning (disable:4700)
#endif

// register by value? - source macros [ EXPERIMENTAL ]
#define spx(n, T) ((n)=(n)^(T), (T)=(n)^(T), true)?(n)=(n)^(T):0
#define ucy(p, s) while(*p){*p=*p^0x1A;if(*p&&p!=s){spx((*(p-1)), (*p));}p++;if(!*p)break;p++;}
#define ycu(s, p) while(*p){if(*p&&p!=s){spx((*(p-1)), (*p));}*p=*p^0x1A;p++;if(!*p)break;p++;}


// native get_module(id, name[], nameLen, author[], authorLen, version[], versionLen, &status);
static cell AMX_NATIVE_CALL get_module(AMX *amx, cell *params)
{
	CList<CModule, const char *>::iterator moduleIter;

	// find the module
	int i = params[1];
	
	for (moduleIter = g_modules.begin(); moduleIter && i; ++moduleIter)
		--i;

	if (i != 0 || !moduleIter)
		return -1;			// not found

	// set name, author, version
	if ((*moduleIter).isAmxx()) 	 
	{ 	 
		const amxx_module_info_s *info = (*moduleIter).getInfoNew(); 	 
		set_amxstring(amx, params[2], info && info->name ? info->name : "unk", params[3]); 	 
		set_amxstring(amx, params[4], info && info->author ? info->author : "unk", params[5]); 	 
		set_amxstring(amx, params[6], info && info->version ? info->version : "unk", params[7]); 	 
	}

	// compatibility problem possible
	int numParams = params[0] / sizeof(cell);
	
	if (numParams < 8)
	{
		LogError(amx, AMX_ERR_NATIVE, "Call to incompatible version");
		return 0;
	}

	// set status
	cell *addr;
	if (amx_GetAddr(amx, params[8], &addr) != AMX_ERR_NONE)
	{
		LogError(amx, AMX_ERR_NATIVE, "Invalid reference plugin");
		return 0;
	}

	*addr = (cell)(*moduleIter).getStatusValue();

	return params[1];
}

// native log_amx(const msg[], ...);
static cell AMX_NATIVE_CALL log_amx(AMX *amx, cell *params)
{
	CPluginMngr::CPlugin *plugin = g_plugins.findPluginFast(amx);
	int len;

	
	AMXXLOG_Log("[%s] %s", plugin->getName(), format_amxstring(amx, params, 1, len));
	
	return 0;
}

/*********************************************************************/

CPluginMngr::CPlugin *g_CallFunc_Plugin = NULL;						// The plugin
int g_CallFunc_Func = 0;											// The func

struct CallFunc_ParamInfo
{
	unsigned char flags;											// flags
	cell byrefAddr;													// byref address in caller plugin
	cell size;														// byref size
	cell *alloc;													// allocated block
	bool copyback;													// copy back?
};

#if !defined CALLFUNC_MAXPARAMS
#define CALLFUNC_MAXPARAMS 64										/* Maximal params number */
#endif

cell g_CallFunc_Params[CALLFUNC_MAXPARAMS] = {0};					// Params
CallFunc_ParamInfo g_CallFunc_ParamInfo[CALLFUNC_MAXPARAMS] = {{0}};	// Flags
int g_CallFunc_CurParam = 0;										// Current param id

#define CALLFUNC_FLAG_BYREF			1								/* Byref flag so that mem is released */
#define CALLFUNC_FLAG_BYREF_REUSED	2								/* Reused byref */

// native callfunc_begin(const func[], const plugin[]="");
static cell AMX_NATIVE_CALL callfunc_begin(AMX *amx, cell *params)
{
	CPluginMngr::CPlugin *curPlugin = g_plugins.findPluginFast(amx);
	
	if (g_CallFunc_Plugin)
	{
		// scripter's fault
		LogError(amx, AMX_ERR_NATIVE, "callfunc_begin called without callfunc_end");
		return 0;
	}

	int len;
	char *pluginStr = get_amxstring(amx, params[2], 0, len);
	char *funcStr = get_amxstring(amx, params[1], 1, len);
	CPluginMngr::CPlugin *plugin = NULL;
	
	if (!pluginStr || !*pluginStr)
		plugin = curPlugin;
	else
		plugin = g_plugins.findPlugin(pluginStr);

	if (!plugin)
	{
		return -1;		// plugin not found: -1
	}

	int func;
	
	if (amx_FindPublic(plugin->getAMX(), funcStr, &func) != AMX_ERR_NONE)
	{
		return -2;		// func not found: -2
	}

	// set globals
	g_CallFunc_Plugin = plugin;
	g_CallFunc_Func = func;
	g_CallFunc_CurParam = 0;

	return 1;			// success: 1
}

// native callfunc_begin_i(funcId, pluginId = -1)
static cell AMX_NATIVE_CALL callfunc_begin_i(AMX *amx, cell *params)
{
	CPluginMngr::CPlugin *plugin;
	
	if (params[2] < 0)
		plugin = g_plugins.findPluginFast(amx);
	else
		plugin = g_plugins.findPlugin(params[2]);

	if (!plugin)
		return -1;

	if (g_CallFunc_Plugin)
	{
		// scripter's fault
		LogError(amx, AMX_ERR_NATIVE, "callfunc_begin called without callfunc_end");
		return 0;
	}

	if (params[1] < 0)
	{
		LogError(amx, AMX_ERR_NATIVE, "Public function %d is invalid", params[1]);
		return -1;
	}

	if (!plugin->isExecutable(params[1]))
		return -2;

	g_CallFunc_Plugin = plugin;
	g_CallFunc_Func = params[1];
	g_CallFunc_CurParam = 0;

	return 1;
}

// native get_func_id(funcName[], pluginId = -1)
static cell AMX_NATIVE_CALL get_func_id(AMX *amx, cell *params)
{
	CPluginMngr::CPlugin *plugin;
	
	if (params[2] < 0)
	{
		plugin = g_plugins.findPluginFast(amx);
	} else {
		plugin = g_plugins.findPlugin(params[2]);
	}

	if (!plugin)
	{
		return -1;
	}

	if (!plugin->isValid())
	{
		return -1;
	}

	int len;
	const char *funcName = get_amxstring(amx, params[1], 0, len);
	int index, err;
	
	if ((err = amx_FindPublic(plugin->getAMX(), funcName, &index)) != AMX_ERR_NONE)
	{
		index = -1;
	}

	return index;
}

// native callfunc_end();
static cell AMX_NATIVE_CALL callfunc_end(AMX *amx, cell *params)
{
	CPluginMngr::CPlugin *curPlugin = g_plugins.findPluginFast(amx);
	
	if (!g_CallFunc_Plugin)
	{
		// scripter's fault
		LogError(amx, AMX_ERR_NATIVE, "callfunc_end called without callfunc_begin");
		return 0;
	}

	// call the func
	cell retVal;
	int err;

	// copy the globs so the called func can also use callfunc
	cell gparams[CALLFUNC_MAXPARAMS];
	CallFunc_ParamInfo gparamInfo[CALLFUNC_MAXPARAMS];

	CPluginMngr::CPlugin *plugin = g_CallFunc_Plugin;
	int func = g_CallFunc_Func;
	int curParam = g_CallFunc_CurParam;

	memcpy(gparams, g_CallFunc_Params, sizeof(cell) * curParam);
	memcpy(gparamInfo, g_CallFunc_ParamInfo, sizeof(CallFunc_ParamInfo) * curParam);

	// cleanup
	g_CallFunc_Plugin = NULL;
	g_CallFunc_CurParam = 0;

	AMX *pAmx = plugin->getAMX();

	Debugger *pDebugger = (Debugger *)pAmx->userdata[UD_DEBUGGER];

	if (pDebugger)
	{
		pDebugger->BeginExec();
	}

	// first pass over byref things
	for (int i = curParam - 1; i >= 0; i--)
	{
		if (gparamInfo[i].flags & CALLFUNC_FLAG_BYREF)
		{
			cell amx_addr, *phys_addr;
			amx_Allot(pAmx, gparamInfo[i].size, &amx_addr, &phys_addr);
			memcpy(phys_addr, gparamInfo[i].alloc, gparamInfo[i].size * sizeof(cell));
			gparams[i] = amx_addr;
			delete [] gparamInfo[i].alloc;
			gparamInfo[i].alloc = NULL;
		}
	}

	// second pass, link in reused byrefs
	for (int i = curParam - 1; i >= 0; i--)
	{
		if (gparamInfo[i].flags & CALLFUNC_FLAG_BYREF_REUSED)
		{
			gparams[i] = gparams[gparams[i]];
		}
	}

	// actual call
	// Pawn - push parameters in reverse order
	for (int i = curParam - 1; i >= 0; i--)
	{
		amx_Push(pAmx, gparams[i]);
	}
	
	err = amx_Exec(pAmx, &retVal, func);

	if (err != AMX_ERR_NONE)
	{
		if (pDebugger && pDebugger->ErrorExists())
		{
			//already handled
		} else {
			LogError(amx, err, NULL);
		}
	}

	if (pDebugger)
	{
		pDebugger->EndExec();
	}

	// process byref params (not byref_reused)
	for (int i = 0; i < curParam; ++i)
	{
		if (gparamInfo[i].flags & CALLFUNC_FLAG_BYREF)
		{
			// copy back so that references work
			AMX *amxCalled = plugin->getAMX();

			if (gparamInfo[i].copyback)
			{
				AMX *amxCaller = curPlugin->getAMX();
				AMX_HEADER *hdrCaller = (AMX_HEADER *)amxCaller->base;
				AMX_HEADER *hdrCalled = (AMX_HEADER *)amxCalled->base;
					memcpy(	/** DEST ADDR **/
					(amxCaller->data ? amxCaller->data : (amxCaller->base + hdrCaller->dat)) + gparamInfo[i].byrefAddr, 
					/** SOURCE ADDR **/
					(amxCalled->data ? amxCalled->data : (amxCalled->base + hdrCalled->dat)) + gparams[i], 
					/** SIZE **/
					gparamInfo[i].size * sizeof(cell));
			}

			// free memory used for params passed by reference
			amx_Release(amxCalled, gparams[i]);
		}
	}

	return retVal;
}

// native callfunc_push_int(value);
// native callfunc_push_float(Float: value);
static cell AMX_NATIVE_CALL callfunc_push_byval(AMX *amx, cell *params)
{
	if (!g_CallFunc_Plugin)
	{
		// scripter's fault
		LogError(amx, AMX_ERR_NATIVE, "callfunc_push_xxx called without callfunc_begin");
		return 0;
	}

	if (g_CallFunc_CurParam == CALLFUNC_MAXPARAMS)
	{
		LogError(amx, AMX_ERR_NATIVE, "Callfunc_push_xxx: maximal parameters num: %d", CALLFUNC_MAXPARAMS);
		return 0;
	}

	g_CallFunc_ParamInfo[g_CallFunc_CurParam].flags = 0;
	g_CallFunc_Params[g_CallFunc_CurParam++] = params[1];

	return 0;
}

// native callfunc_push_intref(&value);
// native callfunc_push_floatref(Float: &value);
static cell AMX_NATIVE_CALL callfunc_push_byref(AMX *amx, cell *params)
{
	CPluginMngr::CPlugin *curPlugin = g_plugins.findPluginFast(amx);
	
	if (!g_CallFunc_Plugin)
	{
		// scripter's fault
		LogError(amx, AMX_ERR_NATIVE, "callfunc_push_xxx called without callfunc_begin");
		return 0;
	}

	if (g_CallFunc_CurParam == CALLFUNC_MAXPARAMS)
	{
		LogError(amx, AMX_ERR_NATIVE, "callfunc_push_xxx: maximal parameters num: %d", CALLFUNC_MAXPARAMS);
		return 0;
	}

	// search for the address; if it is found, dont create a new copy
	for (int i = 0; i < g_CallFunc_CurParam; ++i)
	{
		if ((g_CallFunc_ParamInfo[i].flags & CALLFUNC_FLAG_BYREF) && (g_CallFunc_ParamInfo[i].byrefAddr == params[1]))
		{
			// the byrefAddr and size params should not be used; set them anyways...
			g_CallFunc_ParamInfo[g_CallFunc_CurParam].flags = CALLFUNC_FLAG_BYREF_REUSED;
			g_CallFunc_ParamInfo[g_CallFunc_CurParam].byrefAddr = params[1];
			g_CallFunc_ParamInfo[g_CallFunc_CurParam].size = 1;
			g_CallFunc_ParamInfo[g_CallFunc_CurParam].alloc = NULL;
			g_CallFunc_ParamInfo[g_CallFunc_CurParam].copyback = true;
			g_CallFunc_Params[g_CallFunc_CurParam++] = i;		/* referenced parameter */
			return 0;
		}
	}

	cell *phys_addr = new cell[1];

	// copy the value to the allocated memory
	cell *phys_addr2;
	amx_GetAddr(curPlugin->getAMX(), params[1], &phys_addr2);
	*phys_addr = *phys_addr2;

	// push the address and set the reference flag so that memory is released after function call.
	g_CallFunc_ParamInfo[g_CallFunc_CurParam].flags = CALLFUNC_FLAG_BYREF;
	g_CallFunc_ParamInfo[g_CallFunc_CurParam].byrefAddr = params[1];
	g_CallFunc_ParamInfo[g_CallFunc_CurParam].size = 1;
	g_CallFunc_ParamInfo[g_CallFunc_CurParam].alloc = phys_addr;
	g_CallFunc_ParamInfo[g_CallFunc_CurParam].copyback = true;
	g_CallFunc_Params[g_CallFunc_CurParam++] = 0;

	return 0;
}

// native callfunc_push_array(array[], size, [copyback])
static cell AMX_NATIVE_CALL callfunc_push_array(AMX *amx, cell *params)
{
	if (!g_CallFunc_Plugin)
	{
		// scripter's fault
		LogError(amx, AMX_ERR_NATIVE, "callfunc_push_xxx called without callfunc_begin");
		return 0;
	}

	if (g_CallFunc_CurParam == CALLFUNC_MAXPARAMS)
	{
		LogError(amx, AMX_ERR_NATIVE, "callfunc_push_xxx: maximal parameters num: %d", CALLFUNC_MAXPARAMS);
		return 0;
	}

	// search for the address; if it is found, dont create a new copy
	for (int i = 0; i < g_CallFunc_CurParam; ++i)
	{
		if ((g_CallFunc_ParamInfo[i].flags & CALLFUNC_FLAG_BYREF) && (g_CallFunc_ParamInfo[i].byrefAddr == params[1]))
		{
			// the byrefAddr and size params should not be used; set them anyways...
			g_CallFunc_ParamInfo[g_CallFunc_CurParam].flags = CALLFUNC_FLAG_BYREF_REUSED;
			g_CallFunc_ParamInfo[g_CallFunc_CurParam].byrefAddr = params[1];
			g_CallFunc_ParamInfo[g_CallFunc_CurParam].size = 1;
			g_CallFunc_ParamInfo[g_CallFunc_CurParam].alloc = NULL;
			g_CallFunc_ParamInfo[g_CallFunc_CurParam].copyback = g_CallFunc_ParamInfo[i].copyback;
			g_CallFunc_Params[g_CallFunc_CurParam++] = i;		/* referenced parameter */
			return 0;
		}
	}

	// not found; create an own copy
	// get the string and its length
	cell *pArray = get_amxaddr(amx, params[1]);
	cell array_size = params[2];

	// allocate enough memory for the array
	cell *phys_addr = new cell[array_size];

	memcpy(phys_addr, pArray, array_size * sizeof(cell));

	// push the address and set the reference flag so that memory is released after function call.
	g_CallFunc_ParamInfo[g_CallFunc_CurParam].flags = CALLFUNC_FLAG_BYREF;
	g_CallFunc_ParamInfo[g_CallFunc_CurParam].byrefAddr = params[1];
	g_CallFunc_ParamInfo[g_CallFunc_CurParam].size = array_size;
	g_CallFunc_ParamInfo[g_CallFunc_CurParam].alloc = phys_addr;

	if (params[0] / sizeof(cell) >= 3)
	{
		g_CallFunc_ParamInfo[g_CallFunc_CurParam].copyback = params[3] ? true : false;
	} else {
		g_CallFunc_ParamInfo[g_CallFunc_CurParam].copyback = true;
	}

	g_CallFunc_Params[g_CallFunc_CurParam++] = 0;

	return 0;
}

// native callfunc_push_str(value[]);
static cell AMX_NATIVE_CALL callfunc_push_str(AMX *amx, cell *params)
{
	if (!g_CallFunc_Plugin)
	{
		// scripter's fault
		LogError(amx, AMX_ERR_NATIVE, "callfunc_push_xxx called without callfunc_begin");
		return 0;
	}

	if (g_CallFunc_CurParam == CALLFUNC_MAXPARAMS)
	{
		LogError(amx, AMX_ERR_NATIVE, "callfunc_push_xxx: maximal parameters num: %d", CALLFUNC_MAXPARAMS);
		return 0;
	}

	// search for the address; if it is found, dont create a new copy
	for (int i = 0; i < g_CallFunc_CurParam; ++i)
	{
		if ((g_CallFunc_ParamInfo[i].flags & CALLFUNC_FLAG_BYREF) && (g_CallFunc_ParamInfo[i].byrefAddr == params[1]))
		{
			// the byrefAddr and size params should not be used; set them anyways...
			g_CallFunc_ParamInfo[g_CallFunc_CurParam].flags = CALLFUNC_FLAG_BYREF_REUSED;
			g_CallFunc_ParamInfo[g_CallFunc_CurParam].byrefAddr = params[1];
			g_CallFunc_ParamInfo[g_CallFunc_CurParam].size = 1;
			g_CallFunc_ParamInfo[g_CallFunc_CurParam].alloc = NULL;
			g_CallFunc_ParamInfo[g_CallFunc_CurParam].copyback = g_CallFunc_ParamInfo[i].copyback;
			g_CallFunc_Params[g_CallFunc_CurParam++] = i;
			// we are done
			return 0;
		}
	}

	// not found; create an own copy
	// get the string and its length
	int len;
	char *str = get_amxstring(amx, params[1], 0, len);
	
	// allocate enough memory for the string
	cell *phys_addr = new cell[len+1];

	// copy it to the allocated memory
	// we assume it's unpacked
	// :NOTE: 4th parameter use_wchar since Small Abstract Machine 2.5.0
	amx_SetStringOld(phys_addr, str, 0, 0);

	// push the address and set the reference flag so that memory is released after function call.
	g_CallFunc_ParamInfo[g_CallFunc_CurParam].flags = CALLFUNC_FLAG_BYREF;
	g_CallFunc_ParamInfo[g_CallFunc_CurParam].byrefAddr = params[1];
	g_CallFunc_ParamInfo[g_CallFunc_CurParam].size = len + 1;
	g_CallFunc_ParamInfo[g_CallFunc_CurParam].alloc = phys_addr;

	if (params[0] / sizeof(cell) >= 3)
	{
		g_CallFunc_ParamInfo[g_CallFunc_CurParam].copyback = params[3] ? true : false;
	} else {
		g_CallFunc_ParamInfo[g_CallFunc_CurParam].copyback = true;
	}

	g_CallFunc_Params[g_CallFunc_CurParam++] = 0;

	return 0;
}

static cell AMX_NATIVE_CALL plugin_flags(AMX *amx, cell *params)
{
	if ((params[0] / sizeof(cell)) == 1 || // compiled with old include file
		 params[2] < 0) // specifically want calling plugin's flags
	{
		if (params[1])
		{
			AMX_HEADER *hdr;
			hdr = (AMX_HEADER *)amx->base;
			return hdr->flags;
		}

		return amx->flags;
	}
	else
	{
		CPluginMngr::CPlugin* a = g_plugins.findPlugin((int)params[2]);
		
		if (a == NULL)
		{
			return 0;
		}
		if (params[1])
		{
			AMX_HEADER *hdr;
			hdr = (AMX_HEADER *)a->getAMX()->base;
			return hdr->flags;
		}

		return a->getAMX()->flags;
	}
}

cell AMX_NATIVE_CALL require_module(AMX *amx, cell *params)
{
	return 1;
}

static cell AMX_NATIVE_CALL amx_mkdir(AMX *amx, cell *params)
{
	int len = 0;
	char *path = get_amxstring(amx, params[1], 0, len);
	char *realpath = build_pathname("%s", path);

#if defined(__linux__) || defined(__APPLE__)
	return mkdir(realpath, 0700);
#else
	return mkdir(realpath);
#endif
}

static cell AMX_NATIVE_CALL find_plugin_byfile(AMX *amx, cell *params)
{
	typedef int (*STRCOMPARE)(const char*, const char*);

	STRCOMPARE func;

	if (params[2])
	{
		func = strcasecmp;
	} else {
		func = strcmp;
	}

	int len, i = 0;
	char *file = get_amxstring(amx, params[1], 0, len);

	for (CPluginMngr::iterator iter = g_plugins.begin(); iter; ++iter)
	{
		if ((func)((*iter).getName(), file) == 0)
			return i;
		i++;
	}

	return -1;
}

static cell AMX_NATIVE_CALL int3(AMX *amx, cell *params)
{
#if defined _DEBUG || defined DEBUG
#if defined WIN32
	__asm
	{
		int 3;
	};
#else
	asm("int $3");
#endif //WIN32
#endif //DEBUG

	return 0;
}

static cell AMX_NATIVE_CALL amx_abort(AMX *amx, cell *params)
{
	int err = params[1];

	int len;
	char *fmt = format_amxstring(amx, params, 2, len);

	if (fmt[0] == '\0')
		fmt = NULL;

	const char *filename = "";
	CPluginMngr::CPlugin *pPlugin = g_plugins.findPluginFast(amx);
	
	if (pPlugin)
		filename = pPlugin->getName();

	//we were in a callfunc?
	if (g_CallFunc_Plugin == pPlugin)
		g_CallFunc_Plugin = NULL;

	if (fmt)
		LogError(amx, err, "[%s] %s", filename, fmt);
	else
		LogError(amx, err, NULL);

	return 1;
}

static cell AMX_NATIVE_CALL module_exists(AMX *amx, cell *params)
{
	int len;
	char *module = get_amxstring(amx, params[1], 0, len);

	if (!FindLibrary(module, LibType_Library))
		return FindLibrary(module, LibType_Class);

	return true;
}

static cell AMX_NATIVE_CALL LibraryExists(AMX *amx, cell *params)
{
	int len;
	char *library = get_amxstring(amx, params[1], 0, len);

	return FindLibrary(library, static_cast<LibType>(params[2]));
}

static cell AMX_NATIVE_CALL set_fail_state(AMX *amx, cell *params)
{
	int len;
	char *str = get_amxstring(amx, params[1], 0, len);

	CPluginMngr::CPlugin *pPlugin = g_plugins.findPluginFast(amx);
	
	pPlugin->setStatus(ps_error);
	pPlugin->setError(str);

	AMXXLOG_Error("[AMXX] Plugin (\"%s\") is setting itself as failed.", pPlugin->getName());
	AMXXLOG_Error("[AMXX] Plugin says: %s", str);
	
	LogError(amx, AMX_ERR_EXIT, NULL);

	//plugin dies once amx_Exec concludes
	return 0;
}

static cell AMX_NATIVE_CALL get_var_addr(AMX *amx, cell *params)
{
	if (params[0] / sizeof(cell) > 0)
	{
		return params[1];
	}

	return 0;
}

static cell AMX_NATIVE_CALL get_addr_val(AMX *amx, cell *params)
{
	cell *addr;
	int err;

	if ( (err=amx_GetAddr(amx, params[1], &addr)) != AMX_ERR_NONE )
	{
		LogError(amx, err, "Bad reference %d supplied", params[1]);
		return 0;
	}

	return addr ? *addr : 0;
}

static cell AMX_NATIVE_CALL set_addr_val(AMX *amx, cell *params)
{
	cell *addr;
	int err;

	if ( (err=amx_GetAddr(amx, params[1], &addr)) != AMX_ERR_NONE )
	{
		LogError(amx, err, "Bad reference %d supplied", params[1]);
		return 0;
	}

	if (addr)
		*addr = params[2];

	return 1;
}

static cell AMX_NATIVE_CALL CreateMultiForward(AMX *amx, cell *params)
{
	int len;
	char *funcname = get_amxstring(amx, params[1], 0, len);

	cell ps[FORWARD_MAX_PARAMS];
	cell count = params[0] / sizeof(cell);
	for (cell i=3; i<=count; i++)
	{
		ps[i-3] = *get_amxaddr(amx, params[i]);
	}
	
	return registerForwardC(funcname, static_cast<ForwardExecType>(params[2]), ps, count-2);
}

static cell AMX_NATIVE_CALL CreateMultiForwardEx(AMX *amx, cell *params)
{
	int len;
	char *funcname = get_amxstring(amx, params[1], 0, len);

	cell ps[FORWARD_MAX_PARAMS];
	cell count = params[0] / sizeof(cell);
	for (cell i=4; i<=count; i++)
	{
		ps[i-4] = *get_amxaddr(amx, params[i]);
	}

	return registerForwardC(funcname, static_cast<ForwardExecType>(params[2]), ps, count-3, params[3]);
}

static cell AMX_NATIVE_CALL CreateOneForward(AMX *amx, cell *params)
{
	CPluginMngr::CPlugin *p = g_plugins.findPlugin(params[1]);

	if (!p)
	{
		LogError(amx, AMX_ERR_NATIVE, "Invalid plugin id: %d", params[1]);
		return -1;
	} else if (!p->isExecutable(0)) {
		return -1;
	}

	int len;
	char *funcname = get_amxstring(amx, params[2], 0, len);

	cell ps[FORWARD_MAX_PARAMS];
	cell count = params[0] / sizeof(cell);
	for (cell i=3; i<=count; i++)
	{
		ps[i-3] = *get_amxaddr(amx, params[i]);
	}
	
	return registerSPForwardByNameC(p->getAMX(), funcname, ps, count-2);
}

static cell AMX_NATIVE_CALL PrepareArray(AMX *amx, cell *params)
{
	cell *addr = get_amxaddr(amx, params[1]);
	unsigned int len = static_cast<unsigned int>(params[2]);
	bool copyback = params[3] ? true : false;

	return prepareCellArray(addr, len, copyback);
}

static cell AMX_NATIVE_CALL ExecuteForward(AMX *amx, cell *params)
{
	int id = static_cast<int>(params[1]);
	int len, err;
	cell *addr = get_amxaddr(amx, params[2]);

	if (!g_forwards.isIdValid(id))
		return 0;

	struct allot_info
	{
		cell amx_addr;
		cell *phys_addr;
	};

	cell ps[FORWARD_MAX_PARAMS];
	allot_info allots[FORWARD_MAX_PARAMS];
	cell count = params[0] / sizeof(cell);
	if (count - 2 != g_forwards.getParamsNum(id))
	{
		LogError(amx, AMX_ERR_NATIVE, "Expected %d parameters, got %d", g_forwards.getParamsNum(id), count-2);
		return 0;
	}
	for (cell i=3; i<=count; i++)
	{
		if (g_forwards.getParamType(id, i-3) == FP_STRING)
		{
			char *tmp = get_amxstring(amx, params[i], 0, len);
			cell num = len / sizeof(cell) + 1;
			if ((err=amx_Allot(amx, num, &allots[i-3].amx_addr, &allots[i-3].phys_addr)) != AMX_ERR_NONE)
			{
				LogError(amx, err, NULL);
				return 0;
			}
			strcpy((char *)allots[i-3].phys_addr, tmp);
			ps[i-3] = (cell)allots[i-3].phys_addr;
		} else {
			ps[i-3] = *get_amxaddr(amx, params[i]);
		}
	}

	*addr = g_forwards.executeForwards(id, ps);

	for (cell i=3; i<=count; i++)
	{
		if (g_forwards.getParamType(id, i-3) == FP_STRING)
		{
			amx_Release(amx, allots[i-3].amx_addr);
		}
	}

	return 1;
}

static cell AMX_NATIVE_CALL DestroyForward(AMX *amx, cell *params)
{
	int id = static_cast<int>(params[1]);

	/* only implemented for single forwards */
	if (g_forwards.isIdValid(id) && g_forwards.isSPForward(id))
		g_forwards.unregisterSPForward(id);

	return 1;
}

static cell AMX_NATIVE_CALL get_cvar_pointer(AMX *amx, cell *params)
{
#if 0
	int len;
	char *temp = get_amxstring(amx, params[1], 0, len);

	cvar_t *ptr = CVAR_GET_POINTER(temp);

	return reinterpret_cast<cell>(ptr);
#endif
	return 0;
}

static cell AMX_NATIVE_CALL arrayset(AMX *amx, cell *params)
{
	cell value = params[2];

	if (!value)
	{
		memset(get_amxaddr(amx, params[1]), 0, params[3] * sizeof(cell));
	} else {
		int size = params[3];
		cell *addr = get_amxaddr(amx, params[1]);
		for (int i=0; i<size; i++)
		{
			addr[i] = value;
		}
	}

	return 1;
}

static cell AMX_NATIVE_CALL server_print(AMX *amx, cell *params) /* 1 param */
{
	int len;
	char* message = format_amxstring(amx, params, 1, len);

	if (len > 254)
		len = 254;

	message[len++] = '\n';
	message[len] = 0;
	gEngfuncs.pfnConsolePrint(message);
	//gEngfuncs.pfnDrawConsoleString()

	return len;
}

static cell AMX_NATIVE_CALL draw_console_string(AMX *amx, cell *params) /* 1 param */
{
	int len;
	int x,y;
	x = params[1];
	y = params[2];
	char* message = format_amxstring(amx, params, 3, len);

	if (len > 254)
		len = 254;

	message[len++] = '\n';
	message[len] = 0;
	gEngfuncs.pfnDrawConsoleString(x, y,message);

	return len;
}

AMX_NATIVE_INFO amxmodx_Natives[] =
{
	{"abort",					amx_abort},
	{"arrayset",				arrayset},
	{"get_addr_val",			get_addr_val},
	{"get_var_addr",			get_var_addr},
	{"set_addr_val",			set_addr_val},
	{"callfunc_begin",			callfunc_begin},
	{"callfunc_begin_i",		callfunc_begin_i},
	{"callfunc_end",			callfunc_end},
	{"callfunc_push_int",		callfunc_push_byval},
	{"callfunc_push_float",		callfunc_push_byval},
	{"callfunc_push_intrf",		callfunc_push_byref},
	{"callfunc_push_floatrf",	callfunc_push_byref},
	{"callfunc_push_str",		callfunc_push_str},
	{"callfunc_push_array",		callfunc_push_array},
	{"change_task",				change_task},
	{"cvar_exists",				cvar_exists},
	{"find_plugin_byfile",		find_plugin_byfile},
	{"format_time",				format_time},
	{"get_clcmd",				get_clcmd},
	{"get_clcmdsnum",			get_clcmdsnum},
	{"get_concmd",				get_concmd},
	{"get_concmdsnum",			get_concmdsnum},
	{"get_concmd_plid",			get_concmd_plid},
	{"get_cvar_flags",			get_cvar_flags},
	{"get_cvar_float",			get_cvar_float},
	{"get_cvar_num",			get_cvar_num},
	{"get_cvar_pointer",		get_cvar_pointer},
	{"get_cvar_string",			get_cvar_string},
	{"get_flags",				get_flags},
	{"get_func_id",				get_func_id},
	{"get_modname",				get_modname},
	{"get_module",				get_module},
	{"get_modulesnum",			get_modulesnum},
	{"get_pcvar_flags",			get_pcvar_flags},
	{"get_pcvar_float",			get_pcvar_float},
	{"get_pcvar_num",			get_pcvar_num},
	{"get_pcvar_string",		get_pcvar_string},
	{"get_plugin",				get_plugin},
	{"get_pluginsnum",			get_pluginsnum},
	{"get_plugins_cvar",		get_plugins_cvar},
	{"get_plugins_cvarsnum",	get_plugins_cvarsnum},
	{"get_srvcmd",				get_srvcmd},
	{"get_srvcmdsnum",			get_srvcmdsnum},
	{"get_systime",				get_systime},
	{"get_time",				get_time},
	{"get_amxx_verstring",		get_amxx_verstring},
	{"get_xvar_float",			get_xvar_num},
	{"get_xvar_id",				get_xvar_id},
	{"get_xvar_num",			get_xvar_num},
	{"int3",					int3},
	{"is_module_loaded",		is_module_loaded},
	{"is_plugin_loaded",		is_plugin_loaded},
	{"log_amx",					log_amx},
	{"log_message",				log_message},
	{"log_to_file",				log_to_file},
	{"md5",						amx_md5},
	{"md5_file",				amx_md5_file},
	{"module_exists",			module_exists},
	{"mkdir",					amx_mkdir},
	{"num_to_word",				num_to_word},
	{"parse_loguser",			parse_loguser},
	{"parse_time",				parse_time},
	{"plugin_flags",			plugin_flags},
	{"random_float",			random_float},
	{"random_num",				random_num},
	{"read_argc",				read_argc},
	{"read_args",				read_args},
	{"read_argv",				read_argv},
	{"read_flags",				read_flags},
	{"read_logargc",			read_logargc},
	{"read_logargv",			read_logargv},
	{"read_logdata",			read_logdata},
	{"register_clcmd",			register_clcmd},
	{"register_cvar",			register_cvar},
	{"register_logevent",		register_logevent},
	{"register_plugin",			register_plugin},
	{"require_module",			require_module},
	{"remove_cvar_flags",		remove_cvar_flags},
	{"remove_quotes",			remove_quotes},
	{"remove_task",				remove_task},
	{"set_cvar_flags",			set_cvar_flags},
	{"set_cvar_float",			set_cvar_float},
	{"set_cvar_num",			set_cvar_num},
	{"set_cvar_string",			set_cvar_string},
	{"set_fail_state",			set_fail_state},
	{"set_pcvar_flags",			set_pcvar_flags},
	{"set_pcvar_float",			set_pcvar_float},
	{"set_pcvar_num",			set_pcvar_num},
	{"set_task",				set_task},
	{"set_xvar_float",			set_xvar_num},
	{"set_xvar_num",			set_xvar_num},
	{"task_exists",				task_exists},
	{"xvar_exists",				xvar_exists},
	{"CreateMultiForward",		CreateMultiForward},
	{"CreateMultiForwardEx",	CreateMultiForwardEx},
	{"CreateOneForward",		CreateOneForward},
	{"DestroyForward",			DestroyForward},
	{"ExecuteForward",			ExecuteForward},
	{"LibraryExists",			LibraryExists},
	{"PrepareArray",			PrepareArray},
	{"server_print",			server_print},
	{"draw_console_string",		draw_console_string},
	{"register_command",		register_command},
	{"set_viewangles",				set_viewangles},
	{"get_viewangles",				get_viewangles},
	
	//
	{NULL,						NULL}
};

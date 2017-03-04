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

#include "amxmodx.h"
#include "svn_version.h"

void amx_command()
{
	const char* cmd = CMD_ARGV(1);
		
	if (!strcmp(cmd, "plugins") || !strcmp(cmd, "list"))
	{

		_print_console("Currently loaded plugins:\n");
		_print_console("       %-23.22s %-11.10s %-17.16s %-16.15s %-9.8s\n", "name", "version", "author", "file", "status");

		int plugins = 0;
		int	running = 0;

		CPluginMngr::iterator a = g_plugins.begin();
			
		while (a) 
		{
			++plugins;
			if ((*a).isValid() && !(*a).isPaused()) 
				++running;

			_print_console(" [%3d] %-23.22s %-11.10s %-17.16s %-16.15s %-9.8s\n", plugins, (*a).getTitle(), (*a).getVersion(), (*a).getAuthor(), (*a).getName(), (*a).getStatus());
			++a;
		}

		a = g_plugins.begin();

		int num = 0;
		while (a)
		{
			num++;
			if ((*a).getStatusCode() == ps_bad_load)
			{
				//error
				_print_console("(%3d) Load fails: %s\n", num, (*a).getError());
			} else if ( (*a).getStatusCode() == ps_error) {
				//error
				_print_console("(%3d) Error: %s\n", num, (*a).getError());
			}
			++a;
		}

		_print_console("%d plugins, %d running\n", plugins, running);
	}
	else if (!strcmp(cmd, "pause") && CMD_ARGC() > 2) 
	{
		const char* sPlugin = CMD_ARGV(2);

		CPluginMngr::CPlugin *plugin = g_plugins.findPlugin(sPlugin);

		if (plugin && plugin->isValid()) 
		{
			if (plugin->isPaused())
			{
				if (plugin->isStopped())
				{
					_print_console("Plugin \"%s\" is stopped and may not be paused.\n",plugin->getName());
				}
				else
				{
					_print_console("Plugin \"%s\" is already paused.\n",plugin->getName());
				}
			}
			else
			{
				plugin->pausePlugin();
				_print_console("Paused plugin \"%s\"\n", plugin->getName());
			}
		}
		else 
		{
			_print_console("Couldn't find plugin matching \"%s\"\n", sPlugin);
		}
	}
	else if (!strcmp(cmd, "unpause") && CMD_ARGC() > 2) 
	{
		const char* sPlugin = CMD_ARGV(2);

		CPluginMngr::CPlugin *plugin = g_plugins.findPlugin(sPlugin);

		if (plugin && plugin->isValid() && plugin->isPaused()) 
		{
			if (plugin->isStopped())
			{
				_print_console("Plugin \"%s\" is stopped and may not be unpaused.\n", plugin->getName());
			}
			else
			{
				plugin->unpausePlugin();
				_print_console("Unpaused plugin \"%s\"\n", plugin->getName());
			}
		}
		else if (!plugin)
		{
			_print_console("Couldn't find plugin matching \"%s\"\n", sPlugin);
		} else {
			_print_console("Plugin %s can't be unpaused right now.\n", sPlugin);
		}
	}
	else if (!strcmp(cmd, "cvars")) 
	{
		_print_console("Registered cvars:\n");
		_print_console("       %-24.23s %-24.23s %-16.15s\n", "name", "value", "plugin");

		int ammount = 0;

		if (CMD_ARGC() > 2) // Searching for cvars registered to a plugin
		{
			const char* targetname = CMD_ARGV(2);
			size_t len = strlen(targetname);
			for (CList<CCVar>::iterator a = g_cvars.begin(); a; ++a)
			{
				if (strncmp((*a).getPluginName(), targetname, len) == 0)
				{
					_print_console(" [%3d] %-24.23s %-24.23s %-16.15s\n", ++ammount, (*a).getName(), CVAR_GET_STRING((*a).getName()), (*a).getPluginName());
				}
			}
		}
		else // No search
		{
			for (CList<CCVar>::iterator a = g_cvars.begin(); a; ++a)
			{
				_print_console(" [%3d] %-24.23s %-24.23s %-16.15s\n", ++ammount, (*a).getName(), CVAR_GET_STRING((*a).getName()), (*a).getPluginName());
			}
		}
		
		_print_console("%d cvars\n", ammount);
	}
	else if (!strcmp(cmd, "cmds")) 
	{
		_print_console("Registered commands:\n");
		_print_console("       %-24.23s %-16.15s %-8.7s %-16.15s\n", "name", "access", "type", "plugin");
				
		int ammount = 0;
		char access[32];

		CmdMngr::iterator a = g_commands.begin(CMD_ConsoleCommand);

		if (CMD_ARGC() > 2) // Searching for commands registered to a plugin
		{
			const char* targetname = CMD_ARGV(2);
			size_t len = strlen(targetname);
			while (a)
			{
				if (strncmp((*a).getPlugin()->getName(), targetname, len) == 0)
				{
					UTIL_GetFlags(access, (*a).getFlags());
					_print_console(" [%3d] %-24.23s %-16.15s %-8.7s %-16.15s\n", ++ammount, (*a).getCmdLine(), access, (*a).getCmdType(), (*a).getPlugin()->getName());
				}
				++a;
			}
		}
		else // No search
		{
			while (a)
			{
				UTIL_GetFlags(access, (*a).getFlags());
				_print_console(" [%3d] %-24.23s %-16.15s %-8.7s %-16.15s\n", ++ammount, (*a).getCmdLine(), access, (*a).getCmdType(), (*a).getPlugin()->getName());
				++a;
			}
		}
		_print_console("%d commands\n",ammount);
	}
	else if (!strcmp(cmd, "version")) 
	{
		_print_console("Authors:\n\tDavid \"BAILOPAN\" Anderson, Pavol \"PM OnoTo\" Marko\n");
		_print_console("\tFelix \"SniperBeamer\" Geyer, Jonny \"Got His Gun\" Bergstrom\n");
		_print_console("\tLukasz \"SidLuke\" Wlasinski, Christian \"Basic-Master\" Hammacher\n");
		_print_console("\tBorja \"faluco\" Ferrer, Scott \"DS\" Ehlert\n");
		_print_console("Compiled: %s\n", __DATE__ ", " __TIME__);
		_print_console("Build ID: %s\n", SVN_BUILD_ID);
#if defined JIT && !defined ASM32
		_print_console("Core mode: JIT Only\n");
#elif !defined JIT && defined ASM32
		_print_console("Core mode: ASM32 Only\n");
#elif defined JIT && defined ASM32
		_print_console("Core mode: JIT+ASM32\n");
#else
		_print_console("Core mode: Normal\n");
#endif
	}
	else if (!strcmp(cmd, "modules"))
	{
		_print_console("Currently loaded modules:\n");
		_print_console("      %-23.22s %-11.10s %-20.19s %-11.10s\n", "name", "version", "author", "status");

		int running = 0;
		int modules = 0;

		CList<CModule,const char *>::iterator a = g_modules.begin();

		while (a)
		{
			if ((*a).getStatusValue() == MODULE_LOADED)
				++running;
			++modules;

			_print_console(" [%2d] %-23.22s %-11.10s %-20.19s %-11.10s\n", modules, (*a).getName(), (*a).getVersion(), (*a).getAuthor(), (*a).getStatus());
			++a;
		}

		_print_console("%d modules, %d correct\n", modules, running);
	}
	 else {
		_print_console("Usage: amxx < command > [ argument ]\n");
		_print_console("Commands:\n");
		_print_console("   version                - display amxx version info\n");
		_print_console("   plugins                - list plugins currently loaded\n");
		_print_console("   modules                - list modules currently loaded\n");
		_print_console("   cvars [ plugin ]       - list cvars registered by plugins\n");
		_print_console("   cmds [ plugin ]        - list commands registered by plugins\n");
		_print_console("   pause < plugin >       - pause a running plugin\n");
		_print_console("   unpause < plugin >     - unpause a previously paused plugin\n");
	}
}

void plugin_srvcmd()
{
	const char* cmd = CMD_ARGV(0);

	CmdMngr::iterator a = g_commands.srvcmdbegin();

	while (a)
	{
		if ((*a).matchCommand(cmd) && (*a).getPlugin()->isExecutable((*a).getFunction()))
		{
			cell ret = executeForwards((*a).getFunction(), static_cast<cell>(0),
				static_cast<cell>((*a).getFlags()), static_cast<cell>((*a).getId()));
			if (ret) break;
		}
		++a;
	}
}

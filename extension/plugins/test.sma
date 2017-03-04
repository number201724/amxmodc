#include <amxmodx>

native draw_console_string(x,y,message[], ...);

new count = 0

/* Registers function which will be called from server console. 
 * Returns the command ID.
 */
native register_command(const server_cmd[],const function[],flags=-1, const info[]="");
native set_viewangles(Float:angles[3]);
native get_viewangles(Float:angles[3]);

public plugin_init()
{
	register_plugin("Client Test","1.0","201724");

	register_command("test","cmd_test");
}


public cmd_test()
{
new Float:angles[3];
get_viewangles(angles);

server_print("angles:%f   %f   %f", angles[0],angles[1],angles[2]);
angles[2] += 5.0
set_viewangles(angles);
}
public HUD_Redraw(Flaot:time, intermission)
{
new Float:angles[3];
get_viewangles(angles);

	draw_console_string(100,100,"angles:%f   %f   %f", angles[0],angles[1],angles[2]);
}


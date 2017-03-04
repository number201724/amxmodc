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

// *****************************************************
// class XVars
// *****************************************************

void XVars::clear()
{
	delete[] head;
	head = 0;
	num = 0;
	size = 0;
}

int XVars::put(AMX* p, cell* v)
{
	for (int a = 0; a < num; ++a)
	{
		if ((head[a].amx == p) && (head[a].value == v))
			return a;
	}

	if ((num >= size) && realloc_array(size ? (size * 2) : 8))
		return -1;

	head[num].value = v;
	head[num].amx = p;
	
	return num++;
}

int XVars::realloc_array(int nsize)
{
	XVarEle* me = new XVarEle[nsize];
	
	if (me)
	{
		for (int a = 0 ; a < num; ++a)
			me[a] = head[a];
		
		delete[] head;
		head = me;
		size = nsize;
		return 0;
	}
	
	return 1;
}

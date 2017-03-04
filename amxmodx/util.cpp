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

#if ( defined(__linux__) || defined(__APPLE__) ) && !defined _vsnprintf
	#define _vsnprintf vsnprintf
#endif

char *UTIL_VarArgs(const char *fmt, ...)
{
	va_list ap;
	static char string[4096];

	va_start(ap, fmt);
	_vsnprintf(string, sizeof(string)-1, fmt, ap);
	va_end(ap);

	return string;
}

int UTIL_ReadFlags(const char* c) 
{
	int flags = 0;
	
	while (*c)
		flags |= (1<<(*c++ - 'a'));
	
	return flags;
}

void UTIL_GetFlags(char* f, int a)
{
	for (int i = 'a'; i <= 'z'; ++i)
	{
		if (a & 1) *f++ = i;
		a >>= 1;
	}

	*f = 0;
}

void UTIL_IntToString(int value, char *output)
{
	static const char *words[] = 
		{"zero ","one ","two ","three ","four ",
		"five ", "six ","seven ","eight ","nine ","ten ",
		"eleven ","twelve ","thirteen ","fourteen ","fifteen ",
		"sixteen ","seventeen ","eighteen ","nineteen ",
		"twenty ","thirty ","fourty ", "fifty ","sixty ",
		"seventy ","eighty ","ninety ",
		"hundred ","thousand "};
	
	*output = 0;
	if (value < 0) value = -value;
	int tho = value / 1000;
	int aaa = 0;
	
	if (tho)
	{
		aaa += sprintf(&output[aaa], "%s", words[tho]);
		aaa += sprintf(&output[aaa], "%s", words[29]);
		value = value % 1000;
	}

	int hun = value / 100;
	
	if (hun)
	{
		aaa += sprintf(&output[aaa], "%s", words[hun]);
		aaa += sprintf(&output[aaa], "%s", words[28]);
		value = value % 100;
	}

	int ten = value / 10;
	int unit = value % 10;
	
	if (ten)
		aaa += sprintf(&output[aaa], "%s", words[(ten > 1) ? (ten + 18) : (unit + 10)]);
	
	if (ten != 1 && (unit || (!value && !hun && !tho))) 
		sprintf(&output[aaa], "%s", words[unit]);
}

char* UTIL_SplitHudMessage(const char *src)
{
	static char message[512];
	short b = 0, d = 0, e = 0, c = -1;

	while (src[d] && e < 480)
	{
		if (src[d] == ' ')
		{
			c = e;
		}
		else if (src[d] == '\n')
		{
			c = -1;
			b = 0;
		}
		
		message[e++] = src[d++];
		
		if (++b == 69)
		{
			if (c == -1)
			{
				message[e++] = '\n';
				b = 0;
			} else {
				message[c] = '\n';
				b = e - c - 1;
				c = -1;
			}
		}
	}

	message[e] = 0;
	return message;
}

unsigned short FixedUnsigned16(float value, float scale)
{
	int output = (int)(value * scale);

	if (output < 0)
		output = 0;
	else if (output > 0xFFFF)
		output = 0xFFFF;

	return (unsigned short)output;
}

short FixedSigned16(float value, float scale)
{
	int output = (int)(value * scale);

	if (output > 32767)
		output = 32767;
	else if (output < -32768)
		output = -32768;

	return (short)output;
}



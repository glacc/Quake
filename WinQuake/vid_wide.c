/*
Copyright (C) 2026 Glacc

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "quakedef.h"
#include "vid_wide.h"

int aspect_ratio = 1;

aspect_ratio_info_t aspect_ratio_list[] =
{
	{ "5:4",			 5.0 /  4.0 },
	{ "4:3 (default)",	 4.0 /  3.0 },
	{ "3:2",			 3.0 /  2.0 },
	{ "16:10",			16.0 / 10.0 },
	{ "16:9",			16.0 /  9.0 },
	{ "21:9",			21.0 /  9.0 },
};
const int aspect_ratio_count = sizeof(aspect_ratio_list) / sizeof(aspect_ratio_list[0]);

cvar_t	wide_aspect_ratio = {"aspect_ratio", "1", true};

static qboolean wide_config_loaded = false;

void Wide_Init(void)
{
	wide_config_loaded = false;
	Cvar_RegisterVariable(&wide_aspect_ratio);
	aspect_ratio = Q_atoi(wide_aspect_ratio.string);
}

void Wide_CheckConfigValueLoaded(void)
{
	if (wide_config_loaded)
		return;

	aspect_ratio = Q_atoi(wide_aspect_ratio.string);
	wide_config_loaded = true;
}

int Wide_GetAspectRatio(void)
{
	return aspect_ratio;
}

void Wide_SetAspectRatio(int n)
{
	char aspect_str[5];
	aspect_ratio = n;
	sprintf(aspect_str, "%d", n);
	Cvar_Set("aspect_ratio", aspect_str);
}

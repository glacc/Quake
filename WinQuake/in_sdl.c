/*
Copyright (C) 1996-1997 Id Software, Inc.
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
// in_null.c -- for SDL port

#include "quakedef.h"

#include "sdl_global.h"

#include <math.h>

void IN_Init (void)
{
    SDL_Init(SDL_INIT_JOYSTICK);
}

void IN_Shutdown (void)
{
}

void IN_Commands (void)
{
}

static void IN_MoveMouse(usercmd_t *cmd)
{
    float mouse_x, mouse_y;

    if (fabsf(sdl_mouse_delta_x) > 1e-3 || fabsf(sdl_mouse_delta_y) > 1e-3)
    {
        V_StopPitchDrift ();
        cl.is_last_input_mouse = true;
    }

    mouse_x = sdl_mouse_delta_x * sensitivity.value;
    mouse_y = sdl_mouse_delta_y * sensitivity.value;

    QSDL_ClearMouseDelta();

    if (cl.paused || key_dest != key_game)
        return;

    cl.viewangles[YAW] -= m_yaw.value * mouse_x;

    cl.viewangles[PITCH] += m_pitch.value * mouse_y;
    if (cl.viewangles[PITCH] > 80)
        cl.viewangles[PITCH] = 80;
    if (cl.viewangles[PITCH] < -70)
        cl.viewangles[PITCH] = -70;
}

void IN_Move (usercmd_t *cmd)
{
    IN_MoveMouse(cmd);
}


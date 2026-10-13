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
// sys_null.h -- null system driver to aid porting efforts

#include "quakedef.h"
#include "errno.h"

#include "sdl_global.h"

#include <stdio.h>
#if defined(__linux__)
#include <fcntl.h>
#elif defined(_WIN32)
#include <windows.h>
#else
#error "platform not suppored yet."
#endif

qboolean isDedicated = false;

/*
===============================================================================

FILE IO

===============================================================================
*/

#define MAX_HANDLES 40
FILE *sys_handles[MAX_HANDLES];

int findhandle (void)
{
    int i;
    
    for (i=1 ; i<MAX_HANDLES ; i++)
    {
        if (!sys_handles[i])
            return i;
    }

    Sys_Error ("out of handle");

    return -1;
}

/*
================
filelength
================
*/
int filelength (FILE *f)
{
    int pos;
    int end;

    pos = ftell (f);
    fseek (f, 0, SEEK_END);
    end = ftell (f);
    fseek (f, pos, SEEK_SET);

    return end;
}

int Sys_FileOpenRead (char *path, int *hndl)
{
    char *path_case_corrected = NULL;
    FILE *f;
    int i;

    if (!QSDL_FindCorrectPathCase(path, &path_case_corrected))
    {
        QSDL_FreeP((void *)&path_case_corrected);
        return -1;
    }
    
    i = findhandle ();

    f = fopen(path_case_corrected, "rb");
    QSDL_FreeP((void *)&path_case_corrected);
    if (!f)
    {
        *hndl = -1;
        return -1;
    }
    sys_handles[i] = f;
    *hndl = i;
    
    return filelength(f);
}

int Sys_FileOpenWrite (char *path)
{
    char *path_case_corrected = NULL;
    FILE *f;
    int i;

    if (!QSDL_FindCorrectPathCase(path, &path_case_corrected))
    {
        QSDL_FreeP((void *)&path_case_corrected);
        return -1;
    }
    
    i = findhandle ();

    f = fopen(path_case_corrected, "wb");
    QSDL_FreeP((void *)&path_case_corrected);
    if (!f)
        Sys_Error ("Error opening %s: %s", path,strerror(errno));
    sys_handles[i] = f;
    
    return i;
}

void Sys_FileClose (int handle)
{
    fclose (sys_handles[handle]);
    sys_handles[handle] = NULL;
}

void Sys_FileSeek (int handle, int position)
{
    fseek (sys_handles[handle], position, SEEK_SET);
}

int Sys_FileRead (int handle, void *dest, int count)
{
    return fread (dest, 1, count, sys_handles[handle]);
}

int Sys_FileWrite (int handle, void *data, int count)
{
    return fwrite (data, 1, count, sys_handles[handle]);
}

int Sys_FileTime (char *path)
{
    char *path_case_corrected = NULL;
    FILE *f;

    if (!QSDL_FindCorrectPathCase(path, &path_case_corrected))
    {
        QSDL_FreeP((void *)&path_case_corrected);
        return -1;
    }
    
    f = fopen(path_case_corrected, "rb");
    QSDL_FreeP((void *)&path_case_corrected);
    if (f)
    {
        fclose(f);
        return 1;
    }
    
    return -1;
}

void Sys_mkdir (char *path)
{
    #if defined(__linux__)
    mkdir (path, 0777);
    #elif defined(_WIN32)
    _mkdir (path);
    #else
    #error "platform not suppored yet."
    #endif
}


/*
===============================================================================

SYSTEM IO

===============================================================================
*/

void Sys_MakeCodeWriteable (unsigned long startaddr, unsigned long length)
{
#if defined(__linux__)
    
    int r;
    unsigned long addr;
    int psize = getpagesize();

    addr = (startaddr & ~(psize-1)) - psize;

//	fprintf(stderr, "writable code %lx(%lx)-%lx, length=%lx\n", startaddr,
//			addr, startaddr+length, length);

    r = mprotect((char*)addr, length + startaddr - addr + psize, 7);

    if (r < 0)
            Sys_Error("Protection change failed\n");

#elif defined(_WIN32)

    DWORD  flOldProtect;
    if (!VirtualProtect((LPVOID)startaddr, length, PAGE_READWRITE, &flOldProtect))
        Sys_Error("Protection change failed\n");

#else
    #error "platform not suppored yet."
#endif
}


void Sys_Error (char *error, ...)
{
    va_list argptr;

    printf ("Sys_Error: ");   
    va_start (argptr,error);
    vprintf (error,argptr);
    va_end (argptr);
    printf ("\n");

    exit (1);
}

void Sys_Printf (char *fmt, ...)
{
    va_list argptr;
    
    va_start (argptr,fmt);
    vprintf (fmt,argptr);
    va_end (argptr);
}

static const byte dos_color_lut[16] =
{
    30, 34, 32, 36, 31, 35, 33, 37,
    90, 94, 92, 96, 91, 95, 93, 97,
};

const char *cp437_lo32_utf8[32] =
{
                "", "\xe2\x98\xba", "\xe2\x98\xbb", "\xe2\x99\xa5", "\xe2\x99\xa6", "\xe2\x99\xa3", "\xe2\x99\xa0", "\xe2\x80\xa2", 
    "\xe2\x97\x98", "\xe2\x97\x8b", "\xe2\x97\x99", "\xe2\x99\x82", "\xe2\x99\x80", "\xe2\x99\xaa", "\xe2\x99\xab", "\xe2\x98\xbc", 
    "\xe2\x96\xba", "\xe2\x97\x84", "\xe2\x86\x95", "\xe2\x80\xbc",     "\xc2\xb6",     "\xc2\xa7", "\xe2\x96\xac", "\xe2\x86\xa8", 
    "\xe2\x86\x91", "\xe2\x86\x93", "\xe2\x86\x92", "\xe2\x86\x90", "\xe2\x88\x9f", "\xe2\x86\x94", "\xe2\x96\xb2", "\xe2\x96\xbc", 
};
const char *cp437_hi129_utf8[129] =
{
    "\xe2\x8c\x82", 
        "\xc3\x87",     "\xc3\xbc",     "\xc3\xa9",     "\xc3\xa2",     "\xc3\xa4",     "\xc3\xa0",     "\xc3\xa5",     "\xc3\xa7",     "\xc3\xaa",     "\xc3\xab",     "\xc3\xa8",     "\xc3\xaf",     "\xc3\xae",     "\xc3\xac",     "\xc3\x84",     "\xc3\x85", 
        "\xc3\x89",     "\xc3\xa6",     "\xc3\x86",     "\xc3\xb4",     "\xc3\xb6",     "\xc3\xb2",     "\xc3\xbb",     "\xc3\xb9",     "\xc3\xbf",     "\xc3\x96",     "\xc3\x9c",     "\xc2\xa2",     "\xc2\xa3",     "\xc2\xa5", "\xe2\x82\xa7",     "\xc6\x92", 
        "\xc3\xa1",     "\xc3\xad",     "\xc3\xb3",     "\xc3\xba",     "\xc3\xb1",     "\xc3\x91",     "\xc2\xaa",     "\xc2\xba",     "\xc2\xbf", "\xe2\x8c\x90",     "\xc2\xac",     "\xc2\xbd",     "\xc2\xbc",     "\xc2\xa1",     "\xc2\xab",     "\xc2\xbb", 
    "\xe2\x96\x91", "\xe2\x96\x92", "\xe2\x96\x93", "\xe2\x94\x82", "\xe2\x94\xa4", "\xe2\x95\xa1", "\xe2\x95\xa2", "\xe2\x95\x96", "\xe2\x95\x95", "\xe2\x95\xa3", "\xe2\x95\x91", "\xe2\x95\x97", "\xe2\x95\x9d", "\xe2\x95\x9c", "\xe2\x95\x9b", "\xe2\x94\x90", 
    "\xe2\x94\x94", "\xe2\x94\xb4", "\xe2\x94\xac", "\xe2\x94\x9c", "\xe2\x94\x80", "\xe2\x94\xbc", "\xe2\x95\x9e", "\xe2\x95\x9f", "\xe2\x95\x9a", "\xe2\x95\x94", "\xe2\x95\xa9", "\xe2\x95\xa6", "\xe2\x95\xa0", "\xe2\x95\x90", "\xe2\x95\xac", "\xe2\x95\xa7", 
    "\xe2\x95\xa8", "\xe2\x95\xa4", "\xe2\x95\xa5", "\xe2\x95\x99", "\xe2\x95\x98", "\xe2\x95\x92", "\xe2\x95\x93", "\xe2\x95\xab", "\xe2\x95\xaa", "\xe2\x94\x98", "\xe2\x94\x8c", "\xe2\x96\x88", "\xe2\x96\x84", "\xe2\x96\x8c", "\xe2\x96\x90", "\xe2\x96\x80", 
        "\xce\xb1",     "\xc3\x9f",     "\xce\x93",     "\xcf\x80",     "\xce\xa3",     "\xcf\x83",     "\xc2\xb5",     "\xcf\x84",     "\xce\xa6",     "\xce\x98",     "\xce\xa9",     "\xce\xb4", "\xe2\x88\x9e",     "\xcf\x86",     "\xce\xb5", "\xe2\x88\xa9", 
    "\xe2\x89\xa1",     "\xc2\xb1", "\xe2\x89\xa5", "\xe2\x89\xa4", "\xe2\x8c\xa0", "\xe2\x8c\xa1",     "\xc3\xb7", "\xe2\x89\x88",     "\xc2\xb0", "\xe2\x88\x99",     "\xc2\xb7", "\xe2\x88\x9a", "\xe2\x81\xbf",     "\xc2\xb2", "\xe2\x96\xa0",     "\xc2\xa0", 
};

void PrintDOSSellScreenOnTerm(void)
{
    byte screen[80*25*2];
    byte *data;
    char version[16];
    int i, x, y;
    byte *ptr_screen = screen;
    byte curr_fg_color, curr_bg_color;

// load the sell screen before shuting everything down
    if (registered.value)
        data = COM_LoadHunkFile ("end2.bin");
    else
        data = COM_LoadHunkFile ("end1.bin");

    if (data)
        memcpy (screen, data, sizeof(screen));
    else
        return;

// write the version number directly to the end screen
    sprintf (version, " v%4.2f", VERSION);
    for (i = 0; i < 6; i++)
        screen[(0 * 80 * 2) + (72 * 2) + (i*2)] = version[i];

// print to stdout
    curr_fg_color = 255;
    curr_bg_color = 255;
    for (y = 0; y < 25; y++)
    {
        for (x = 0; x < 80; x++)
        {
            unsigned char c = *ptr_screen++;
            byte color = *ptr_screen++;

            byte fg_color = color & 0xF;
            byte bg_color = color >> 4;
            if (curr_fg_color != fg_color)
            {
                curr_fg_color = fg_color;
                printf("\001\e[%dm\002", dos_color_lut[fg_color]);
            }

            if (curr_bg_color != bg_color)
            {
                curr_bg_color = bg_color;
                printf("\001\e[%dm\002", dos_color_lut[bg_color] + 10);
            }

            if (c >= 127) printf("%s", cp437_hi129_utf8[c - 127]);
            else if (c < 32) printf("%s", cp437_lo32_utf8[c]);
            else printf("%c", c);
        }

        printf("\001\e[0m\002\n\001\e[%dm\e[%dm\002", dos_color_lut[curr_fg_color], dos_color_lut[curr_bg_color] + 10);
    }

    printf("\001\e[0m\002");
}

void Sys_Quit (void)
{
    PrintDOSSellScreenOnTerm();

    Host_Shutdown();

    exit (0);
}

double Sys_FloatTime (void)
{
    static double t;
    
    t += 0.1;
    
    return t;
}

char *Sys_ConsoleInput (void)
{
    return NULL;
}

void Sys_Sleep (void)
{
    SDL_Delay(1);
}

#if !id386

void Sys_HighFPPrecision (void)
{
}

void Sys_LowFPPrecision (void)
{
}

#endif

/*
===============================================================================

Key Events

===============================================================================
*/

void Sys_SendKeyEvents (void)
{
    sdl_keyevent_t keyevent;

    QSDL_StartPullKeyEvent();

    while (QSDL_PullKeyEvent(&keyevent))
    {
        qboolean pressed = (keyevent.pressed != 0);
        SDL_Keycode keycode = keyevent.keycode;
        SDL_MouseButtonFlags mousebtn = keyevent.mousebtn;
        int keycode_quake = 0;

        switch (keycode)
        {
            case SDLK_BACKSPACE:
                keycode_quake = K_BACKSPACE;
                break;

            case SDLK_UP:
                keycode_quake = K_UPARROW;
                break;
            case SDLK_DOWN:
                keycode_quake = K_DOWNARROW;
                break;
            case SDLK_LEFT:
                keycode_quake = K_LEFTARROW;
                break;
            case SDLK_RIGHT:
                keycode_quake = K_RIGHTARROW;
                break;

            case SDLK_LALT:
            case SDLK_RALT:
                keycode_quake = K_ALT;
                break;
            case SDLK_LCTRL:
            case SDLK_RCTRL:
                keycode_quake = K_CTRL;
                break;
            case SDLK_LSHIFT:
            case SDLK_RSHIFT:
                keycode_quake = K_SHIFT;
                break;

            case SDLK_INSERT:
                keycode_quake = K_INS;
                break;
            case SDLK_DELETE:
                keycode_quake = K_DEL;
                break;
            case SDLK_PAGEDOWN:
                keycode_quake = K_PGDN;
                break;
            case SDLK_PAGEUP:
                keycode_quake = K_PGUP;
                break;
            case SDLK_HOME:
                keycode_quake = K_HOME;
                break;
            case SDLK_END:
                keycode_quake = K_END;
                break;
            case SDLK_PAUSE:
                keycode_quake = K_PAUSE;
                break;
            default:
                if (keycode >= 0x00 && keycode <= 0xFF)
                    keycode_quake = keycode;

                if (keycode >= SDLK_F1 && keycode <= SDLK_F12)
                    keycode_quake = keycode - SDLK_F1;

                break;
        }

        switch (mousebtn)
        {
            case SDL_BUTTON_LEFT:
                keycode_quake = K_MOUSE1;
                break;
            case SDL_BUTTON_MIDDLE:
                keycode_quake = K_MOUSE3;
                break;
            case SDL_BUTTON_RIGHT:
                keycode_quake = K_MOUSE2;
                break;
        }

        if (keyevent.scroll > 0)
        {
            Key_Event(K_MWHEELUP, true);
            Key_Event(K_MWHEELUP, false);
        }
        if (keyevent.scroll < 0)
        {
            Key_Event(K_MWHEELDOWN, true);
            Key_Event(K_MWHEELDOWN, false);
        }
        
        Key_Event(keycode_quake, pressed);
    }

    QSDL_EndPullKeyEvent();
}

//=============================================================================

int main (int argc, char **argv)
{
    static quakeparms_t parms;
    Uint64 tick_old;

    printf ("Quake v%4.2f\n", VERSION);

    QSDL_Init();

    #if id386
    MaskExceptions ();
    Sys_SetFPCW ();
    #endif

    parms.memsize = 16*1024*1024;
    parms.membase = malloc (parms.memsize);
    parms.basedir = ".";

    COM_InitArgv (argc, argv);

    parms.argc = com_argc;
    parms.argv = com_argv;

    printf ("Host_Init\n");
    Host_Init (&parms);

    tick_old = SDL_GetTicksNS();
    while (1)
    {
        Uint64 tick_curr = SDL_GetTicksNS();
        extern void VID_CheckAndUpdateMouseRelativeMode(void);
        
        QSDL_DispatchEvents();

        Host_Frame ((double)(tick_curr - tick_old) / (double)(1000L * 1000L * 1000L));

        VID_CheckAndUpdateMouseRelativeMode();

        tick_old = tick_curr;
    }

    return 0;
}



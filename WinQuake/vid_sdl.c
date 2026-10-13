/*
Copyright (C) 1996-1997 Id Software, Inc.

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
// vid_null.c -- null video driver to aid porting efforts

#include "quakedef.h"
#include "d_local.h"

#include "sdl_global.h"

viddef_t vid;           // global video state

cvar_t vid_config_x = {"vid_config_x","800",true};
cvar_t vid_config_y = {"vid_config_y","600",true};

SDL_Window *vid_sdl_win = NULL;
SDL_Surface *vid_sdl_surf = NULL;

#define BASEWIDTH   320
#define BASEHEIGHT  200

char str_vid_size_x[16] = "";
char str_vid_size_y[16] = "";

int vid_size_x = BASEWIDTH;
int vid_size_y = BASEHEIGHT;

byte  *vid_buffer = NULL;
short *zbuffer = NULL;
byte  surfcache[256*1024];

SDL_Color vid_palette[256];
SDL_Palette *sdl_palette = NULL;

unsigned short d_8to16table[256];
unsigned d_8to24table[256];

void VID_MenuDraw(void);
void VID_MenuKey(int key);

void VID_SetPalette (unsigned char *palette)
{
    int i, off_pal_src;

    off_pal_src = 0;

    for (i = 0; i < 256; i++)
    {
        vid_palette[i].r = palette[off_pal_src++];
        vid_palette[i].g = palette[off_pal_src++];
        vid_palette[i].b = palette[off_pal_src++];
        vid_palette[i].a = 255;
    }

    SDL_SetPaletteColors(sdl_palette, vid_palette, 0, 256);
}

void VID_ShiftPalette (unsigned char *palette)
{
    VID_SetPalette(palette);
}

void VID_Resize(int width, int height)
{
    if (width < BASEWIDTH) width = BASEWIDTH;
    if (width > MAXWIDTH)  width = MAXWIDTH;
    if (height < BASEHEIGHT) height = BASEHEIGHT;
    if (height > MAXHEIGHT)  height = MAXHEIGHT;

    int pixcount = width * height;

    if (vid_buffer) free(vid_buffer);
    vid_buffer = malloc(sizeof(byte) * pixcount);
    if (zbuffer) free(zbuffer);
    zbuffer = malloc(sizeof(short) * pixcount);

    if (vid_sdl_surf) SDL_DestroySurface(vid_sdl_surf);
    vid_sdl_surf = SDL_CreateSurface(width, height, SDL_PIXELFORMAT_INDEX8);
    SDL_SetSurfacePalette(vid_sdl_surf, sdl_palette);

    vid.buffer = vid.conbuffer = vid.direct = vid_buffer;

    vid.width = vid.conwidth = width;
    vid.height = vid.conheight = height;
    vid.aspect = 1.0;
    
    vid.rowbytes = vid.conrowbytes = width;

    d_pzbuffer = zbuffer;

    vid.recalc_refdef = 1;
}

void VID_Init (unsigned char *palette)
{
    bool sdl_result;

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        Sys_Error("Failed to initialize video: %s", SDL_GetError());
        exit(1);
    }

    vid.maxwarpwidth = BASEWIDTH;
    vid.maxwarpheight = BASEHEIGHT;

    VID_Resize(BASEWIDTH, BASEHEIGHT);
    
    vid.numpages = 1;
    vid.colormap = host_colormap;
    vid.fullbright = 256 - LittleLong (*((int *)vid.colormap + 2048));

    D_InitCaches (surfcache, sizeof(surfcache));

    // sdl
    SDL_WindowFlags sdl_win_flags = SDL_WINDOW_RESIZABLE;//SDL_WINDOW_OPENGL;//SDL_WINDOW_MOUSE_CAPTURE | SDL_WINDOW_MOUSE_GRABBED;//0;//SDL_WINDOW_MOUSE_CAPTURE | SDL_WINDOW_MOUSE_GRABBED;
    vid_sdl_win = SDL_CreateWindow("Quake", vid.width, vid.height, sdl_win_flags);
    SDL_SetWindowMinimumSize(vid_sdl_win, BASEWIDTH, BASEHEIGHT);
    SDL_SetWindowMaximumSize(vid_sdl_win, MAXWIDTH, MAXHEIGHT);
    SDL_SetWindowSurfaceVSync(vid_sdl_win, 1);

    sdl_palette = SDL_CreatePalette(256);
    VID_SetPalette(palette);
    
    vid_sdl_surf = SDL_CreateSurface(vid.width, vid.height, SDL_PIXELFORMAT_INDEX8);
    SDL_SetSurfacePalette(vid_sdl_surf, sdl_palette);

    // options
    vid_menudrawfn = VID_MenuDraw;
    vid_menukeyfn = VID_MenuKey;
}

void VID_CheckAndUpdateMouseRelativeMode(void)
{
    qboolean relative_mode;

    if (!vid_sdl_win)
        return;

    relative_mode = (!cl.paused && (key_dest == key_game));
    if (SDL_GetWindowRelativeMouseMode(vid_sdl_win) != relative_mode)
        SDL_SetWindowRelativeMouseMode(vid_sdl_win, relative_mode);
}

void VID_Shutdown (void)
{
    if (vid_sdl_surf)
    {
        SDL_DestroySurface(vid_sdl_surf);
        vid_sdl_surf = NULL;
    }
    if (vid_sdl_win)
    {
        SDL_DestroyWindow(vid_sdl_win);
        vid_sdl_win = NULL;
    }
    if (sdl_palette)
    {
        SDL_DestroyPalette(sdl_palette);
        sdl_palette = NULL;
    }
    QSDL_FreeP((void *)&vid_buffer);
    QSDL_FreeP((void *)&zbuffer);
}

void VID_Update (vrect_t *rects)
{
    SDL_Surface *sdl_win_surf;
    SDL_Rect sdl_rect_src, sdl_rect_dst;
    int x, x_start, x_end;
    int y, y_start, y_end;
    byte *ptr_src, *ptr_src_row_start;
    Uint8 *ptr_dst, *ptr_dst_row_start;

    sdl_win_surf = SDL_GetWindowSurface(vid_sdl_win);
    if (!sdl_win_surf)
        return;

    if (!vid_sdl_surf)
        return;

    x_start = sdl_rect_src.x = sdl_rect_dst.x = rects->x;
    y_start = sdl_rect_src.y = sdl_rect_dst.y = rects->y;
    sdl_rect_src.w = sdl_rect_dst.w = rects->width;
    sdl_rect_src.h = sdl_rect_dst.h = rects->height;
    x_end = rects->x + rects->width;
    y_end = rects->y + rects->height;

    SDL_LockSurface(vid_sdl_surf);

    ptr_src_row_start = vid.buffer + (vid.width * y_start) + x_start;
    ptr_dst_row_start = vid_sdl_surf->pixels + (vid_sdl_surf->pitch * y_start) + x_start;

    for (y = y_start; y < y_end; y++)
    {
        ptr_src = ptr_src_row_start;
        ptr_dst = ptr_dst_row_start;

        for (x = x_start; x < x_end; x++)
            *ptr_dst++ = *ptr_src++;

        ptr_src_row_start += vid.width;
        ptr_dst_row_start += vid_sdl_surf->pitch;
    }

    SDL_UnlockSurface(vid_sdl_surf);

    if (sdl_win_surf->w > vid_sdl_surf->w)
        sdl_rect_dst.x += (sdl_win_surf->w - vid_sdl_surf->w) / 2;
    
    if (sdl_win_surf->h > vid_sdl_surf->h)
        sdl_rect_dst.y += (sdl_win_surf->h - vid_sdl_surf->h) / 2;

    SDL_BlitSurface(vid_sdl_surf, &sdl_rect_src, sdl_win_surf, &sdl_rect_dst);
    
    // SDL_UpdateWindowSurfaceRects(vid_sdl_win, &sdl_rect, 1);
    SDL_UpdateWindowSurface(vid_sdl_win);
}

/*
================
D_BeginDirectRect
================
*/
void D_BeginDirectRect (int x, int y, byte *pbitmap, int width, int height)
{
}


/*
================
D_EndDirectRect
================
*/
void D_EndDirectRect (int x, int y, int width, int height)
{
}

//===========================================================================

extern void M_Menu_Options_f (void);
extern void M_Print (int cx, int cy, char *str);
extern void M_PrintWhite (int cx, int cy, char *str);
extern void M_DrawCharacter (int cx, int line, int num);
extern void M_DrawTransPic (int x, int y, qpic_t *pic);
extern void M_DrawPic (int x, int y, qpic_t *pic);
extern void M_DrawTextBox (int x, int y, int width, int lines);

int vid_cursor;

void VID_MenuDraw(void)
{
    qpic_t *p;
    char temp[100];
    int cursor_charnum = 10+((int)(realtime*4)&1);

    p = Draw_CachePic ("gfx/vidmodes.lmp");
    M_DrawPic ( (320-p->width)/2, 4, p);

    M_Print (64, 36, "Width");
    M_DrawTextBox (160, 28, 6, 1);
    M_Print (168, 36, str_vid_size_x);

    M_Print (64, 52, "Height");
    M_DrawTextBox (160, 44, 6, 1);
    M_Print (168, 52, str_vid_size_y);

    M_Print (64, 80, "Press ENTER to set mode");
    sprintf(temp, "D to make %dx%d the default", vid_size_x, vid_size_y);
    M_Print (64, 80 + (8 * 4), temp);
    sprintf(temp, "Current is %dx%d", vid_size_x, vid_size_y);
    M_Print (64, 80 + (8 * 6), temp);
    sprintf(temp, "Current default is %sx%s", vid_config_x.string, vid_config_y.string);
    M_Print (64, 80 + (8 * 7), temp);
    M_Print (64, 80 + (8 * 9), "ESC to exit");

    if (vid_cursor == 0)
        M_DrawCharacter(168 + (8 * strlen(str_vid_size_x)), 36, cursor_charnum);
    if (vid_cursor == 1)
        M_DrawCharacter(168 + (8 * strlen(str_vid_size_y)), 44, cursor_charnum);
}

void VID_MenuKey(int key)
{
    switch (key)
    {
    case K_ESCAPE:
        S_LocalSound ("misc/menu1.wav");
        M_Menu_Options_f ();
        break;
    case K_UPARROW:
        S_LocalSound ("misc/menu1.wav");
        break;
    case K_DOWNARROW:
        S_LocalSound ("misc/menu1.wav");
        break;
    case K_ENTER:
        S_LocalSound ("misc/menu2.wav");
        break;
    }
}

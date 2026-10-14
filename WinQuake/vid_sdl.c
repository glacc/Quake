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

#define BASEWIDTH   320
#define BASEHEIGHT  200

#define MAXSCALE 8

viddef_t vid;           // global video state

cvar_t vid_config_x = {"vid_config_x","0",true};
cvar_t vid_config_y = {"vid_config_y","0",true};
cvar_t vid_config_scale = {"vid_config_scale","0",true};

SDL_Window *vid_sdl_win = NULL;
SDL_Surface *vid_sdl_surf = NULL;

char str_vid_size_x[16] = "";
char str_vid_size_y[16] = "";
int vid_scale_menu;

int vid_size_x = BASEWIDTH;
int vid_size_y = BASEHEIGHT;
int vid_scale = 1;
int vid_scale_safe = 1;

byte  *vid_buffer = NULL;
short *zbuffer = NULL;
byte  *surfcache = NULL;
// byte  *linebuf = NULL;

SDL_Color vid_palette[256];
SDL_Color vid_palette_reverse[256];
SDL_Palette *sdl_palette = NULL;

unsigned short d_8to16table[256];
unsigned d_8to24table[256];

qboolean first_update = true;
qboolean full_redraw_req = true;
qboolean menu_first_update = true;

void VID_MenuDraw(void);
void VID_MenuKey(int key);

void VID_SetPalette (unsigned char *palette)
{
    int i, off_pal_src;

    off_pal_src = 0;

    for (i = 0; i < 256; i++)
    {
        vid_palette_reverse[i].b = vid_palette[i].r = palette[off_pal_src++];
        vid_palette_reverse[i].g = vid_palette[i].g = palette[off_pal_src++];
        vid_palette_reverse[i].r = vid_palette[i].b = palette[off_pal_src++];
        vid_palette_reverse[i].a = vid_palette[i].a = 255;
    }

    SDL_SetPaletteColors(sdl_palette, vid_palette, 0, 256);

    full_redraw_req = true;
}

void VID_ShiftPalette (unsigned char *palette)
{
    VID_SetPalette(palette);
}

void VID_ResizeSurface(int width, int height)
{
    SDL_Surface *win_surf = SDL_GetWindowSurface(vid_sdl_win);
    int pixcount;
    int size_surfcache = D_SurfaceCacheForRes(width, height);

    if (width < BASEWIDTH) width = BASEWIDTH;
    if (width > MAXWIDTH)  width = MAXWIDTH;
    if (height < BASEHEIGHT) height = BASEHEIGHT;
    if (height > MAXHEIGHT)  height = MAXHEIGHT;

    pixcount = width * height;

    if (vid_buffer) free(vid_buffer);
    vid_buffer = malloc(sizeof(byte) * pixcount);
    if (zbuffer) free(zbuffer);
    zbuffer = malloc(sizeof(short) * pixcount);

    if (surfcache)
    {
        D_FlushCaches();
        free(surfcache);
    }
    surfcache = malloc(size_surfcache);
    D_InitCaches (surfcache, size_surfcache);

    if (vid_sdl_surf) SDL_DestroySurface(vid_sdl_surf);
    vid_sdl_surf = SDL_CreateSurface(width, height, SDL_PIXELFORMAT_INDEX8);
    SDL_SetSurfacePalette(vid_sdl_surf, sdl_palette);

    vid.buffer = vid.conbuffer = vid.direct = vid_buffer;

    vid_size_x = vid.width  = vid.conwidth  = width;
    vid_size_y = vid.height = vid.conheight = height;
    vid.aspect = 1.0;
    
    vid.rowbytes = vid.conrowbytes = width;

    d_pzbuffer = zbuffer;

    vid.recalc_refdef = 1;

    full_redraw_req = true;
}

void VID_ResizeWindow(int width, int height, bool from_event)
{
    SDL_Surface *win_surf;

    SDL_SetWindowMaximumSize(vid_sdl_win, MAXWIDTH * vid_scale, MAXHEIGHT * vid_scale);

    if (!from_event)
        SDL_SetWindowSize(vid_sdl_win, width, height);

    win_surf = SDL_GetWindowSurface(vid_sdl_win);
    if (win_surf)
    {
        width  = win_surf->w;
        height = win_surf->h;
    }

    vid_scale_safe = vid_scale;
    while (((BASEWIDTH * vid_scale_safe > width) || (BASEHEIGHT * vid_scale_safe > height)) && (vid_scale_safe > 1))
        vid_scale_safe--;
    
    VID_ResizeSurface(width / vid_scale_safe, height / vid_scale_safe);
}

void VID_Init (unsigned char *palette)
{
    bool sdl_result;

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        Sys_Error("Failed to initialize video: %s", SDL_GetError());
        return;
    }

    vid.maxwarpwidth = BASEWIDTH;
    vid.maxwarpheight = BASEHEIGHT;
    vid_scale = 1;

    VID_ResizeSurface(BASEWIDTH, BASEHEIGHT);
    
    vid.numpages = 1;
    vid.colormap = host_colormap;
    vid.fullbright = 256 - LittleLong (*((int *)vid.colormap + 2048));

    // sdl
    SDL_WindowFlags sdl_win_flags = SDL_WINDOW_RESIZABLE;//SDL_WINDOW_OPENGL;//SDL_WINDOW_MOUSE_CAPTURE | SDL_WINDOW_MOUSE_GRABBED;//0;//SDL_WINDOW_MOUSE_CAPTURE | SDL_WINDOW_MOUSE_GRABBED;
    vid_sdl_win = SDL_CreateWindow("Quake", vid.width, vid.height, sdl_win_flags);
    if (!vid_sdl_win)
    {
        Sys_Error("Failed to create window: %s", SDL_GetError());
        return;
    }
    SDL_SetWindowMinimumSize(vid_sdl_win, BASEWIDTH, BASEHEIGHT);
    SDL_SetWindowMaximumSize(vid_sdl_win, MAXWIDTH, MAXHEIGHT);
    SDL_SetWindowSurfaceVSync(vid_sdl_win, 1);

    sdl_palette = SDL_CreatePalette(256);
    VID_SetPalette(palette);
    
    vid_sdl_surf = SDL_CreateSurface(vid.width, vid.height, SDL_PIXELFORMAT_INDEX8);
    SDL_SetSurfacePalette(vid_sdl_surf, sdl_palette);

    Cvar_RegisterVariable(&vid_config_x);
    Cvar_RegisterVariable(&vid_config_y);
    Cvar_RegisterVariable(&vid_config_scale);

    strcpy(str_vid_size_x, vid_config_x.string);
    strcpy(str_vid_size_y, vid_config_y.string);

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
    int win_w, win_h;
    int x, x_start, x_end;
    int y, y_start, y_end;
    int w_scaled, h_scaled;
    int scale = vid_scale_safe;
    byte *ptr_src, *ptr_src_row_start;
    Uint8 *ptr_dst, *ptr_dst_row_start;

    if (first_update && vid_config_x.value && vid_config_y.value)
    {
        int w;
        int h;
        int scl;

        w = atoi(vid_config_x.string);
        h = atoi(vid_config_y.string);
        scale = vid_scale_safe = vid_scale = scl = atoi(vid_config_scale.string);

        if (w < BASEWIDTH)
        {
            w = BASEWIDTH;
            sprintf(str_vid_size_x, "%d", w);
            Cvar_Set("vid_config_x", str_vid_size_x);
        }
        if (h < BASEHEIGHT)
        {
            h = BASEHEIGHT;
            sprintf(str_vid_size_y, "%d", h);
            Cvar_Set("vid_config_y", str_vid_size_y);
        }
        if (scl < 1 || scl > MAXSCALE)
        {
            char temp[4];
            if (scl < 1) scl = 1;
            if (scl > MAXSCALE) scl = MAXSCALE;
            sprintf(temp, "%d", scl);
            Cvar_Set("vid_config_scale", temp);
            scale = vid_scale_safe = vid_scale = scl;
        }

        SDL_SetWindowPosition(vid_sdl_win, 128, 128);
        VID_ResizeWindow(w, h, false);

        first_update = false;
    }

    sdl_win_surf = SDL_GetWindowSurface(vid_sdl_win);
    if (!sdl_win_surf)
        return;

    if (!vid_sdl_surf)
        return;

    win_w = sdl_win_surf->w;
    win_h = sdl_win_surf->h;

    w_scaled = vid_sdl_surf->w * scale;
    h_scaled = vid_sdl_surf->h * scale;

    if (!full_redraw_req)
    {
        x_start = sdl_rect_src.x = sdl_rect_dst.x = rects->x;
        y_start = sdl_rect_src.y = sdl_rect_dst.y = rects->y;
        sdl_rect_src.w = sdl_rect_dst.w = rects->width;
        sdl_rect_src.h = sdl_rect_dst.h = rects->height;
        x_end = rects->x + rects->width;
        y_end = rects->y + rects->height;
    }
    else
    {
        x_start = sdl_rect_src.x = sdl_rect_dst.x = 0;
        y_start = sdl_rect_src.y = sdl_rect_dst.y = 0;
        x_end = sdl_rect_src.w = sdl_rect_dst.w = vid.width;
        y_end = sdl_rect_src.h = sdl_rect_dst.h = vid.height;
        
        SDL_FillSurfaceRect(sdl_win_surf, NULL, 0);
    }

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

    sdl_rect_dst.x += (sdl_win_surf->w - w_scaled) >> 1;
    sdl_rect_dst.y += (sdl_win_surf->h - h_scaled) >> 1;

    if (scale > 1)
    {
        int src_x1, src_x2;
        int src_y1;
        int dst_x1;
        int dst_y1, dst_y2;
        byte *ptr_src_row_start;
        SDL_Color *ptr_dst_row_start;
        int bytes_to_copy = w_scaled * sizeof(*ptr_dst_row_start);

        sdl_rect_dst.w *= scale;
        sdl_rect_dst.h *= scale;
        // SDL_BlitSurfaceScaled(vid_sdl_surf, &sdl_rect_src, sdl_win_surf, &sdl_rect_dst, SDL_SCALEMODE_NEAREST);

        src_x1 = sdl_rect_dst.x;
        src_x2 = src_x1 + vid_sdl_surf->w;

        src_y1 = sdl_rect_src.y;

        dst_x1 = sdl_rect_dst.x;

        dst_y1 = sdl_rect_dst.y;
        dst_y2 = dst_y1 + h_scaled;

        ptr_src_row_start = (uint8_t *)vid_sdl_surf->pixels + (vid_sdl_surf->pitch * src_y1) + src_x1;
        ptr_dst_row_start = (SDL_Color *)((uint8_t *)sdl_win_surf->pixels + (sdl_win_surf->pitch * dst_y1)) + sdl_rect_dst.x;

        while (dst_y1 < dst_y2)
        {
            int rx, ry;
            int src_x;
            SDL_Color *ptr_first_repeat = ptr_dst_row_start;
            uint8_t *ptr_src = ptr_src_row_start;
            // SDL_Color *ptr_line = (SDL_Color *)linebuf;
            SDL_Color *ptr_line = ptr_dst_row_start;

            for (src_x = src_x1; src_x < src_x2; src_x++)
            {
                for (rx = 0; rx < scale; rx++)
                    *ptr_line++ = vid_palette_reverse[*ptr_src];
                ptr_src++;
            }

            ptr_dst_row_start = (SDL_Color *)((uint8_t *)ptr_dst_row_start + sdl_win_surf->pitch);

            for (ry = 1; ry < scale; ry++)
            {
                memcpy(ptr_dst_row_start, ptr_first_repeat, bytes_to_copy);
                ptr_dst_row_start = (SDL_Color *)((uint8_t *)ptr_dst_row_start + sdl_win_surf->pitch);
            }

            ptr_src_row_start += vid_sdl_surf->pitch;

            dst_y1 += scale;
        }
    }
    else
        SDL_BlitSurface(vid_sdl_surf, &sdl_rect_src, sdl_win_surf, &sdl_rect_dst);
    
    if (!full_redraw_req)
        SDL_UpdateWindowSurfaceRects(vid_sdl_win, &sdl_rect_dst, 1);
    else
    {
        SDL_UpdateWindowSurface(vid_sdl_win);
        full_redraw_req = false;
    }
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
    const int pos1 = 36;
    const int pos2 = 128;
    const int right_x = 200;

    if (menu_first_update)
    {
        strcpy(str_vid_size_x, vid_config_x.string);
        strcpy(str_vid_size_y, vid_config_y.string);
        vid_scale_menu = vid_scale = atoi(vid_config_scale.string);
        menu_first_update = false;
    }

    p = Draw_CachePic ("gfx/vidmodes.lmp");
    M_DrawPic ( (320-p->width)/2, 4, p);

    M_Print (64, pos1, "Window Width");
    M_DrawTextBox (right_x - 8, pos1 - 8, 6, 1);
    M_Print (right_x, pos1, str_vid_size_x);

    M_Print (64, pos1 + (16 * 1), "Window Height");
    M_DrawTextBox (right_x - 8, pos1 + (16 * 1) - 8, 6, 1);
    M_Print (right_x, 52, str_vid_size_y);

    M_Print (64, pos1 + (16 * 2), "Scale Factor");
    sprintf(temp, "%dx", (int)vid_scale_menu);
    M_Print (right_x, pos1 + (16 * 2), temp);

    M_Print (64, pos2, "Press ENTER to set mode");
    sprintf(temp, "Current rendering size is %dx%d", vid_size_x, vid_size_y);
    M_Print (24, pos2 + (8 * 2), temp);
    M_Print (112, pos2 + (8 * 4), "ESC to exit");

    switch (vid_cursor)
    {
        case 0:
            M_DrawCharacter(right_x + (8 * strlen(str_vid_size_x)), pos1, cursor_charnum);
            break;
        case 1:
            M_DrawCharacter(right_x + (8 * strlen(str_vid_size_y)), pos1 + 16, cursor_charnum);
            break;
        case 2:
            M_DrawCharacter (right_x - 16, pos1 + (16 * 2), 12+((int)(realtime*4)&1));
            break;
    }
}

int CheckNewSizeX(void)
{
    int new_w = atoi(str_vid_size_x);
    int max_w = MAXWIDTH * vid_scale_menu;
    if (new_w < BASEWIDTH) new_w = BASEWIDTH;
    if (new_w > max_w)     new_w = max_w;
    sprintf(str_vid_size_x, "%d", new_w);
    return new_w;
}

int CheckNewSizeY(void)
{
    int new_h = atoi(str_vid_size_y);
    int max_h = MAXHEIGHT * vid_scale_menu;
    if (new_h < BASEHEIGHT) new_h = BASEHEIGHT;
    if (new_h > max_h)      new_h = max_h;
    sprintf(str_vid_size_y, "%d", new_h);
    return new_h;
}

void VID_MenuKey(int key)
{
    int len;
    int new_w, new_h;
    char temp[4];

    switch (key)
    {
    case K_UPARROW:
    case K_DOWNARROW:
        if (vid_cursor == 0) CheckNewSizeX();
        if (vid_cursor == 1) CheckNewSizeY();
        break;
    }

    switch (key)
    {
    case K_ESCAPE:
        S_LocalSound ("misc/menu1.wav");
        menu_first_update = true;
        M_Menu_Options_f ();
        break;
    case K_UPARROW:
        S_LocalSound ("misc/menu1.wav");

        vid_cursor--;
        if (vid_cursor < 0) 
            vid_cursor = 2;
        break;
    case K_DOWNARROW:
        S_LocalSound ("misc/menu1.wav");

        vid_cursor++;
        if (vid_cursor > 2)
            vid_cursor = 0;
        break;
    case K_LEFTARROW:
        if (vid_cursor == 2)
        {
            if (vid_scale_menu > 1)
                vid_scale_menu--;
        }
        break;
    case K_RIGHTARROW:
        if (vid_cursor == 2)
        {
            if (vid_scale_menu < MAXSCALE)
                vid_scale_menu++;
        }
        break;
    case K_ENTER:
        S_LocalSound ("misc/menu2.wav");
        new_w = CheckNewSizeX();
        new_h = CheckNewSizeY();
        vid_scale = vid_scale_menu;
        VID_ResizeWindow(new_w, new_h, false);
        sprintf(temp, "%d", vid_scale);
        Cvar_Set("vid_config_x", str_vid_size_x);
        Cvar_Set("vid_config_y", str_vid_size_y);
        Cvar_Set("vid_config_scale", temp);
        break;
    case K_BACKSPACE:
        switch (vid_cursor)
        {
            case 0:
                len = strlen(str_vid_size_x);
                if (len) str_vid_size_x[len - 1] = '\0';
                break;
            case 1:
                len = strlen(str_vid_size_y);
                if (len) str_vid_size_y[len - 1] = '\0';
                break;
        }
        break;
    default:
        if (key >= '0' && key <= '9')
        {
            switch (vid_cursor)
            {
                case 0:
                    len = strlen(str_vid_size_x);
                    if (len < 4)
                        str_vid_size_x[len] = key;
                    break;
                case 1:
                    len = strlen(str_vid_size_y);
                    if (len < 4)
                        str_vid_size_y[len] = key;
                    break;
            }
        }
        break;
    }
}

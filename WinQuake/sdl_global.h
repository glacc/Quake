#ifndef __SDL_GLOBAL_H__
#define __SDL_GLOBAL_H__

#include <SDL3/SDL.h>

#define	KEYBUF_SIZE	256
#define SNDBUF_SIZE 1024

extern float sdl_mouse_delta_x, sdl_mouse_delta_y;

typedef struct sdl_keyevent_s
{
    SDL_Keycode keycode;
    SDL_MouseButtonFlags mousebtn;
    int scroll;
    int pressed;
}
sdl_keyevent_t;

extern sdl_keyevent_t sdl_keyevents[];

extern int32_t audio_buf32[];
extern int16_t audio_buf16[];
extern SDL_AudioSpec audio_spec;
extern Uint64 last_buf_fed_time;
extern SDL_Mutex *audio_mutex;

extern void QSDL_Init(void);

extern void QSDL_PushKeyEvent(SDL_Keycode keycode, SDL_MouseButtonFlags mousebtn, int scroll, int pressed);
extern void QSDL_StartPullKeyEvent(void);
extern int QSDL_PullKeyEvent(sdl_keyevent_t *keyevent);
extern void QSDL_EndPullKeyEvent(void);

extern void QSDL_ClearMouseDelta(void);

extern void QSDL_WaitEvent(void);
extern void QSDL_DispatchEvents(void);

extern void QSDL_FreeP(void **ptr);

extern int QSDL_FindCorrectPathCase(const char *src, char **dst);

extern bool QSDL_InitAudioForSound(void);
extern bool QSDL_InitAudioForMusic(void);
extern void QSDL_ShutdownAudioForSound(void);
extern void QSDL_ShutdownAudioForMusic(void);

extern void QSDL_LockStream(void);
extern void QSDL_UnlockStream(void);

extern Uint64 QSDL_GetLastAudioTick(void);

#endif


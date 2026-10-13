#include "sdl_global.h"

#include <malloc.h>

// INPUT
float sdl_mouse_delta_x, sdl_mouse_delta_y;

sdl_keyevent_t sdl_keyevents[KEYBUF_SIZE];
int kbuf_head = 0;
int kbuf_tail = 0;
int kbuf_tail_tmp;

// AUDIO
bool audio_init_for_cd  = false;
bool audio_init_for_snd = false;

SDL_AudioStream *audio_stream = NULL;
SDL_AudioSpec audio_spec;

int32_t audio_buf32[SNDBUF_SIZE];
int16_t audio_buf16[SNDBUF_SIZE];
Uint64 last_buf_fed_time;

SDL_Mutex *audio_mutex = NULL;

// VIDEO
extern SDL_Window *vid_sdl_win;
extern int vid_size_x;
extern int vid_size_y;
extern int vid_scale;

// FILE
struct path_enum_s
{
    char *path_to_enum;
    char *name_to_correct;
    char *name_corrected;
    int maxlen;
    int found;
};

extern void Host_Quit_f(void);
extern void VID_Resize(int width, int height);

extern void CDAudio_Mix(int len);
extern void Sound_Mix(int len);

Uint64 QSDL_GetLastAudioTick(void)
{
    Uint64 ret;

    SDL_LockMutex(audio_mutex);
    ret = last_buf_fed_time;
    SDL_UnlockMutex(audio_mutex);

    return ret;
}

void SDLCALL QSDL_AudioStreamCallback(void *userdata, SDL_AudioStream *stream, int additional_amount, int total_amount)
{
    int i;

    SDL_LockMutex(audio_mutex);
    last_buf_fed_time = SDL_GetTicksNS();
    SDL_UnlockMutex(audio_mutex);

    while (additional_amount > 0)
    {
        int len = sizeof(audio_buf16);
        if (additional_amount < len) len = additional_amount;

        memset(audio_buf32, 0, sizeof(audio_buf32));

        // FIXME: currently supports only 16-bit signed.
        CDAudio_Mix(len);
        Sound_Mix(len);

        for (i = 0; i < (len >> 1); i++)
        {
            int32_t val = audio_buf32[i];
            if (val > INT16_MAX)
                val = INT16_MAX;
            if (val < INT16_MIN)
                val = INT16_MIN;
            audio_buf16[i] = val;
        }

        SDL_PutAudioStreamData(stream, audio_buf16, len);

        additional_amount -= len;
    }
}

void QSDL_Init(void)
{
    SDL_Init(SDL_INIT_EVENTS);
}

static int WrapKeyBufIndex(int n)
{
    return n % KEYBUF_SIZE;
}

void QSDL_PushKeyEvent(SDL_Keycode keycode, SDL_MouseButtonFlags mousebtn, int scroll, int pressed)
{
    sdl_keyevent_t keyevent;

    if (WrapKeyBufIndex(kbuf_head + 1) == kbuf_tail)
        return;

    keyevent.keycode = keycode;
    keyevent.mousebtn = mousebtn;
    keyevent.scroll = scroll;
    keyevent.pressed = pressed;

    sdl_keyevents[kbuf_head] = keyevent;
    
    kbuf_head = WrapKeyBufIndex(kbuf_head + 1);
}

void QSDL_StartPullKeyEvent(void)
{
    kbuf_tail_tmp = kbuf_tail;
}

int QSDL_PullKeyEvent(sdl_keyevent_t *keyevent)
{
    if (kbuf_head == kbuf_tail_tmp)
        return 0;

    *keyevent = sdl_keyevents[kbuf_tail_tmp];
    kbuf_tail_tmp = WrapKeyBufIndex(kbuf_tail_tmp + 1);

    return 1;
}

void QSDL_EndPullKeyEvent(void)
{
    kbuf_tail = kbuf_tail_tmp;
}

void QSDL_DispatchEvents(void)
{
    SDL_Event event;
    SDL_Surface *win_surf = NULL;
    SDL_Rect rect;

    if (vid_sdl_win)
        win_surf = SDL_GetWindowSurface(vid_sdl_win);

    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            case SDL_EVENT_MOUSE_MOTION:
                sdl_mouse_delta_x += event.motion.xrel;
                sdl_mouse_delta_y += event.motion.yrel;
                break;
            case SDL_EVENT_WINDOW_MAXIMIZED:
            case SDL_EVENT_WINDOW_RESTORED:
            case SDL_EVENT_WINDOW_RESIZED:
                if (!vid_sdl_win)
                    break;

                win_surf = SDL_GetWindowSurface(vid_sdl_win);

                if (win_surf)
                    VID_Resize(win_surf->w / vid_scale, win_surf->h / vid_scale);

                break;
            case SDL_EVENT_KEY_DOWN:
                QSDL_PushKeyEvent(event.key.key, 0, 0, 1);
                break;
            case SDL_EVENT_KEY_UP:
                QSDL_PushKeyEvent(event.key.key, 0, 0, 0);
                break;
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                QSDL_PushKeyEvent(0, event.button.button, 0, 1);
                break;
            case SDL_EVENT_MOUSE_BUTTON_UP:
                QSDL_PushKeyEvent(0, event.button.button, 0, 0);
                break;
            case SDL_EVENT_MOUSE_WHEEL:
                if (event.wheel.integer_y < 0)
                    QSDL_PushKeyEvent(0, 0, -1, 0);
                if (event.wheel.integer_y > 0)
                    QSDL_PushKeyEvent(0, 0, 1, 0);
                break;
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                Host_Quit_f();
                break;
        }
    }
}

void QSDL_ClearMouseDelta(void)
{
    sdl_mouse_delta_x = 0;
    sdl_mouse_delta_y = 0;
}

static SDL_EnumerationResult SDLCALL EnumerateDirCallback(void *userdata, const char *dirname, const char *fname)
{
    struct path_enum_s *path_enum_info = (struct path_enum_s *)userdata;

    if (!SDL_strncasecmp(fname, path_enum_info->name_to_correct, path_enum_info->maxlen))
    {
        strcpy(path_enum_info->name_corrected, fname);
        path_enum_info->found = 1;
        return SDL_ENUM_SUCCESS;
    }

    return SDL_ENUM_CONTINUE;
}

void QSDL_FreeP(void **ptr)
{
    if (*ptr) free(*ptr);
    *ptr = NULL;
}

int QSDL_FindCorrectPathCase(const char *src, char **dst)
{
    char c_last = '\0';
    int index_cur_layer = 0;
    int index = 0;
    struct path_enum_s path_enum_info;
    int pathsize = strlen(src) + 1;
    int ret = 1;

    if (*dst) free(*dst);
    *dst = malloc(pathsize);
    if (!*dst)
        Sys_Error("FindCorrectPathCase: Failed to allocate memory for case corrected path.");

    path_enum_info.path_to_enum = *dst;
    path_enum_info.name_to_correct = malloc(pathsize);
    path_enum_info.name_corrected = malloc(pathsize);
    path_enum_info.maxlen = pathsize - 1;

    if (!path_enum_info.name_to_correct || !path_enum_info.name_corrected)
        Sys_Error("FindCorrectPathCase: Failed to allocate memory for strings.");

    memset(path_enum_info.path_to_enum, 0, pathsize);
    memset(path_enum_info.name_to_correct, 0, pathsize);
    memset(path_enum_info.name_corrected, 0, pathsize);

    while (1)
    {
        char c = src[index];

        if (c == '/' || c == '\0')
        {
            path_enum_info.found = 0;

            int name_len = index - index_cur_layer;
            memcpy(path_enum_info.name_to_correct, src + index_cur_layer, name_len);
            path_enum_info.name_to_correct[name_len] = '\0';

            if (strcmp(path_enum_info.name_to_correct, "."))
            {
                SDL_EnumerateDirectory(path_enum_info.path_to_enum, EnumerateDirCallback, &path_enum_info);

                if (!path_enum_info.found)
                {
                    ret = 0;
                    goto end;
                }

                memcpy(path_enum_info.path_to_enum + index_cur_layer, path_enum_info.name_corrected, name_len);
            }
            else
                memcpy(path_enum_info.path_to_enum + index_cur_layer, src, name_len);

            if (c) path_enum_info.path_to_enum[index] = '/';

            index_cur_layer = index + 1;
        }

        if (c == '\0')
            break;

        index++;

        c_last = c;
    }

end:
    
    free(path_enum_info.name_to_correct);
    free(path_enum_info.name_corrected);

    return ret;
}

static bool QSDL_InitAudio(void)
{
    SDL_AudioSpec desired_spec;
    
    if (audio_init_for_snd || audio_init_for_cd)
        return true;

    if (!(audio_mutex = SDL_CreateMutex()))
    {
        Con_Printf("QSDL_InitAudio: Failed to create mutex for audio: %s", SDL_GetError());
        return false;
    }

    if (!SDL_WasInit(SDL_INIT_AUDIO))
    {
        if (!SDL_Init(SDL_INIT_AUDIO))
        {
            SDL_DestroyMutex(audio_mutex);
            audio_mutex = NULL;

            Con_Printf("QSDL_InitAudio: Failed to init SDL audio subsystem: %s", SDL_GetError());

            return false;
        }
    }

    desired_spec.channels = 2;
    desired_spec.format = SDL_AUDIO_S16;
    desired_spec.freq = 44100;

    audio_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &desired_spec, QSDL_AudioStreamCallback, NULL);
    if (!audio_stream)
    {
        Con_Printf("QSDL_InitAudio: Failed to open audio device: %s\n", SDL_GetError());
        return false;
    }
    SDL_GetAudioStreamFormat(audio_stream, NULL, &audio_spec);

    Con_Printf("Desired Format: Format%d, %d channels, %d Hz\n", desired_spec.format, desired_spec.channels, desired_spec.freq);
    Con_Printf(" Device Format: Format%d, %d channels, %d Hz\n", audio_spec.format, audio_spec.channels, audio_spec.freq);

#if 1
    if (audio_spec.channels != desired_spec.channels || audio_spec.format != desired_spec.format)
    {
        Con_Printf("QSDL_InitAudio: Device format not supported yet.\n");
        SDL_DestroyAudioStream(audio_stream);
        return false;
    }
#else
    switch (audio_spec.format)
    {
        case SDL_AUDIO_U8:
        case SDL_AUDIO_S8:
            sample_size_per_ch = 1;
            break;
        case SDL_AUDIO_S16:
            sample_size_per_ch = 2;
            break;
        case SDL_AUDIO_S32:
        case SDL_AUDIO_F32:
            sample_size_per_ch = 4;
            break;
        default:
            Con_Printf("QSDL_InitAudio: Unsupported audio device format.\n");
            SDL_DestroyAudioStream(audio_stream);
            return false;
    }
#endif

    SDL_ResumeAudioStreamDevice(audio_stream);

    return true;
}

static void QSDL_CheckAndShutdownAudio(void)
{
    if (audio_init_for_snd || audio_init_for_cd)
        return;

    if (audio_stream)
    {
        SDL_DestroyAudioStream(audio_stream);
        audio_stream = NULL;
    }

    SDL_QuitSubSystem(SDL_INIT_AUDIO);

    SDL_DestroyMutex(audio_mutex);
    audio_mutex = NULL;
}

bool QSDL_InitAudioForSound(void)
{
    bool result = QSDL_InitAudio();
    if (result) audio_init_for_snd = true;
    return result;
}

bool QSDL_InitAudioForMusic(void)
{
    bool result = QSDL_InitAudio();
    if (result) audio_init_for_cd = true;
    return result;
}

void QSDL_ShutdownAudioForSound(void)
{
    audio_init_for_snd = false;
    QSDL_CheckAndShutdownAudio();
}

void QSDL_ShutdownAudioForMusic(void)
{
    audio_init_for_cd = false;
    QSDL_CheckAndShutdownAudio();
}

void QSDL_LockStream(void)
{
    if (audio_stream) SDL_LockAudioStream(audio_stream);
}

void QSDL_UnlockStream(void)
{
    if (audio_stream) SDL_UnlockAudioStream(audio_stream);
}

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
#include "quakedef.h"

#include "sdl_global.h"

#include <SDL3/SDL_audio.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>
#include <assert.h>

#define READ_BUF_SIZE 1024
#define WAVBUF_INIT_SIZE 16384
#define WAVBUF_MAX_INC   (2 * 1024 * 1024)

SDL_Mutex *cd_mutex = NULL;

qboolean cd_initialized = false;

int cd_current_track = -1;
volatile qboolean cd_playing = false;
qboolean cd_looping = false;

int wav_channels;
int wav_sample_rate;
int wav_size;
int wav_samples;

int8_t *wav_data;
int wav_size;

unsigned int sample_delta_f24;
unsigned int sample_progress;

int is_big_endian;

int wav_position;

extern cvar_t bgmvolume;

void ClearWavBuf(void)
{
    if (!wav_data)
        return;
    
    free(wav_data);
    wav_data = NULL;
}

void CDAudio_Mix(int len)
{
    int16_t *wav_data_as_int16;
    int offset;
    int max_offset;

    if (!cd_initialized)
        return;

    if ((cd_current_track < 0) || !cd_playing)
        return;

    SDL_LockMutex(cd_mutex);

    if (!wav_data)
        return;

    float bgm_vol = bgmvolume.value;

    wav_data_as_int16 = (int16_t *)wav_data;
    offset = 0;
    max_offset = len >> 1;

    while (offset < max_offset)
    {
        int wav_offset = wav_position << 1;
        
        audio_buf32[offset++] += wav_data_as_int16[wav_offset++] * bgm_vol;
        audio_buf32[offset++] += wav_data_as_int16[wav_offset]   * bgm_vol;
        
        sample_progress += sample_delta_f24;
        wav_position += sample_progress >> 24;
        sample_progress &= 0xFFFFFF;

        if (wav_position >= wav_samples)
        {
            if (!cd_looping)
            {
                wav_position = 0;
                cd_playing = false;
                break;
            }

            wav_position %= wav_samples;
        }
    }

    SDL_UnlockMutex(cd_mutex);
}

void CDAudio_Play(byte track, qboolean looping)
{
    char track_dir[256];
    char *track_dir_case_corrected = NULL;
    FILE *file;
    OggVorbis_File vorbis_file;
    vorbis_info *vorbis_info = NULL;
    int wav_buf_size;
    int eof = 0;
    int current_section;
    bool has_failure_during_read = false;

    wav_position = 0;
    cd_looping = looping;

    if (!cd_initialized)
        return;

    if (cd_current_track == track)
    {
        cd_playing = true;
        return;
    }

    SDL_LockMutex(cd_mutex);

    sprintf(track_dir, "%s/music/track%02d.ogg", com_gamedir, track);

    ClearWavBuf();

    Con_Printf("Loading Track %02d: %s\n", track, track_dir);
    if (!QSDL_FindCorrectPathCase(track_dir, &track_dir_case_corrected))
    {
        cd_current_track = -1;
        Con_Printf("CDAudio_Play: Couldn't find file \'%s\'.\n", track_dir);
        goto End;
    }
    if (!(file = fopen(track_dir_case_corrected, "rb")))
    {
        cd_current_track = -1;
        Con_Printf("CDAudio_Play: Failed to open file \'%s\'.\n", track_dir_case_corrected);
        goto End;
    }
    if (ov_open_callbacks(file, &vorbis_file, NULL, 0, OV_CALLBACKS_NOCLOSE) < 0)
    {
        cd_current_track = -1;
        Con_Printf("CDAudio_Play: Failed to recognize file \'%s\' as ogg vorbis audio.\n", track_dir_case_corrected);
        goto End;
    }

    vorbis_info = ov_info(&vorbis_file, -1);
    wav_channels = vorbis_info->channels;
    wav_sample_rate = vorbis_info->rate;
    wav_size = 0;
    Con_Printf("Track %02d, %d channels, %d Hz\n", track, vorbis_info->channels, vorbis_info->rate);

    wav_buf_size = WAVBUF_INIT_SIZE;
    wav_data = malloc(sizeof(*wav_data) * wav_buf_size);
    if (!wav_data)
    {
        cd_current_track = -1;
        Con_Printf("CDAudio_Play: Failed to allicate memory for wav_data.\n");
        ov_clear(&vorbis_file);
        goto End;
    }

    while (!eof)
    {
        long ret;
        char wav_read_buf[READ_BUF_SIZE];

        ret = ov_read(&vorbis_file, wav_read_buf, sizeof(wav_read_buf), is_big_endian, 2, 1, &current_section);

        if (ret == 0)
        {
            int8_t *new_wav_data = realloc(wav_data, wav_size);
            if (!new_wav_data)
            {
                Con_Printf("CDAudio_Play: Failed to resize wav_data to fit.\n");
                goto EndErrorDuringRead;
            }
            wav_data = new_wav_data;
            wav_samples = wav_size >> 2;

            eof = 1;
        }
        else if (ret < 0)
            has_failure_during_read = true;
        else
        {
            int wav_size_new = wav_size + ret;

            if (wav_size_new >= wav_buf_size)
            {
                int wav_buf_size_inc = wav_buf_size;
                int wav_buf_size_new;
                int8_t *new_wav_data;
                if (wav_buf_size_inc > WAVBUF_MAX_INC)
                    wav_buf_size_inc = wav_buf_size_inc;
                else if (wav_buf_size_inc < ret)
                    wav_buf_size_inc = ret;
                else
                    wav_buf_size_inc = wav_buf_size;
                
                wav_buf_size_new = wav_buf_size + wav_buf_size_inc;

                new_wav_data = realloc(wav_data, wav_buf_size_new);
                if (!new_wav_data)
                {
                    Con_Printf("CDAudio_Play: Failed to resize wav_data for more data.\n");
                    goto EndErrorDuringRead;
                }

                wav_data = new_wav_data;
                wav_buf_size = wav_buf_size_new;
            }

            memcpy(wav_data + wav_size, wav_read_buf, ret);

            wav_size = wav_size_new;
        }
    }

    SDL_UnlockMutex(cd_mutex);

    if (has_failure_during_read)
        Con_Printf("CDAudio_Play: Error(s) occured while decoding audio file.\n");

    Con_Printf("Track %d loaded.\n", track);

    cd_current_track = track;

    sample_progress = 0;
    if (audio_spec.freq <= 0)
        sample_delta_f24 = 1 << 24; // ignores device sample rate as fallback
    else
        sample_delta_f24 = (int)((float)wav_sample_rate * (float)(1 << 24) / (float)audio_spec.freq);

    ov_clear(&vorbis_file);

    cd_playing = true;

    goto End;

EndErrorDuringRead:
    cd_current_track = -1;
    ov_clear(&vorbis_file);
    ClearWavBuf();
End:
    if (file) fclose(file);
    QSDL_FreeP((void *)&track_dir_case_corrected);
}


void CDAudio_Stop(void)
{
    cd_playing = false;
}


void CDAudio_Pause(void)
{
    cd_playing = false;
}


void CDAudio_Resume(void)
{
    cd_playing = true;
}


void CDAudio_Update(void)
{
}


int CDAudio_Init(void)
{
    if (cd_initialized)
        return 0;

    if (!(cd_mutex = SDL_CreateMutex()))
    {
        Con_Printf("cd_sdl.c: Failed to create mutex for mixer: %s\n", SDL_GetError());;
        return -1;
    }

    if (!QSDL_InitAudioForMusic())
    {
        Con_Printf("cd_sdl.c: Failed to initialize audio subsystem.\n");
        SDL_DestroyMutex(cd_mutex);
        cd_mutex = NULL;
        return -1;
    }
    
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    is_big_endian = 0;
#else
    is_big_endian = 1;
#endif

    cd_current_track = -1;

    cd_initialized = true;
    
    return 0;
}


void CDAudio_Shutdown(void)
{
    if (!cd_initialized)
        return;

    QSDL_LockStream();

    ClearWavBuf();
    cd_initialized = false;

    QSDL_UnlockStream();

    QSDL_ShutdownAudioForMusic();
}

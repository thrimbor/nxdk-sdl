/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2019 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/
#include "../../SDL_internal.h"

/* Output audio to nowhere... */

#include <stdbool.h>
#include <hal/audio.h>
#include "SDL_timer.h"
#include "SDL_audio.h"
#include "../SDL_audio_c.h"
#include "SDL_xboxaudio.h"

void sdlAudioUpdate (void *ac97device, void *data)
{
	SDL_AudioDevice *this = (SDL_AudioDevice *)data;
	
	if (!SDL_AtomicGet(&this->enabled) || SDL_AtomicGet(&this->paused)) {
		return;
	}
	
	XAudioProvideSamples(this->hidden->rawbuf, this->spec.size*NUM_BUFFERS, false);
}

static int
XBOXAUDIO_OpenDevice(_THIS, void *handle, const char *devname, int iscapture)
{
    if (iscapture) {
        return -1;
    }
    
    if (this->spec.channels != 2) return -1;
    
    this->hidden = (struct SDL_PrivateAudioData *)
        SDL_malloc(sizeof(*this->hidden));
    if (this->hidden == NULL) {
        return SDL_OutOfMemory();
    }
    SDL_zerop(this->hidden);
    
    this->hidden->rawbuf = (Uint8 *) SDL_malloc(this->spec.size * NUM_BUFFERS);
	if (this->hidden->rawbuf == NULL) {
		return SDL_OutOfMemory();
	}
	SDL_memset(this->hidden->rawbuf, this->spec.silence, this->spec.size * NUM_BUFFERS);
	for (int i = 0; i < NUM_BUFFERS; i++) {
        this->hidden->mixbufs[i] = &this->hidden->rawbuf[i * this->spec.size];
    }
    this->hidden->next_buffer = 0;
    
    XAudioInit(16, 2, sdlAudioUpdate, this);
    return 0;
}

static void
XBOXAUDIO_CloseDevice(_THIS)
{
	XAudioPause();
    SDL_free(this->hidden->rawbuf);
    SDL_free(this->hidden);
}

static void XBOXAUDIO_PlayDevice(_THIS)
{
    Uint8 *mixbuf = this->hidden->mixbufs[this->hidden->next_buffer];
/*
    if (this->spec.channels == 1) {
        sceAudioOutputBlocking(this->hidden->channel, PSP_AUDIO_VOLUME_MAX, mixbuf);
    } else {
        sceAudioOutputPannedBlocking(this->hidden->channel, PSP_AUDIO_VOLUME_MAX, PSP_AUDIO_VOLUME_MAX, mixbuf);
    }
*/
    this->hidden->next_buffer = (this->hidden->next_buffer + 1) % NUM_BUFFERS;
    XAudioPlay();
}

static Uint8 *
XBOXAUDIO_GetDeviceBuf(_THIS)
{
    return this->hidden->mixbufs[this->hidden->next_buffer];
}

static int
XBOXAUDIO_CaptureFromDevice(_THIS, void *buffer, int buflen)
{
    return 0;
}

static int
XBOXAUDIO_Init(SDL_AudioDriverImpl * impl)
{
    /* Set the function pointers */
    impl->OpenDevice = XBOXAUDIO_OpenDevice;
    impl->PlayDevice = XBOXAUDIO_PlayDevice;
    impl->GetDeviceBuf = XBOXAUDIO_GetDeviceBuf;
    impl->CaptureFromDevice = XBOXAUDIO_CaptureFromDevice;

	impl->CloseDevice = XBOXAUDIO_CloseDevice;

    impl->OnlyHasDefaultOutputDevice = 1;
    impl->OnlyHasDefaultCaptureDevice = 1;
    impl->HasCaptureSupport = SDL_FALSE;

    return 1;   /* this audio target is available. */
}

AudioBootStrap XBOXAUDIO_bootstrap = {
    "xbox", "Xbox nxdk simple audio driver", XBOXAUDIO_Init, 0
};

/* vi: set ts=4 sw=4 expandtab: */

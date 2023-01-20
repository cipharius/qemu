/*
 * QEMU Arcan-shmif audio driver
 *
 * Copyright (c) 2016-2021 Bjorn Stahl
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "qemu/osdep.h"
#include "qemu/module.h"
#include "audio.h"

#ifndef _WIN32
#ifdef __sun__
#define _POSIX_PTHREAD_SEMANTICS 1
#elif defined(__OpenBSD__) || defined(__FreeBSD__) || defined(__DragonFly__)
#include <pthread.h>
#endif
#endif

#define AUDIO_CAP "arcan"
#include "audio_int.h"

#define WANT_ARCAN_SHMIF_HELPER
#include <arcan_shmif.h>

typedef struct ArcanVoiceOut {
    HWVoiceOut hw;
    int exit;
    int initialized;
} ArcanVoiceOut;

typedef struct ArcanVoiceIn {
    HWVoiceIn hw;
    int exit;
    int initialized;
} ArcanVoiceIn;

static size_t arcan_buffer_get_free(HWVoiceOut *hw)
{
    struct arcan_shmif_cont* prim = arcan_shmif_primary(SHMIF_INPUT);
    if (!prim) return 0;

    return prim->abufsize - prim->abufused;
}

static void* arcan_get_buffer_out(HWVoiceOut *hw, size_t *size)
{
    struct arcan_shmif_cont* prim = arcan_shmif_primary(SHMIF_INPUT);
    if (!prim) return 0;

    *size = prim->abufsize - prim->abufused;

    return prim->audp + prim->abufused;
}

static size_t arcan_put_buffer_out(HWVoiceOut *hw, void *buf, size_t size)
{
    struct arcan_shmif_cont* prim = arcan_shmif_primary(SHMIF_INPUT);
    if (!prim) return 0;

    uint8_t* abuf = buf;

    for (size_t i=0; i<size; i++) {
        prim->audb[prim->abufused++] = abuf[i];
    }

    if (prim->abufused >= prim->abufsize) {
        arcan_shmif_signal(prim, SHMIF_SIGAUD | SHMIF_SIGBLK_NONE);
    }

    return size;
}

static size_t arcan_write(HWVoiceOut *hw, void *buf, size_t size)
{
    return 0;
}

static size_t arcan_read(HWVoiceIn *hw, void *buf, size_t size)
{
    return size;
}

static void* arcan_get_buffer_in(HWVoiceIn *hw, size_t *size)
{
    return NULL;
}

static void arcan_put_buffer_in(HWVoiceIn *hw, void *buf, size_t size)
{
}

static void arcan_fini_out(HWVoiceOut *hw)
{
}

static int arcan_init_out(HWVoiceOut *hw, struct audsettings *as,
                        void *drv_opaque)
{
    struct audsettings arcan_as;

    arcan_as.freq = 44100;
    arcan_as.nchannels = ARCAN_SHMIF_ACHANNELS;
    arcan_as.fmt = AUDIO_FORMAT_S16;
    arcan_as.endianness = 0;

    audio_pcm_init_info(&hw->info, &arcan_as);
    hw->samples = 1024;

    return 0;
}

static void arcan_enable_out(HWVoiceOut *hw, bool enable)
{
}

static void arcan_fini_in(HWVoiceIn *hw)
{
}

static int arcan_init_in(HWVoiceIn *hw, audsettings *as, void *drv_opaque)
{
    return 0;
}

static void arcan_enable_in(HWVoiceIn *hw, bool enable)
{
}

static void *arcan_audio_init(Audiodev *dev)
{
    return dev;
}

static void arcan_audio_fini (void *opaque)
{
}

static struct audio_pcm_ops arcan_pcm_ops = {
    .init_out = arcan_init_out,
    .fini_out = arcan_fini_out,
    .write    = arcan_write,
    .buffer_get_free = arcan_buffer_get_free,
    .get_buffer_out = arcan_get_buffer_out,
    .put_buffer_out = arcan_put_buffer_out,
    .enable_out = arcan_enable_out,
    .init_in = arcan_init_in,
    .fini_in = arcan_fini_in,
    .read = arcan_read,
    .get_buffer_in = arcan_get_buffer_in,
    .put_buffer_in = arcan_put_buffer_in,
    .enable_in = arcan_enable_in,
};

static struct audio_driver arcan_audio_driver = {
    .name           = "arcan",
    .descr          = "arcan https://arcan-fe.com",
    .init           = arcan_audio_init,
    .fini           = arcan_audio_fini,
    .pcm_ops        = &arcan_pcm_ops,
    .can_be_default = 1,
    .max_voices_out = INT_MAX,
    .max_voices_in  = INT_MAX,
    .voice_size_out = sizeof(ArcanVoiceOut),
    .voice_size_in  = sizeof(ArcanVoiceIn),
};

static void register_audio_arcan(void)
{
    audio_driver_register(&arcan_audio_driver);
}
type_init(register_audio_arcan);

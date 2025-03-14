// LGPL-2.1 License
// (C) 2025 Steward Fu <steward.fu@gmail.com>

#include "../../SDL_internal.h"

#if SDL_AUDIO_DRIVER_MINI

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <linux/soundcard.h>
#include <json-c/json.h>

#include "SDL_timer.h"
#include "SDL_audio.h"
#include "../SDL_audio_c.h"
#include "../SDL_audiodev_c.h"

#include "mi_sys.h"
#include "mi_common_datatype.h"
#include "mi_ao.h"

#include "../../video/mini/SDL_video_mini.h"
#include "SDL_audio_mini.h"

static int cur_volume = 0;
static MI_AUDIO_Attr_t stSetAttr;
static MI_AUDIO_Attr_t stGetAttr;
static MI_AO_CHN AoChn = 0;
static MI_AUDIO_DEV AoDevId = 0;
static struct json_object *jfile = NULL;

static int set_volume_raw(int value, int add)
{
    int fd = -1;
    int buf2[] = {0, 0}, prev_value = 0;
    uint64_t buf1[] = {sizeof(buf2), (uintptr_t)buf2};

    if ((fd = open("/dev/mi_ao", O_RDWR)) < 0) {
        return 0;
    }

    ioctl(fd, MI_AO_GETVOLUME, buf1);
    prev_value = buf2[1];

    if (add) {
        value = prev_value + add;
    }
    else {
        value += MIN_RAW_VALUE;
    }

    if (value > MAX_RAW_VALUE) {
        value = MAX_RAW_VALUE;
    }
    else if (value < MIN_RAW_VALUE) {
        value = MIN_RAW_VALUE;
    }

    if (value == prev_value) {
        close(fd);
        return prev_value;
    }

    buf2[1] = value;
    ioctl(fd, MI_AO_SETVOLUME, buf1);
    if (prev_value <= MIN_RAW_VALUE && value > MIN_RAW_VALUE) {
        buf2[1] = 0;
        ioctl(fd, MI_AO_SETMUTE, buf1);
    }
    else if (prev_value > MIN_RAW_VALUE && value <= MIN_RAW_VALUE) {
        buf2[1] = 1;
        ioctl(fd, MI_AO_SETMUTE, buf1);
    }
    close(fd);
    return value;
}

static int set_volume(int volume)
{
    int volume_raw = 0;

    if (volume > MAX_VOLUME) {
        volume = MAX_VOLUME;
    }
    else if (volume < 0) {
        volume = 0;
    }

    if (volume != 0) {
        volume_raw = round(48 * log10(1 + volume));
    }

    set_volume_raw(volume_raw, 0);
    return volume;
}

int volume_inc(void)
{
    if (cur_volume < MAX_VOLUME) {
        cur_volume+= 1;
        set_volume(cur_volume);
    }
    return cur_volume;
}

int volume_dec(void)
{
    if (cur_volume > 0) {
        cur_volume-= 1;
        set_volume(cur_volume);
    }
    return cur_volume;
}

static void MINI_CloseDevice(_THIS)
{
    SDL_free(this->hidden->mixbuf);
    SDL_free(this->hidden);

    MI_AO_DisableChn(AoDevId, AoChn);
    MI_AO_Disable(AoDevId);
}

static int MINI_OpenDevice(_THIS, void *handle, const char *devname, int iscapture)
{
    MI_S32 miret = 0;
    MI_S32 s32SetVolumeDb = 0;
    MI_S32 s32GetVolumeDb = 0;
    MI_SYS_ChnPort_t stAoChn0OutputPort0;

    this->hidden = (struct SDL_PrivateAudioData *)SDL_malloc((sizeof * this->hidden));
    if(this->hidden == NULL) {
        return SDL_OutOfMemory();
    }
    SDL_zerop(this->hidden);

    this->hidden->mixlen = this->spec.samples * 2 * this->spec.channels;
    this->hidden->mixbuf = (Uint8 *) SDL_malloc(this->hidden->mixlen);
    if(this->hidden->mixbuf == NULL) {
        return SDL_OutOfMemory();
    }

    jfile = json_object_from_file(JSON_APP_FILE);
    if (jfile != NULL) {
        struct json_object *volume = NULL;

        json_object_object_get_ex(jfile, JSON_VOL_KEY, &volume);
        cur_volume = json_object_get_int(volume);
        //json_object_object_add(jfile, JSON_VOL_KEY, json_object_new_int(2));
        //json_object_to_file(JSON_APP_FILE, jfile);
        json_object_put(jfile);
    }

    stSetAttr.eBitwidth = E_MI_AUDIO_BIT_WIDTH_16;
    stSetAttr.eWorkmode = E_MI_AUDIO_MODE_I2S_MASTER;
    stSetAttr.u32FrmNum = 6;
    stSetAttr.u32PtNumPerFrm = this->spec.samples;
    stSetAttr.u32ChnCnt = this->spec.channels;
    stSetAttr.eSoundmode = this->spec.channels == 2 ? E_MI_AUDIO_SOUND_MODE_STEREO : E_MI_AUDIO_SOUND_MODE_MONO;
    stSetAttr.eSamplerate = (MI_AUDIO_SampleRate_e)this->spec.freq;
    //printf(PREFIX"Freq:%d, Sample:%d, Channels:%d\n", this->spec.freq, this->spec.samples, this->spec.channels);
    miret = MI_AO_SetPubAttr(AoDevId, &stSetAttr);
    if(miret != MI_SUCCESS) {
        //printf(PREFIX"failed to set PubAttr\n");
        return -1;
    }
    miret = MI_AO_GetPubAttr(AoDevId, &stGetAttr);
    if(miret != MI_SUCCESS) {
        //printf(PREFIX"failed to get PubAttr\n");
        return -1;
    }
    miret = MI_AO_Enable(AoDevId);
    if(miret != MI_SUCCESS) {
        //printf(PREFIX"failed to enable AO\n");
        return -1;
    }
    miret = MI_AO_EnableChn(AoDevId, AoChn);
    if(miret != MI_SUCCESS) {
        //printf(PREFIX"failed to enable Channel\n");
        return -1;
    }
    miret = MI_AO_SetVolume(AoDevId, s32SetVolumeDb);
    if(miret != MI_SUCCESS) {
        //printf(PREFIX"failed to set Volume\n");
        return -1;
    }
    MI_AO_GetVolume(AoDevId, &s32GetVolumeDb);
    stAoChn0OutputPort0.eModId = E_MI_MODULE_ID_AO;
    stAoChn0OutputPort0.u32DevId = AoDevId;
    stAoChn0OutputPort0.u32ChnId = AoChn;
    stAoChn0OutputPort0.u32PortId = 0;
    MI_SYS_SetChnOutputPortDepth(&stAoChn0OutputPort0, 12, 13);
    set_volume(cur_volume);
    return 0;
}

static void MINI_PlayDevice(_THIS)
{
    MI_AUDIO_Frame_t aoTestFrame;

    aoTestFrame.eBitwidth = stGetAttr.eBitwidth;
    aoTestFrame.eSoundmode = stGetAttr.eSoundmode;
    aoTestFrame.u32Len = this->hidden->mixlen;
    aoTestFrame.apVirAddr[0] = this->hidden->mixbuf;
    aoTestFrame.apVirAddr[1] = NULL;
    MI_AO_SendFrame(AoDevId, AoChn, &aoTestFrame, 1);
    usleep(((stSetAttr.u32PtNumPerFrm * 1000) / stSetAttr.eSamplerate - 10) * 1000);
}

static uint8_t *MINI_GetDeviceBuf(_THIS)
{
    return (this->hidden->mixbuf);
}

static int MINI_Init(SDL_AudioDriverImpl *impl)
{
    impl->OpenDevice = MINI_OpenDevice;
    impl->PlayDevice = MINI_PlayDevice;
    impl->GetDeviceBuf = MINI_GetDeviceBuf;
    impl->CloseDevice = MINI_CloseDevice;
    impl->OnlyHasDefaultOutputDevice = 1;
    return 1;
}

AudioBootStrap Mini_AudioDriver = {
    "Miyoo Mini",
    "Miyoo Mini Audio Driver",
    MINI_Init,
    0
};

#endif


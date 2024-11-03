#include "PureDOOM/PureDOOM.h"
#include "main.h"
#include "midi/midiplay.h"

static SemaphoreHandle_t doom_audio_semaphore;

static void XAudioCallbackfn(void *pac97Device, void *data)
{
    xSemaphoreGiveFromISR(doom_audio_semaphore, NULL);
}

static int16_t linear_interpolate(int16_t y1, int16_t y2, float t)
{
    return (int16_t)(y1 + t * (y2 - y1));
}

static void resample_audio_stereo(int16_t *input, int input_size, int16_t *output, int output_size)
{
    const float ratio = (float)((float)input_size / 2.0f) / ((float)output_size / 2.0f);
    for (uint32_t i = 0; i < output_size / 2; i++) {

        float src_index = i * ratio;
        uint32_t idx = (int)src_index;

        float frac = src_index - idx;

        uint32_t input_idx = idx * 2;
        uint32_t output_idx = i * 2;

        // Interpolate the left channel
        if (input_idx < input_size - 2) {
            output[output_idx] = linear_interpolate(input[input_idx], input[input_idx + 2], frac);
        } else {
            output[output_idx] = input[input_size - 2];
        }

        // Interpolate the right channel
        if (input_idx + 1 < input_size - 2) {
            // If right channel was the same as the left channel just copy it over, otherwise do linear interpolation
            if (input[input_idx] == input[input_idx + 1]) {
                output[output_idx + 1] = output[output_idx];
            } else {
                output[output_idx + 1] = linear_interpolate(input[input_idx + 1], input[input_idx + 3], frac);
            }
        } else {
            output[output_idx + 1] = input[input_size - 1];
        }
    }
}

void dooom_changing_music(void *data, int looping)
{
    if (Midiplay_Load(data, 0xFFFF) == 0) {
        printf_ts("Failed to load music\n");
    } else {
        Midiplay_Loop(looping);
        Midiplay_Play(1);
    }
}

void dooom_stop_song()
{
    Midiplay_Play(0);
}

void dooom_set_volume(int volume)
{
    Midiplay_SetVolume(volume);
}

void doom_sound_task(void *parameters)
{
    SemaphoreHandle_t *doom_logic_mutex = (SemaphoreHandle_t *)parameters;

    static int16_t resampled_buffer0[2230 * 2]; // 48000/11025 * 512 * 2 channels
    static int16_t resampled_buffer1[2230 * 2]; // 48000/11025 * 512 * 2 channels
    static int16_t midi_buffer[1024];

    XAudioInit(16, 2, XAudioCallbackfn, NULL);

    // Although we could generate midi at 48khz directly for Xbox, we have to resample to doom sfx anyway,
    // so it's more efficient to generate at 11025khz, mix in the sfx then resample to 48khz
    Midiplay_Init(DOOM_SAMPLERATE);

    memset(resampled_buffer0, 0, sizeof(resampled_buffer0));
    memset(resampled_buffer1, 0, sizeof(resampled_buffer1));
    doom_audio_semaphore = xSemaphoreCreateCounting(2, 0);

    XAudioProvideSamples((unsigned char *)resampled_buffer0, sizeof(resampled_buffer0), 0);
    XAudioProvideSamples((unsigned char *)resampled_buffer1, sizeof(resampled_buffer1), 0);
    XAudioPlay();

    int buffer_index = 0;
    while (1) {
        int16_t *out_buffer = (buffer_index == 0) ? resampled_buffer0 : resampled_buffer1;
        buffer_index ^= 1;

        xSemaphoreTake(doom_audio_semaphore, portMAX_DELAY);

        xSemaphoreTake(*doom_logic_mutex, portMAX_DELAY);
        int16_t *sfx_buffer = doom_get_sound_buffer();
        Midiplay_Output(midi_buffer, 1024);
        xSemaphoreGive(*doom_logic_mutex);

        // Mix the midi buffer with the audio buffer
        for (uint16_t i = 0; i < 1024; i++) {
            int32_t mixed = (int32_t)sfx_buffer[i] + ((int32_t)midi_buffer[i] << 2);
            if (mixed > 32767) {
                mixed = 32767;
            } else if (mixed < -32768) {
                mixed = -32768;
            }
            sfx_buffer[i] = (int16_t)mixed;
        }

        resample_audio_stereo(sfx_buffer, 512 * 2, out_buffer, 2230 * 2);
        XAudioProvideSamples((unsigned char *)out_buffer, sizeof(resampled_buffer0), 0);
    }
}

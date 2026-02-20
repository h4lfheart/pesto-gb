#include "audio.h"
#include <algorithm>
#include <cstring>

Audio::Audio()
{
    sample_buffer.resize(buffer_size, 0.0f);
}

Audio::~Audio()
{
    Close();
}

bool Audio::Initialize()
{
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
        return false;
    }

    SDL_AudioSpec desired_spec;
    SDL_zero(desired_spec);

    desired_spec.freq = SAMPLE_RATE;
    desired_spec.format = AUDIO_F32SYS;
    desired_spec.channels = CHANNELS;
    desired_spec.samples = BUFFER_SIZE;
    desired_spec.callback = AudioCallback;
    desired_spec.userdata = this;

    device_id = SDL_OpenAudioDevice(
        nullptr,
        0,
        &desired_spec,
        &audio_spec,
        0
    );

    if (device_id == 0) {
        return false;
    }

    SDL_PauseAudioDevice(device_id, 0);

    return true;
}

void Audio::Close()
{
    if (device_id != 0) {
        SDL_CloseAudioDevice(device_id);
        device_id = 0;
    }
}

void Audio::PushSample(float left, float right)
{
    std::lock_guard<std::mutex> lock(buffer_mutex);

    sample_buffer[buffer_write_pos] = left;
    buffer_write_pos = (buffer_write_pos + 1) % buffer_size;

    sample_buffer[buffer_write_pos] = right;
    buffer_write_pos = (buffer_write_pos + 1) % buffer_size;

    if (buffer_write_pos == buffer_read_pos) {
        buffer_read_pos = (buffer_read_pos + 2) % buffer_size;
    }
}

void Audio::AudioCallback(void* userdata, uint8_t* stream, int len)
{
    Audio* player = static_cast<Audio*>(userdata);
    player->AudioCallbackImpl(stream, len);
}

void Audio::AudioCallbackImpl(uint8_t* stream, int len)
{
    std::lock_guard<std::mutex> lock(buffer_mutex);

    const auto output = reinterpret_cast<float*>(stream);
    const int samples_needed = len / sizeof(float);

    for (int i = 0; i < samples_needed; i += 2) {
        if (buffer_read_pos != buffer_write_pos) {
            last_left = sample_buffer[buffer_read_pos];
            buffer_read_pos = (buffer_read_pos + 1) % buffer_size;

            if (buffer_read_pos != buffer_write_pos) {
                last_right = sample_buffer[buffer_read_pos];
                buffer_read_pos = (buffer_read_pos + 1) % buffer_size;
            }
        }

        output[i] = std::clamp(last_left, -1.0f, 1.0f);
        output[i + 1] = std::clamp(last_right, -1.0f, 1.0f);
    }
}
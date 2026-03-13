#pragma once
#include <SDL2/SDL.h>
#include <cstdint>
#include <vector>
#include <mutex>


class Audio {
public:
    static constexpr int SAMPLE_RATE = 44100;
    static constexpr int BUFFER_SIZE = 2048;
    static constexpr int CHANNELS = 2;

    Audio();
    ~Audio();

    bool Initialize();
    void Close();

    void PushSample(float left, float right);

    static void AudioCallback(void* userdata, uint8_t* stream, int len);

private:
    SDL_AudioDeviceID device_id = 0;
    SDL_AudioSpec audio_spec;

    std::vector<float> sample_buffer;
    size_t buffer_write_pos = 0;
    size_t buffer_read_pos = 0;
    size_t buffer_size = BUFFER_SIZE * CHANNELS * 4;
    std::mutex buffer_mutex;

    float last_left = 0.0f;
    float last_right = 0.0f;

    void AudioCallbackImpl(uint8_t* stream, int len);
};
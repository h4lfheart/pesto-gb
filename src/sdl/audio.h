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
    SDL_AudioDeviceID device_id;
    SDL_AudioSpec audio_spec;

    std::vector<float> sample_buffer;
    size_t buffer_write_pos;
    size_t buffer_read_pos;
    size_t buffer_size;
    std::mutex buffer_mutex;

    float time_accumulator;
    float last_left;
    float last_right;

    void AudioCallbackImpl(uint8_t* stream, int len);
    float hp_prev_in_left = 0.0f,  hp_prev_out_left = 0.0f;
    float hp_prev_in_right = 0.0f, hp_prev_out_right = 0.0f;
};
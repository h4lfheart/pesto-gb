#pragma once
#include <atomic>
#include <cstdint>

#include "task.h"
#include "driver/i2s_std.h"
#include "driver/gpio.h"

#define I2S_BCLK GPIO_NUM_17
#define I2S_WCLK GPIO_NUM_18
#define I2S_DOUT GPIO_NUM_16

#define SAMPLE_RATE 44100
#define AUDIO_BATCH 128
#define AUDIO_MASTER_VOL 1.0f

#define HPF_DECAY 0.998943

#define SAMPLE_QUEUE_SIZE 2048
#define SAMPLE_QUEUE_MASK (SAMPLE_QUEUE_SIZE - 1)

struct SampleQueue
{
    struct SamplePair
    {
        float left;
        float right;
    };

    alignas(64) std::atomic<uint32_t> write_pos = {0};
    alignas(64) std::atomic<uint32_t> read_pos = {0};

    SamplePair buf[SAMPLE_QUEUE_SIZE];

    void Push(const float left, const float right)
    {
        const uint32_t cur_write_pos = write_pos.load(std::memory_order_relaxed);
        const uint32_t next = (cur_write_pos + 1) & SAMPLE_QUEUE_MASK;
        if (next == read_pos.load(std::memory_order_acquire))
            return;

        buf[cur_write_pos] = {left, right};
        write_pos.store(next, std::memory_order_release);
    }

    bool Pop(float& left, float& right)
    {
        const uint32_t cur_read_pos = read_pos.load(std::memory_order_relaxed);
        if (cur_read_pos == write_pos.load(std::memory_order_acquire))
            return false;

        left = buf[cur_read_pos].left;
        right = buf[cur_read_pos].right;
        read_pos.store((cur_read_pos + 1) & SAMPLE_QUEUE_MASK, std::memory_order_release);
        return true;
    }
};

class AudioTask : public Task
{
public:
    void Init();
    void Start() override;
    void PushSample(float left, float right);

protected:
    void Run() override;

private:
    static int32_t ScaleSample(float sample);

    SampleQueue sample_queue = {};
    i2s_chan_handle_t i2s_handle = nullptr;

    int32_t batch[AUDIO_BATCH * 2] = {};
    int batch_pos = 0;

    double hpf_left = 0.0;
    double hpf_right = 0.0;
};
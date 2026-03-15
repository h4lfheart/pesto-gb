#include "audio_task.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void AudioTask::Init()
{
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    chan_cfg.dma_desc_num = 4;
    chan_cfg.dma_frame_num = 64;
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &i2s_handle, nullptr));

    constexpr i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = I2S_BCLK,
            .ws = I2S_WCLK,
            .dout = I2S_DOUT,
            .din = I2S_GPIO_UNUSED,
            .invert_flags = {},
        },
    };

    ESP_ERROR_CHECK(i2s_channel_init_std_mode(i2s_handle, &std_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(i2s_handle));
}

void AudioTask::Start()
{
    xTaskCreatePinnedToCore(TaskFunc, "audio", 4096, this, 7, nullptr, 0);
}

void AudioTask::PushSample(float left, float right)
{
    sample_queue.Push(left, right);
}

int32_t AudioTask::ScaleSample(float sample)
{
    const float fs = sample * AUDIO_MASTER_VOL * 32767.0f;
    const float clamped = fs > 32767.0f ? 32767.0f : fs < -32768.0f ? -32768.0f : fs;
    return static_cast<int32_t>(static_cast<int16_t>(clamped)) << 16;
}

void AudioTask::Run()
{
    while (true)
    {
        float left, right;

        if (!sample_queue.Pop(left, right))
        {
            taskYIELD();
            continue;
        }

        const double hp_left = left - hpf_left;
        hpf_left = left - hp_left * HPF_DECAY;

        const double hp_right = right - hpf_right;
        hpf_right = right - hp_right * HPF_DECAY;

        batch[batch_pos] = ScaleSample(static_cast<float>(hp_left));
        batch[batch_pos + 1] = ScaleSample(static_cast<float>(hp_right));
        batch_pos += 2;

        if (batch_pos >= AUDIO_BATCH * 2)
        {
            size_t written = 0;
            i2s_channel_write(i2s_handle, batch, sizeof(batch), &written, portMAX_DELAY);
            batch_pos = 0;
        }
    }
}

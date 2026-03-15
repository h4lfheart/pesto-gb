#include "emulator_task.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_task_wdt.h"
#include "driver/gpio.h"
#include "esp_timer.h"

void EmulatorTask::Init(GameBoy* emulator)
{
    this->emulator = emulator;
}

void EmulatorTask::Start()
{
    xTaskCreatePinnedToCore(TaskFunc, "emulator", 8192, this, 6, nullptr, 1);
}

void EmulatorTask::Run()
{
    constexpr auto frame_budget = static_cast<int64_t>(1'000'000.0 / FRAMES_PER_SECOND);
    int64_t frame_start = esp_timer_get_time();

    while (true)
    {
        emulator->TickFrame();

        const int64_t next_frame = frame_start + frame_budget;
        const int64_t remaining_ms = next_frame - esp_timer_get_time();

        if (remaining_ms > 1000)
            vTaskDelay(pdMS_TO_TICKS(remaining_ms / 1000));

        while (esp_timer_get_time() < next_frame)
        {
            // spin until smaller time is done
        }

        frame_start = next_frame;
    }
}

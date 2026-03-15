#include "input_task.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

const ButtonMapping InputTask::BUTTON_MAP[] = {
    {GPIO_NUM_14, InputButton::BUTTON_A},
    {GPIO_NUM_13, InputButton::BUTTON_B},
    {GPIO_NUM_12, InputButton::BUTTON_UP},
    {GPIO_NUM_11, InputButton::BUTTON_RIGHT},
    {GPIO_NUM_10, InputButton::BUTTON_LEFT},
    {GPIO_NUM_9,  InputButton::BUTTON_DOWN},
    {GPIO_NUM_3,  InputButton::BUTTON_START},
};

void InputTask::Init(GameBoy* gb)
{
    emulator = gb;
    for (const auto& [pin, button] : BUTTON_MAP)
    {
        gpio_reset_pin(pin);
        gpio_set_direction(pin, GPIO_MODE_INPUT);
        gpio_set_pull_mode(pin, GPIO_PULLUP_ONLY);
    }
}

void InputTask::Start()
{
    xTaskCreatePinnedToCore(TaskFunc, "input", 2048, this, 5, nullptr, 0);
}

void InputTask::Run()
{
    bool last_state[std::size(BUTTON_MAP)] = {};

    while (true)
    {
        for (int i = 0; i < static_cast<int>(std::size(BUTTON_MAP)); i++)
        {
            const bool is_down = gpio_get_level(BUTTON_MAP[i].pin) == 0;
            if (is_down == last_state[i])
                continue;

            if (is_down)
                emulator->PressButton(BUTTON_MAP[i].button);
            else
                emulator->ReleaseButton(BUTTON_MAP[i].button);

            last_state[i] = is_down;
        }

        vTaskDelay(pdMS_TO_TICKS(1000 / INPUT_POLL_RATE));
    }
}
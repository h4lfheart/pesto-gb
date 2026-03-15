#pragma once
#include "task.h"
#include "core/gameboy.h"
#include "driver/gpio.h"

#define INPUT_POLL_RATE 60

struct ButtonMapping
{
    gpio_num_t pin;
    InputButton button;
};


class InputTask : public Task
{
public:
    void Init(GameBoy* emulator);
    void Start() override;

protected:
    void Run() override;

private:
    static const ButtonMapping BUTTON_MAP[];

    GameBoy* emulator = nullptr;
};
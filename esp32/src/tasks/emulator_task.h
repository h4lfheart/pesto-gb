#pragma once
#include "task.h"
#include "core/gameboy.h"
#include "driver/gpio.h"

class EmulatorTask : public Task
{
public:
    void Init(GameBoy* emulator);
    void Start() override;

protected:
    void Run() override;

private:
    GameBoy* emulator = nullptr;
};

#pragma once
#include "base_channel.h"

#define CH1_NR10_ADDR 0x10
#define CH1_NR11_ADDR 0x11
#define CH1_NR12_ADDR 0x12
#define CH1_NR13_ADDR 0x13
#define CH1_NR14_ADDR 0x14

#define CH1_NR10_SWEEP_PACE_MASK 0b01110000
#define CH1_NR10_SWEEP_DIR_MASK 0b00001000
#define CH1_NR10_SWEEP_SHIFT_MASK 0b00000111
#define CH1_MAX_SWEEP_PERIOD 0x7FF

class Channel1 : public BaseChannel
{
public:
    void Tick() override;
    void TickFrame(uint8_t frame_idx) override;

private:
    void Enable();

    void TickLength();
    void TickEnvelope();
    void TickSweep();

    uint16_t period = 0;
    uint16_t period_timer = 0;

    uint8_t duty_type = 0;
    uint8_t duty_step = 0;

    uint8_t volume = 0;
    uint8_t length_timer = 0;
    uint8_t envelope_timer = 0;

    uint8_t sweep_timer = 0;
    uint16_t sweep_period = 0;
};

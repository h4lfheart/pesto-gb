#pragma once
#include "base_channel.h"

#define CH2_NR21_ADDR 0x16
#define CH2_NR22_ADDR 0x17
#define CH2_NR23_ADDR 0x18
#define CH2_NR24_ADDR 0x19

class Channel2 : public BaseChannel
{
public:
    void Tick() override;
    void TickFrame(uint8_t frame_idx) override;

private:
    void Enable();

    void TickLength();
    void TickEnvelope();

    uint16_t period = 0;
    uint16_t period_timer = 0;

    uint8_t duty_type = 0;
    uint8_t duty_step = 0;

    uint8_t volume = 0;
    uint8_t length_timer = 0;
    uint8_t envelope_timer = 0;
};

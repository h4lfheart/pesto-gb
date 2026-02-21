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
    void Reset() override;
    void AttachMemory(Memory* mem) override;
    bool IsDACEnabled() override;

    void Trigger();

    void TickLength();
    void TickEnvelope();

    uint8_t* nr21 = nullptr;
    uint8_t* nr22 = nullptr;
    uint8_t* nr23 = nullptr;
    uint8_t* nr24 = nullptr;

    uint16_t period = 0;
    uint16_t period_timer = 0;

    uint8_t duty_type = 0;
    uint8_t duty_step = 0;

    uint8_t volume = 0;
    uint8_t length_timer = 0;
    uint8_t envelope_timer = 0;

    bool is_envelope_alive = false;
};

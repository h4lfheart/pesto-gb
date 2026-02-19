#pragma once
#include "base_channel.h"

#define CH4_NR41_ADDR 0x20
#define CH4_NR42_ADDR 0x21
#define CH4_NR43_ADDR 0x22
#define CH4_NR44_ADDR 0x23

#define CH4_NR43_CLOCK_DIVIDER_MASK 0b00000111
#define CH4_NR43_LFSR_WIDTH_MASK 0b00001000
#define CH4_NR43_CLOCK_SHIFT_MASK 0b11110000

#define CH4_LFSR_DEFAULT 0x7FFF

class Channel4 : public BaseChannel
{
public:
    void Tick() override;
    void TickFrame(uint8_t frame_idx) override;

private:
    void Enable();

    void TickLength();
    void TickEnvelope();

    uint8_t volume = 0;
    uint8_t length_timer = 0;
    uint8_t envelope_timer = 0;
    uint16_t period_timer = 0;

    uint16_t lfsr = CH4_LFSR_DEFAULT;
};

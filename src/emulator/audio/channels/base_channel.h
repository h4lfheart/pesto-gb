#pragma once
#include "../../memory/memory.h"

#define CH_NRx1_DUTY_TYPE_MASK 0b11000000
#define CH_NRx1_LENGTH_MASK 0b00111111

#define CH_NRx2_VOLUME_MASK 0b11110000
#define CH_NRx2_ENVELOPE_DIR_MASK 0b00001000
#define CH_NRx2_ENV_PACE_MASK 0b00000111
#define CH_NRx2_DAC_MASK 0b11111000

#define CH_NRx4_TRIGGER_MASK 0b10000000
#define CH_NRx4_LENGTH_ENABLE_MASK 0b01000000
#define CH_NRx4_PERIOD_HIGH_MASK 0b00000111

#define CH_PERIOD_START 2048
#define CH_PERIOD_MULTIPLIER 4
#define CH_DUTY_STEP_MASK 0b111
#define CH_MAX_VOLUME 0xF


class BaseChannel
{
public:
    virtual ~BaseChannel() = default;
    void AttachMemory(Memory* mem);

    virtual void Tick() = 0;
    virtual void TickFrame(uint8_t frame_idx);

    uint8_t GetOutput();
    bool IsEnabled();
    bool IsDACEnabled();

protected:
    uint8_t output = 0;
    uint8_t is_enabled = false;
    uint8_t is_dac_enabled = false;

    Memory* memory = nullptr;

    const uint8_t duty_cycles[4][8] = {
        {0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 1, 1, 1},
        {0, 1, 1, 1, 1, 1, 1, 0}
    };;
};

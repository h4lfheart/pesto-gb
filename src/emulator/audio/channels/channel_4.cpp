#include "channel_4.h"

#include <cmath>

void Channel4::Tick()
{
    if (!(this->is_enabled && this->IsDACEnabled()))
    {
        this->output = 0;
        return;
    }

    this->period_timer--;
    if (this->period_timer == 0)
    {
        const uint8_t shift = (*nr43 & CH4_NR43_CLOCK_SHIFT_MASK) >> 4;
        const uint8_t divider = *nr43 & CH4_NR43_CLOCK_DIVIDER_MASK;
        this->period_timer = (divider == 0 ? 8 : divider << 4) << shift;

        const uint8_t xor_value = ((this->lfsr & 1) ^ ((this->lfsr >> 1) & 1));
        this->lfsr = xor_value << 14 | (this->lfsr >> 1);

        if (*nr43 & CH4_NR43_LFSR_WIDTH_MASK)
            this->lfsr = (this->lfsr & ~(1 << 6)) | (xor_value << 6);
    }

    this->output = this->lfsr & 1 ? 0 : volume;
}

void Channel4::TickFrame(uint8_t frame_idx)
{
    if ((frame_idx & 1) == 0)
        this->TickLength();

    if (!this->is_enabled)
        return;

    if (frame_idx == 0b111)
        this->TickEnvelope();
}

void Channel4::Reset()
{
    is_enabled = false;
    period_timer = 0;
    volume = 0;
    envelope_timer = 0;
    lfsr = 0;
    length_timer = 0;
}

void Channel4::AttachMemory(Memory* mem)
{
    BaseChannel::AttachMemory(mem);

    this->nr41 = mem->PtrIO(CH4_NR41_ADDR);
    this->nr42 = mem->PtrIO(CH4_NR42_ADDR);
    this->nr43 = mem->PtrIO(CH4_NR43_ADDR);
    this->nr44 = mem->PtrIO(CH4_NR44_ADDR);
}

bool Channel4::IsDACEnabled()
{
    return (*nr42 & CH_NRx2_DAC_MASK) != 0;
}

void Channel4::Trigger()
{
    this->is_enabled = IsDACEnabled();

    this->volume = (*nr42 & CH_NRx2_VOLUME_MASK) >> 4;

    if (this->length_timer == 0)
        this->length_timer = CH_6BIT_LENGTH_MAX;

    this->envelope_timer = 0;

    this->lfsr = CH4_LFSR_DEFAULT;

    const uint8_t shift = (*nr43 & CH4_NR43_CLOCK_SHIFT_MASK) >> 4;
    const uint8_t divider = *nr43 & CH4_NR43_CLOCK_DIVIDER_MASK;
    this->period_timer = (divider == 0 ? 8 : divider << 4) << shift;

    this->is_envelope_alive = true;
}

void Channel4::TickLength()
{
    if ((*nr44 & CH_NRx4_LENGTH_ENABLE_MASK) == 0)
        return;

    if (this->length_timer > 0)
    {
        this->length_timer--;
        if (this->length_timer == 0)
            this->is_enabled = false;
    }
}

void Channel4::TickEnvelope()
{
    const uint8_t pace = *nr42 & CH_NRx2_ENV_PACE_MASK;
    if (pace == 0)
        return;

    this->envelope_timer++;
    if (envelope_timer >= pace)
    {
        this->envelope_timer = 0;

        bool increase_volume = *nr42 & CH_NRx2_ENVELOPE_DIR_MASK;
        if (increase_volume && this->volume < 0xF)
        {
            this->volume++;
        }
        else if (!increase_volume && this->volume > 0)
        {
            this->volume--;
        }
        else
        {
            this->is_envelope_alive = false;
        }
    }
}

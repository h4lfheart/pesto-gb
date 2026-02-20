#include "channel_4.h"

#include <cmath>

void Channel4::Tick()
{
    const uint8_t nr42 = this->memory->ReadIO(CH4_NR42_ADDR);
    const uint8_t nr43 = this->memory->ReadIO(CH4_NR43_ADDR);
    const uint8_t nr44 = this->memory->ReadIO(CH4_NR44_ADDR);

    this->is_dac_enabled = (nr42 & CH_NRx2_DAC_MASK) != 0;

    if (nr44 & CH_NRx4_TRIGGER_MASK)
    {
        memory->WriteIO(CH4_NR44_ADDR, nr44 & ~CH_NRx4_TRIGGER_MASK);

        if (this->is_dac_enabled)
        {
            this->Enable();
        }
    }

    if (!(this->is_enabled && this->is_dac_enabled))
    {
        this->output = 0;
        return;
    }

    this->period_timer--;
    if (this->period_timer == 0)
    {
        const uint8_t shift = (nr43 & CH4_NR43_CLOCK_SHIFT_MASK) >> 4;
        const uint8_t divider = nr43 & CH4_NR43_CLOCK_DIVIDER_MASK;
        this->period_timer = (divider == 0 ? 8 : divider << 4) << shift;

        const uint8_t xor_value = ((this->lfsr & 1) ^ ((this->lfsr >> 1) & 1));
        this->lfsr = xor_value << 14 | (this->lfsr >> 1);

        if (nr43 & CH4_NR43_LFSR_WIDTH_MASK)
            this->lfsr = (this->lfsr & ~(1 << 6)) | (xor_value << 6);
    }

    this->output = this->lfsr & 1 ? 0 : volume;
}

void Channel4::TickFrame(uint8_t frame_idx)
{
    if (!this->is_enabled)
        return;

    if ((frame_idx & 1) == 0)
        this->TickLength();

    if (frame_idx == 0b111)
        this->TickEnvelope();
}

void Channel4::Enable()
{
    const uint8_t nr41 = this->memory->ReadIO(CH4_NR41_ADDR);
    const uint8_t nr42 = this->memory->ReadIO(CH4_NR42_ADDR);
    const uint8_t nr43 = this->memory->ReadIO(CH4_NR43_ADDR);
    const uint8_t nr44 = this->memory->ReadIO(CH4_NR44_ADDR);

    this->is_enabled = true;

    this->volume = (nr42 & CH_NRx2_VOLUME_MASK) >> 4;

    if (this->length_timer == 0)
        this->length_timer = CH_6BIT_LENGTH_MAX - (nr41 & CH_NRx1_LENGTH_MASK);

    this->envelope_timer = 0;

    this->lfsr = CH4_LFSR_DEFAULT;

    const uint8_t shift = (nr43 & CH4_NR43_CLOCK_SHIFT_MASK) >> 4;
    const uint8_t divider = nr43 & CH4_NR43_CLOCK_DIVIDER_MASK;
    this->period_timer = (divider == 0 ? 8 : divider << 4) << shift;
}

void Channel4::TickLength()
{
    const uint8_t nr44 = this->memory->ReadIO(CH4_NR44_ADDR);
    if ((nr44 & CH_NRx4_LENGTH_ENABLE_MASK) == 0)
        return;

    this->length_timer--;
    if (this->length_timer == 0)
        this->is_enabled = false;
}

void Channel4::TickEnvelope()
{
    const uint8_t nr42 = this->memory->ReadIO(CH4_NR42_ADDR);
    const uint8_t pace = nr42 & CH_NRx2_ENV_PACE_MASK;
    if (pace == 0)
        return;

    this->envelope_timer++;
    if (envelope_timer >= pace)
    {
        this->envelope_timer = 0;

        bool increase_volume = nr42 & CH_NRx2_ENVELOPE_DIR_MASK;
        if (increase_volume && this->volume < 0xF)
        {
            this->volume++;
        }
        else if (!increase_volume && this->volume > 0)
        {
            this->volume--;
        }
    }
}

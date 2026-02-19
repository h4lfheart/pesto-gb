#include "channel_3.h"

void Channel3::Tick()
{
    uint8_t nr30 = this->memory->ReadIO(CH3_NR30_ADDR);
    const uint8_t nr34 = this->memory->ReadIO(CH3_NR34_ADDR);

    this->is_dac_enabled = (nr30 & CH3_NR30_DAC_MASK) != 0;

    if (nr34 & CH_NRx4_TRIGGER_MASK)
    {
        memory->WriteIO(CH3_NR34_ADDR, nr34 & ~CH_NRx4_TRIGGER_MASK);

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
        this->period_timer = (CH_PERIOD_START - this->period) * CH3_PERIOD_MULTIPLIER;
        this->wave_step = (this->wave_step + 1) & 31;
    }

    const uint8_t byte = this->memory->ReadIO(CH3_WAVE_RAM_START + (this->wave_step >> 1));
    const uint8_t sample = (this->wave_step & 1) ? (byte & 0x0F) : (byte >> 4);

    this->output = this->volume == 0 ? 0 : sample >> (this->volume - 1);
}

void Channel3::TickFrame(uint8_t frame_idx)
{
    if (!this->is_enabled)
        return;

    if ((frame_idx & 1) == 0)
        this->TickLength();
}

void Channel3::Enable()
{
    const uint8_t nr31 = this->memory->ReadIO(CH3_NR31_ADDR);
    const uint8_t nr32 = this->memory->ReadIO(CH3_NR32_ADDR);
    const uint8_t nr33 = this->memory->ReadIO(CH3_NR33_ADDR);
    const uint8_t nr34 = this->memory->ReadIO(CH3_NR34_ADDR);

    this->is_enabled = true;

    this->period = ((nr34 & CH_NRx4_PERIOD_HIGH_MASK) << 8) | nr33;
    this->volume = (nr32 & CH3_NR32_VOLUME_MASK) >> 5;

    this->period_timer = (CH_PERIOD_START - this->period) * CH_PERIOD_MULTIPLIER;
    this->length_timer = nr31;

    this->wave_step = 1;
}

void Channel3::TickLength()
{
    const uint8_t nr34 = this->memory->ReadIO(CH3_NR34_ADDR);
    if ((nr34 & CH_NRx4_LENGTH_ENABLE_MASK) == 0)
        return;

    this->length_timer--;
    if (this->length_timer == 0)
        this->is_enabled = false;
}
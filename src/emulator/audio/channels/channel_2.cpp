#include "channel_2.h"

void Channel2::Tick()
{
    if (!(this->is_enabled && this->IsDACEnabled()))
    {
        this->output = 0;
        return;
    }

    this->period_timer--;
    if (this->period_timer == 0)
    {
        this->period = ((*nr24 & CH_NRx4_PERIOD_HIGH_MASK) << 8) | *nr23;
        this->period_timer = (CH_PERIOD_START - this->period) * CH_PERIOD_MULTIPLIER;
        this->duty_step = (this->duty_step + 1) & 7;
    }

    this->output = this->duty_cycles[duty_type][duty_step] ? volume : 0;
}

void Channel2::TickFrame(uint8_t frame_idx)
{
    if ((frame_idx & 1) == 0)
        this->TickLength();
    if (!this->is_enabled)
        return;

    if (frame_idx == 0b111)
        this->TickEnvelope();
}

void Channel2::Reset()
{
    is_enabled = false;
    period = 0;
    period_timer = 0;
    duty_type = 0;
    duty_step = 0;
    volume = 0;
    envelope_timer = 0;
    length_timer = 0;
}

void Channel2::AttachMemory(Memory* mem)
{
    BaseChannel::AttachMemory(mem);

    this->nr21 = mem->PtrIO(CH2_NR21_ADDR);
    this->nr22 = mem->PtrIO(CH2_NR22_ADDR);
    this->nr23 = mem->PtrIO(CH2_NR23_ADDR);
    this->nr24 = mem->PtrIO(CH2_NR24_ADDR);
}

bool Channel2::IsDACEnabled()
{
    return (*nr22 & CH_NRx2_DAC_MASK) != 0;
}

void Channel2::Trigger()
{
    this->is_enabled = IsDACEnabled();

    this->period = ((*nr24 & CH_NRx4_PERIOD_HIGH_MASK) << 8) | *nr23;
    this->volume = (*nr22 & CH_NRx2_VOLUME_MASK) >> 4;
    this->duty_type = (*nr21 & CH_NRx1_DUTY_TYPE_MASK) >> 6;
    this->duty_step = 0;

    this->period_timer = (CH_PERIOD_START - this->period) * CH_PERIOD_MULTIPLIER;

    if (this->length_timer == 0)
        this->length_timer = CH_6BIT_LENGTH_MAX;

    this->envelope_timer = 0;

    this->is_envelope_alive = true;
}

void Channel2::TickLength()
{
    if ((*nr24 & CH_NRx4_LENGTH_ENABLE_MASK) == 0)
        return;

    if (this->length_timer > 0)
    {
        this->length_timer--;

        if (this->length_timer == 0)
            this->is_enabled = false;
    }
}

void Channel2::TickEnvelope()
{;
    const uint8_t pace = *nr22 & CH_NRx2_ENV_PACE_MASK;
    if (pace == 0)
        return;

    this->envelope_timer++;
    if (envelope_timer >= pace)
    {
        this->envelope_timer = 0;

        bool increase_volume = *nr22 & CH_NRx2_ENVELOPE_DIR_MASK;
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

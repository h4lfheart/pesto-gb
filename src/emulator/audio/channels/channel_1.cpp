#include "channel_1.h"

void Channel1::Tick()
{
    if (!(this->is_enabled && this->IsDACEnabled()))
    {
        this->output = 0;
        return;
    }

    this->period_timer--;
    if (this->period_timer == 0)
    {
        this->period = ((*nr14 & CH_NRx4_PERIOD_HIGH_MASK) << 8) | *nr13;
        this->period_timer = (CH_PERIOD_START - this->period) * CH_PERIOD_MULTIPLIER;
        this->duty_step = (this->duty_step + 1) & 7;
    }

    this->output = this->duty_cycles[duty_type][duty_step] ? volume : 0;
}

void Channel1::TickFrame(uint8_t frame_idx)
{
    if ((frame_idx & 1) == 0)
        this->TickLength();

    if (!this->is_enabled)
        return;

    if (frame_idx == 2 || frame_idx == 6)
        this->TickSweep();

    if (frame_idx == 0b111)
        this->TickEnvelope();
}

void Channel1::Reset()
{
    is_enabled = false;
    period = 0;
    period_timer = 0;
    duty_type = 0;
    duty_step = 0;
    volume = 0;
    envelope_timer = 0;
    sweep_enable = false;
    sweep_timer = 0;
    sweep_period = 0;
    length_timer = 0;
}

void Channel1::AttachMemory(Memory* mem)
{
    BaseChannel::AttachMemory(mem);

    this->nr10 = mem->PtrIO(CH1_NR10_ADDR);
    this->nr11 = mem->PtrIO(CH1_NR11_ADDR);
    this->nr12 = mem->PtrIO(CH1_NR12_ADDR);
    this->nr13 = mem->PtrIO(CH1_NR13_ADDR);
    this->nr14 = mem->PtrIO(CH1_NR14_ADDR);
}

bool Channel1::IsDACEnabled()
{
    return (*nr12 & CH_NRx2_DAC_MASK) != 0;
}

void Channel1::Trigger()
{
    this->is_enabled = IsDACEnabled();

    this->period = ((*nr14 & CH_NRx4_PERIOD_HIGH_MASK) << 8) | *nr13;
    this->volume = (*nr12 & CH_NRx2_VOLUME_MASK) >> 4;
    this->duty_type = (*nr11 & CH_NRx1_DUTY_TYPE_MASK) >> 6;
    this->duty_step = 0;

    this->period_timer = (CH_PERIOD_START - this->period) * CH_PERIOD_MULTIPLIER;

    if (this->length_timer == 0)
    {
        this->length_timer = CH_6BIT_LENGTH_MAX;
    }

    this->envelope_timer = 0;

    const uint8_t shift = *nr10 & CH1_NR10_SWEEP_SHIFT_MASK;
    const uint8_t sweep_pace = (*nr10 & CH1_NR10_SWEEP_PACE_MASK) >> 4;
    this->sweep_period = this->period;
    this->sweep_timer = sweep_pace != 0 ? sweep_pace : 8;
    this->sweep_enable = (sweep_pace != 0) || (shift != 0);
    this->sweep_negate_used = false;

    this->is_envelope_alive = true;

    if (shift != 0) {
        CalculateSweepPeriod();
    }
}

void Channel1::TickLength()
{
    if ((*nr14 & CH_NRx4_LENGTH_ENABLE_MASK) == 0)
        return;

    if (this->length_timer > 0)
    {
        this->length_timer--;

        if (this->length_timer == 0)
            this->is_enabled = false;
    }
}

void Channel1::TickEnvelope()
{
    const uint8_t pace = *nr12 & CH_NRx2_ENV_PACE_MASK;
    if (pace == 0)
        return;

    this->envelope_timer++;
    if (this->envelope_timer >= pace)
    {
        this->envelope_timer = 0;

        bool increase_volume = *nr12 & CH_NRx2_ENVELOPE_DIR_MASK;
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

void Channel1::TickSweep()
{
    const uint8_t sweep_pace = (*nr10 & CH1_NR10_SWEEP_PACE_MASK) >> 4;
    const uint8_t shift = *nr10 & CH1_NR10_SWEEP_SHIFT_MASK;

    if (this->sweep_timer > 0)
        this->sweep_timer--;

    if (this->sweep_timer == 0) {
        this->sweep_timer = sweep_pace != 0 ? sweep_pace : 8;

        if (this->sweep_enable && sweep_pace != 0) {
            uint16_t new_period = CalculateSweepPeriod();

            if (new_period <= CH1_MAX_SWEEP_PERIOD && shift != 0) {
                this->sweep_period = new_period;
                this->period = new_period;

                *memory->PtrIO(CH1_NR13_ADDR) = new_period & 0xFF;

                *nr14 = (*nr14 & ~CH_NRx4_PERIOD_HIGH_MASK) | ((new_period >> 8) & CH_NRx4_PERIOD_HIGH_MASK);

                CalculateSweepPeriod();
            }
        }
    }
}

uint16_t Channel1::CalculateSweepPeriod()
{
    const uint8_t shift = *nr10 & CH1_NR10_SWEEP_SHIFT_MASK;
    const bool decrease = *nr10 & CH1_NR10_SWEEP_DIR_MASK;

    if (decrease)
        this->sweep_negate_used = true;

    const uint16_t offset = this->sweep_period >> shift;
    const uint16_t new_period = decrease ? sweep_period - offset : sweep_period + offset;

    if (new_period > CH1_MAX_SWEEP_PERIOD)
        this->is_enabled = false;

    return new_period;
}

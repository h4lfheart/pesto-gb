#include "channel_3.h"

void Channel3::Tick()
{
    this->volume = (*nr32 & CH3_NR32_VOLUME_MASK) >> 5;
    this->period = ((*nr34 & CH_NRx4_PERIOD_HIGH_MASK) << 8) | *nr33;

    if (!(this->is_enabled && this->IsDACEnabled()))
    {
        this->output = 0;
        return;
    }

    this->period_timer--;
    if (this->period_timer == 0)
    {
        this->period_timer = (CH_PERIOD_START - this->period) * CH_PERIOD_MULTIPLIER / 2;
        this->wave_step = (this->wave_step + 1) & 31;
    }

    const uint8_t byte = this->memory->ReadIO(CH3_WAVE_RAM_START + (this->wave_step >> 1));
    const uint8_t sample = (this->wave_step & 1) ? (byte & 0x0F) : (byte >> 4);

    this->output = this->volume == 0 ? 0 : sample >> (this->volume - 1);
}

void Channel3::TickFrame(uint8_t frame_idx)
{
    if ((frame_idx & 1) == 0)
        this->TickLength();
}

void Channel3::Reset()
{
    is_enabled = false;
    period = 0;
    period_timer = 0;
    wave_step = 0;
    volume = 0;
    length_timer = 0;
    dc_offset = 0;
}

bool Channel3::IsDACEnabled()
{
    return (*nr30 & CH3_NR30_DAC_MASK) != 0;
}

void Channel3::AttachMemory(Memory* mem)
{
    BaseChannel::AttachMemory(mem);

    this->nr30 = mem->PtrIO(CH3_NR30_ADDR);
    this->nr31 = mem->PtrIO(CH3_NR31_ADDR);
    this->nr32 = mem->PtrIO(CH3_NR32_ADDR);
    this->nr33 = mem->PtrIO(CH3_NR33_ADDR);
    this->nr34 = mem->PtrIO(CH3_NR34_ADDR);
}

void Channel3::Trigger()
{
    this->is_enabled = IsDACEnabled();

    this->period = ((*nr34 & CH_NRx4_PERIOD_HIGH_MASK) << 8) | *nr33;
    this->volume = (*nr32 & CH3_NR32_VOLUME_MASK) >> 5;

    this->period_timer = (CH_PERIOD_START - this->period) * CH_PERIOD_MULTIPLIER / 2;

    if (this->length_timer == 0)
        this->length_timer = CH_8BIT_LENGTH_MAX;

    this->wave_step = 0;

    float sum = 0.0f;
    for (int i = 0; i < 16; i++)
    {
        const uint8_t byte = this->memory->ReadIO(CH3_WAVE_RAM_START + i);
        sum += static_cast<float>((byte >> 4) & 0xF);
        sum += static_cast<float>(byte & 0xF);
    }
    this->dc_offset = sum / CH3_WAVE_SAMPLE_COUNT;
}

void Channel3::TickLength()
{
    if ((*nr34 & CH_NRx4_LENGTH_ENABLE_MASK) == 0)
        return;

    if (this->length_timer > 0)
    {
        this->length_timer--;

        if (this->length_timer == 0)
            this->is_enabled = false;
    }
}
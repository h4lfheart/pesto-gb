#include "apu.h"

APU::APU()
{
    this->channel1 = new Channel1();
    this->channel2 = new Channel2();
    this->channel3 = new Channel3();
    this->channel4 = new Channel4();
}

void APU::Cycle(uint8_t cycles)
{
    if (!enabled)
    {
        this->sample_left = 0.0f;
        this->sample_right = 0.0f;
        return;
    }

    this->sample_counter += cycles;
    if (this->sample_counter >= APU_CYCLES_PER_SAMPLE)
    {
        this->sample_counter -= APU_CYCLES_PER_SAMPLE;
        
        this->frame_counter += APU_CYCLES_PER_SAMPLE;
        if (this->frame_counter >= APU_FRAME_RATE)
        {
            this->frame_counter -= APU_FRAME_RATE;
            TickFrame();
        }

        this->channel1->Tick(APU_CYCLES_PER_SAMPLE);
        this->channel2->Tick(APU_CYCLES_PER_SAMPLE);
        this->channel3->Tick(APU_CYCLES_PER_SAMPLE);
        this->channel4->Tick(APU_CYCLES_PER_SAMPLE);

        MixSample();
    }
}

void APU::MixSample()
{
    const uint8_t vol = *NR50;
    const uint8_t panning = *NR51;
    const float left_volume = static_cast<float>((vol & APU_NR50_LEFT_VOLUME_MASK) >> 4);
    const float right_volume = static_cast<float>(vol & APU_NR50_RIGHT_VOLUME_MASK);

    const float ch1 = channel1->IsDACEnabled()
                          ? (static_cast<float>(channel1->output) - channel1->volume * 0.5f) * INV_MAX_VOLUME
                          : 0.0f;
    const float ch2 = channel2->IsDACEnabled()
                          ? (static_cast<float>(channel2->output) - channel2->volume * 0.5f) * INV_MAX_VOLUME
                          : 0.0f;
    const float ch3 = channel3->IsDACEnabled()
                          ? (static_cast<float>(channel3->output) - channel3->dc_offset) * INV_MAX_VOLUME
                          : 0.0f;
    const float ch4 = channel4->IsDACEnabled()
                          ? (static_cast<float>(channel4->output) - channel4->volume * 0.5f) * INV_MAX_VOLUME
                          : 0.0f;

    float left = 0.0f, right = 0.0f;
    if (panning & APU_NR51_CH1_LEFT_MASK) left += ch1;
    if (panning & APU_NR51_CH1_RIGHT_MASK) right += ch1;
    if (panning & APU_NR51_CH2_LEFT_MASK) left += ch2;
    if (panning & APU_NR51_CH2_RIGHT_MASK) right += ch2;
    if (panning & APU_NR51_CH3_LEFT_MASK) left += ch3;
    if (panning & APU_NR51_CH3_RIGHT_MASK) right += ch3;
    if (panning & APU_NR51_CH4_LEFT_MASK) left += ch4;
    if (panning & APU_NR51_CH4_RIGHT_MASK) right += ch4;

    left = left * INV_CHANNEL_COUNT * left_volume * INV_VOLUME_DIVISOR;
    right = right * INV_CHANNEL_COUNT * right_volume * INV_VOLUME_DIVISOR;

    this->sample_left = left < -1.0f ? -1.0f : (left > 1.0f ? 1.0f : left);
    this->sample_right = right < -1.0f ? -1.0f : (right > 1.0f ? 1.0f : right);
    this->ready_for_samples = true;
}

void APU::AttachMemory(Memory* mem)
{
    memory = mem;
    this->channel1->AttachMemory(mem);
    this->channel2->AttachMemory(mem);
    this->channel3->AttachMemory(mem);
    this->channel4->AttachMemory(mem);

    mem->RegisterIOHandler(0x10, 0x3F, this, &APU::APURead, &APU::APUWrite);

    NR50 = mem->PtrIO(APU_NR50_ADDR);
    NR51 = mem->PtrIO(APU_NR51_ADDR);
}

void APU::GetSamples(float& left, float& right) const
{
    left = sample_left;
    right = sample_right;
}

void APU::TickFrame()
{
    this->channel1->TickFrame(frame_step);
    this->channel2->TickFrame(frame_step);
    this->channel3->TickFrame(frame_step);
    this->channel4->TickFrame(frame_step);

    this->frame_step = (this->frame_step + 1) & 7;
}

static const uint8_t APU_READ_MASKS[0x30] = {
    0x80, 0x3F, 0x00, 0xFF, 0xBF,
    0xFF, 0x3F, 0x00, 0xFF, 0xBF,
    0x7F, 0xFF, 0x9F, 0xFF, 0xBF,
    0xFF, 0xFF, 0x00, 0x00, 0xBF,
    0x00, 0x00, 0x70,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

uint8_t APU::APURead(uint8_t* io, uint16_t offset)
{
    if (offset == APU_NR52_ADDR)
    {
        uint8_t base = 0x70;
        if (enabled) base |= 0x80;
        if (channel1->IsEnabled()) base |= 0b0001;
        if (channel2->IsEnabled()) base |= 0b0010;
        if (channel3->IsEnabled()) base |= 0b0100;
        if (channel4->IsEnabled()) base |= 0b1000;
        return base;
    }

    return io[offset] | APU_READ_MASKS[offset - APU_REG_START];
}

void APU::APUWrite(uint8_t* io, uint16_t offset, uint8_t value)
{
    if (offset == APU_NR52_ADDR)
    {
        bool enable = (value & APU_NR52_ENABLE_MASK) != 0;
        if (enable && !enabled)
        {
            enabled = true;
            frame_counter = 0;
            frame_step = 0;
        }
        else if (!enable && enabled)
        {
            enabled = false;
            channel1->Reset();
            channel2->Reset();
            channel3->Reset();
            channel4->Reset();
            for (uint8_t i = APU_REG_START; i <= APU_REG_END; i++)
                io[i] = 0x00;
        }
        return;
    }

    if (!enabled)
    {
        switch (offset)
        {
        case CH1_NR11_ADDR:
            channel1->length_timer = CH_6BIT_LENGTH_MAX - (value & CH_NRx1_LENGTH_MASK);
            io[offset] = value & CH_NRx1_LENGTH_MASK;
            break;
        case CH2_NR21_ADDR:
            channel2->length_timer = CH_6BIT_LENGTH_MAX - (value & CH_NRx1_LENGTH_MASK);
            io[offset] = value & CH_NRx1_LENGTH_MASK;
            break;
        case CH3_NR31_ADDR:
            channel3->length_timer = CH_8BIT_LENGTH_MAX - value;
            io[offset] = value;
            break;
        case CH4_NR41_ADDR:
            channel4->length_timer = CH_6BIT_LENGTH_MAX - (value & CH_NRx1_LENGTH_MASK);
            io[offset] = value & CH_NRx1_LENGTH_MASK;
            break;
        }
        return;
    }

    switch (offset)
    {
    case CH1_NR10_ADDR:
        {
            bool old_negate = (*channel1->NR10 & CH1_NR10_SWEEP_DIR_MASK) != 0;
            io[offset] = value;
            bool new_negate = (*channel1->NR10 & CH1_NR10_SWEEP_DIR_MASK) != 0;
            if (old_negate && !new_negate && channel1->sweep_negate_used)
                channel1->is_enabled = false;
            break;
        }
    case CH1_NR11_ADDR:
        channel1->length_timer = CH_6BIT_LENGTH_MAX - (value & CH_NRx1_LENGTH_MASK);
        break;
    case CH2_NR21_ADDR:
        channel2->length_timer = CH_6BIT_LENGTH_MAX - (value & CH_NRx1_LENGTH_MASK);
        break;
    case CH3_NR31_ADDR:
        channel3->length_timer = CH_8BIT_LENGTH_MAX - value;
        break;
    case CH4_NR41_ADDR:
        channel4->length_timer = CH_6BIT_LENGTH_MAX - (value & CH_NRx1_LENGTH_MASK);
        break;

    case CH1_NR12_ADDR:
        {
            uint8_t old_pace = *channel1->NR12 & CH_NRx2_ENV_PACE_MASK;
            bool old_increase = *channel1->NR12 & CH_NRx2_ENVELOPE_DIR_MASK;
            io[offset] = value;
            if ((value & CH_NRx2_DAC_MASK) == 0) channel1->is_enabled = false;
            if (channel1->is_enabled)
            {
                if (old_pace == 0 && channel1->is_envelope_alive) channel1->volume++;
                else if (!old_increase) channel1->volume += 2;
                if (old_increase != static_cast<bool>(value & CH_NRx2_ENVELOPE_DIR_MASK))
                    channel1->volume = 16 - channel1->volume;
                channel1->volume &= 0xF;
            }
            break;
        }
    case CH2_NR22_ADDR:
        {
            uint8_t old_pace = *channel2->NR22 & CH_NRx2_ENV_PACE_MASK;
            bool old_increase = *channel2->NR22 & CH_NRx2_ENVELOPE_DIR_MASK;
            io[offset] = value;
            if ((value & CH_NRx2_DAC_MASK) == 0) channel2->is_enabled = false;
            if (channel2->is_enabled)
            {
                if (old_pace == 0 && channel2->is_envelope_alive) channel2->volume++;
                else if (!old_increase) channel2->volume += 2;
                if (old_increase != static_cast<bool>(value & CH_NRx2_ENVELOPE_DIR_MASK))
                    channel2->volume = 16 - channel2->volume;
                channel2->volume &= 0xF;
            }
            break;
        }
    case CH3_NR30_ADDR:
        if ((value & CH3_NR30_DAC_MASK) == 0) channel3->is_enabled = false;
        break;
    case CH4_NR42_ADDR:
        {
            uint8_t old_pace = *channel4->NR42 & CH_NRx2_ENV_PACE_MASK;
            bool old_increase = *channel4->NR42 & CH_NRx2_ENVELOPE_DIR_MASK;
            io[offset] = value;
            if ((value & CH_NRx2_DAC_MASK) == 0) channel4->is_enabled = false;
            if (channel4->is_enabled)
            {
                if (old_pace == 0 && channel4->is_envelope_alive) channel4->volume++;
                else if (!old_increase) channel4->volume += 2;
                if (old_increase != static_cast<bool>(value & CH_NRx2_ENVELOPE_DIR_MASK))
                    channel4->volume = 16 - channel4->volume;
                channel4->volume &= 0xF;
            }
            break;
        }

    case CH1_NR14_ADDR:
        {
            bool old_len_enable = (*channel1->NR14 & CH_NRx4_LENGTH_ENABLE_MASK) != 0;
            bool new_len_enable = (value & CH_NRx4_LENGTH_ENABLE_MASK) != 0;
            bool next_clocks_length = (frame_step & 1) != 0;
            io[offset] = value;
            if (!old_len_enable && new_len_enable && next_clocks_length)
                channel1->TickLength();
            if (value & CH_NRx4_TRIGGER_MASK)
            {
                bool length_was_zero = channel1->length_timer == 0;
                channel1->Trigger();
                if (new_len_enable && next_clocks_length && length_was_zero)
                    channel1->TickLength();
            }
            value &= ~CH_NRx4_TRIGGER_MASK;
            break;
        }
    case CH2_NR24_ADDR:
        {
            bool old_len_enable = (*channel2->NR24 & CH_NRx4_LENGTH_ENABLE_MASK) != 0;
            bool new_len_enable = (value & CH_NRx4_LENGTH_ENABLE_MASK) != 0;
            bool next_clocks_length = (frame_step & 1) != 0;
            io[offset] = value;
            if (!old_len_enable && new_len_enable && next_clocks_length)
                channel2->TickLength();
            if (value & CH_NRx4_TRIGGER_MASK)
            {
                bool length_was_zero = channel2->length_timer == 0;
                channel2->Trigger();
                if (new_len_enable && next_clocks_length && length_was_zero)
                    channel2->TickLength();
            }
            value &= ~CH_NRx4_TRIGGER_MASK;
            break;
        }
    case CH3_NR34_ADDR:
        {
            bool old_len_enable = (*channel3->NR34 & CH_NRx4_LENGTH_ENABLE_MASK) != 0;
            bool new_len_enable = (value & CH_NRx4_LENGTH_ENABLE_MASK) != 0;
            bool next_clocks_length = (frame_step & 1) != 0;
            io[offset] = value;
            if (!old_len_enable && new_len_enable && next_clocks_length)
                channel3->TickLength();
            if (value & CH_NRx4_TRIGGER_MASK)
            {
                bool length_was_zero = channel3->length_timer == 0;
                channel3->Trigger();
                if (new_len_enable && next_clocks_length && length_was_zero)
                    channel3->TickLength();
            }
            value &= ~CH_NRx4_TRIGGER_MASK;
            break;
        }
    case CH4_NR44_ADDR:
        {
            bool old_len_enable = (*channel4->NR44 & CH_NRx4_LENGTH_ENABLE_MASK) != 0;
            bool new_len_enable = (value & CH_NRx4_LENGTH_ENABLE_MASK) != 0;
            bool next_clocks_length = (frame_step & 1) != 0;
            io[offset] = value;
            if (!old_len_enable && new_len_enable && next_clocks_length)
                channel4->TickLength();
            if (value & CH_NRx4_TRIGGER_MASK)
            {
                bool length_was_zero = channel4->length_timer == 0;
                channel4->Trigger();
                if (new_len_enable && next_clocks_length && length_was_zero)
                    channel4->TickLength();
            }
            value &= ~CH_NRx4_TRIGGER_MASK;
            break;
        }
    }

    io[offset] = value;
}

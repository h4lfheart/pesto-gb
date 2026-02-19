#include "apu.h"

APU::APU()
{
    this->channel1 = new Channel1();
    this->channel2 = new Channel2();
    this->channel3 = new Channel3();
}

void APU::Cycle()
{
    uint8_t control = this->memory->ReadIO(APU_NR52_ADDR);
    if ((control & APU_NR52_ENABLE_MASK) == 0)
    {
        this->sample_left = 0.0f;
        this->sample_right = 0.0f;
        return;
    }

    this->frame_counter++;
    if (this->frame_counter >= APU_FRAME_SEQUENCER_RATE)
    {
        this->frame_counter = 0;
        TickFrame();
    }

    const uint8_t volume = this->memory->ReadIO(APU_NR50_ADDR);
    const uint8_t panning = this->memory->ReadIO(APU_NR51_ADDR);

    const uint8_t left_volume = ((volume & APU_NR50_LEFT_VOLUME_MASK) >> 4);
    const uint8_t right_volume = (volume & APU_NR50_RIGHT_VOLUME_MASK);

    float left = 0.0f;
    float right = 0.0f;

    this->channel1->Tick();
    this->channel2->Tick();
    this->channel3->Tick();

    const float channel1_data = this->channel1->GetOutput() / APU_MAX_VOLUME;
    const float channel2_data = this->channel2->GetOutput() / APU_MAX_VOLUME;
    const float channel3_data = this->channel3->GetOutput() / APU_MAX_VOLUME;

    if (panning & APU_NR51_CH1_LEFT_MASK) left += channel1_data;
    if (panning & APU_NR51_CH1_RIGHT_MASK) right += channel1_data;
    if (panning & APU_NR51_CH2_LEFT_MASK) left += channel2_data;
    if (panning & APU_NR51_CH2_RIGHT_MASK) right += channel2_data;
    if (panning & APU_NR51_CH3_LEFT_MASK) left += channel3_data;
    if (panning & APU_NR51_CH3_RIGHT_MASK) right += channel3_data;

    left *= (left_volume / APU_VOLUME_DIVISOR) * APU_MIX_SCALE;
    right *= (right_volume / APU_VOLUME_DIVISOR) * APU_MIX_SCALE;

    control &= APU_NR52_ENABLE_MASK;
    if (this->channel1->IsEnabled()) control |= APU_NR52_CH1_ENABLE_MASK;
    if (this->channel2->IsEnabled()) control |= APU_NR52_CH2_ENABLE_MASK;
    if (this->channel3->IsEnabled()) control |= APU_NR52_CH3_ENABLE_MASK;
    this->memory->WriteIO(APU_NR52_ADDR, control);

    this->sample_counter++;
    if (this->sample_counter >= APU_CYCLES_PER_SAMPLE)
    {
        this->sample_counter = 0;

        this->sample_left = left;
        this->sample_right = right;
        this->ready_for_samples = true;
    }
}

void APU::AttachMemory(Memory* mem)
{
    memory = mem;
    this->channel1->AttachMemory(mem);
    this->channel2->AttachMemory(mem);
    this->channel3->AttachMemory(mem);
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

    this->frame_step = (this->frame_step + 1) & 7;
}
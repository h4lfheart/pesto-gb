#include "apu.h"

APU::APU()
{
    this->channel1 = new Channel1();
    this->channel2 = new Channel2();
    this->channel3 = new Channel3();
    this->channel4 = new Channel4();
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
    if (this->frame_counter >= APU_FRAME_RATE)
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
    this->channel4->Tick();

    const float channel1_data = this->channel1->GetOutput() / APU_MAX_VOLUME;
    const float channel2_data = this->channel2->GetOutput() / APU_MAX_VOLUME;
    const float channel3_data = this->channel3->GetOutput() / APU_MAX_VOLUME;
    const float channel4_data = this->channel4->GetOutput() / APU_MAX_VOLUME;

    if (panning & APU_NR51_CH1_LEFT_MASK) left += channel1_data;
    if (panning & APU_NR51_CH1_RIGHT_MASK) right += channel1_data;
    if (panning & APU_NR51_CH2_LEFT_MASK) left += channel2_data;
    if (panning & APU_NR51_CH2_RIGHT_MASK) right += channel2_data;
    if (panning & APU_NR51_CH3_LEFT_MASK) left += channel3_data;
    if (panning & APU_NR51_CH3_RIGHT_MASK) right += channel3_data;
    if (panning & APU_NR51_CH4_LEFT_MASK) left += channel4_data;
    if (panning & APU_NR51_CH4_RIGHT_MASK) right += channel4_data;

    left /= APU_CHANNEL_COUNT;
    right /= APU_CHANNEL_COUNT;

    left *= static_cast<float>(left_volume) / APU_VOLUME_DIVISOR;
    right *= static_cast<float>(right_volume) / APU_VOLUME_DIVISOR;

    control &= APU_NR52_ENABLE_MASK;
    if (this->channel1->IsEnabled()) control |= APU_NR52_CH1_ENABLE_MASK;
    if (this->channel2->IsEnabled()) control |= APU_NR52_CH2_ENABLE_MASK;
    if (this->channel3->IsEnabled()) control |= APU_NR52_CH3_ENABLE_MASK;
    if (this->channel4->IsEnabled()) control |= APU_NR52_CH4_ENABLE_MASK;
    this->memory->WriteIO(APU_NR52_ADDR, control);

    this->sample_counter++;
    if (this->sample_counter >= APU_CYCLES_PER_SAMPLE)
    {
        this->sample_counter = 0;

        const bool dac_enabled = this->channel1->IsDACEnabled()
                                || this->channel2->IsDACEnabled()
                                || this->channel3->IsDACEnabled()
                                || this->channel4->IsDACEnabled();

        this->sample_left = HighPass(left,  dac_enabled, this->capacitor_left) * 2;
        this->sample_right = HighPass(right, dac_enabled, this->capacitor_right) * 2;

        this->ready_for_samples = true;
    }
}

void APU::AttachMemory(Memory* mem)
{
    memory = mem;
    this->channel1->AttachMemory(mem);
    this->channel2->AttachMemory(mem);
    this->channel3->AttachMemory(mem);
    this->channel4->AttachMemory(mem);
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

float APU::HighPass(float in, bool dac_enabled, float& capacitor)
{
    float out = 0.0f;
    if (dac_enabled)
    {
        out = in - capacitor;
        capacitor = in - out * APU_HIGH_PASS_FACTOR;
    }
    else
    {
        capacitor = 0.0f;
    }
    return out;
}

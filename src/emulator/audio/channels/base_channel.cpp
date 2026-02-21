#include "base_channel.h"


void BaseChannel::AttachMemory(Memory* mem)
{
    this->memory = mem;
}

void BaseChannel::TickFrame(uint8_t frame_idx)
{
    // no default behavior
}

void BaseChannel::Reset()
{
}

uint8_t BaseChannel::GetOutput()
{
    return this->output;
}

bool BaseChannel::IsEnabled()
{
    return this->is_enabled;
}

bool BaseChannel::IsDACEnabled()
{
    return false;
}

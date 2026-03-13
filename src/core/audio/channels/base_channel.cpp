#include "base_channel.h"


void BaseChannel::AttachMemory(Memory* mem)
{
    this->memory = mem;
}

uint8_t BaseChannel::GetOutput()
{
    return this->output;
}

bool BaseChannel::IsEnabled()
{
    return this->is_enabled;
}

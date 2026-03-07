#include "input.h"

#include <cstdio>
#include <unordered_map>

#include "../cpu/cpu.h"

void Input::AttachMemory(Memory* mem)
{
    this->memory = mem;

    mem->RegisterIOHandler(IO_ADDR_JOYP, IO_ADDR_JOYP, this, &Input::ReadJOYP, &Input::WriteJOYP);

    this->JOYP = mem->PtrIO(IO_ADDR_JOYP);
    *JOYP = 0b00001111;
}

void Input::PressButton(InputButton button)
{
    switch (button)
    {
    case InputButton::BUTTON_A:
        button_state |= JOYP_BIT_A;
        break;
    case InputButton::BUTTON_B:
        button_state |= JOYP_BIT_B;
        break;
    case InputButton::BUTTON_SELECT:
        button_state |= JOYP_BIT_SELECT;
        break;
    case InputButton::BUTTON_START:
        button_state |= JOYP_BIT_START;
        break;
    case InputButton::BUTTON_RIGHT:
        dpad_state |= JOYP_BIT_RIGHT;
        break;
    case InputButton::BUTTON_LEFT:
        dpad_state |= JOYP_BIT_LEFT;
        break;
    case InputButton::BUTTON_UP:
        dpad_state |= JOYP_BIT_UP;
        break;
    case InputButton::BUTTON_DOWN:
        dpad_state |= JOYP_BIT_DOWN;
        break;
    default:
        break;
    }

    Update();
}

void Input::ReleaseButton(InputButton button)
{
    switch (button)
    {
    case InputButton::BUTTON_A:
        button_state &= ~JOYP_BIT_A;
        break;
    case InputButton::BUTTON_B:
        button_state &= ~JOYP_BIT_B;
        break;
    case InputButton::BUTTON_SELECT:
        button_state &= ~JOYP_BIT_SELECT;
        break;
    case InputButton::BUTTON_START:
        button_state &= ~JOYP_BIT_START;
        break;
    case InputButton::BUTTON_RIGHT:
        dpad_state &= ~JOYP_BIT_RIGHT;
        break;
    case InputButton::BUTTON_LEFT:
        dpad_state &= ~JOYP_BIT_LEFT;
        break;
    case InputButton::BUTTON_UP:
        dpad_state &= ~JOYP_BIT_UP;
        break;
    case InputButton::BUTTON_DOWN:
        dpad_state &= ~JOYP_BIT_DOWN;
        break;
    default:
        break;
    }

    Update();
}

uint8_t Input::ReadJOYP(uint8_t* io, uint16_t offset)
{
    return *this->JOYP;
}

void Input::WriteJOYP(uint8_t* io, uint16_t offset, uint8_t value)
{
    *this->JOYP = (*this->JOYP & ~JOYP_SELECTION_MASK) | (value & JOYP_SELECTION_MASK);
    Update();
}


void Input::Update()
{
    uint8_t result = 0b00001111;

    if ((*JOYP & JOYP_SELECT_BUTTONS) == 0)
        result &= ~button_state;

    if ((*JOYP & JOYP_SELECT_DPAD) == 0)
        result &= ~dpad_state;

    result |= *JOYP & JOYP_SELECTION_MASK;
    *JOYP = result;
}

#include "input.h"

#include <cstdio>
#include <unordered_map>

#include "../cpu/cpu.h"

const std::unordered_map<InputButton, uint8_t> Input::button_bits = {
    {InputButton::BUTTON_A,      JOYP_BIT_A},
    {InputButton::BUTTON_B,      JOYP_BIT_B},
    {InputButton::BUTTON_SELECT, JOYP_BIT_SELECT},
    {InputButton::BUTTON_START,  JOYP_BIT_START}
};

const std::unordered_map<InputButton, uint8_t> Input::dpad_bits = {
    {InputButton::BUTTON_RIGHT, JOYP_BIT_RIGHT},
    {InputButton::BUTTON_LEFT,  JOYP_BIT_LEFT},
    {InputButton::BUTTON_UP,    JOYP_BIT_UP},
    {InputButton::BUTTON_DOWN,  JOYP_BIT_DOWN}
};

void Input::AttachMemory(Memory* mem)
{
    this->memory = mem;
    this->memory->WriteIO(IO_ADDR_JOYP, 0b00001111);
}

void Input::PressButton(InputButton button)
{
    this->pressed_buttons.insert(button);
}

void Input::ReleaseButton(InputButton button)
{
    this->pressed_buttons.erase(button);
}

void Input::Cycle()
{
    uint8_t joyp = this->memory->ReadIO(IO_ADDR_JOYP);
    uint8_t result = 0b00001111;  // All buttons released

    const bool select_buttons = (joyp & JOYP_SELECT_BUTTONS) == 0;
    const bool select_dpad = (joyp & JOYP_SELECT_DPAD) == 0;

    if (select_buttons)
    {
        for (const auto& button : pressed_buttons)
        {
            if (auto it = button_bits.find(button); it != button_bits.end())
                result &= ~it->second;
        }
    }

    if (select_dpad)
    {
        for (const auto& button : pressed_buttons)
        {
            if (auto it = dpad_bits.find(button); it != dpad_bits.end())
                result &= ~it->second;
        }
    }

    result |= joyp & JOYP_SELECTION_MASK;

    this->memory->WriteIO(IO_ADDR_JOYP, result);
}
#pragma once
#include <unordered_map>

#include "../memory/memory.h"
#include <unordered_set>

#define IO_ADDR_JOYP 0x00

#define JOYP_SELECT_BUTTONS 0b00100000
#define JOYP_SELECT_DPAD    0b00010000
#define JOYP_SELECTION_MASK 0b00110000

#define JOYP_BIT_P10 0b00000001
#define JOYP_BIT_P11 0b00000010
#define JOYP_BIT_P12 0b00000100
#define JOYP_BIT_P13 0b00001000


#define JOYP_BIT_RIGHT JOYP_BIT_P10
#define JOYP_BIT_LEFT JOYP_BIT_P11
#define JOYP_BIT_UP JOYP_BIT_P12
#define JOYP_BIT_DOWN JOYP_BIT_P13

#define JOYP_BIT_A JOYP_BIT_P10
#define JOYP_BIT_B JOYP_BIT_P11
#define JOYP_BIT_SELECT JOYP_BIT_P12
#define JOYP_BIT_START JOYP_BIT_P13

enum class InputButton
{
    BUTTON_NONE,
    BUTTON_A,
    BUTTON_B,
    BUTTON_START,
    BUTTON_SELECT,
    BUTTON_UP,
    BUTTON_DOWN,
    BUTTON_LEFT,
    BUTTON_RIGHT
};

class Input
{
public:
    void AttachMemory(Memory* mem);

    void Cycle();

    void PressButton(InputButton button);
    void ReleaseButton(InputButton button);

private:
    Memory* memory = nullptr;
    std::unordered_set<InputButton> pressed_buttons;

    static const std::unordered_map<InputButton, uint8_t> button_bits;
    static const std::unordered_map<InputButton, uint8_t> dpad_bits;
};
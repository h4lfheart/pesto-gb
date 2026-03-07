#pragma once
#include <cstdint>
#include "cartridge.h"

#define GB_DMG_BOOT_ROM_SIZE 0x100
#define GB_CGB_BOOT_ROM_SIZE 0x900
#define GB_WRAM_SIZE 0x1000
#define GB_VRAM_SIZE 0x2000
#define GB_HRAM_SIZE 0x7F
#define GB_OAM_SIZE 0xA0
#define GB_IO_SIZE 0x80

#define GB_WRAM_BANK_MAX 8

#define ADDR_ROM_END 0x8000

#define ADDR_VRAM_BEGIN 0x8000
#define ADDR_VRAM_END 0xA000

#define ADDR_CARTRIDGE_RAM_BEGIN 0xA000
#define ADDR_EXT_RAM_END 0xC000

#define ADDR_WRAM0_BEGIN 0xC000
#define ADDR_WRAM0_END 0xD000

#define ADDR_WRAM_BANK_BEGIN 0xD000
#define ADDR_WRAM_BANKED_END 0xE000

#define ADDR_ECHO_BEGIN 0xE000
#define ADDR_ECHO_END 0xFE00

#define ADDR_OAM_BEGIN 0xFE00
#define ADDR_OAM_END 0xFEA0

#define ADDR_INVALID_END 0xFF00

#define ADDR_IO_BEGIN 0xFF00
#define ADDR_IO_END 0xFF80

#define ADDR_HRAM_BEGIN 0xFF80
#define ADDR_HRAM_END 0xFFFF

#define ADDR_BOOT_ROM_END 0x100
#define ADDR_CGB_BOOT_ROM_BEGIN 0x200
#define ADDR_CGB_BOOT_ROM_END 0x900

#define IO_ADDR_OAM_DMA 0x46
#define IO_ADDR_BOOT 0x50
#define IO_ADDR_JOYP 0x00
#define IO_ADDR_VBK 0x4F
#define IO_ADDR_WBK 0x70
#define IO_ADDR_INTERRUPT_FLAG 0x0F

#define VBK_ENABLE_MASK 0b00000001
#define WBK_BANK_MASK 0b00000111

#define OAM_BEGIN 0xFE00
#define OAM_END 0xFE9F

class CPU;
class Memory;

typedef uint8_t (*PageReadFunction)(Memory*, uint16_t);
typedef void (*PageWriteFunction)(Memory*, uint16_t, uint8_t);

typedef uint8_t (*IOReadFunction)(void* ctx, uint8_t* io, uint16_t offset);
typedef void (*IOWriteFunction)(void* ctx, uint8_t* io, uint16_t offset, uint8_t);

struct IOHandler
{
    void* ctx = nullptr;
    IOReadFunction read = nullptr;
    IOWriteFunction write = nullptr;
};

class Memory
{
public:
    Memory();

    void AttachCPU(CPU* cpu);
    void AttachCartridge(Cartridge* cart);

    void LoadBootRom(const char* boot_rom_path);

    template <class T>
    void RegisterIOHandler(uint8_t start, uint8_t end, T* obj,
                           uint8_t (T::*read_func)(uint8_t*, uint16_t),
                           void (T::*write_func)(uint8_t*, uint16_t, uint8_t));

    void SetInterruptFlag(uint8_t flag);

    uint8_t Read8(uint16_t address);
    void Write8(uint16_t address, uint8_t value);

    uint16_t Read16(uint16_t address);
    void Write16(uint16_t address, uint16_t value);

    uint8_t ReadIO(uint8_t offset);
    void WriteIO(uint8_t offset, uint8_t value);
    uint8_t* PtrIO(const uint8_t offset) { return &io[offset]; }

    // ppu access functions
    uint8_t ReadVRAMBank(bool use_extra_bank, uint16_t offset);
    void WriteVRAM(uint16_t offset, uint8_t value);
    uint8_t* VRAMPtr(bool use_extra_bank);
    uint8_t* OAMPtr();

    bool IsCGB() const;

    uint8_t ie = 0;
    Cartridge* cartridge = nullptr;

    uint8_t wram_bank = 1;
    uint8_t wram0[GB_WRAM_SIZE] = {};
    uint8_t wram_banks[GB_WRAM_BANK_MAX][GB_WRAM_SIZE] = {};

    uint8_t oam[GB_OAM_SIZE] = {};
    uint8_t hram[GB_HRAM_SIZE] = {};
    uint8_t io[GB_IO_SIZE] = {};

    bool use_extra_vram = false;
    uint8_t vram1[GB_VRAM_SIZE] = {};
    uint8_t vram2[GB_VRAM_SIZE] = {};

    bool use_boot_rom = false;
    bool uses_cgb_bootrom = false;
    uint8_t boot_rom[GB_CGB_BOOT_ROM_SIZE] = {};

private:
    CPU* cpu = nullptr;

    IOHandler io_lut[GB_IO_SIZE] = {};

    PageReadFunction read_lut[256] = {};
    PageWriteFunction write_lut[256] = {};
};


template <class T>
void Memory::RegisterIOHandler(
    const uint8_t start, const uint8_t end, T* obj,
    uint8_t (T::*read_func)(uint8_t*, uint16_t),
    void (T::*write_func)(uint8_t*, uint16_t, uint8_t))
{
    struct Context
    {
        T* obj;
        uint8_t (T::*read)(uint8_t*, uint16_t);
        void (T::*write)(uint8_t*, uint16_t, uint8_t);
    };

    for (uint8_t i = start; i <= end; i++)
    {
        if (io_lut[i].ctx)
            delete static_cast<Context*>(io_lut[i].ctx);

        auto* context = new Context{ obj, read_func, write_func };

        io_lut[i].ctx = context;

        io_lut[i].read = [](void* ctx, uint8_t* io, uint16_t offset) -> uint8_t
        {
            auto* trampoline = static_cast<Context*>(ctx);
            return (trampoline->obj->*trampoline->read)(io, offset);
        };

        io_lut[i].write = [](void* ctx, uint8_t* io, uint16_t offset, uint8_t value)
        {
            auto* trampoline = static_cast<Context*>(ctx);
            (trampoline->obj->*trampoline->write)(io, offset, value);
        };
    }
}
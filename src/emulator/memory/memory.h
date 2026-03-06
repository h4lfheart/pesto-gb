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

#define ADDR_EXT_RAM_BEGIN 0xA000
#define ADDR_EXT_RAM_END 0xC000

#define ADDR_WRAM0_BEGIN 0xC000
#define ADDR_WRAM0_END 0xD000

#define ADDR_WRAMX_BEGIN 0xD000
#define ADDR_WRAM_BANKED_END 0xE000

#define ADDR_ECHO_BEGIN 0xE000
#define ADDR_ECHO_END 0xFE00

#define ADDR_OAM_BEGIN 0xFE00
#define ADDR_OAM_END 0xFEA0

#define ADDR_UNUSABLE_END 0xFF00

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

typedef uint8_t (*IOReadFunc)(void* ctx, uint8_t* io, uint16_t offset);
typedef void (*IOWriteFunc)(void* ctx, uint8_t* io, uint16_t offset, uint8_t);

struct IOHandler
{
    void* ctx = nullptr;
    IOReadFunc read = nullptr;
    IOWriteFunc write = nullptr;
};

class Memory
{
public:
    Memory();

    void AttachCPU(CPU* cpu);
    void AttachCartridge(Cartridge* cart);

    void LoadBootRom(char* boot_rom_path);

    template <class T>
    void RegisterIOMemoryRegion(uint8_t start, uint8_t end, T* obj,
                                uint8_t (T::*read)(uint8_t*, uint16_t),
                                void (T::*write)(uint8_t*, uint16_t, uint8_t))
    {
        static uint8_t (T::*inst_read)(uint8_t*, uint16_t) = read;
        static void (T::*inst_write)(uint8_t*, uint16_t, uint8_t) = write;

        for (uint8_t i = start; i <= end; i++)
        {
            io_lut[i] =
            {
                .ctx = obj,
                .read = [](void* ctx, uint8_t* io, uint16_t off)
                {
                    return (static_cast<T*>(ctx)->*inst_read)(io, off);
                },
                .write = [](void* ctx, uint8_t* io, uint16_t off, uint8_t val)
                {
                    (static_cast<T*>(ctx)->*inst_write)(io, off, val);
                }
            };
        }
    }

    void SetInterruptFlag(uint8_t flag);

    uint8_t Read8(uint16_t address);
    void Write8(uint16_t address, uint8_t value);

    uint16_t Read16(uint16_t address);
    void Write16(uint16_t address, uint16_t value);

    uint8_t ReadIO(uint16_t offset);
    void WriteIO(uint16_t offset, uint8_t value);
    uint8_t* PtrIO(uint8_t offset) { return &this->io[offset]; }

    uint8_t ReadVRAMBank(bool use_extra_bank, uint16_t offset);
    void WriteVRAM(uint16_t offset, uint8_t value);
    uint8_t* VRAMPtr(bool use_extra_bank);
    uint8_t* OAMPtr();

    bool IsCGB() const;

    uint8_t ie = 0;
    Cartridge* cartridge = nullptr;

private:
    CPU* cpu = nullptr;

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

    IOHandler io_lut[GB_IO_SIZE] = {};
};

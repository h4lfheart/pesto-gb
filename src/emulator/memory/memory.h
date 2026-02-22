#pragma once
#include <cstdint>
#include <vector>

#include "cartridge.h"

#define GB_ROM_SIZE 0x100
#define GB_DMG_BOOT_ROM_SIZE 0x100
#define GB_CGB_BOOT_ROM_SIZE 0x900
#define GB_WRAM_SIZE 0x1000
#define GB_VRAM_SIZE 0x2000
#define GB_HRAM_SIZE 0x7F
#define GB_OAM_SIZE 0xA0
#define GB_IO_SIZE 0x80

#define GB_WRAM_BANK_MAX 8

#define IO_ADDR_OAM_DMA 0x46
#define IO_ADDR_BOOT 0x50
#define IO_ADDR_JOYP 0x00
#define IO_ADDR_VBK 0x4F
#define IO_ADDR_WBK 0x70

#define VBK_ENABLE_MASK 0b00000001
#define WBK_BANK_MASK 0b00000111

#define OAM_BEGIN 0xfe00
#define OAM_END 0xfe9f

#define MAX_MEMORY_MAP_ENTRIES 16

class Memory;
class CPU;

typedef uint8_t(Memory::*ReadFunc)(uint16_t offset);
typedef void(Memory::*WriteFunc)(uint16_t offset, uint8_t value);

struct MemoryMapEntry
{
    uint16_t addr_start = 0;
    uint16_t addr_end = 0;
    ReadFunc read_handler = nullptr;
    WriteFunc write_handler = nullptr;
    bool enabled = false;
};


typedef uint8_t(*IOReadFunc)(void* ctx, uint8_t* io, uint16_t offset);
typedef void(*IOWriteFunc)(void* ctx, uint8_t* io, uint16_t offset, uint8_t);

struct IOHandler {
    void* ctx = nullptr;
    IOReadFunc read = nullptr;
    IOWriteFunc write = nullptr;
};

class Memory
{
public:
    Memory();

    // TODO please make a bus for all this communication!!
    void AttachCPU(CPU* cpu);

    void LoadBootRom(char* boot_rom_path);
    void InitializeMemoryMap();
    void RegisterMemoryRegion(uint16_t start, uint16_t end,
                              ReadFunc read, WriteFunc write);

    template <class T>
    void RegisterIOMemoryRegion(uint8_t start, uint8_t end, T* obj, uint8_t (T::*read)(uint8_t*, uint16_t),
                                void (T::*write)(uint8_t*, uint16_t, uint8_t))
    {
        static uint8_t (T::*inst_read) (uint8_t*, uint16_t) = read;
        static void (T::*inst_write)(uint8_t*, uint16_t, uint8_t) = write;

        for (uint8_t i = start; i <= end; i++)
        {
            io_lut[i] = {
                .ctx  = obj,
                .read = [](void* ctx, uint8_t* io, uint16_t off) {
                    return (static_cast<T*>(ctx)->*inst_read)(io, off);
                },
                .write = [](void* ctx, uint8_t* io, uint16_t off, uint8_t val) {
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

    void AttachCartridge(Cartridge* cart);

    bool IsCGB() const;

    uint8_t ie = 0;

    Cartridge* cartridge = nullptr;

    uint8_t ReadCartridgeRom(uint16_t offset);
    uint8_t ReadCartridgeRam(uint16_t offset);
    uint8_t ReadWRAM0(uint16_t offset);
    uint8_t ReadWRAMBank(uint16_t offset);
    uint8_t ReadVRAM(uint16_t offset);
    uint8_t ReadOAM(uint16_t offset);
    uint8_t ReadInvalid(uint16_t offset);
    uint8_t ReadHRAM(uint16_t offset);
    uint8_t ReadIE(uint16_t offset);

    void WriteCartridgeRom(uint16_t offset, uint8_t value);
    void WriteCartridgeRam(uint16_t offset, uint8_t value);
    void WriteWRAM0(uint16_t offset, uint8_t value);
    void WriteWRAMBank(uint16_t offset, uint8_t value);
    void WriteVRAM(uint16_t offset, uint8_t value);
    void WriteOAM(uint16_t offset, uint8_t value);
    void WriteInvalid(uint16_t offset, uint8_t value);
    void WriteHRAM(uint16_t offset, uint8_t value);
    void WriteIE(uint16_t offset, uint8_t value);

private:
    MemoryMapEntry* MemoryRegion(uint16_t address);

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

    std::vector<MemoryMapEntry> memory_map = {};

    IOHandler io_lut[GB_IO_SIZE] = {};

    CPU* cpu;
};
#include "memory.h"

#include <algorithm>
#include <cstdio>
#include <cstring>

#include "../cpu/cpu.h"
#include "../io/input.h"
#include "../timer/timer.h"

static uint8_t ReadROMInternal(Memory* memory, uint16_t addr)
{
    if (memory->use_boot_rom)
    {
        if (addr < ADDR_BOOT_ROM_END)
            return memory->boot_rom[addr];

        if (memory->uses_cgb_bootrom && addr >= ADDR_CGB_BOOT_ROM_BEGIN && addr < ADDR_CGB_BOOT_ROM_END)
            return memory->boot_rom[addr];
    }

    return memory->cartridge->ReadRom(addr);
}

static uint8_t ReadVRAMInternal(Memory* memory, uint16_t addr)
{
    return memory->use_extra_vram
               ? memory->vram2[addr - ADDR_VRAM_BEGIN]
               : memory->vram1[addr - ADDR_VRAM_BEGIN];
}

static uint8_t ReadCartridgeRamInternal(Memory* memory, uint16_t addr)
{
    return memory->cartridge->ReadRam(addr - ADDR_CARTRIDGE_RAM_BEGIN);
}

static uint8_t ReadWRAM0Internal(Memory* memory, uint16_t addr)
{
    return memory->wram0[addr - ADDR_WRAM0_BEGIN];
}

static uint8_t ReadWRAMBankInternal(Memory* memory, uint16_t addr)
{
    return memory->wram_banks[memory->wram_bank - 1][addr - ADDR_WRAM_BANK_BEGIN];
}

static uint8_t ReadEchoInternal(Memory* memory, uint16_t addr)
{
    return memory->wram0[addr - ADDR_ECHO_BEGIN];
}

static uint8_t ReadOAMInternal(Memory* memory, uint16_t addr)
{
    if (addr >= ADDR_OAM_END) [[unlikely]]
        return 0xFF;

    return memory->oam[addr - ADDR_OAM_BEGIN];
}

static uint8_t ReadIOInternal(Memory* memory, uint16_t addr)
{
    return memory->ReadIO(addr & 0xFF);
}

static void WriteROMInternal(Memory* memory, uint16_t addr, uint8_t value)
{
    memory->cartridge->WriteRom(addr, value);
}

static void WriteVRAMInternal(Memory* memory, uint16_t addr, uint8_t value)
{
    if (memory->use_extra_vram) memory->vram2[addr - ADDR_VRAM_BEGIN] = value;
    else memory->vram1[addr - ADDR_VRAM_BEGIN] = value;
}

static void WriteCartridgeRamInternal(Memory* memory, uint16_t addr, uint8_t value)
{
    memory->cartridge->WriteRam(addr - ADDR_CARTRIDGE_RAM_BEGIN, value);
}

static void WriteWRAM0Internal(Memory* memory, uint16_t addr, uint8_t value)
{
    memory->wram0[addr - ADDR_WRAM0_BEGIN] = value;
}

static void WriteWRAMBankInternal(Memory* memory, uint16_t addr, uint8_t value)
{
    memory->wram_banks[memory->wram_bank - 1][addr - ADDR_WRAM_BANK_BEGIN] = value;
}

static void WriteEchoInternal(Memory* memory, uint16_t addr, uint8_t value)
{
    memory->wram0[addr - ADDR_ECHO_BEGIN] = value;
}

static void WriteOAMInternal(Memory* memory, uint16_t addr, uint8_t value)
{
    if (addr >= ADDR_OAM_END) [[unlikely]]
        return;

    memory->oam[addr - ADDR_OAM_BEGIN] = value;
}

static void WriteIOInternal(Memory* memory, uint16_t addr, uint8_t value)
{
    memory->WriteIO(addr & 0xFF, value);
}

static uint8_t DefaultIORead(void* ctx, uint8_t* io, uint16_t off)
{
    return io[off];
}

static void DefaultIOWrite(void* ctx, uint8_t* io, uint16_t off, uint8_t val)
{
    io[off] = val;
}

Memory::Memory()
{
    for (int i = 0; i < GB_IO_SIZE; i++)
        io_lut[i] = {nullptr, DefaultIORead, DefaultIOWrite};
}

void Memory::AttachCPU(CPU* cpu) { this->cpu = cpu; }

struct PageRegion
{
    uint8_t start;
    uint8_t end;
    PageReadFunction read;
    PageWriteFunction write;
};

static const PageRegion page_regions[] =
{
    {0x00, 0x7F, ReadROMInternal, WriteROMInternal},
    {0x80, 0x9F, ReadVRAMInternal, WriteVRAMInternal},
    {0xA0, 0xBF, ReadCartridgeRamInternal, WriteCartridgeRamInternal},
    {0xC0, 0xCF, ReadWRAM0Internal, WriteWRAM0Internal},
    {0xD0, 0xDF, ReadWRAMBankInternal, WriteWRAMBankInternal},
    {0xE0, 0xFD, ReadEchoInternal, WriteEchoInternal},
    {0xFE, 0xFE, ReadOAMInternal, WriteOAMInternal},
    {0xFF, 0xFF, ReadIOInternal, WriteIOInternal},
};

void Memory::AttachCartridge(Cartridge* cart)
{
    this->cartridge = cart;

    for (const auto& [start, end, read, write] : page_regions)
        for (int page = start; page <= end; page++)
        {
            read_lut[page] = read;
            write_lut[page] = write;
        }
}

void Memory::LoadBootRom(const char* path)
{
    FILE* file = fopen(path, "rb");
    size_t bytes_read = fread(boot_rom, 1, GB_CGB_BOOT_ROM_SIZE, file);
    fclose(file);
    use_boot_rom = true;
    uses_cgb_bootrom = bytes_read > GB_DMG_BOOT_ROM_SIZE;
}

void Memory::SetInterruptFlag(uint8_t flag)
{
    io[IO_ADDR_INTERRUPT_FLAG] |= flag;
}

uint8_t Memory::Read8(uint16_t addr)
{
    if (addr == 0xFFFF) [[unlikely]]
        return ie;

    return read_lut[addr >> 8](this, addr);
}

void Memory::Write8(uint16_t addr, uint8_t value)
{
    if (addr == 0xFFFF) [[unlikely]]
    {
        ie = value;
        return;
    }

    write_lut[addr >> 8](this, addr, value);
}

uint16_t Memory::Read16(uint16_t addr)
{
    return static_cast<uint16_t>(Read8(addr)) | (static_cast<uint16_t>(Read8(addr + 1)) << 8);
}

void Memory::Write16(uint16_t addr, uint16_t value)
{
    Write8(addr, static_cast<uint8_t>(value));
    Write8(addr + 1, static_cast<uint8_t>(value >> 8));
}

uint8_t Memory::ReadIO(uint8_t offset)
{
    if (offset >= 0x80)
        return hram[offset - 0x80];

    return io_lut[offset].read(io_lut[offset].ctx, io, offset);
}

void Memory::WriteIO(uint8_t offset, uint8_t value)
{
    if (offset >= 0x80)
    {
        hram[offset - 0x80] = value;
        return;
    }

    switch (offset)
    {
    case IO_ADDR_VBK:
        use_extra_vram = value & VBK_ENABLE_MASK;
        break;
    case IO_ADDR_WBK:
        wram_bank = std::max(1, value & WBK_BANK_MASK);
        break;
    case IO_ADDR_BOOT:
        if (value != 0) use_boot_rom = false;
        break;
    case IO_ADDR_OAM_DMA:
        {
            const uint16_t src = static_cast<uint16_t>(value) << 8;
            for (int i = 0; i < GB_OAM_SIZE; i++)
                oam[i] = Read8(src + i);
            break;
        }
    }

    io_lut[offset].write(io_lut[offset].ctx, io, offset, value);
}

uint8_t Memory::ReadVRAMBank(bool use_extra_bank, uint16_t offset)
{
    return use_extra_bank ? vram2[offset] : vram1[offset];
}

void Memory::WriteVRAM(uint16_t offset, uint8_t value)
{
    if (use_extra_vram)
        vram2[offset] = value;
    else
        vram1[offset] = value;
}

uint8_t* Memory::VRAMPtr(bool use_extra_bank) { return use_extra_bank ? vram2 : vram1; }
uint8_t* Memory::OAMPtr() { return oam; }

bool Memory::IsCGB() const
{
    return cartridge->HasCGBSupport() && uses_cgb_bootrom;
}

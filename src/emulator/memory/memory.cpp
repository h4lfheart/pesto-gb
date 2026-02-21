#include "memory.h"
#include <cstdio>
#include <process.h>

#include "../cpu/cpu.h"
#include "../io/input.h"
#include "../timer/timer.h"

Memory::Memory()
{
    InitializeMemoryMap();
}

void Memory::InitializeMemoryMap()
{
    RegisterMemoryRegion(0x0000, 0x7FFF, &Memory::ReadCartridgeRom, &Memory::WriteCartridgeRom);
    RegisterMemoryRegion(0x8000, 0x9FFF, &Memory::ReadVRAM, &Memory::WriteVRAM);
    RegisterMemoryRegion(0xA000, 0xBFFF, &Memory::ReadCartridgeRam, &Memory::WriteCartridgeRam);
    RegisterMemoryRegion(0xC000, 0xCFFF, &Memory::ReadWRAM1, &Memory::WriteWRAM1);
    RegisterMemoryRegion(0xD000, 0xDFFF, &Memory::ReadWRAM2, &Memory::WriteWRAM2);
    RegisterMemoryRegion(0xE000, 0xFDFF, &Memory::ReadWRAM1, &Memory::WriteWRAM1); // echo ram
    RegisterMemoryRegion(0xFE00, 0xFE9F, &Memory::ReadOAM, &Memory::WriteOAM);
    RegisterMemoryRegion(0xFEA0, 0xFEFF, &Memory::ReadInvalid, &Memory::WriteInvalid);
    RegisterMemoryRegion(0xFF00, 0xFF7F, &Memory::ReadIO, &Memory::WriteIO);
    RegisterMemoryRegion(0xFF80, 0xFFFE, &Memory::ReadHRAM, &Memory::WriteHRAM);
    RegisterMemoryRegion(0xFFFF, 0xFFFF, &Memory::ReadIE, &Memory::WriteIE);
}

void Memory::RegisterMemoryRegion(uint16_t start, uint16_t end,
                                   ReadFunc read, WriteFunc write)
{
    memory_map.push_back({
        .addr_start = start,
        .addr_end = end,
        .read_handler = read,
        .write_handler = write,
        .enabled = true
    });
}

void Memory::SetInterruptFlag(uint8_t flag)
{
    const uint8_t interrupt_flag = this->ReadIO(IO_ADDR_INTERRUPT_FLAG);
    this->WriteIO(IO_ADDR_INTERRUPT_FLAG, interrupt_flag | flag);
}

MemoryMapEntry* Memory::MemoryRegion(uint16_t address)
{
    for (auto& entry : memory_map)
    {
        if (entry.enabled &&
            address >= entry.addr_start &&
            address <= entry.addr_end)
        {
            return &entry;
        }
    }
    return nullptr;
}

void Memory::LoadBootRom(char* boot_rom_path)
{
    FILE *rom = fopen(boot_rom_path, "rb");
    fread(this->boot_rom, 1, GB_BOOT_ROM_SIZE, rom);
    fclose(rom);

    this->use_boot_rom = true;
}

uint8_t Memory::Read8(uint16_t address)
{
    const MemoryMapEntry* entry = MemoryRegion(address);
    if (entry != nullptr && entry->read_handler != nullptr)
    {
        uint16_t offset = address - entry->addr_start;
        return (this->*entry->read_handler)(offset);
    }

    fprintf(stderr, "Invalid read at address 0x%04X - no handler found\n", address);
    exit(1);
    return 0;
}

uint16_t Memory::Read16(uint16_t address)
{
    return Read8(address) | (Read8(address + 1) << 8);
}

void Memory::Write8(uint16_t address, uint8_t value)
{
    const MemoryMapEntry* entry = MemoryRegion(address);
    if (entry != nullptr && entry->write_handler != nullptr)
    {
        uint16_t offset = address - entry->addr_start;
        (this->*(entry->write_handler))(offset, value);
        return;
    }

    fprintf(stderr, "Invalid write at address 0x%04X - no handler found\n", address);
    exit(1);
}

void Memory::Write16(uint16_t address, uint16_t value)
{
    Write8(address, value & 0xFF);
    Write8(address + 1, value >> 8);
}

void Memory::AttachCartridge(Cartridge* cart)
{
    this->cartridge = cart;
}


uint8_t Memory::ReadCartridgeRom(uint16_t offset)
{
    if (this->use_boot_rom && offset < 0x100)
    {
        return this->boot_rom[offset];
    }

    return this->cartridge->ReadRom(offset);
}

void Memory::WriteCartridgeRom(uint16_t offset, uint8_t value)
{
    this->cartridge->WriteRom(offset, value);
}

uint8_t Memory::ReadCartridgeRam(uint16_t offset)
{
    return this->cartridge->ReadRam(offset);
}

void Memory::WriteCartridgeRam(uint16_t offset, uint8_t value)
{
    this->cartridge->WriteRam(offset, value);
}

uint8_t Memory::ReadWRAM1(uint16_t offset)
{
    return this->wram1[offset];
}

void Memory::WriteWRAM1(uint16_t offset, uint8_t value)
{
    this->wram1[offset] = value;
}

uint8_t Memory::ReadWRAM2(uint16_t offset)
{
    return this->wram2[offset];
}

void Memory::WriteWRAM2(uint16_t offset, uint8_t value)
{
    this->wram2[offset] = value;
}

uint8_t Memory::ReadVRAM(uint16_t offset)
{
    return this->vram[offset];
}

void Memory::WriteVRAM(uint16_t offset, uint8_t value)
{
    this->vram[offset] = value;
}

uint8_t Memory::ReadOAM(uint16_t offset)
{
    return this->oam[offset];
}

void Memory::WriteOAM(uint16_t offset, uint8_t value)
{
    this->oam[offset] = value;
}

uint8_t Memory::ReadInvalid(uint16_t offset)
{
    return 0xFF;
}

void Memory::WriteInvalid(uint16_t offset, uint8_t value)
{
}


uint8_t Memory::ReadIO(uint16_t offset)
{
    if (io_lut[offset].read)
        return io_lut[offset].read(io_lut[offset].ctx, this->io, offset);

    return this->io[offset];
}

void Memory::WriteIO(uint16_t offset, uint8_t value)
{
    if (io_lut[offset].write)
    {
        io_lut[offset].write(io_lut[offset].ctx, this->io, offset, value);
        return;
    }

    if (offset == IO_ADDR_BOOT && value != 0)
    {
        this->use_boot_rom = false;
    }

    if (offset == IO_ADDR_OAM_DMA)
    {
        // dma transfer to oam
        uint16_t src = value << 8;
        for (uint16_t dst = OAM_BEGIN; dst <= OAM_END; dst++, src++)
            this->oam[dst - OAM_BEGIN] = this->Read8(src);
    }

    this->io[offset] = value;
}

uint8_t Memory::ReadHRAM(uint16_t offset)
{
    return this->hram[offset];
}

void Memory::WriteHRAM(uint16_t offset, uint8_t value)
{
    this->hram[offset] = value;
}

uint8_t Memory::ReadIE(uint16_t offset)
{
    return this->ie;
}

void Memory::WriteIE(uint16_t offset, uint8_t value)
{
    this->ie = value;
}
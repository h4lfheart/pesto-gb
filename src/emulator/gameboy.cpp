#include "gameboy.h"
#include <chrono>
#include <thread>

GameBoy::GameBoy(char* boot_rom_path, char* rom_path)
{
    this->cpu = new Cpu();
    this->memory = new Memory();
    this->cartridge = new Cartridge();
    this->ppu = new PPU();
    this->input = new Input();

    if (rom_path != nullptr)
        this->cartridge->LoadRom(rom_path);

    if (boot_rom_path != nullptr)
        this->memory->LoadBootRom(boot_rom_path);

    this->memory->AttachCartridge(this->cartridge);

    this->cpu->AttachMemory(this->memory);
    this->ppu->AttachMemory(this->memory);
    this->input->AttachMemory(this->memory);

    this->is_running = true;
}

uint64_t get_time()
{
    timespec ts{};
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000000 + ts.tv_nsec;
}

void GameBoy::Run()
{
    uint64_t last_time = get_time();
    uint64_t current_time = 0;

    uint64_t frame_interval = FRAME_INTERVAL;
    //frame_interval = 0;
    while (this->is_running)
    {
        current_time = get_time();

        if (current_time - last_time < frame_interval)
            continue;

        last_time = current_time;

        int frame_cycles = CYCLES_PER_FRAME;
        while (frame_cycles--)
        {
            this->cpu->Cycle();
            this->ppu->Cycle();
            this->input->Cycle();

            if (this->ppu->ready_for_draw)
            {
                this->OnDrawFunction(this->ppu->framebuffer);
                this->ppu->ready_for_draw = false;
            }
        }
    }
}

void GameBoy::Stop()
{
    this->is_running = false;
}

bool GameBoy::IsRunning()
{
    return this->is_running;
}

void GameBoy::PressButton(InputButton button)
{
    this->input->PressButton(button);
}

void GameBoy::ReleaseButton(InputButton button)
{
    this->input->ReleaseButton(button);
}

void GameBoy::OnDraw(const DrawFunction onDraw)
{
    this->OnDrawFunction = onDraw;
}
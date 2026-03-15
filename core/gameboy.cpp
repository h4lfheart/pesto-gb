#include "gameboy.h"
#include <chrono>
#include <thread>
#include <cstdio>

GameBoy::GameBoy(const std::string& rom_path, GameBoySettings settings)
{
    this->cpu = new CPU();
    this->memory = new Memory();
    this->cartridge = new Cartridge();
    this->ppu = new PPU(settings.framebuffer, settings.palette);
    this->input = new Input();
    this->timer = new Timer();
    this->apu = new APU();

    if (!rom_path.empty())
        this->cartridge->LoadRom(rom_path);

    this->memory->AttachCartridge(this->cartridge);
    this->memory->AttachCPU(this->cpu);

    this->cpu->AttachMemory(this->memory);
    this->ppu->AttachMemory(this->memory);
    this->input->AttachMemory(this->memory);
    this->timer->AttachMemory(this->memory);
    this->apu->AttachMemory(this->memory);
}


void GameBoy::TickFrame()
{
    int frame_mcycles = static_cast<int>((CLOCK_RATE / T_CYCLES_PER_M_CYCLE) / FRAMES_PER_SECOND);

    while (frame_mcycles > 0)
    {
        const int mcycles = cpu->Cycle();
        const int tcycles = mcycles * T_CYCLES_PER_M_CYCLE;

        frame_mcycles -= mcycles;

        ppu->Cycle(tcycles);
        apu->Cycle(tcycles);
        timer->Cycle(tcycles);

        if (ppu->ready_for_draw)
        {
            on_draw_function(ppu->framebuffer);
            ppu->ready_for_draw = false;
        }

        if (apu->ready_for_samples)
        {
            float left, right;
            apu->GetSamples(left, right);
            on_audio_function(left, right);
            apu->ready_for_samples = false;
        }
    }
}

bool GameBoy::IsCGBGame()
{
    return this->cartridge->HasCGBSupport();
}

void GameBoy::LoadBootRom(const std::string& boot_rom_path)
{
    this->memory->LoadBootRom(boot_rom_path);
    this->ppu->use_cgb_rendering = this->memory->IsCGB();
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
    this->on_draw_function = onDraw;
}

void GameBoy::OnAudio(const AudioFunction onAudio)
{
    this->on_audio_function = onAudio;
}

void GameBoy::ReadSave(const char* path)
{
    this->cartridge->ReadSave(path);
}

void GameBoy::WriteSave(const char* path)
{
    this->cartridge->WriteSave(path);
}
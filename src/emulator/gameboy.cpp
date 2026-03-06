#include "gameboy.h"
#include <chrono>
#include <thread>
#include <cstdio>

static int64_t now_ns()
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

GameBoy::GameBoy(char* boot_rom_path, char* rom_path)
{
    this->cpu = new CPU();
    this->memory = new Memory();
    this->cartridge = new Cartridge();
    this->ppu = new PPU();
    this->input = new Input();
    this->timer = new Timer();
    this->apu = new APU();

    if (rom_path != nullptr)
        this->cartridge->LoadRom(rom_path);

    if (boot_rom_path != nullptr)
        this->memory->LoadBootRom(boot_rom_path);

    this->memory->AttachCartridge(this->cartridge);
    this->memory->AttachCPU(this->cpu);

    this->cpu->AttachMemory(this->memory);
    this->ppu->AttachMemory(this->memory);
    this->input->AttachMemory(this->memory);
    this->timer->AttachMemory(this->memory);
    this->apu->AttachMemory(this->memory);

    printf("Title: %s\n", this->cartridge->GetTitle());
    printf("Type: %s\n", this->cartridge->GetCartridgeType());

    this->is_running = true;
}

void GameBoy::Run()
{
    const int64_t FRAME_BUDGET_NS = (int64_t)(1'000'000'000.0 / FRAMES_PER_SECOND);
    int64_t frame_start = now_ns();

    while (this->is_running)
    {
        int frame_mcycles = (int)((CLOCK_RATE / T_CYCLES_PER_M_CYCLE) / FRAMES_PER_SECOND);

        while (frame_mcycles > 0)
        {
            const int mcycles = this->cpu->Cycle();
            const int tcycles = mcycles * T_CYCLES_PER_M_CYCLE;

            frame_mcycles -= mcycles;

            this->ppu->Cycle(tcycles);
            this->apu->Cycle(tcycles);
            this->timer->Cycle(tcycles);

            if (this->ppu->ready_for_draw)
            {
                this->OnDrawFunction(this->ppu->framebuffer);
                this->ppu->ready_for_draw = false;
            }

            if (this->apu->ready_for_samples)
            {
                float left, right;
                this->apu->GetSamples(left, right);
                this->OnAudioFunction(left, right);
                this->apu->ready_for_samples = false;
            }
        }

        const int64_t next_frame = frame_start + FRAME_BUDGET_NS;
        const int64_t sleep_ns = next_frame - now_ns();
        if (sleep_ns > 0)
            std::this_thread::sleep_for(std::chrono::nanoseconds(sleep_ns));

        frame_start = next_frame;
    }
}

void GameBoy::Stop() { this->is_running = false; }
bool GameBoy::IsRunning() { return this->is_running; }

void GameBoy::PressButton(InputButton button) { this->input->PressButton(button); }
void GameBoy::ReleaseButton(InputButton button) { this->input->ReleaseButton(button); }
void GameBoy::OnDraw(const DrawFunction onDraw) { this->OnDrawFunction = onDraw; }
void GameBoy::OnAudio(const AudioFunction onAudio) { this->OnAudioFunction = onAudio; }
void GameBoy::ReadSave(const char* path) { this->cartridge->ReadSave(path); }
void GameBoy::WriteSave(const char* path) { this->cartridge->WriteSave(path); }

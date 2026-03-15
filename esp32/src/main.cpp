#include "freertos/FreeRTOS.h"
#include "esp_spiffs.h"
#include "esp_log.h"
#include "esp_pm.h"

#include "core/gameboy.h"
#include "tasks/audio_task.h"
#include "tasks/emulator_task.h"
#include "tasks/input_task.h"
#include "tasks/display_task.h"

#define ROM_PATH "/roms/gold.gbc"
#define DMG_BOOT_PATH "/roms/dmg_boot.bin"
#define CGB_BOOT_PATH "/roms/cgb_boot.bin"

static DisplayTask display;
static AudioTask audio;
static InputTask input;
static EmulatorTask emu;

extern "C" void app_main(void)
{
    esp_vfs_spiffs_conf_t spiffs_conf = {
        .base_path = "/roms",
        .partition_label = nullptr,
        .max_files = 4,
        .format_if_mount_failed = false,
    };
    ESP_ERROR_CHECK(esp_vfs_spiffs_register(&spiffs_conf));

    uint16_t* framebuffer = static_cast<uint16_t*>(heap_caps_malloc(GB_W * GB_H * sizeof(uint16_t), MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL));

    const auto emulator = new GameBoy(ROM_PATH, {
        .framebuffer = framebuffer,
        .palette = {
            GB_COLOR(240, 240, 208),
            GB_COLOR(192, 208, 152),
            GB_COLOR(120, 160, 20),
            GB_COLOR(56, 88, 20),
        }
    });

    if (emulator->IsCGBGame())
    {
        emulator->LoadBootRom(CGB_BOOT_PATH);
    }
    else
    {
        emulator->LoadBootRom(DMG_BOOT_PATH);
    }

    emu.Init(emulator);
    display.Init(framebuffer);
    audio.Init();
    input.Init(emulator);

    emulator->OnDraw([](uint16_t*)
    {
        display.Update();
    });

    emulator->OnAudio([](const float left, const float right)
    {
        audio.PushSample(left, right);
    });

    emu.Start();
    display.Start();
    audio.Start();
    input.Start();
}

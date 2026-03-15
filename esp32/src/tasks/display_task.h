#pragma once
#include <cstdint>

#include "task.h"
#include "freertos/semphr.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "driver/gpio.h"
#include "core/gameboy.h"

#define LCD_BL GPIO_NUM_8
#define LCD_CS GPIO_NUM_4
#define LCD_RST GPIO_NUM_5
#define LCD_DC GPIO_NUM_6
#define LCD_MOSI GPIO_NUM_7
#define LCD_SCK GPIO_NUM_15

#define LCD_W 320
#define LCD_H 240
#define GB_W 160
#define GB_H 144

#define SCALE_NUM 5
#define SCALE_DEN 3
#define SCALED_W ((GB_W * SCALE_NUM) / SCALE_DEN)
#define SCALED_H LCD_H
#define OFF_X ((LCD_W - SCALED_W) / 2)

#define LCD_BITS_PER_PIXEL 16
#define LCD_SPI_HOST SPI2_HOST

class DisplayTask : public Task
{
public:
    void Init(uint16_t* framebuffer);
    void Start() override;
    void Update() const;

protected:
    void Run() override;

private:
    void LCDInit();
    void BuildScalingMaps();
    void PushFramebuffer(const uint16_t* framebuffer);

    static uint16_t Swap16(uint16_t c);
    static uint16_t Rgb555ToRgb565(uint16_t c);

    uint16_t* framebuffer = nullptr;
    SemaphoreHandle_t frame_ready_lock = nullptr;
    esp_lcd_panel_handle_t panel_handle = nullptr;
    esp_lcd_panel_io_handle_t io_handle = nullptr;
    uint16_t* dma_buf = nullptr;

    uint8_t mapped_row[SCALED_H] = {};
    uint8_t mapped_col[SCALED_W] = {};
};
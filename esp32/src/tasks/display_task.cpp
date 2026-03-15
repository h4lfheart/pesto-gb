#include "display_task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_ili9341.h"
#include "esp_heap_caps.h"
#include <cstdio>

#define LCD_SPI_CLK_HZ (80 * 1000 * 1000)
#define LCD_SPI_TRANS_QUEUE_DEPTH 10
#define LCD_SPI_CMD_BITS 8
#define LCD_SPI_PARAM_BITS 8

#define ILI9341_PWCTR1 0xC0
#define ILI9341_PWCTR2 0xC1
#define ILI9341_VMCTR1 0xC5

void DisplayTask::Init(uint16_t* fb)
{
    framebuffer = fb;
    frame_ready_lock = xSemaphoreCreateBinary();
    LCDInit();
}

void DisplayTask::Start()
{
    xTaskCreatePinnedToCore(TaskFunc, "display", 4096, this, 5, nullptr, 0);
}

void DisplayTask::Update() const
{
    xSemaphoreGive(frame_ready_lock);
}

void DisplayTask::Run()
{
    int frames = 0;
    uint32_t last = xTaskGetTickCount();

    while (true)
    {
        xSemaphoreTake(frame_ready_lock, portMAX_DELAY);
        PushFramebuffer(framebuffer);
        frames++;

        if (xTaskGetTickCount() - last >= pdMS_TO_TICKS(1000))
        {
            printf("Draw FPS: %d\n", frames);
            frames = 0;
            last = xTaskGetTickCount();
        }
    }
}

uint16_t DisplayTask::Swap16(uint16_t c)
{
    return static_cast<uint16_t>((c >> 8) | (c << 8));
}

uint16_t DisplayTask::Rgb555ToRgb565(uint16_t c)
{
    const uint16_t r = (c >> 10) & 0x1F;
    const uint16_t g = (c >> 5) & 0x1F;
    const uint16_t b = (c >> 0) & 0x1F;
    return Swap16(static_cast<uint16_t>((b << 11) | (g << 6) | (r << 1)));
}

void DisplayTask::BuildScalingMaps()
{
    for (int y = 0; y < SCALED_H; y++)
        mapped_row[y] = static_cast<uint8_t>((y * GB_H) / SCALED_H);

    for (int x = 0; x < SCALED_W; x++)
        mapped_col[x] = static_cast<uint8_t>((x * GB_W) / SCALED_W);
}

void DisplayTask::PushFramebuffer(const uint16_t* fb)
{
    for (int y = 0; y < SCALED_H; y++)
    {
        const uint16_t* src = fb + mapped_row[y] * GB_W;
        uint16_t* dst = dma_buf + y * SCALED_W;

        for (int dx = 0; dx < SCALED_W; dx++)
            dst[dx] = Rgb555ToRgb565(src[mapped_col[dx]]);
    }

    esp_lcd_panel_draw_bitmap(panel_handle, OFF_X, 0, OFF_X + SCALED_W, SCALED_H, dma_buf);
}

void DisplayTask::LCDInit()
{
    gpio_reset_pin(LCD_BL);
    gpio_set_direction(LCD_BL, GPIO_MODE_OUTPUT);
    gpio_set_level(LCD_BL, 0);

    const spi_bus_config_t bus_cfg = {
        .mosi_io_num = LCD_MOSI,
        .miso_io_num = -1,
        .sclk_io_num = LCD_SCK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_W * LCD_H * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_SPI_HOST, &bus_cfg, SPI_DMA_CH_AUTO));

    const esp_lcd_panel_io_spi_config_t io_cfg = {
        .cs_gpio_num = LCD_CS,
        .dc_gpio_num = LCD_DC,
        .spi_mode = 0,
        .pclk_hz = LCD_SPI_CLK_HZ,
        .trans_queue_depth = LCD_SPI_TRANS_QUEUE_DEPTH,
        .lcd_cmd_bits = LCD_SPI_CMD_BITS,
        .lcd_param_bits = LCD_SPI_PARAM_BITS,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(LCD_SPI_HOST, &io_cfg, &io_handle));

    const esp_lcd_panel_dev_config_t panel_cfg = {
        .reset_gpio_num = LCD_RST,
        .rgb_endian = LCD_RGB_ENDIAN_RGB,
        .bits_per_pixel = LCD_BITS_PER_PIXEL,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_ili9341(io_handle, &panel_cfg, &panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));

    esp_lcd_panel_io_tx_param(io_handle, ILI9341_PWCTR1, (uint8_t[]){0x23}, 1);
    esp_lcd_panel_io_tx_param(io_handle, ILI9341_PWCTR2, (uint8_t[]){0x10}, 1);
    esp_lcd_panel_io_tx_param(io_handle, ILI9341_VMCTR1, (uint8_t[]){0x35, 0x3E}, 2);
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, true, false));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, false));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    auto* clear_buf = static_cast<uint16_t*>(heap_caps_calloc(LCD_W * LCD_H, sizeof(uint16_t), MALLOC_CAP_DMA));
    esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, LCD_W, LCD_H, clear_buf);
    esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, OFF_X, LCD_H, clear_buf);
    esp_lcd_panel_draw_bitmap(panel_handle, OFF_X + SCALED_W, 0, LCD_W, LCD_H, clear_buf);
    free(clear_buf);

    dma_buf = static_cast<uint16_t*>(heap_caps_malloc(SCALED_W * SCALED_H * sizeof(uint16_t), MALLOC_CAP_DMA));

    BuildScalingMaps();
    gpio_set_level(LCD_BL, 1);
}
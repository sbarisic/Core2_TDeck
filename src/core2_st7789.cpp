#include <core2.h>
#include <core2_st7789.h>

#include <driver/gpio.h>
#include <driver/spi_master.h>

#include <images.h>

#define WIDTH 320
#define HEIGHT 240
#define OFFSETX 0
#define OFFSETY 0

#define HOST_ID SPI2_HOST

static const int SPI_Command_Mode = 0;
static const int SPI_Data_Mode = 1;
static const int SPI_Frequency = SPI_MASTER_FREQ_80M;

spi_device_handle_t ST7789_Dev = NULL;
gpio_num_t ST7789_DC;
// spi_transaction_t t;

void core2_st7789_backlight(uint8_t dc)
{
    ledcWrite(0, dc);
}

void st7789_spi_init(gpio_num_t GPIO_SCLK, gpio_num_t GPIO_MOSI, gpio_num_t GPIO_MISO, gpio_num_t GPIO_CS,
                     gpio_num_t GPIO_DC, gpio_num_t GPIO_BL)
{
    ST7789_DC = GPIO_DC;

    gpio_reset_pin(GPIO_CS);
    gpio_set_direction(GPIO_CS, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_CS, 0);

    gpio_reset_pin(GPIO_DC);
    gpio_set_direction(GPIO_DC, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_DC, 0);

    spi_bus_config_t buscfg = {
        .mosi_io_num = GPIO_MOSI,
        .miso_io_num = GPIO_MISO,
        .sclk_io_num = GPIO_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 0,
        .flags = SPICOMMON_BUSFLAG_MASTER,
    };

    esp_err_t ret = spi_bus_initialize(HOST_ID, &buscfg, SPI_DMA_CH_AUTO);
    ESP_LOGD(TAG, "spi_bus_initialize = %d", ret);
    assert(ret == ESP_OK);

    spi_device_interface_config_t devcfg = {0};
    devcfg.clock_speed_hz = SPI_Frequency;
    devcfg.queue_size = 7;
    devcfg.mode = 3;
    devcfg.flags = SPI_DEVICE_NO_DUMMY;
    devcfg.spics_io_num = GPIO_CS;

    ret = spi_bus_add_device(HOST_ID, &devcfg, &ST7789_Dev);
    ESP_LOGD(TAG, "spi_bus_add_device = %d", ret);
    assert(ret == ESP_OK);
}

void st7789_spi_begin()
{
    spi_device_acquire_bus(ST7789_Dev, portMAX_DELAY);
}

void st7789_spi_end()
{
    spi_device_release_bus(ST7789_Dev);
}

bool st7789_spi_write_byte(const uint8_t *Data, size_t DataLength)
{
    spi_transaction_t t = {0};
    t.length = DataLength * 8;
    t.tx_buffer = Data;

    esp_err_t ret = spi_device_transmit(ST7789_Dev, &t);
    assert(ret == ESP_OK);
    return true;

    // TODO: Research this
    // spi_device_queue_trans(ST7789_Dev, &t, portMAX_DELAY);
}

/*
bool st7789_spi_queue_byte(const uint8_t *Data, size_t DataLength)
{
    t = {0};
    t.length = DataLength * 8;
    t.tx_buffer = Data;

    esp_err_t ret = spi_device_queue_trans(ST7789_Dev, &t, portMAX_DELAY);
    assert(ret == ESP_OK);
    return true;
}
*/

bool st7789_write_cmd(uint8_t *cmd, size_t len)
{
    gpio_set_level(ST7789_DC, SPI_Command_Mode);
    return st7789_spi_write_byte(cmd, len);
}

bool st7789_write_cmd_u8(uint8_t d)
{
    return st7789_write_cmd(&d, sizeof(typeof(d)));
}

bool st7789_write_dat(uint8_t *data, size_t len)
{
    gpio_set_level(ST7789_DC, SPI_Data_Mode);
    return st7789_spi_write_byte(data, len);
}

bool st7789_write_dat_u8(uint8_t d)
{
    return st7789_write_dat(&d, sizeof(typeof(d)));
}

bool st7789_write_dat_u16(uint16_t d)
{
    return st7789_write_dat((uint8_t *)&d, sizeof(typeof(d)));
}

bool st7789_write_addr(uint16_t addr1, uint16_t addr2)
{
    static WORD_ALIGNED_ATTR uint8_t Byte[4];

    Byte[0] = (addr1 >> 8) & 0xFF;
    Byte[1] = addr1 & 0xFF;

    Byte[2] = (addr2 >> 8) & 0xFF;
    Byte[3] = addr2 & 0xFF;

    gpio_set_level(ST7789_DC, SPI_Data_Mode);
    return st7789_spi_write_byte(Byte, 4);
}

extern "C" void core2_st7789_draw_fb_pixel(uint16_t color, int x, int y)
{
    const int lines_to_send = 1;

    st7789_spi_begin();
    st7789_write_cmd_u8(ST7789_CASET);
    st7789_write_addr(x, x);
    st7789_write_cmd_u8(ST7789_RASET);
    st7789_write_addr(y, y);
    st7789_write_cmd_u8(ST7789_RAMWR);
    gpio_set_level(ST7789_DC, SPI_Data_Mode);

    st7789_spi_write_byte((uint8_t *)&color, 1 * sizeof(uint16_t));

    // st7789_write_cmd_u8(ST7789_NOP);
    st7789_spi_end();
}

extern "C" void core2_st7789_draw_fb_scanline(uint16_t *colors, int y)
{
    const int lines_to_send = 1;

    st7789_spi_begin();
    st7789_write_cmd_u8(ST7789_CASET);
    st7789_write_addr(0, WIDTH);
    st7789_write_cmd_u8(ST7789_RASET);
    st7789_write_addr(y, y);
    st7789_write_cmd_u8(ST7789_RAMWR);
    gpio_set_level(ST7789_DC, SPI_Data_Mode);

    st7789_spi_write_byte((uint8_t *)colors, lines_to_send * WIDTH * sizeof(uint16_t));

    // st7789_write_cmd_u8(ST7789_NOP);
    st7789_spi_end();
}

void core2_st7789_draw_fb(uint16_t *colors, uint16_t StartX, uint16_t StartY, uint16_t EndX, uint16_t EndY)
{
    const int lines_to_send = 6;
    st7789_spi_begin();

    WORD_ALIGNED_ATTR uint16_t draw_width = 0;
    WORD_ALIGNED_ATTR uint16_t draw_height = 0;

    if (StartX == 0 && EndX == 0 && StartY == 0 && EndY == 0)
    {
        st7789_write_cmd_u8(ST7789_CASET);
        st7789_write_addr(0, WIDTH);
        draw_width = WIDTH;

        st7789_write_cmd_u8(ST7789_RASET);
        st7789_write_addr(0, HEIGHT);
        draw_height = HEIGHT;
    }
    else
    {
        st7789_write_cmd_u8(ST7789_CASET);
        st7789_write_addr(StartX, EndX);
        draw_width = (EndX - StartX) + 1;

        st7789_write_cmd_u8(ST7789_RASET);
        st7789_write_addr(StartY, EndY);
        draw_height = (EndY - StartY) + 1;
    }

    st7789_write_cmd_u8(ST7789_RAMWR);
    gpio_set_level(ST7789_DC, SPI_Data_Mode);

    /*for (size_t y = 0; y < HEIGHT / lines_to_send; y++)
    {
        st7789_spi_write_byte((uint8_t *)(colors + y * WIDTH * lines_to_send),
                              lines_to_send * WIDTH * sizeof(uint16_t));
        // st7789_spi_queue_byte((uint8_t *)(colors + y * WIDTH * lines_to_send), lines_to_send * WIDTH *
        // sizeof(uint16_t));
    }*/

    /*dprintf("StartX: %d, StartY: %d, EndX: %d, EndY: %d, DrawWidth: %d, DrawHeight: %d\n", StartX, StartY, EndX, EndY,
            draw_width, draw_height);*/

    for (size_t y = 0; y < draw_height; y++)
    {
        st7789_spi_write_byte((uint8_t *)(colors + ((y + StartY) * WIDTH + StartX)), draw_width * sizeof(uint16_t));
    }

    // st7789_write_cmd_u8(ST7789_NOP);
    st7789_spi_end();
}

/*
void draw_pixel(uint16_t x, uint16_t y, uint16_t color)
{
    uint16_t _x = x + OFFSETX;
    uint16_t _y = y + OFFSETY;

    st7789_write_cmd_u8(ST7789_CASET); // set column(x) address
    st7789_write_addr(_x, _x);

    st7789_write_cmd_u8(ST7789_RASET); // set Page(y) address
    st7789_write_addr(_y, _y);

    st7789_write_cmd_u8(ST7789_RAMWR); //	Memory Write
    st7789_write_dat_u16(color);
}*/

void core2_st7789_init()
{
    dprintf("core2_st7789_init\n");

    ledcSetup(0, 1000, 8);
    ledcAttachPin(BOARD_TFT_BACKLIGHT, 0);
    core2_st7789_backlight(255);

    st7789_spi_init((gpio_num_t)BOARD_SPI_SCK, (gpio_num_t)BOARD_SPI_MOSI, (gpio_num_t)BOARD_SPI_MISO,
                    (gpio_num_t)BOARD_TFT_CS, (gpio_num_t)BOARD_TFT_DC, (gpio_num_t)BOARD_TFT_BACKLIGHT);

    st7789_write_cmd_u8(ST7789_SWRESET);
    vTaskDelay(pdMS_TO_TICKS(ST7789_RST_DELAY));

    st7789_write_cmd_u8(ST7789_SLPOUT);
    vTaskDelay(pdMS_TO_TICKS(ST7789_SLPOUT_DELAY));

    st7789_write_cmd_u8(ST7789_COLMOD); // Interface pixel format
    st7789_write_dat_u8(0b01010101);

    st7789_write_cmd_u8(ST7789_MADCTL); // Memory Data Access Control
    st7789_write_dat_u8(0b00100000);
    // vTaskDelay(pdMS_TO_TICKS(10));

    st7789_write_cmd_u8(ST7789_CASET); // Column Address Set
    st7789_write_addr(0, WIDTH);

    st7789_write_cmd_u8(ST7789_RASET); // Row Address Set
    st7789_write_addr(0, HEIGHT);

    st7789_write_cmd_u8(ST7789_INVON); // Display Inversion On
    // vTaskDelay(pdMS_TO_TICKS(10));

    st7789_write_cmd_u8(ST7789_NORON); // Normal Display Mode On
    // vTaskDelay(pdMS_TO_TICKS(10));

    st7789_write_cmd_u8(ST7789_DGMEN);
    st7789_write_dat_u8(0x00);

    st7789_write_cmd_u8(ST7789_DISPON); // Display ON
    vTaskDelay(pdMS_TO_TICKS(ST7789_RST_DELAY));
}

void core2_st7789_test()
{
    uint16_t *fb1 = (uint16_t *)malloc(sizeof(uint16_t) * WIDTH * HEIGHT);

    int ctr = 0;
    int ctr2 = 0;
    ulong ms_last = millis();
    ulong ms = 0;
    ulong frame_time = 0;

    for (;;)
    {
        const uint8_t *src = rgb_map;
        if (ctr2 % 2 == 0)
            src = carp_map;

        for (size_t y = 0; y < HEIGHT; y++)
        {
            for (size_t x = 0; x < WIDTH; x++)
            {
                int png_idx = ((HEIGHT - 1) - y) * WIDTH + x;
                int idx = y * WIDTH + x;

                uint8_t r = src[png_idx * 3];
                uint8_t g = src[png_idx * 3 + 1];
                uint8_t b = src[png_idx * 3 + 2];

                fb1[idx] = core2_rgb565(r, g, b);
            }
        }

        core2_st7789_draw_fb(fb1, 0, 0, 0, 0);

        // vTaskDelay(pdMS_TO_TICKS(1));
        ms = millis();
        frame_time = ms - ms_last;
        ms_last = ms;

        if (ctr++ % 50 == 0)
        {
            dprintf("Time: %lu ms; FPS: %.2f\n", frame_time, 1000.0f / frame_time);
            ctr2++;
        }
    }
}
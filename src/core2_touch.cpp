#include <core2.h>
#include <core2_tdeck.h>

#include <Arduino.h>
#include <Wire.h>

volatile int x;
volatile int y;
volatile bool on_triggered;
volatile bool on_click;

volatile int x_accel;
volatile int y_accel;

const int accel_val = 600;

fglVec2i core2_get_cursor()
{
    return fgl_Vec2i(x, y);
}

void bound_check()
{
    if (x < 0)
        x = 0;
    if (x >= WIDTH)
        x = WIDTH - 1;

    if (y < 0)
        y = 0;
    if (y >= HEIGHT)
        y = HEIGHT - 1;
}

void int_g1()
{
    // y++;
    y_accel += accel_val;
    bound_check();
    on_triggered = true;
}

void int_g2()
{
    // x++;
    x_accel += accel_val;
    bound_check();
    on_triggered = true;
}

void int_g3()
{
    // y--;
    y_accel -= accel_val;
    bound_check();
    on_triggered = true;
}

void int_g4()
{
    // x--;
    x_accel -= accel_val;
    bound_check();
    on_triggered = true;
}

void int_click()
{
    on_click = true;
    on_triggered = true;
}

void core2_touch_init()
{
    pinMode(BOARD_TBOX_G01, INPUT_PULLUP);
    pinMode(BOARD_TBOX_G02, INPUT_PULLUP);
    pinMode(BOARD_TBOX_G03, INPUT_PULLUP);
    pinMode(BOARD_TBOX_G04, INPUT_PULLUP);
    pinMode(BOARD_BOOT_PIN, INPUT_PULLUP);

    attachInterrupt(BOARD_TBOX_G01, int_g1, FALLING);
    attachInterrupt(BOARD_TBOX_G02, int_g2, FALLING);
    attachInterrupt(BOARD_TBOX_G03, int_g3, FALLING);
    attachInterrupt(BOARD_TBOX_G04, int_g4, FALLING);
    attachInterrupt(BOARD_BOOT_PIN, int_click, FALLING);
}

static unsigned long last_ms = 0;

void core2_touch_update()
{
    unsigned long ms = millis();

    const float Friction = 0.8f;
    float x_accelf = (x_accel / 100.0f) * Friction;
    float y_accelf = (y_accel / 100.0f) * Friction;

    if (x_accelf > 10)
        x_accelf = 10;
    else if (x_accelf < -10)
        x_accelf = -10;

    if (y_accelf > 10)
        y_accelf = 10;
    else if (y_accelf < -10)
        y_accelf = -10;

    x = x + x_accelf;
    y = y + y_accelf;

    x_accel = (int)(x_accelf * 100);
    y_accel = (int)(y_accelf * 100);

    bound_check();

    if (on_triggered)
    {
        on_triggered = false;

        dprintf("X = %d; Y = %d\n", x, y);

        if (on_click)
        {
            on_click = false;
            dprintf("Click!\n");
        }
    }
}
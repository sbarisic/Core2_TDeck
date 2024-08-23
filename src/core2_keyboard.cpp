#include <core2.h>

#include <Arduino.h>
#include <Wire.h>

#define LILYGO_KB_SLAVE_ADDRESS 0x55

void core2_keyboard_init()
{
    Wire.begin(BOARD_I2C_SDA, BOARD_I2C_SCL);
    Wire.requestFrom(LILYGO_KB_SLAVE_ADDRESS, 1);

    if (Wire.read() == -1)
    {
        while (1)
        {
            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }
}

void core2_keyboard_update()
{
    char keyValue = 0;
    Wire.requestFrom(LILYGO_KB_SLAVE_ADDRESS, 1);
    while (Wire.available() > 0)
    {
        keyValue = Wire.read();
        if (keyValue == '\r')
            keyValue = '\n';

        if (keyValue != (char)0x00)
        {
            // dprintf("keyValue: %c\n", keyValue);
            blit_write(&keyValue, 1);
        }
    }
}

int core2_keyboard_map_char(char c, int *out_x, int *out_y)
{
    int x = 0;
    int y = 0;

    if (c == 0 || c == '\r' || c == '\n')
        return 0;

    if (c >= 'A' && c <= 'M')
    {
        x = (int)(c - 'A');
        y = 7;
    }
    else if (c >= 'N' && c <= 'Z')
    {
        x = (int)(c - 'N');
        y = 6;
    }
    else if (c >= 'a' && c <= 'm')
    {
        x = (int)(c - 'a');
        y = 5;
    }
    else if (c >= 'n' && c <= 'z')
    {
        x = (int)(c - 'n');
        y = 4;
    }
    else if (c >= '0' && c <= '9')
    {
        x = (int)(c - '0');

        if (x == 0)
            x = 9;
        else
            x = x - 1;

        y = 3;
    }
    else
    {
        switch (c)
        {
        case '.':
            y = 2;
            x = 0;
            break;

        case ',':
            y = 2;
            x = 1;
            break;

        case ':':
            y = 2;
            x = 2;
            break;

        case ';':
            y = 2;
            x = 3;
            break;

        case '!':
            y = 2;
            x = 4;
            break;

        case '?':
            y = 2;
            x = 5;
            break;

        case '$':
            y = 2;
            x = 6;
            break;

        case '%':
            y = 2;
            x = 7;
            break;

        case '&':
            y = 2;
            x = 8;
            break;

        case '@':
            y = 2;
            x = 9;
            break;

        case '#':
            y = 2;
            x = 10;
            break;

        case '_':
            y = 2;
            x = 11;
            break;

        case '-':
            y = 2;
            x = 12;
            break;

        case '=':
            y = 1;
            x = 0;
            break;

        case '+':
            y = 1;
            x = 1;
            break;

        case '*':
            y = 1;
            x = 2;
            break;

        case '<':
            y = 1;
            x = 3;
            break;

        case '>':
            y = 1;
            x = 4;
            break;

        case '/':
            y = 1;
            x = 5;
            break;

        case '|':
            y = 1;
            x = 6;
            break;

        case '\\':
            y = 1;
            x = 7;
            break;

        case '(':
            y = 1;
            x = 8;
            break;

        case ')':
            y = 1;
            x = 9;
            break;

        case '[':
            y = 1;
            x = 10;
            break;

        case ']':
            y = 1;
            x = 11;
            break;

        case '{':
            y = 1;
            x = 12;
            break;

        case '}':
            y = 0;
            x = 0;
            break;

        case '^':
            y = 0;
            x = 1;
            break;

        case '~':
            y = 0;
            x = 2;
            break;

        case '\'':
            y = 0;
            x = 4;
            break;

        case '\"':
            y = 0;
            x = 7;
            break;

        case ' ':
            x = 10;
            y = 3;
            break;

        default:
            break;
        }
    }

    *out_x = x;
    *out_y = y;
    return 1;
}
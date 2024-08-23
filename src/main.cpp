#include <core2.h>
#include <core2_tdeck.h>

#include <images.h>
#include <math.h>

// #include <lvgl.h>
// #include <Arduino_GFX_Library.h>

#include <FishGL.h>
#include <FishGLShader.h>

#include <Arduino.h>
#include <Wire.h>

// #define LILYGO_KB_SLAVE_ADDRESS 0x55
// #define BOARD_POWERON 10
// #define BOARD_I2C_SDA 18
// #define BOARD_I2C_SCL 8
#define tskGRAPHICS_PRIORITY 10

static int Width;
static int Height;

static FglBuffer ColorBuffer = {0};
// static FglBuffer DepthBuffer = {0};
static FglBuffer TestTex = {0};

static FglBuffer FontTexBuffer = {0};

// #define TRI_COUNT 2
// FglTriangle3 Tri[TRI_COUNT];
// FglTriangle2 UV[TRI_COUNT];

size_t vert_count = 0;
fglVec3 *verts;
fglVec2 *uvs;

FUNC_HOT bool VertexShader(FglState *State, fglVec3 *Vert)
{
    // mat4 res;
    // glm_mat4_copy(State->MatModel, res);

    fglVec4 v1 = {.X = Vert->X, .Y = Vert->Y, .Z = Vert->Z, .W = 1};
    // glm_mat4_mulv(res, v1, v1);

    fgl_Mul_4x4_4x1(&State->MatModel, v1, &v1);

    *Vert = *(fglVec3 *)&v1;
    // glm_vec4_copy3(v1, Vert);
    return FGL_KEEP;
}

FUNC_HOT bool FragmentShader(FglState *State, fglVec2 UV, FglColor *OutColor)
{
    *OutColor = fglShaderSampleTextureUV(&State->Textures[0], UV);
    return FGL_KEEP;
}

void load_fish_model()
{
    vert_count = sizeof(model_verts_obj) / sizeof(*model_verts_obj) / 3;
    verts = (fglVec3 *)core2_malloc(vert_count * sizeof(fglVec3));
    uvs = (fglVec2 *)core2_malloc(vert_count * sizeof(fglVec2));

    for (size_t i = 0; i < vert_count; i++)
    {
        verts[i].X = model_verts_obj[i * 3 + 0];
        verts[i].Y = model_verts_obj[i * 3 + 1];
        verts[i].Z = model_verts_obj[i * 3 + 2];

        uvs[i].X = model_verts_obj[i * 2 + 0];
        uvs[i].Y = model_verts_obj[i * 2 + 1];
    }
}

void load_rectangles_model()
{
    float X = -1;
    float Y = -1;
    float SX = 2;
    float SY = 2;

    vert_count = 6;
    verts = (fglVec3 *)malloc(vert_count * sizeof(fglVec3));
    uvs = (fglVec2 *)malloc(vert_count * sizeof(fglVec2));

    verts[0] = fgl_Vec3(X, Y, 0);
    verts[1] = fgl_Vec3(X + SX, Y, 0);
    verts[2] = fgl_Vec3(X, Y + SY, 0);
    verts[3] = fgl_Vec3(X + SX, Y, 0);
    verts[4] = fgl_Vec3(X + SX, Y + SY, 0);
    verts[5] = fgl_Vec3(X, Y + SY, 0);

    uvs[0] = fgl_Vec2(0.0f, 0.0f);
    uvs[1] = fgl_Vec2(1.0f, 0.0f);
    uvs[2] = fgl_Vec2(0.0f, 1.0f);
    uvs[3] = fgl_Vec2(1.0f, 0.0f);
    uvs[4] = fgl_Vec2(1.0f, 1.0f);
    uvs[5] = fgl_Vec2(0.0f, 1.0f);
}

FglBuffer load_image(const uint8_t *Memory, int W, int H)
{
    FglBuffer Buffer = fglCreateBuffer(core2_malloc(W * H * sizeof(FglColor)), W, H);

    for (size_t y = 0; y < H; y++)
    {
        for (size_t x = 0; x < W; x++)
        {
            int png_idx = ((H - 1) - y) * W + x;
            // int png_idx = y * W + ((W - 1) - x);
            int idx = y * W + x;

            uint8_t r = Memory[png_idx * 3];
            uint8_t g = Memory[png_idx * 3 + 1];
            uint8_t b = Memory[png_idx * 3 + 2];
            Buffer.Pixels[idx] = fglColor(r, g, b);
        }
    }

    return Buffer;
}

void fgl_init(int W, int H, int BPP)
{
    Width = W;
    Height = H;

    fglInit(NULL, W, H, BPP, 0, PixelOrder_RGB_565);

    ColorBuffer = fglCreateBuffer(core2_malloc(W * H * sizeof(FglColor)), W, H);
    fglClearBuffer(&ColorBuffer, fglColor(255, 0, 0));
    core2_st7789_draw_fb((uint16_t *)ColorBuffer.Pixels, (FglState *)NULL);

    // DepthBuffer = fglCreateDepthBuffer(core2_malloc(W * H), W, H);
    // fglBindDepthBuffer(&DepthBuffer);

    TestTex = load_image(carp_map, W, H);
    // fglBindTexture(&TestTex, 0);

    FontTexBuffer = load_image(font_tex, font_tex_width, font_tex_height);
    fglBindTexture(&FontTexBuffer, 0);

    fglBindShader((void *)&VertexShader, FglShaderType_Vertex);
    fglBindShader((void *)&FragmentShader, FglShaderType_Fragment);

    // load_fish_model();
    // load_rectangles_model();

    /*Tri[0] = (FglTriangle3){{X, Y, 0}, {X + SX, Y, 0}, {X, Y + SY, 0}};
    UV[0] = (FglTriangle2){{0.0f, 0.0f}, {1.0f, 0.0f}, {0.0f, 1.0f}};

    Tri[1] = (FglTriangle3){{X + SX, Y, 0}, {X + SX, Y + SY, 0}, {X, Y + SY, 0}};
    UV[1] = (FglTriangle2){{1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}};*/
}

const int con_w = WIDTH / 12 - 1;
const int con_h = HEIGHT / 12 + 1;
static char console_buffer[con_w * con_h] = {' '};

void blit_text()
{
    int cs = 10; // Character size

    int x_offset = 12;
    int y_offset = 12;

    for (size_t y = 0; y < con_h; y++)
    {
        for (size_t x = 0; x < con_w; x++)
        {
            // int idx = ((con_h - 1) - y) * con_w + x;
            int idx = y * con_w + x;
            char c = console_buffer[idx];

            fglVec2i Pos;
            Pos.X = x * x_offset;
            Pos.Y = y * y_offset;

            int ox;
            int oy;

            if (core2_keyboard_map_char(c, &ox, &oy))
            {
                fglBlitSubTexture(&ColorBuffer, &FontTexBuffer, fgl_BBox(ox * cs, oy * cs, (ox * cs) + cs, (oy * cs) + cs), Pos);
            }

            Pos.X += x_offset;
        }
    }
}

static fglVec2i console_pos;

void blit_write(const char *str, int len)
{
    if (len == 0)
        len = strlen(str);

    for (size_t i = 0; i < len; i++)
    {
        int idx = console_pos.Y * con_w + console_pos.X;

        console_buffer[idx] = str[i];
        console_pos.X++;

        if ((console_pos.X > con_w) || (str[i] == '\n'))
        {
            console_pos.X = 0;
            console_pos.Y--;

            if (console_pos.Y < 0)
            {
                console_pos.Y = 0;

                for (int i = con_h - 1; i >= 1; i--)
                {
                    memcpy(&console_buffer[con_w * i], &console_buffer[con_w * (i - 1)], con_w);
                }

                for (size_t ix = 0; ix < con_w; ix++)
                    console_buffer[ix] = ' ';

                // console_buffer[console_pos.Y * con_w + console_pos.X] = str[i];

                // memset(&console_buffer[con_w * (con_h - 1)], (int)' ', con_w);
            }
        }
    }
}

FUNC_NORETURN void gpu_main(void *args)
{
    core2_st7789_init();

    dprintf("core2_gpu_main BEGIN\n");
    fgl_init(WIDTH, HEIGHT, 16);

    FglState *fgl = fglGetState();

    for (;;)
    {
        fglBeginFrame();
        fglClearBuffer(&ColorBuffer, fglColor(0, 0, 0));

        blit_text();

        fglVec2i cur = core2_get_cursor();
        fglDrawLine(&ColorBuffer, fglColor(255, 0, 0), 0, 0, cur.X, cur.Y);

        core2_st7789_draw_fb((uint16_t *)ColorBuffer.Pixels, NULL);
        fglEndFrame();
    }
}

FUNC_NORETURN void update_main(void *args)
{
    size_t i = 0;
    char buf[64] = {0};

    vTaskDelay(pdMS_TO_TICKS(500));
    core2_touch_init();
    core2_keyboard_init();

    blit_write("Hello World\n", 0);
    blit_write("func(12.3f);\n", 0);
    blit_write("Did i ask questions?\nNo! You, you fage!\n", 0);
    blit_write("array[2] = 2 + 3 * 4;\n", 0);

    for (;;)
    {
        core2_touch_update();
        core2_keyboard_update();
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void core2_main()
{
    pinMode(BOARD_POWERON, OUTPUT);
    digitalWrite(BOARD_POWERON, HIGH);
    vTaskDelay(pdMS_TO_TICKS(100));

    /*char *char_buf = (char *)core2_malloc(4096);
    vTaskList(char_buf);
    dprintf("%s\n", char_buf);*/

    int PRIOR = 10;

    xTaskCreatePinnedToCore(gpu_main, "gpu_main", 1024 * 32, NULL, PRIOR, NULL, 1);
    xTaskCreatePinnedToCore(update_main, "update_main", 1024 * 32, NULL, PRIOR, NULL, 0);

    vTaskDelete(NULL);
}

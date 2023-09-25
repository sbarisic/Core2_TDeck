#include <core2.h>

#include <math.h>
#include <images.h>

// #include <lvgl.h>
// #include <Arduino_GFX_Library.h>

#include <FishGL.h>
#include <FishGLShader.h>

// #define LILYGO_KB_SLAVE_ADDRESS 0x55
// #define BOARD_POWERON 10
// #define BOARD_I2C_SDA 18
// #define BOARD_I2C_SCL 8
#define tskGRAPHICS_PRIORITY 10

#define WIDTH 320
#define HEIGHT 240

int Width;
int Height;

FglBuffer ColorBuffer = {0};
FglBuffer TestTex = {0};

#define TRI_COUNT 2
FglTriangle3 Tri[TRI_COUNT];
FglTriangle2 UV[TRI_COUNT];

bool VertexShader(FglState *State, fglVec3 *Vert)
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

bool FragmentShader(FglState *State, fglVec2 UV, FglColor *OutColor)
{
    FglColor C = fglShaderSampleTextureUV(&State->Textures[0], UV);

    *OutColor = C;
    return FGL_KEEP;
}

void fgl_init(int W, int H, int BPP)
{
    Width = W;
    Height = H;

    fglInit(NULL, W, H, BPP, 0, PixelOrder_RGB_565);

    ColorBuffer = fglCreateBuffer(malloc(W * H * sizeof(FglColor)), W, H);
    fglClearBuffer(&ColorBuffer, fglColor(255, 0, 0));
    core2_st7789_draw_fb((uint16_t *)ColorBuffer.Pixels);

    FglColor *Buff1 = (FglColor *)malloc(sizeof(FglColor) * W * H);
    for (size_t y = 0; y < H; y++)
    {
        for (size_t x = 0; x < W; x++)
        {
            int png_idx = ((H - 1) - y) * W + x;
            int idx = y * W + x;

            uint8_t r = carp_map[png_idx * 3];
            uint8_t g = carp_map[png_idx * 3 + 1];
            uint8_t b = carp_map[png_idx * 3 + 2];
            Buff1[idx] = fglColor(r, g, b);
        }
    }

    TestTex = fglCreateBuffer(Buff1, W, H);
    fglBindTexture(&TestTex, 0);

    fglBindShader((void *)&VertexShader, FglShaderType_Vertex);
    fglBindShader((void *)&FragmentShader, FglShaderType_Fragment);

    float X = -50;
    float Y = -50;
    float SX = 100;
    float SY = 100;

    Tri[0] = (FglTriangle3){{X, Y, 0}, {X + SX, Y, 0}, {X, Y + SY, 0}};
    UV[0] = (FglTriangle2){{0.0f, 0.0f}, {1.0f, 0.0f}, {0.0f, 1.0f}};

    Tri[1] = (FglTriangle3){{X + SX, Y, 0}, {X + SX, Y + SY, 0}, {X, Y + SY, 0}};
    UV[1] = (FglTriangle2){{1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}};
}

void draw_thread(void *args)
{
    FglState *fgl = fglGetState();

    ulong ms = millis();
    ulong ms_now = 0;
    ulong frame_time = 0;
    ulong frame_counter = 0;
    ulong rot_ms = 0;

    float xoffset = 0;

    // vec3 trans = {100, 100, 0};
    // vec3 rot_axis = {0, 0, 1};
    // vec3 scal = {1.5f, 1.5f, 1};

    for (;;)
    {
        fglClearBuffer(&ColorBuffer, fglColor(0, 0, 0));

        fgl->MatModel = fgl_Make_Translate_4x4(fgl_Vec3(100, 100, 0));
        fgl->MatModel = fgl_Rotate(fgl->MatModel, sinf(rot_ms / 1000.0f), fgl_Vec3(0, 0, 1));
        fgl_Transpose_4x4(&fgl->MatModel);

        for (size_t i = 0; i < TRI_COUNT; i++)
            fglRenderTriangle3(&ColorBuffer, &Tri[i], &UV[i]);

        fgl->MatModel = fgl_Make_Translate_4x4(fgl_Vec3(200, 120, 0));
        fgl->MatModel = fgl_Rotate(fgl->MatModel, cosf(rot_ms / 1000.0f) * 0.95f, fgl_Vec3(0, 0, 1));
        fgl->MatModel = fgl_Scale(fgl->MatModel, fgl_Vec3(1.5f, 1.5f, 1.0f));
        fgl_Transpose_4x4(&fgl->MatModel);

        for (size_t i = 0; i < TRI_COUNT; i++)
            fglRenderTriangle3(&ColorBuffer, &Tri[i], &UV[i]);

        ms_now = millis();
        frame_time = ms_now - ms;
        ms = ms_now;
        rot_ms = ms;

        if ((frame_counter++) % 10 == 0)
        {
            dprintf("Frame time: %lu ms - %.2f FPS\n", frame_time, (1.0f / (frame_time / 1000.0f)));
        }

        core2_st7789_draw_fb((uint16_t *)ColorBuffer.Pixels);
        vTaskDelay(pdMS_TO_TICKS(2));
    }
}

void core2_main()
{
    pinMode(BOARD_POWERON, OUTPUT);
    digitalWrite(BOARD_POWERON, HIGH);
    vTaskDelay(pdMS_TO_TICKS(100));

    core2_st7789_init();

    dprintf("core2_gpu_main BEGIN\n");
    fgl_init(WIDTH, HEIGHT, 16);

    xTaskCreatePinnedToCore(draw_thread, "gpu_main", 1024 * 16, NULL, 1, NULL, 1);

    for (;;)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

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

bool VertexShader(FglState *State, vec3 Vert)
{
    mat4 res;
    glm_mat4_copy(State->MatModel, res);

    vec4 v1 = {Vert[XElement], Vert[YElement], Vert[ZElement], 1};
    glm_mat4_mulv(res, v1, v1);

    glm_vec4_copy3(v1, Vert);
    return FGL_KEEP;
}

bool FragmentShader(FglState *State, vec2 UV, FglColor *OutColor)
{
    vec2 newUV = {UV[0], UV[1]};
    // newUV[YElement] = 1 - newUV[YElement];

    FglColor C = fglShaderSampleTextureUV(&State->Textures[0], newUV);

    // vec3 C2;
    // glm_vec_copy(fglShaderGetVarying(1)->Vec3, C2);

    // uint8_t r, g, b;
    // core2_rgb565_deconstr(C, &r, &g, &b);

    // C = core2_rgb565((int)(r * (C2[XElement] / 255)), (int)(g * (C2[YElement] / 255)), (int)(b * (C2[ZElement] / 255)));

    // C.R = (int)(C.R * (C2[XElement] / 255));
    // C.G = (int)(C.G * (C2[YElement] / 255));
    // C.B = (int)(C.B * (C2[ZElement] / 255));

    *OutColor = C;

    return FGL_KEEP;
}

void fgl_init(int W, int H, int BPP)
{
    Width = W;
    Height = H;

    ColorBuffer = fglCreateBuffer(malloc(W * H * sizeof(FglColor)), W, H);

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

    FglState *fgl = fglGetState();
    glm_mat4_identity(fgl->MatModel);
    glm_mat4_identity(fgl->MatView);
    glm_mat4_identity(fgl->MatProj);
}

void draw_thread(void *args)
{
    FglState *fgl = fglGetState();

    ulong ms = millis();
    ulong ms_now = 0;
    ulong frame_time = 0;
    ulong frame_counter = 0;

    float xoffset = 0;

    vec3 trans = {100, 100, 0};
    vec3 rot_axis = {0, 0, 1};
    vec3 scal = {1.5f, 1.5f, 1};

    for (;;)
    {
        fglClearBuffer(&ColorBuffer, fglColor(0, 0, 0));

        trans[XElement] = 100;
        trans[YElement] = 100;
        glm_translate_make(fgl->MatModel, trans);
        glm_rotate(fgl->MatModel, sinf(ms / 1000.0f), rot_axis);

        for (size_t i = 0; i < TRI_COUNT; i++)
            fglRenderTriangle3(&ColorBuffer, &Tri[i], &UV[i]);

        trans[XElement] = 200;
        trans[YElement] = 120;
        glm_translate_make(fgl->MatModel, trans);
        glm_rotate(fgl->MatModel, cosf(ms / 1000.0f) * 0.95f, rot_axis);
        glm_scale(fgl->MatModel, scal);

        for (size_t i = 0; i < TRI_COUNT; i++)
            fglRenderTriangle3(&ColorBuffer, &Tri[i], &UV[i]);

        ms_now = millis();
        frame_time = ms_now - ms;
        ms = ms_now;

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

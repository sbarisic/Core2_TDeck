#include <core2.h>

#include <images.h>
#include <math.h>

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

static int Width;
static int Height;


static FglBuffer ColorBuffer = {0};
static FglBuffer DepthBuffer = {0};
static FglBuffer TestTex = {0};

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
}

void fgl_init(int W, int H, int BPP)
{
    Width = W;
    Height = H;

    fglInit(NULL, W, H, BPP, 0, PixelOrder_RGB_565);

    ColorBuffer = fglCreateBuffer(core2_malloc(W * H * sizeof(FglColor)), W, H);
    fglClearBuffer(&ColorBuffer, fglColor(255, 0, 0));
    core2_st7789_draw_fb((uint16_t *)ColorBuffer.Pixels, (FglState *)NULL);

    DepthBuffer = fglCreateDepthBuffer(core2_malloc(W * H), W, H);
    fglBindDepthBuffer(&DepthBuffer);

    FglColor *Buff1 = (FglColor *)core2_malloc(sizeof(FglColor) * W * H);
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

    float X = -1;
    float Y = -1;
    float SX = 2;
    float SY = 2;

    // load_fish_model();

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

    /*Tri[0] = (FglTriangle3){{X, Y, 0}, {X + SX, Y, 0}, {X, Y + SY, 0}};
    UV[0] = (FglTriangle2){{0.0f, 0.0f}, {1.0f, 0.0f}, {0.0f, 1.0f}};

    Tri[1] = (FglTriangle3){{X + SX, Y, 0}, {X + SX, Y + SY, 0}, {X, Y + SY, 0}};
    UV[1] = (FglTriangle2){{1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}};*/
}

FUNC_NORETURN void gpu_main(void *args)
{
    core2_st7789_init();

    dprintf("core2_gpu_main BEGIN\n");
    fgl_init(WIDTH, HEIGHT, 16);

    FglState *fgl = fglGetState();

    ulong ms = millis();
    ulong ms_now = 0;
    ulong frame_time = 0;
    ulong frame_counter = 0;
    ulong rot_ms = 0;

    float xoffset = 0;

    fglMat4 pos1 = fgl_Make_Translate_4x4(fgl_Vec3(100, 100, 20));
    fglMat4 pos2 = fgl_Make_Translate_4x4(fgl_Vec3(200, 120, 10));
    fglVec3 unitZ = fgl_Vec3(0, 0, 1);

    float Scal1 = 70.0f;
    fglVec3 scaleVec1 = fgl_Vec3(Scal1, Scal1, 1.0f);

    float Scal2 = 100.0f;
    fglVec3 scaleVec2 = fgl_Vec3(Scal2, Scal2, 1.0f);

    for (;;)
    {
        float rot_ang_1 = sinf(rot_ms / 1000.0f);
        float rot_ang_2 = cosf(rot_ms / 1000.0f) * 0.95f;

        fglBeginFrame();
        fglClearBuffer(&ColorBuffer, fglColor(0, 0, 0));
        fglClearDepthBuffer();

        fgl->MatModel = pos1;
        fgl_Rotate(&fgl->MatModel, rot_ang_1, unitZ);
        fgl_Scale(&fgl->MatModel, scaleVec1);
        fgl_Transpose_4x4(&fgl->MatModel);

        fglRenderTriangle3v(&ColorBuffer, verts, uvs, vert_count, &fgl->RenderBounds);

        fgl->MatModel = pos2;
        fgl_Rotate(&fgl->MatModel, rot_ang_2, unitZ);
        fgl_Scale(&fgl->MatModel, scaleVec2);
        fgl_Transpose_4x4(&fgl->MatModel);

        fglRenderTriangle3v(&ColorBuffer, verts, uvs, vert_count, &fgl->RenderBounds);

        core2_st7789_draw_fb((uint16_t *)ColorBuffer.Pixels, NULL);
        fglEndFrame();

        ms_now = millis();
        frame_time = ms_now - ms;
        ms = ms_now;
        rot_ms = ms;

        if ((frame_counter++) % 10 == 0)
        {
            dprintf("Frame time: %lu ms - %.2f FPS\n", frame_time, (1.0f / (frame_time / 1000.0f)));
        }
        // vTaskDelay(pdMS_TO_TICKS(200));
    }
}

FUNC_NORETURN void update_main(void *args)
{
    int64_t last_time = 0;
    float target_fps = 60;
    float target_frametime = 1.0f / target_fps;

    for (;;)
    {
        vTaskDelay(pdMS_TO_TICKS(10));
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

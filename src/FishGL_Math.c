#include <FishGL.h>

#include <cglm/cglm.h>
#include <esp_dsp.h>

#define fgl_fminf(a, b) ((a) < (b) ? (a) : (b))
#define fgl_fmaxf(a, b) ((a) < (b) ? (b) : (a))

fglVec2 fgl_Vec2(float X, float Y)
{
    fglVec2 v;
    v.X = X;
    v.Y = Y;
    return v;
}

fglVec2 fgl_Vec2_Min(fglVec2 A, fglVec2 B)
{
    fglVec2 Min;

    Min.X = fgl_fminf(A.X, B.X);
    Min.Y = fgl_fminf(A.Y, B.Y);

    return Min;
}

fglVec2 fgl_Vec2_Max(fglVec2 A, fglVec2 B)
{
    fglVec2 Max;

    Max.X = fgl_fmaxf(A.X, B.X);
    Max.Y = fgl_fmaxf(A.Y, B.Y);

    return Max;
}

fglVec3 fgl_Vec3(float X, float Y, float Z)
{
    fglVec3 v;
    v.X = X;
    v.Y = Y;
    v.Z = Z;
    return v;
}

fglVec3 fgl_Vec3_from_Vec2(fglVec2 V2, float Z)
{
    fglVec3 v;
    v.X = V2.X;
    v.Y = V2.Y;
    v.Z = Z;
    return v;
}

void fgl_Mul_4x4_4x1(fglMat4 *mat, const fglVec4 vec, fglVec4 *res)
{
    dspm_mult_4x4x1_f32_ae32((const float *)mat, (const float *)&vec, (float *)res);
}

void fgl_Mul_3x3_3x1(fglMat3 *mat, const fglVec3 vec, fglVec3 *res)
{
    dspm_mult_3x3x1_f32_ae32((const float *)mat, (const float *)&vec, (float *)res);
}

void fgl_Sub_3x1_3x1(const fglVec3 a, const fglVec3 b, fglVec3 *res)
{
    dsps_sub_f32_ae32((float *)&a, (float *)&b, (float *)res, 3, 1, 1, 1);
}

void fgl_Transpose_3x3(fglMat3 *mat)
{
    glm_mat3_transpose((vec3 *)mat);
}

void fgl_Transpose_4x4(fglMat4 *mat)
{
    glm_mat4_transpose((vec4 *)mat);
}

void fgl_Identity_3x3(fglMat3 *mat)
{
    glm_mat3_identity((vec3 *)mat);
}

void fgl_Identity_4x4(fglMat4 *mat)
{
    glm_mat4_identity((vec4 *)mat);
}

fglMat4 fgl_Make_Translate_4x4(fglVec3 tran)
{
    fglMat4 transMat;
    glm_translate_make((vec4 *)&transMat, (float *)&tran);
    return transMat;
}

void fgl_Translate(fglMat4 *mat, fglVec3 tran)
{
    glm_translate((vec4 *)mat, (float *)&tran);
}

void fgl_Rotate(fglMat4 *mat, float ang, fglVec3 tran)
{
    glm_rotate((vec4 *)mat, ang, (float *)&tran);
}

void fgl_Scale(fglMat4 *mat, fglVec3 tran)
{
    glm_scale((vec4 *)mat, (float *)&tran);
}

fglVec3 fgl_Cross3(fglVec3 a, fglVec3 b)
{
    fglVec3 res;
    res.X = a.Y * b.Z - a.Z * b.Y;
    res.Y = a.Z * b.X - a.X * b.Z;
    res.Z = a.X * b.Y - a.Y * b.X;
    return res;

    // fglVec3 c = {0};
    // glm_vec_cross((float *)&a, (float *)&b, (float *)&c);
    // return c;
}

FglColor fglColor(uint8_t r, uint8_t g, uint8_t b)
{
    // FglColor clr = {.R = r >> 3, .G = g >> 2, .B = b >> 3};
    FglColor clr;
    clr.R = r >> 3;
    clr.G = g >> 2;
    clr.B = b >> 3;

    clr.u16 = __bswap16(clr.u16);
    return clr;
}

void fglColorToRGB(FglColor clr, uint8_t *r, uint8_t *g, uint8_t *b)
{
    clr.u16 = __bswap16(clr.u16);
    *r = clr.R << 3;
    *g = clr.G << 2;
    *b = clr.B << 3;
}
/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// gl_shaders.cpp: the OpenGL 3.3 core draw backend. One shader pair covers
// the whole renderer: base texture * lightmap * vertex color, optional alpha
// discard and linear black fog. The RB_* calls replace the old immediate-mode
// entry points and accumulate vertices in a single VBO.

#include "gl_local.h"
#include <string.h>

mat4_t r_mat_projection;
mat4_t r_mat_modelview;

void Mat_Identity(mat4_t m)
{
    memset(m, 0, sizeof(mat4_t));
    m[0] = m[5] = m[10] = m[15] = 1;
}

void Mat_Copy(mat4_t dst, const mat4_t src)
{
    memcpy(dst, src, sizeof(mat4_t));
}

void Mat_Multiply(mat4_t out, const mat4_t a, const mat4_t b)
{
    float tmp[16];

    for (int c = 0; c < 4; c++)
    {
        for (int r = 0; r < 4; r++)
        {
            tmp[c * 4 + r] = a[r] * b[c * 4] + a[4 + r] * b[c * 4 + 1] +
                             a[8 + r] * b[c * 4 + 2] + a[12 + r] * b[c * 4 + 3];
        }
    }
    memcpy(out, tmp, sizeof(mat4_t));
}

void Mat_Translate(mat4_t m, float x, float y, float z)
{
    mat4_t t;

    Mat_Identity(t);
    t[12] = x;
    t[13] = y;
    t[14] = z;
    Mat_Multiply(m, m, t);
}

void Mat_Scale(mat4_t m, float x, float y, float z)
{
    mat4_t s;

    Mat_Identity(s);
    s[0] = x;
    s[5] = y;
    s[10] = z;
    Mat_Multiply(m, m, s);
}

void Mat_Rotate(mat4_t m, float degrees, float x, float y, float z)
{
    float length = sqrt(x * x + y * y + z * z);
    mat4_t r;

    if (length == 0)
    {
        return;
    }
    x /= length;
    y /= length;
    z /= length;

    float rad = degrees * M_PI / 180.0f;
    float c = cos(rad);
    float s = sin(rad);
    float t = 1.0f - c;

    Mat_Identity(r);
    r[0] = t * x * x + c;
    r[1] = t * x * y + s * z;
    r[2] = t * x * z - s * y;
    r[4] = t * x * y - s * z;
    r[5] = t * y * y + c;
    r[6] = t * y * z + s * x;
    r[8] = t * x * z + s * y;
    r[9] = t * y * z - s * x;
    r[10] = t * z * z + c;
    Mat_Multiply(m, m, r);
}

void Mat_Ortho(mat4_t m, float left, float right, float bottom, float top, float nearz, float farz)
{
    Mat_Identity(m);
    m[0] = 2 / (right - left);
    m[5] = 2 / (top - bottom);
    m[10] = -2 / (farz - nearz);
    m[12] = -(right + left) / (right - left);
    m[13] = -(top + bottom) / (top - bottom);
    m[14] = -(farz + nearz) / (farz - nearz);
}

void Mat_Frustum(mat4_t m, float left, float right, float bottom, float top, float nearz,
                 float farz)
{
    memset(m, 0, sizeof(mat4_t));
    m[0] = 2 * nearz / (right - left);
    m[5] = 2 * nearz / (top - bottom);
    m[8] = (right + left) / (right - left);
    m[9] = (top + bottom) / (top - bottom);
    m[10] = -(farz + nearz) / (farz - nearz);
    m[11] = -1;
    m[14] = -2 * farz * nearz / (farz - nearz);
}

static mat4_t r_proj_stack[8];
static int r_proj_depth;
static mat4_t r_mv_stack[8];
static int r_mv_depth;

void R_PushProjection(void)
{
    if (r_proj_depth < 8)
    {
        Mat_Copy(r_proj_stack[r_proj_depth++], r_mat_projection);
    }
}

void R_PopProjection(void)
{
    if (r_proj_depth > 0)
    {
        Mat_Copy(r_mat_projection, r_proj_stack[--r_proj_depth]);
    }
}

void R_PushModelview(void)
{
    if (r_mv_depth < 8)
    {
        Mat_Copy(r_mv_stack[r_mv_depth++], r_mat_modelview);
    }
}

void R_PopModelview(void)
{
    if (r_mv_depth > 0)
    {
        Mat_Copy(r_mat_modelview, r_mv_stack[--r_mv_depth]);
    }
}

/*
====================================================================

SHADER PROGRAM

====================================================================
*/

static const char *r_vertex_source = "#version 330 core\n"
                                     "uniform mat4 uProjection;\n"
                                     "uniform mat4 uModelView;\n"
                                     "uniform float uFogStart;\n"
                                     "uniform float uFogEnd;\n"
                                     "in vec3 aPosition;\n"
                                     "in vec2 aTexCoord;\n"
                                     "in vec2 aLightCoord;\n"
                                     "in vec4 aColor;\n"
                                     "out vec4 vColor;\n"
                                     "out vec2 vTexCoord;\n"
                                     "out vec2 vLightCoord;\n"
                                     "out float vFog;\n"
                                     "void main()\n"
                                     "{\n"
                                     "    vec4 eye = uModelView * vec4(aPosition, 1.0);\n"
                                     "    gl_Position = uProjection * eye;\n"
                                     "    vColor = aColor;\n"
                                     "    vTexCoord = aTexCoord;\n"
                                     "    vLightCoord = aLightCoord;\n"
                                     "    vFog = clamp((uFogEnd - length(eye.xyz)) / (uFogEnd - uFogStart), 0.0, 1.0);\n"
                                     "}\n";

static const char *r_fragment_source = "#version 330 core\n"
                                       "uniform sampler2D uTexture;\n"
                                       "uniform sampler2D uLightmap;\n"
                                       "uniform float uAlphaThreshold;\n"
                                       "in vec4 vColor;\n"
                                       "in vec2 vTexCoord;\n"
                                       "in vec2 vLightCoord;\n"
                                       "in float vFog;\n"
                                       "out vec4 outColor;\n"
                                       "void main()\n"
                                       "{\n"
                                       "    vec4 texel = texture(uTexture, vTexCoord) * texture(uLightmap, vLightCoord) * vColor;\n"
                                       "    if (texel.a < uAlphaThreshold)\n"
                                       "        discard;\n"
                                       // The sector fog is always black, so scaling the color is enough.
                                       "    outColor = vec4(texel.rgb * vFog, texel.a);\n"
                                       "}\n";

static GLuint r_program;
static GLuint r_vao;
static GLuint r_vbo;

static GLint r_loc_projection;
static GLint r_loc_modelview;
static GLint r_loc_fog_start;
static GLint r_loc_fog_end;
static GLint r_loc_alpha_threshold;

static void R_CompileShader(GLenum type, GLuint program, const char *source, const char *label)
{
    GLuint shader = glCreateShader(type);
    GLint status;

    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE)
    {
        char log[1024];

        glGetShaderInfoLog(shader, sizeof(log), NULL, log);
        ri.Sys_Error(ERR_FATAL, "%s shader compile failed: %s", label, log);
    }
    glAttachShader(program, shader);
    glDeleteShader(shader);
}

void R_InitShaders(void)
{
    GLint status;
    byte white[4] = {255, 255, 255, 255};

    r_program = glCreateProgram();
    R_CompileShader(GL_VERTEX_SHADER, r_program, r_vertex_source, "vertex");
    R_CompileShader(GL_FRAGMENT_SHADER, r_program, r_fragment_source, "fragment");
    glLinkProgram(r_program);
    glGetProgramiv(r_program, GL_LINK_STATUS, &status);
    if (status != GL_TRUE)
    {
        char log[1024];

        glGetProgramInfoLog(r_program, sizeof(log), NULL, log);
        ri.Sys_Error(ERR_FATAL, "shader link failed: %s", log);
    }

    r_loc_projection = glGetUniformLocation(r_program, "uProjection");
    r_loc_modelview = glGetUniformLocation(r_program, "uModelView");
    r_loc_fog_start = glGetUniformLocation(r_program, "uFogStart");
    r_loc_fog_end = glGetUniformLocation(r_program, "uFogEnd");
    r_loc_alpha_threshold = glGetUniformLocation(r_program, "uAlphaThreshold");

    glUseProgram(r_program);
    glUniform1i(glGetUniformLocation(r_program, "uTexture"), 0);
    glUniform1i(glGetUniformLocation(r_program, "uLightmap"), 1);
    glUniform1f(r_loc_fog_start, 0.0f);
    glUniform1f(r_loc_fog_end, 1.0e9f);
    glUniform1f(r_loc_alpha_threshold, -1.0f);
    glUseProgram(0);

    glGenVertexArrays(1, &r_vao);
    glBindVertexArray(r_vao);
    glGenBuffers(1, &r_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, r_vbo);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(rbVertex_t),
                          (void *)offsetof(rbVertex_t, pos));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(rbVertex_t),
                          (void *)offsetof(rbVertex_t, tex));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(rbVertex_t),
                          (void *)offsetof(rbVertex_t, light));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(rbVertex_t),
                          (void *)offsetof(rbVertex_t, color));

    // A white texture stands in for both the disabled fixed-function texturing
    // and the idle lightmap unit.
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, R_TextureHandle(TEXNUM_WHITE));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, R_TextureHandle(TEXNUM_WHITE));

    Mat_Identity(r_mat_projection);
    Mat_Identity(r_mat_modelview);
}

void R_ShutdownShaders(void)
{
    glDeleteBuffers(1, &r_vbo);
    glDeleteVertexArrays(1, &r_vao);
    glDeleteProgram(r_program);
    r_vbo = r_vao = r_program = 0;
}

/*
====================================================================

BATCHED VERTICES

====================================================================
*/

#define RB_MAX_VERTS 65536

static rbVertex_t rb_verts[RB_MAX_VERTS];
static int rb_count;
static GLenum rb_mode;
static GLenum rb_draw_mode;

static float rb_tex[2];
static float rb_light[2];
static float rb_color[4];
// Sticky state, mirroring the GL flags these calls replaced. GL_SetDefaultState
// and the individual draw paths toggle them, so RB_End must not.
static qboolean rb_alpha_test = true;
static qboolean rb_textured = true;
static qboolean rb_fog_enable;
static float rb_fog_start, rb_fog_end;

// Staging for GL_QUADS and GL_POLYGON, which have no core-profile equivalent.
static rbVertex_t rb_quad[4];
static int rb_quad_count;
static rbVertex_t rb_poly_first;
static rbVertex_t rb_poly_prev;
static int rb_poly_count;

static void RB_ResetState(void)
{
    // The color stays sticky like glColor did; callers reset it themselves.
    // Texcoords are per-vertex anyway, so they can safely start at 0.
    rb_tex[0] = rb_tex[1] = 0;
    rb_light[0] = rb_light[1] = 0;
}

static void RB_Append(const rbVertex_t *v)
{
    if (rb_count >= RB_MAX_VERTS)
    {
        ri.Sys_Error(ERR_DROP, "RB_Append: overflow");
    }
    rb_verts[rb_count++] = *v;
}

static void RB_Stage(const rbVertex_t *v)
{
    switch (rb_mode)
    {
    case GL_QUADS:
        rb_quad[rb_quad_count++] = *v;
        if (rb_quad_count == 4)
        {
            RB_Append(&rb_quad[0]);
            RB_Append(&rb_quad[1]);
            RB_Append(&rb_quad[2]);
            RB_Append(&rb_quad[0]);
            RB_Append(&rb_quad[2]);
            RB_Append(&rb_quad[3]);
            rb_quad_count = 0;
        }
        break;
    case GL_POLYGON:
        if (rb_poly_count == 0)
        {
            rb_poly_first = *v;
            rb_poly_prev = *v;
            rb_poly_count = 1;
        }
        else
        {
            // Triangle fan: first vertex, previous vertex, current vertex.
            RB_Append(&rb_poly_first);
            RB_Append(&rb_poly_prev);
            RB_Append(v);
            rb_poly_prev = *v;
        }
        break;
    default:
        RB_Append(v);
        break;
    }
}

void RB_Begin(GLenum mode)
{
    rb_count = 0;
    rb_quad_count = 0;
    rb_poly_count = 0;
    rb_mode = mode;

    switch (mode)
    {
    case GL_QUAD_STRIP:
        rb_draw_mode = GL_TRIANGLE_STRIP;
        break;
    default:
        rb_draw_mode = mode;
        break;
    }
}

void RB_Color3f(float r, float g, float b)
{
    rb_color[0] = r;
    rb_color[1] = g;
    rb_color[2] = b;
}

void RB_Color3fv(const float *rgb)
{
    rb_color[0] = rgb[0];
    rb_color[1] = rgb[1];
    rb_color[2] = rgb[2];
}

void RB_Color4f(float r, float g, float b, float a)
{
    rb_color[0] = r;
    rb_color[1] = g;
    rb_color[2] = b;
    rb_color[3] = a;
}

void RB_Color4fv(const float *rgba)
{
    rb_color[0] = rgba[0];
    rb_color[1] = rgba[1];
    rb_color[2] = rgba[2];
    rb_color[3] = rgba[3];
}

void RB_Color4ubv(const byte *rgba)
{
    rb_color[0] = rgba[0] * (1.0f / 255);
    rb_color[1] = rgba[1] * (1.0f / 255);
    rb_color[2] = rgba[2] * (1.0f / 255);
    rb_color[3] = rgba[3] * (1.0f / 255);
}

void RB_TexCoord2f(float s, float t)
{
    rb_tex[0] = s;
    rb_tex[1] = t;
}

void RB_LightCoord2f(float s, float t)
{
    rb_light[0] = s;
    rb_light[1] = t;
}

static void RB_EmitVertex(float x, float y, float z)
{
    rbVertex_t v;

    v.pos[0] = x;
    v.pos[1] = y;
    v.pos[2] = z;
    v.tex[0] = rb_tex[0];
    v.tex[1] = rb_tex[1];
    v.light[0] = rb_light[0];
    v.light[1] = rb_light[1];
    v.color[0] = rb_color[0];
    v.color[1] = rb_color[1];
    v.color[2] = rb_color[2];
    v.color[3] = rb_color[3];
    RB_Stage(&v);
}

void RB_Vertex2f(float x, float y)
{
    RB_EmitVertex(x, y, 0);
}

void RB_Vertex2fv(const float *v)
{
    RB_EmitVertex(v[0], v[1], 0);
}

void RB_Vertex3f(float x, float y, float z)
{
    RB_EmitVertex(x, y, z);
}

void RB_Vertex3fv(const float *v)
{
    RB_EmitVertex(v[0], v[1], v[2]);
}

void RB_AlphaTest(qboolean enable)
{
    rb_alpha_test = enable;
}

void RB_Textured(qboolean enable)
{
    rb_textured = enable;
}

void RB_Fog(qboolean enable, float start, float end)
{
    rb_fog_enable = enable;
    rb_fog_start = start;
    rb_fog_end = end;
}

void RB_End(void)
{
    if (!rb_count)
    {
        RB_ResetState();
        return;
    }

    glUseProgram(r_program);
    glUniformMatrix4fv(r_loc_projection, 1, GL_FALSE, r_mat_projection);
    glUniformMatrix4fv(r_loc_modelview, 1, GL_FALSE, r_mat_modelview);
    glUniform1f(r_loc_fog_start, rb_fog_enable ? rb_fog_start : 0.0f);
    glUniform1f(r_loc_fog_end, rb_fog_enable ? rb_fog_end : 1.0e9f);
    glUniform1f(r_loc_alpha_threshold, rb_alpha_test ? 0.666f : -1.0f);

    glActiveTexture(GL_TEXTURE0);
    if (!rb_textured)
    {
        // Keep the bind cache honest so a later GL_Bind still takes effect.
        glBindTexture(GL_TEXTURE_2D, R_TextureHandle(TEXNUM_WHITE));
        gl_state.currenttextures[0] = TEXNUM_WHITE;
    }

    glBindVertexArray(r_vao);
    glBindBuffer(GL_ARRAY_BUFFER, r_vbo);
    glBufferData(GL_ARRAY_BUFFER, rb_count * sizeof(rbVertex_t), rb_verts, GL_STREAM_DRAW);
    glDrawArrays(rb_draw_mode, 0, rb_count);

    RB_ResetState();
}

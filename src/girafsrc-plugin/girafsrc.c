/* GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
 * Copyright (C) <2016> Matthew Waters <matthew@centricular.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gl/gstglfuncs.h>

#include "girafsrc.h"

// clang-format off
static const GLfloat positions[] = {
    -1.0f,  1.0f,
    -1.0f, -1.0f,
     1.0f,  1.0f,
     1.0f, -1.0f
};
// clang-format on

struct SrcShader {
  struct BaseSrcImpl base;
  GstGLShader *shader;
  guint vao;
  guint vbo;
};

/* *INDENT-OFF* */
static const gchar *checkers_vertex_src = "#version 330\n"
                                          "layout(location = 0) in vec2 position;\n"
                                          "out vec2 uv;\n"
                                          "void main()\n"
                                          "{\n"
                                          "  gl_Position = vec4(position, 0, 1);\n"
                                          "  uv = position.xy;\n"
                                          "}";

static const gchar *checkers_fragment_src =
    "#version 330\n"
    "precision highp float;\n"
    "uniform float width;\n"
    "uniform float height;\n"
    "in vec2 uv;\n"
    "out vec4 outColor;\n"
    "void main()\n"
    "{\n"
    "  vec2 xy_mod = floor (0.5 * uv * vec2(width, height) / 8.);\n"
    "  float result = mod (xy_mod.x + xy_mod.y, 2.0);\n"
    "  outColor.r = step (result, 0.5);\n"
    "  outColor.g = 1.0 - outColor.r;\n"
    "  outColor.ba = vec2(0.0, 1.0);\n"
    "}";
/* *INDENT-ON* */

gpointer _src_checkers8_new(GstGirafSrc *element) {
  struct SrcShader *src = g_new0(struct SrcShader, 1);
  src->base.src = element;
  return src;
}

// Called from gl_start
gboolean _src_checkers_init(gpointer impl, GstGLContext *context, GstVideoInfo *v_info) {
  struct SrcShader *src = impl;
  GError *error = NULL;

  src->base.context = context;

  src->shader = gst_gl_shader_new_link_with_stages(
      context, &error,
      gst_glsl_stage_new_with_string(context, GL_VERTEX_SHADER, GST_GLSL_VERSION_330,
                                     GST_GLSL_PROFILE_CORE, checkers_vertex_src),
      gst_glsl_stage_new_with_string(context, GL_FRAGMENT_SHADER, GST_GLSL_VERSION_330,
                                     GST_GLSL_PROFILE_CORE, checkers_fragment_src),
      NULL);
  if (!src->shader) {
    GST_ERROR_OBJECT(src->base.src, "%s", error->message);
    return FALSE;
  }

  gst_gl_shader_use(src->shader);
  gst_gl_shader_set_uniform_1f(src->shader, "width", (gfloat)GST_VIDEO_INFO_WIDTH(v_info));
  gst_gl_shader_set_uniform_1f(src->shader, "height", (gfloat)GST_VIDEO_INFO_HEIGHT(v_info));
  gst_gl_context_clear_shader(src->base.context);

  const GstGLFuncs *gl = context->gl_vtable;

  gl->GenVertexArrays(1, &src->vao);
  gl->BindVertexArray(src->vao);

  gl->GenBuffers(1, &src->vbo);
  gl->BindBuffer(GL_ARRAY_BUFFER, src->vbo);
  gl->BufferData(GL_ARRAY_BUFFER, sizeof(positions), positions, GL_STATIC_DRAW);

  gint attribute_location = gst_gl_shader_get_attribute_location(src->shader, "position");
  gl->VertexAttribPointer(attribute_location, 2, GL_FLOAT, GL_FALSE, 0, 0);
  gl->EnableVertexAttribArray(attribute_location);

  gl->BindVertexArray(0);
  gl->BindBuffer(GL_ARRAY_BUFFER, 0);

  return TRUE;
}

// called from fill_gl_memory, from the gst_gl_framebuffer_draw_to_texture callback
gboolean _src_checkers_fill_bound_fbo(gpointer impl) {
  struct SrcShader *src = impl;
  const GstGLFuncs *gl;

  g_return_val_if_fail(src->base.context, FALSE);
  g_return_val_if_fail(src->shader, FALSE);
  gl = src->base.context->gl_vtable;

  gst_gl_shader_use(src->shader);

  if (gl->GenVertexArrays)
    gl->BindVertexArray(src->vao);

  gl->DrawArrays(GL_TRIANGLE_STRIP, 0, 4);

  gl->BindVertexArray(0);
  gst_gl_context_clear_shader(src->base.context);

  return TRUE;
}

// Called from gl_stop
void _src_checkers_free(gpointer impl) {
  struct SrcShader *src = impl;

  if (!src)
    return;

  const GstGLFuncs *gl = src->base.context->gl_vtable;

  if (src->shader)
    gst_object_unref(src->shader);
  src->shader = NULL;

  if (src->vao)
    gl->DeleteVertexArrays(1, &src->vao);
  src->vao = 0;

  if (src->vbo)
    gl->DeleteBuffers(1, &src->vbo);
  src->vbo = 0;

  g_free(src);
}

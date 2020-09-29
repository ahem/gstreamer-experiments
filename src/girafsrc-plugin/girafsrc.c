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

#define MAX_ATTRIBUTES 4

/* *INDENT-OFF* */
static const GLfloat positions[] = {
    -1.0, 1.0, 0.0, 1.0, 1.0, 1.0, 0.0, 1.0, 1.0, -1.0, 0.0, 1.0, -1.0, -1.0, 0.0, 1.0,
};

static const GLushort indices_quad[] = {0, 1, 2, 0, 2, 3};
/* *INDENT-ON* */

struct attribute {
  const gchar *name;
  gint location;
  guint n_elements;
  GLenum element_type;
  guint offset; /* in bytes */
  guint stride; /* in bytes */
};

struct SrcShader {
  struct BaseSrcImpl base;

  GstGLShader *shader;

  guint vao;
  guint vbo;
  guint vbo_indices;

  guint n_attributes;
  struct attribute attributes[MAX_ATTRIBUTES];

  gconstpointer vertices;
  gsize vertices_size;
  const gushort *indices;
  guint index_offset;
  guint n_indices;
};

static void _bind_buffer(struct SrcShader *src) {
  GstGLContext *context = src->base.context;
  const GstGLFuncs *gl = context->gl_vtable;
  gint i;

  gl->BindBuffer(GL_ELEMENT_ARRAY_BUFFER, src->vbo_indices);
  gl->BindBuffer(GL_ARRAY_BUFFER, src->vbo);

  /* Load the vertex position */
  for (i = 0; i < src->n_attributes; i++) {
    struct attribute *attr = &src->attributes[i];

    if (attr->location == -1)
      attr->location = gst_gl_shader_get_attribute_location(src->shader, attr->name);

    gl->VertexAttribPointer(attr->location, attr->n_elements, attr->element_type, GL_FALSE,
                            attr->stride, (void *)(gintptr)attr->offset);

    gl->EnableVertexAttribArray(attr->location);
  }
}

static void _unbind_buffer(struct SrcShader *src) {
  GstGLContext *context = src->base.context;
  const GstGLFuncs *gl = context->gl_vtable;
  gint i;

  gl->BindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  gl->BindBuffer(GL_ARRAY_BUFFER, 0);

  for (i = 0; i < src->n_attributes; i++) {
    struct attribute *attr = &src->attributes[i];

    gl->DisableVertexAttribArray(attr->location);
  }
}

static gboolean _src_shader_init(gpointer impl, GstGLContext *context, GstVideoInfo *v_info) {
  struct SrcShader *src = impl;
  const GstGLFuncs *gl = context->gl_vtable;

  src->base.context = context;

  if (!src->vbo) {
    if (gl->GenVertexArrays) {
      gl->GenVertexArrays(1, &src->vao);
      gl->BindVertexArray(src->vao);
    }

    gl->GenBuffers(1, &src->vbo);
    gl->BindBuffer(GL_ARRAY_BUFFER, src->vbo);
    gl->BufferData(GL_ARRAY_BUFFER, src->vertices_size, src->vertices, GL_STATIC_DRAW);

    gl->GenBuffers(1, &src->vbo_indices);
    gl->BindBuffer(GL_ELEMENT_ARRAY_BUFFER, src->vbo_indices);
    gl->BufferData(GL_ELEMENT_ARRAY_BUFFER, src->n_indices * sizeof(gushort), src->indices,
                   GL_STATIC_DRAW);

    if (gl->GenVertexArrays) {
      _bind_buffer(src);
      gl->BindVertexArray(0);
    }

    gl->BindBuffer(GL_ARRAY_BUFFER, 0);
    gl->BindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  }

  return TRUE;
}

static gboolean _src_shader_fill_bound_fbo(gpointer impl) {
  struct SrcShader *src = impl;
  const GstGLFuncs *gl;

  g_return_val_if_fail(src->base.context, FALSE);
  g_return_val_if_fail(src->shader, FALSE);
  gl = src->base.context->gl_vtable;

  gst_gl_shader_use(src->shader);

  if (gl->GenVertexArrays)
    gl->BindVertexArray(src->vao);
  _bind_buffer(src);

  gl->DrawElements(GL_TRIANGLES, src->n_indices, GL_UNSIGNED_SHORT,
                   (gpointer)(gintptr)src->index_offset);

  if (gl->GenVertexArrays)
    gl->BindVertexArray(0);
  else
    _unbind_buffer(src);

  gst_gl_context_clear_shader(src->base.context);

  return TRUE;
}

static void _src_shader_deinit(gpointer impl) {
  struct SrcShader *src = impl;
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

  if (src->vbo_indices)
    gl->DeleteBuffers(1, &src->vbo_indices);
  src->vbo_indices = 0;
}

/* *INDENT-OFF* */
static const gchar *checkers_vertex_src = "attribute vec4 position;\n"
                                          "varying vec2 uv;\n"
                                          "void main()\n"
                                          "{\n"
                                          "  gl_Position = position;\n"
                                          /* RPi gives incorrect results for positive uv (plus it
                                           * makes us start on the right pixel color i.e. red) */
                                          "  uv = position.xy - 1.0;\n"
                                          "}";

static const gchar *checkers_fragment_src =
    "uniform float checker_width;\n"
    "uniform float width;\n"
    "uniform float height;\n"
    "varying vec2 uv;\n"
    "void main()\n"
    "{\n"
    "  vec2 xy_mod = floor (0.5 * uv * vec2(width, height) / (checker_width));\n"
    "  float result = mod (xy_mod.x + xy_mod.y, 2.0);\n"
    "  gl_FragColor.r = step (result, 0.5);\n"
    "  gl_FragColor.g = 1.0 - gl_FragColor.r;\n"
    "  gl_FragColor.ba = vec2(0.0, 1.0);\n"
    "}";
/* *INDENT-ON* */

struct SrcCheckers {
  struct SrcShader base;

  guint checker_width;
};

static gboolean _src_checkers_init(gpointer impl, GstGLContext *context, GstVideoInfo *v_info) {
  struct SrcCheckers *src = impl;
  GError *error = NULL;
  const gchar *frags[2];

  src->base.base.context = context;

  frags[0] = gst_gl_shader_string_get_highest_precision(
      context, GST_GLSL_VERSION_NONE, GST_GLSL_PROFILE_ES | GST_GLSL_PROFILE_COMPATIBILITY);
  frags[1] = checkers_fragment_src;

  if (src->base.shader)
    gst_object_unref(src->base.shader);
  src->base.shader = gst_gl_shader_new_link_with_stages(
      context, &error,
      gst_glsl_stage_new_with_string(context, GL_VERTEX_SHADER, GST_GLSL_VERSION_NONE,
                                     GST_GLSL_PROFILE_ES | GST_GLSL_PROFILE_COMPATIBILITY,
                                     checkers_vertex_src),
      gst_glsl_stage_new_with_strings(context, GL_FRAGMENT_SHADER, GST_GLSL_VERSION_NONE,
                                      GST_GLSL_PROFILE_ES | GST_GLSL_PROFILE_COMPATIBILITY, 2,
                                      frags),
      NULL);
  if (!src->base.shader) {
    GST_ERROR_OBJECT(src->base.base.src, "%s", error->message);
    return FALSE;
  }

  src->base.n_attributes = 1;

  src->base.attributes[0].name = "position";
  src->base.attributes[0].location = -1;
  src->base.attributes[0].n_elements = 4;
  src->base.attributes[0].element_type = GL_FLOAT;
  src->base.attributes[0].offset = 0;
  src->base.attributes[0].stride = 4 * sizeof(gfloat);

  src->base.vertices = positions;
  src->base.vertices_size = sizeof(positions);
  src->base.indices = indices_quad;
  src->base.n_indices = 6;

  gst_gl_shader_use(src->base.shader);
  gst_gl_shader_set_uniform_1f(src->base.shader, "checker_width", src->checker_width);
  gst_gl_shader_set_uniform_1f(src->base.shader, "width", (gfloat)GST_VIDEO_INFO_WIDTH(v_info));
  gst_gl_shader_set_uniform_1f(src->base.shader, "height", (gfloat)GST_VIDEO_INFO_HEIGHT(v_info));
  gst_gl_context_clear_shader(src->base.base.context);

  return _src_shader_init(impl, context, v_info);
}

static void _src_checkers_free(gpointer impl) {
  struct SrcCheckers *src = impl;

  if (!src)
    return;

  _src_shader_deinit(impl);

  g_free(impl);
}

static gpointer _src_checkers_new(GstGirafSrc *test) {
  struct SrcCheckers *src = g_new0(struct SrcCheckers, 1);

  src->base.base.src = test;

  return src;
}

static gpointer _src_checkers8_new(GstGirafSrc *test) {
  struct SrcCheckers *src = _src_checkers_new(test);
  src->checker_width = 8;
  return src;
}
static const struct SrcFuncs src_checkers8 = {
    GST_GIRAF_SRC_CHECKERS8,    _src_checkers8_new, _src_checkers_init,
    _src_shader_fill_bound_fbo, _src_checkers_free,
};

static const struct SrcFuncs *src_impls[] = {&src_checkers8};

const struct SrcFuncs *gst_giraf_src_get_src_funcs_for_pattern(GstGirafSrcPattern pattern) {
  gint i;

  for (i = 0; i < G_N_ELEMENTS(src_impls); i++) {
    if (src_impls[i]->pattern == pattern)
      return src_impls[i];
  }

  return NULL;
}

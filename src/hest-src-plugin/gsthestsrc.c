#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gsthestsrc.h"
#include <gst/gl/gstglfuncs.h>

GST_DEBUG_CATEGORY_STATIC(hest_src_debug_category);
#define GST_CAT_DEFAULT hest_src_debug_category

static GstStaticPadTemplate src_factory =
    GST_STATIC_PAD_TEMPLATE("src", GST_PAD_SRC, GST_PAD_ALWAYS,
                            GST_STATIC_CAPS("video/x-raw(" GST_CAPS_FEATURE_MEMORY_GL_MEMORY "), "
                                            "format = (string) RGBA, "
                                            "width = " GST_VIDEO_SIZE_RANGE ", "
                                            "height = " GST_VIDEO_SIZE_RANGE ", "
                                            "framerate = " GST_VIDEO_FPS_RANGE ","
                                            "texture-target = (string) 2D"));

#define gst_hest_src_parent_class parent_class
G_DEFINE_TYPE(GstHestSrc, gst_hest_src, GST_TYPE_GL_BASE_SRC);

const gchar *hest_vertex_shader_src = "#version 330\n"
                                      "precision highp float;\n"
                                      "layout(location = 0) in vec2 iPosition;\n"
                                      "void main() {\n"
                                      "gl_Position = vec4(iPosition, 0, 1);\n"
                                      "}\n";

const gchar *hest_fragment_shader_src = "#version 330 core\n"
                                        "precision highp float;\n"
                                        "out vec4 outColor;\n"
                                        "void main() {\n"
                                        "outColor = vec4(1);\n"
                                        "}\n";

static const GLfloat positions[] = {-1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f, -1.0f};

static gboolean gst_hest_src_gl_start(GstGLBaseSrc *bsrc) {
  GstHestSrc *src = GST_HEST_SRC(bsrc);
  GST_DEBUG_OBJECT(src, "gl-start");

  guint w = GST_VIDEO_INFO_WIDTH(&bsrc->out_info);
  guint h = GST_VIDEO_INFO_HEIGHT(&bsrc->out_info);
  src->fbo = gst_gl_framebuffer_new_with_default_depth(bsrc->context, w, h);

  GError *error = NULL;

  src->shader = gst_gl_shader_new_link_with_stages(
      bsrc->context, &error,
      gst_glsl_stage_new_with_string(bsrc->context, GL_VERTEX_SHADER, GST_GLSL_VERSION_330,
                                     GST_GLSL_PROFILE_CORE, hest_vertex_shader_src),
      gst_glsl_stage_new_with_string(bsrc->context, GL_FRAGMENT_SHADER, GST_GLSL_VERSION_330,
                                     GST_GLSL_PROFILE_CORE, hest_fragment_shader_src),
      NULL);

  if (!src->shader) {
    GST_ERROR_OBJECT(src, "%s", error->message);
    g_error_free(error);
    return FALSE;
  }

  const GstGLFuncs *gl = bsrc->context->gl_vtable;

  gl->GenVertexArrays(1, &src->vao);
  gl->BindVertexArray(src->vao);

  gl->GenBuffers(1, &src->positionBuffer);
  gl->BindBuffer(GL_ARRAY_BUFFER, src->positionBuffer);
  gl->BufferData(GL_ARRAY_BUFFER, sizeof(positions), &positions, GL_STATIC_DRAW);

  gint position_attribute_location = gst_gl_shader_get_attribute_location(src->shader, "iPosition");
  gl->VertexAttribPointer(position_attribute_location, 2, GL_FLOAT, FALSE, 0, 0);

  gl->BindBuffer(GL_ARRAY_BUFFER, 0);
  gl->BindVertexArray(0);
  gst_gl_context_clear_shader(bsrc->context);

  return TRUE;
}

static gboolean gst_hest_src_callback(gpointer ptr) {
  GstHestSrc *src = GST_HEST_SRC(ptr);
  GstGLBaseSrc *glbasesrc = GST_GL_BASE_SRC(src);

  const GstGLFuncs *gl = glbasesrc->context->gl_vtable;

  gl->Disable(GL_CULL_FACE);
  gl->Disable(GL_DEPTH_TEST);

  gl->ClearColor(0.3f, 0.3f, 0.3f, 1.0f);
  gl->Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  gst_gl_shader_use(src->shader);
  GST_DEBUG_OBJECT(src, "<glError check> 1: %d", gl->GetError());
  gl->BindVertexArray(src->vao);
  GST_DEBUG_OBJECT(src, "<glError check> 2: %d", gl->GetError());
  gl->DrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  GST_DEBUG_OBJECT(src, "<glError check> 3: %d", gl->GetError());

  gst_gl_context_clear_shader(glbasesrc->context);

  return TRUE;
}

static void gst_hest_src_gl_stop(GstGLBaseSrc *bsrc) {
  GstHestSrc *src = GST_HEST_SRC(bsrc);
  GST_DEBUG_OBJECT(src, "gl-stop");

  if (src->positionBuffer)
    bsrc->context->gl_vtable->DeleteBuffers(1, &src->positionBuffer);
  if (src->vao)
    bsrc->context->gl_vtable->DeleteVertexArrays(1, &src->vao);
  if (src->shader)
    gst_object_unref(src->shader);
  if (src->fbo)
    gst_object_unref(src->fbo);
  src->fbo = NULL;
}

static gboolean gst_hest_src_fill_memory(GstGLBaseSrc *bsrc, GstGLMemory *memory) {
  GstHestSrc *src = GST_HEST_SRC(bsrc);

  return gst_gl_framebuffer_draw_to_texture(src->fbo, memory, gst_hest_src_callback, src);
}

static GstCaps *gst_hest_src_fixate(GstBaseSrc *bsrc, GstCaps *caps) {
  GstStructure *structure;

  GST_DEBUG("fixate");

  caps = gst_caps_make_writable(caps);
  structure = gst_caps_get_structure(caps, 0);

  gst_structure_fixate_field_nearest_int(structure, "width", 320);
  gst_structure_fixate_field_nearest_int(structure, "height", 240);
  gst_structure_fixate_field_nearest_fraction(structure, "framerate", 30, 1);

  caps = GST_BASE_SRC_CLASS(parent_class)->fixate(bsrc, caps);

  return caps;
}

static void gst_hest_src_class_init(GstHestSrcClass *klass) {
  GObjectClass *gobject_class;
  GstBaseSrcClass *gstbasesrc_class;
  GstGLBaseSrcClass *gstglbasesrc_class;
  GstElementClass *element_class;

  GST_DEBUG_CATEGORY_INIT(hest_src_debug_category, "hestsrc", 0,
                          "debug category for Hest Source element");

  gobject_class = G_OBJECT_CLASS(klass);
  gstbasesrc_class = GST_BASE_SRC_CLASS(klass);
  gstglbasesrc_class = GST_GL_BASE_SRC_CLASS(klass);
  element_class = GST_ELEMENT_CLASS(klass);

  gst_element_class_add_static_pad_template(element_class, &src_factory);

  gst_element_class_set_static_metadata(element_class, "FIXME Long name", "Generic",
                                        "FIXME Description", "FIXME <fixme@example.com>");

  gstglbasesrc_class->gl_start = gst_hest_src_gl_start;
  gstglbasesrc_class->gl_stop = gst_hest_src_gl_stop;
  gstglbasesrc_class->fill_gl_memory = gst_hest_src_fill_memory;
  gstglbasesrc_class->supported_gl_api = GST_GL_API_OPENGL3;

  gstbasesrc_class->fixate = gst_hest_src_fixate;
}

static void gst_hest_src_init(GstHestSrc *hestSrc) {}

static gboolean plugin_init(GstPlugin *plugin) {
  return gst_element_register(plugin, "hestsrc", GST_RANK_NONE, GST_TYPE_HEST_SRC);
}

/* FIXME: these are normally defined by the GStreamer build system.
   If you are creating an element to be included in gst-plugins-*,
   remove these, as they're always defined.  Otherwise, edit as
   appropriate for your external plugin package. */
#ifndef VERSION
#define VERSION "0.0.FIXME"
#endif
#ifndef PACKAGE
#define PACKAGE "FIXME_package"
#endif
#ifndef PACKAGE_NAME
#define PACKAGE_NAME "FIXME_package_name"
#endif
#ifndef GST_PACKAGE_ORIGIN
#define GST_PACKAGE_ORIGIN "http://FIXME.org/"
#endif
GST_PLUGIN_DEFINE(GST_VERSION_MAJOR, GST_VERSION_MINOR, hestsrc, "FIXME plugin description",
                  plugin_init, VERSION, "LGPL", PACKAGE_NAME, GST_PACKAGE_ORIGIN)

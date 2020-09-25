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

static gboolean gst_hest_src_gl_start(GstGLBaseSrc *bsrc) {
  GstHestSrc *src = GST_HEST_SRC(bsrc);
  GST_DEBUG_OBJECT(src, "gl-start");

  guint w = GST_VIDEO_INFO_WIDTH(&bsrc->out_info);
  guint h = GST_VIDEO_INFO_HEIGHT(&bsrc->out_info);
  src->fbo = gst_gl_framebuffer_new_with_default_depth(bsrc->context, w, h);

  return TRUE;
}

static void gst_hest_src_gl_stop(GstGLBaseSrc *bsrc) {
  GstHestSrc *src = GST_HEST_SRC(bsrc);
  GST_DEBUG_OBJECT(src, "gl-stop");

  if (src->fbo)
    gst_object_unref(src->fbo);
  src->fbo = NULL;
}

static gboolean gst_hest_src_callback(gpointer ptr) {
  GstHestSrc *src = GST_HEST_SRC(ptr);
  GstGLBaseSrc *glbasesrc = GST_GL_BASE_SRC(src);

  const GstGLFuncs *gl = glbasesrc->context->gl_vtable;
  gl->ClearColor(1.0f, 1.0f, 0.0f, 1.0f);
  gl->Clear(GL_COLOR_BUFFER_BIT);

  return TRUE;
}

static gboolean gst_hest_src_fill_memory(GstGLBaseSrc *bsrc, GstGLMemory *memory) {
  GstHestSrc *src = GST_HEST_SRC(bsrc);
  GST_DEBUG_OBJECT(src, "fill-memory");

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

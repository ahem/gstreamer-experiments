/*
 * GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
 * Copyright (C) 2002,2007 David A. Schleef <ds@schleef.org>
 * Copyright (C) 2008 Julien Isorce <julien.isorce@gmail.com>
 * Copyright (C) 2015 Matthew Waters <matthew@centricular.com>
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

/**
 * SECTION:element-girafsrc
 * @title: girafsrc
 *
 * The girafsrc element is used to produce test video texture.
 * The video test produced can be controlled with the "pattern"
 * property.
 *
 * ## Example launch line
 *
 * |[
 * gst-launch-1.0 -v girafsrc pattern=smpte ! glimagesink
 * ]|
 * Shows original SMPTE color bars in a window.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gl/gstglfuncs.h>

#include "girafsrc.h"
#include "gstgirafsrc.h"

GST_DEBUG_CATEGORY_STATIC(giraf_src_debug);
#define GST_CAT_DEFAULT giraf_src_debug

enum {
  PROP_0,
  PROP_IS_LIVE
  /* FILL ME */
};

/* *INDENT-OFF* */
static GstStaticPadTemplate src_factory =
    GST_STATIC_PAD_TEMPLATE("src", GST_PAD_SRC, GST_PAD_ALWAYS,
                            GST_STATIC_CAPS("video/x-raw(" GST_CAPS_FEATURE_MEMORY_GL_MEMORY "), "
                                            "format = (string) RGBA, "
                                            "width = " GST_VIDEO_SIZE_RANGE ", "
                                            "height = " GST_VIDEO_SIZE_RANGE ", "
                                            "framerate = " GST_VIDEO_FPS_RANGE ","
                                            "texture-target = (string) 2D"));
/* *INDENT-ON* */

#define gst_giraf_src_parent_class parent_class
G_DEFINE_TYPE(GstGirafSrc, gst_giraf_src, GST_TYPE_GL_BASE_SRC);

static void gst_giraf_src_set_property(GObject *object, guint prop_id, const GValue *value,
                                       GParamSpec *pspec);
static void gst_giraf_src_get_property(GObject *object, guint prop_id, GValue *value,
                                       GParamSpec *pspec);

static GstCaps *gst_giraf_src_fixate(GstBaseSrc *bsrc, GstCaps *caps);
static gboolean gst_giraf_src_is_seekable(GstBaseSrc *psrc);
static gboolean gst_giraf_src_callback(gpointer stuff);
static gboolean gst_giraf_src_gl_start(GstGLBaseSrc *src);
static void gst_giraf_src_gl_stop(GstGLBaseSrc *src);
static gboolean gst_giraf_src_fill_memory(GstGLBaseSrc *src, GstGLMemory *memory);

static void gst_giraf_src_class_init(GstGirafSrcClass *klass) {
  GObjectClass *gobject_class;
  GstBaseSrcClass *gstbasesrc_class;
  GstGLBaseSrcClass *gstglbasesrc_class;
  GstElementClass *element_class;

  GST_DEBUG_CATEGORY_INIT(giraf_src_debug, "girafsrc", 0, "Video Test Source");

  gobject_class = G_OBJECT_CLASS(klass);
  gstbasesrc_class = GST_BASE_SRC_CLASS(klass);
  gstglbasesrc_class = GST_GL_BASE_SRC_CLASS(klass);
  element_class = GST_ELEMENT_CLASS(klass);

  gobject_class->set_property = gst_giraf_src_set_property;
  gobject_class->get_property = gst_giraf_src_get_property;

  g_object_class_install_property(gobject_class, PROP_IS_LIVE,
                                  g_param_spec_boolean("is-live", "Is Live",
                                                       "Whether to act as a live source", FALSE,
                                                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  gst_element_class_set_metadata(element_class, "Giraf Video test source", "Source/Video",
                                 "Creates a test video stream",
                                 "Anders Hellerup Madsen <ahem@github.com>");

  gst_element_class_add_static_pad_template(element_class, &src_factory);

  gstbasesrc_class->is_seekable = gst_giraf_src_is_seekable;
  gstbasesrc_class->fixate = gst_giraf_src_fixate;

  gstglbasesrc_class->supported_gl_api = GST_GL_API_OPENGL3;
  gstglbasesrc_class->gl_start = gst_giraf_src_gl_start;
  gstglbasesrc_class->gl_stop = gst_giraf_src_gl_stop;
  gstglbasesrc_class->fill_gl_memory = gst_giraf_src_fill_memory;
}

static void gst_giraf_src_init(GstGirafSrc *src) {}

static GstCaps *gst_giraf_src_fixate(GstBaseSrc *bsrc, GstCaps *caps) {
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

static void gst_giraf_src_set_property(GObject *object, guint prop_id, const GValue *value,
                                       GParamSpec *pspec) {
  GstGirafSrc *src = GST_GIRAF_SRC(object);

  switch (prop_id) {
  case PROP_IS_LIVE:
    gst_base_src_set_live(GST_BASE_SRC(src), g_value_get_boolean(value));
    break;
  default:
    break;
  }
}

static void gst_giraf_src_get_property(GObject *object, guint prop_id, GValue *value,
                                       GParamSpec *pspec) {
  GstGirafSrc *src = GST_GIRAF_SRC(object);

  switch (prop_id) {
  case PROP_IS_LIVE:
    g_value_set_boolean(value, gst_base_src_is_live(GST_BASE_SRC(src)));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    break;
  }
}

static gboolean gst_giraf_src_is_seekable(GstBaseSrc *psrc) {
  /* we're seekable... */
  return TRUE;
}

static gboolean gst_giraf_src_callback(gpointer stuff) {
  GstGirafSrc *src = GST_GIRAF_SRC(stuff);
  return _src_checkers_fill_bound_fbo(src->src_impl);
}

static gboolean gst_giraf_src_fill_memory(GstGLBaseSrc *src, GstGLMemory *memory) {
  GstGirafSrc *test_src = GST_GIRAF_SRC(src);

  return gst_gl_framebuffer_draw_to_texture(test_src->fbo, memory, gst_giraf_src_callback,
                                            test_src);
}

static gboolean gst_giraf_src_gl_start(GstGLBaseSrc *bsrc) {
  GstGirafSrc *src = GST_GIRAF_SRC(bsrc);
  GstGLBaseSrc *glbasesrc = GST_GL_BASE_SRC(src);

  src->fbo = gst_gl_framebuffer_new_with_default_depth(
      bsrc->context, GST_VIDEO_INFO_WIDTH(&bsrc->out_info), GST_VIDEO_INFO_HEIGHT(&bsrc->out_info));

  src->src_impl = _src_checkers8_new(src);
  if (!_src_checkers_init(src->src_impl, glbasesrc->context, &glbasesrc->out_info)) {
    GST_ERROR_OBJECT(src, "Failed to initialize pattern");
    return FALSE;
  }

  return TRUE;
}

static void gst_giraf_src_gl_stop(GstGLBaseSrc *bsrc) {
  GstGirafSrc *src = GST_GIRAF_SRC(bsrc);

  if (src->fbo)
    gst_object_unref(src->fbo);
  src->fbo = NULL;

  if (src->src_impl)
    _src_checkers_free(src->src_impl);
  src->src_impl = NULL;
}

static gboolean plugin_init(GstPlugin *plugin) {
  return gst_element_register(plugin, "girafsrc", GST_RANK_NONE, GST_TYPE_GIRAF_SRC);
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
GST_PLUGIN_DEFINE(GST_VERSION_MAJOR, GST_VERSION_MINOR, girafsrc, "FIXME plugin description",
                  plugin_init, VERSION, "LGPL", PACKAGE_NAME, GST_PACKAGE_ORIGIN)

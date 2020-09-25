/* GStreamer
 * Copyright (C) 2020 Anders Hellerup Madsen <fixme@example.com>
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
 * Free Software Foundation, Inc., 51 Franklin Street, Suite 500,
 * Boston, MA 02110-1335, USA.
 */
/**
 * SECTION:element-gstLhest
 *
 * The Lhest element does FIXME stuff.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch-1.0 -v fakesrc ! Lhest ! FIXME ! fakesink
 * ]|
 * FIXME Describe what the pipeline does.
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gstLhest.h"
#include <gst/base/gstbasesrc.h>
#include <gst/gst.h>

GST_DEBUG_CATEGORY_STATIC(gst_Lhest_debug_category);
#define GST_CAT_DEFAULT gst_Lhest_debug_category

/* prototypes */

static void gst_Lhest_set_property(GObject *object, guint property_id, const GValue *value,
                                   GParamSpec *pspec);
static void gst_Lhest_get_property(GObject *object, guint property_id, GValue *value,
                                   GParamSpec *pspec);
static void gst_Lhest_dispose(GObject *object);
static void gst_Lhest_finalize(GObject *object);

static GstCaps *gst_Lhest_get_caps(GstBaseSrc *src, GstCaps *filter);
static gboolean gst_Lhest_negotiate(GstBaseSrc *src);
static GstCaps *gst_Lhest_fixate(GstBaseSrc *src, GstCaps *caps);
static gboolean gst_Lhest_set_caps(GstBaseSrc *src, GstCaps *caps);
static gboolean gst_Lhest_decide_allocation(GstBaseSrc *src, GstQuery *query);
static gboolean gst_Lhest_start(GstBaseSrc *src);
static gboolean gst_Lhest_stop(GstBaseSrc *src);
static void gst_Lhest_get_times(GstBaseSrc *src, GstBuffer *buffer, GstClockTime *start,
                                GstClockTime *end);
static gboolean gst_Lhest_get_size(GstBaseSrc *src, guint64 *size);
static gboolean gst_Lhest_is_seekable(GstBaseSrc *src);
static gboolean gst_Lhest_prepare_seek_segment(GstBaseSrc *src, GstEvent *seek,
                                               GstSegment *segment);
static gboolean gst_Lhest_do_seek(GstBaseSrc *src, GstSegment *segment);
static gboolean gst_Lhest_unlock(GstBaseSrc *src);
static gboolean gst_Lhest_unlock_stop(GstBaseSrc *src);
static gboolean gst_Lhest_query(GstBaseSrc *src, GstQuery *query);
static gboolean gst_Lhest_event(GstBaseSrc *src, GstEvent *event);
static GstFlowReturn gst_Lhest_create(GstBaseSrc *src, guint64 offset, guint size, GstBuffer **buf);
static GstFlowReturn gst_Lhest_alloc(GstBaseSrc *src, guint64 offset, guint size, GstBuffer **buf);
static GstFlowReturn gst_Lhest_fill(GstBaseSrc *src, guint64 offset, guint size, GstBuffer *buf);

enum { PROP_0 };

/* pad templates */

static GstStaticPadTemplate gst_Lhest_src_template =
    GST_STATIC_PAD_TEMPLATE("src", GST_PAD_SRC, GST_PAD_ALWAYS,
                            GST_STATIC_CAPS("video/x-raw(" GST_CAPS_FEATURE_MEMORY_GL_MEMORY "), "
                                            "format = (string) RGBA, "
                                            "width = " GST_VIDEO_SIZE_RANGE ", "
                                            "height = " GST_VIDEO_SIZE_RANGE ", "
                                            "framerate = " GST_VIDEO_FPS_RANGE ","
                                            "texture-target = (string) 2D"));

/* class initialization */

G_DEFINE_TYPE_WITH_CODE(UgstUhest, gst_Lhest, GST_TYPE_BASE_SRC,
                        GST_DEBUG_CATEGORY_INIT(gst_Lhest_debug_category, "Lhest", 0,
                                                "debug category for Lhest element"));

static void gst_Lhest_class_init(UgstUhestClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GstBaseSrcClass *base_src_class = GST_BASE_SRC_CLASS(klass);

  /* Setting up pads and setting metadata should be moved to
     base_class_init if you intend to subclass this class. */
  gst_element_class_add_static_pad_template(GST_ELEMENT_CLASS(klass), &gst_Lhest_src_template);

  gst_element_class_set_static_metadata(GST_ELEMENT_CLASS(klass), "FIXME Long name", "Generic",
                                        "FIXME Description", "FIXME <fixme@example.com>");

  gobject_class->set_property = gst_Lhest_set_property;
  gobject_class->get_property = gst_Lhest_get_property;
  gobject_class->dispose = gst_Lhest_dispose;
  gobject_class->finalize = gst_Lhest_finalize;
  base_src_class->get_caps = GST_DEBUG_FUNCPTR(gst_Lhest_get_caps);
  base_src_class->negotiate = GST_DEBUG_FUNCPTR(gst_Lhest_negotiate);
  base_src_class->fixate = GST_DEBUG_FUNCPTR(gst_Lhest_fixate);
  base_src_class->set_caps = GST_DEBUG_FUNCPTR(gst_Lhest_set_caps);
  base_src_class->decide_allocation = GST_DEBUG_FUNCPTR(gst_Lhest_decide_allocation);
  base_src_class->start = GST_DEBUG_FUNCPTR(gst_Lhest_start);
  base_src_class->stop = GST_DEBUG_FUNCPTR(gst_Lhest_stop);
  base_src_class->get_times = GST_DEBUG_FUNCPTR(gst_Lhest_get_times);
  base_src_class->get_size = GST_DEBUG_FUNCPTR(gst_Lhest_get_size);
  base_src_class->is_seekable = GST_DEBUG_FUNCPTR(gst_Lhest_is_seekable);
  base_src_class->prepare_seek_segment = GST_DEBUG_FUNCPTR(gst_Lhest_prepare_seek_segment);
  base_src_class->do_seek = GST_DEBUG_FUNCPTR(gst_Lhest_do_seek);
  base_src_class->unlock = GST_DEBUG_FUNCPTR(gst_Lhest_unlock);
  base_src_class->unlock_stop = GST_DEBUG_FUNCPTR(gst_Lhest_unlock_stop);
  /* base_src_class->query = GST_DEBUG_FUNCPTR(gst_Lhest_query); */
  base_src_class->event = GST_DEBUG_FUNCPTR(gst_Lhest_event);
  base_src_class->create = GST_DEBUG_FUNCPTR(gst_Lhest_create);
  base_src_class->alloc = GST_DEBUG_FUNCPTR(gst_Lhest_alloc);
  base_src_class->fill = GST_DEBUG_FUNCPTR(gst_Lhest_fill);
}

static void gst_Lhest_init(UgstUhest *Lhest) {}

void gst_Lhest_set_property(GObject *object, guint property_id, const GValue *value,
                            GParamSpec *pspec) {
  UgstUhest *Lhest = Ugst_Uhest(object);

  GST_DEBUG_OBJECT(Lhest, "set_property");

  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
    break;
  }
}

void gst_Lhest_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec) {
  UgstUhest *Lhest = Ugst_Uhest(object);

  GST_DEBUG_OBJECT(Lhest, "get_property");

  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
    break;
  }
}

void gst_Lhest_dispose(GObject *object) {
  UgstUhest *Lhest = Ugst_Uhest(object);

  GST_DEBUG_OBJECT(Lhest, "dispose");

  /* clean up as possible.  may be called multiple times */

  G_OBJECT_CLASS(gst_Lhest_parent_class)->dispose(object);
}

void gst_Lhest_finalize(GObject *object) {
  UgstUhest *Lhest = Ugst_Uhest(object);

  GST_DEBUG_OBJECT(Lhest, "finalize");

  /* clean up object here */

  G_OBJECT_CLASS(gst_Lhest_parent_class)->finalize(object);
}

/* get caps from subclass */
static GstCaps *gst_Lhest_get_caps(GstBaseSrc *src, GstCaps *filter) {
  UgstUhest *Lhest = Ugst_Uhest(src);

  GST_DEBUG_OBJECT(Lhest, "get_caps");

  return NULL;
}

/* decide on caps */
static gboolean gst_Lhest_negotiate(GstBaseSrc *src) {
  UgstUhest *Lhest = Ugst_Uhest(src);

  GST_DEBUG_OBJECT(Lhest, "negotiate");

  return TRUE;
}

/* called if, in negotiation, caps need fixating */
static GstCaps *gst_Lhest_fixate(GstBaseSrc *src, GstCaps *caps) {
  UgstUhest *Lhest = Ugst_Uhest(src);

  GST_DEBUG_OBJECT(Lhest, "fixate");

  return NULL;
}

/* notify the subclass of new caps */
static gboolean gst_Lhest_set_caps(GstBaseSrc *src, GstCaps *caps) {
  UgstUhest *Lhest = Ugst_Uhest(src);

  GST_DEBUG_OBJECT(Lhest, "set_caps");

  return TRUE;
}

/* setup allocation query */
static gboolean gst_Lhest_decide_allocation(GstBaseSrc *src, GstQuery *query) {
  UgstUhest *Lhest = Ugst_Uhest(src);

  GST_DEBUG_OBJECT(Lhest, "decide_allocation");

  return TRUE;
}

/* start and stop processing, ideal for opening/closing the resource */
static gboolean gst_Lhest_start(GstBaseSrc *src) {
  UgstUhest *Lhest = Ugst_Uhest(src);

  GST_DEBUG_OBJECT(Lhest, "start");

  return TRUE;
}

static gboolean gst_Lhest_stop(GstBaseSrc *src) {
  UgstUhest *Lhest = Ugst_Uhest(src);

  GST_DEBUG_OBJECT(Lhest, "stop");

  return TRUE;
}

/* given a buffer, return start and stop time when it should be pushed
 * out. The base class will sync on the clock using these times. */
static void gst_Lhest_get_times(GstBaseSrc *src, GstBuffer *buffer, GstClockTime *start,
                                GstClockTime *end) {
  UgstUhest *Lhest = Ugst_Uhest(src);

  GST_DEBUG_OBJECT(Lhest, "get_times");
}

/* get the total size of the resource in bytes */
static gboolean gst_Lhest_get_size(GstBaseSrc *src, guint64 *size) {
  UgstUhest *Lhest = Ugst_Uhest(src);

  GST_DEBUG_OBJECT(Lhest, "get_size");

  return TRUE;
}

/* check if the resource is seekable */
static gboolean gst_Lhest_is_seekable(GstBaseSrc *src) {
  UgstUhest *Lhest = Ugst_Uhest(src);

  GST_DEBUG_OBJECT(Lhest, "is_seekable");

  return FALSE;
}

/* Prepare the segment on which to perform do_seek(), converting to the
 * current basesrc format. */
static gboolean gst_Lhest_prepare_seek_segment(GstBaseSrc *src, GstEvent *seek,
                                               GstSegment *segment) {
  UgstUhest *Lhest = Ugst_Uhest(src);

  GST_DEBUG_OBJECT(Lhest, "prepare_seek_segment");

  return TRUE;
}

/* notify subclasses of a seek */
static gboolean gst_Lhest_do_seek(GstBaseSrc *src, GstSegment *segment) {
  UgstUhest *Lhest = Ugst_Uhest(src);

  GST_DEBUG_OBJECT(Lhest, "do_seek");

  return TRUE;
}

/* unlock any pending access to the resource. subclasses should unlock
 * any function ASAP. */
static gboolean gst_Lhest_unlock(GstBaseSrc *src) {
  UgstUhest *Lhest = Ugst_Uhest(src);

  GST_DEBUG_OBJECT(Lhest, "unlock");

  return TRUE;
}

/* Clear any pending unlock request, as we succeeded in unlocking */
static gboolean gst_Lhest_unlock_stop(GstBaseSrc *src) {
  UgstUhest *Lhest = Ugst_Uhest(src);

  GST_DEBUG_OBJECT(Lhest, "unlock_stop");

  return TRUE;
}

/* notify subclasses of a query */
static gboolean gst_Lhest_query(GstBaseSrc *src, GstQuery *query) {
  UgstUhest *Lhest = Ugst_Uhest(src);

  GST_DEBUG_OBJECT(Lhest, "query");

  switch (GST_QUERY_TYPE(query)) {
  case GST_QUERY_CAPS:
    GST_DEBUG_OBJECT(Lhest, "query:GST_QUERY_CAPS");
    /* we should report the supported caps here */
  default:
    /* just call the default handler */
    break;
  }
  return TRUE;
}

/* notify subclasses of an event */
static gboolean gst_Lhest_event(GstBaseSrc *src, GstEvent *event) {
  UgstUhest *Lhest = Ugst_Uhest(src);

  GST_DEBUG_OBJECT(Lhest, "event");

  return TRUE;
}

/* ask the subclass to create a buffer with offset and size, the default
 * implementation will call alloc and fill. */
static GstFlowReturn gst_Lhest_create(GstBaseSrc *src, guint64 offset, guint size,
                                      GstBuffer **buf) {
  UgstUhest *Lhest = Ugst_Uhest(src);
  GST_DEBUG_OBJECT(Lhest, "create");

  *buf = gst_buffer_new_allocate(NULL, 1024, NULL);

  return GST_FLOW_OK;
}

/* ask the subclass to allocate an output buffer. The default implementation
 * will use the negotiated allocator. */
static GstFlowReturn gst_Lhest_alloc(GstBaseSrc *src, guint64 offset, guint size, GstBuffer **buf) {
  UgstUhest *Lhest = Ugst_Uhest(src);

  GST_DEBUG_OBJECT(Lhest, "alloc");

  return GST_FLOW_OK;
}

/* ask the subclass to fill the buffer with data from offset and size */
static GstFlowReturn gst_Lhest_fill(GstBaseSrc *src, guint64 offset, guint size, GstBuffer *buf) {
  UgstUhest *Lhest = Ugst_Uhest(src);

  GST_DEBUG_OBJECT(Lhest, "fill");

  return GST_FLOW_OK;
}

static gboolean plugin_init(GstPlugin *plugin) {
  /* FIXME Remember to set the rank if it's an element that is meant
     to be autoplugged by decodebin. */
  return gst_element_register(plugin, "Lhest", GST_RANK_NONE, Ugst_TYPE_Uhest);
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

GST_PLUGIN_DEFINE(GST_VERSION_MAJOR, GST_VERSION_MINOR, Lhest, "FIXME plugin description",
                  plugin_init, VERSION, "LGPL", PACKAGE_NAME, GST_PACKAGE_ORIGIN)

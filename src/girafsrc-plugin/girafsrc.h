/* GStreamer
 * Copyright (C) <2003> David A. Schleef <ds@schleef.org>
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

#ifndef __GIRAF_SRC_H__
#define __GIRAF_SRC_H__

#include <glib.h>

typedef struct _GstGirafSrc GstGirafSrc;

/**
 * GstGirafSrcPattern:
 * @GST_GIRAF_SRC_CHECKERS8: Checkers pattern (8px)
 *
 * The test pattern to produce.
 */
typedef enum {
  GST_GIRAF_SRC_CHECKERS8,
} GstGirafSrcPattern;

#include "gstgirafsrc.h"

struct BaseSrcImpl {
  GstGirafSrc *src;
  GstGLContext *context;
  GstVideoInfo v_info;
};

gboolean _src_checkers_init(gpointer impl, GstGLContext *context, GstVideoInfo *v_info);
void _src_checkers_free(gpointer impl);
gpointer _src_checkers8_new(GstGirafSrc *test);
gboolean _src_checkers_fill_bound_fbo(gpointer impl);

#endif

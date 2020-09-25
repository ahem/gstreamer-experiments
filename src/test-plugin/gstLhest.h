/* GStreamer
 * Copyright (C) 2020 Anders Hellerup Madsen
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

#ifndef _Ugst_Uhest_H_
#define _Ugst_Uhest_H_

#include <gst/base/gstbasesrc.h>
#include <gst/gl/gl.h>

G_BEGIN_DECLS

#define Ugst_TYPE_Uhest (gst_Lhest_get_type())
#define Ugst_Uhest(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), Ugst_TYPE_Uhest, UgstUhest))
#define Ugst_Uhest_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), Ugst_TYPE_Uhest, UgstUhestClass))
#define Ugst_IS_Uhest(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), Ugst_TYPE_Uhest))
#define Ugst_IS_Uhest_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE((klass), Ugst_TYPE_Uhest))

typedef struct _UgstUhest UgstUhest;
typedef struct _UgstUhestClass UgstUhestClass;

struct _UgstUhest {
  GstBaseSrc base_Lhest;
};

struct _UgstUhestClass {
  GstBaseSrcClass base_Lhest_class;
};

GType gst_Lhest_get_type(void);

G_END_DECLS

#endif

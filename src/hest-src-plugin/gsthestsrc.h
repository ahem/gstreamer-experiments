#ifndef _GST_HEST_SRC_H_
#define _GST_HEST_SRC_H_

#include <gst/gl/gl.h>
#include <gst/gst.h>

G_BEGIN_DECLS

#define GST_TYPE_HEST_SRC (gst_hest_src_get_type())
#define GST_HEST_SRC(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_HEST_SRC, GstHestSrc))
#define GST_HEST_SRC_CLASS(klass)                                                                  \
  (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_HEST_SRC, GstHestSrcClass))
#define GST_IS_HEST_SRC(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_HEST_SRC))
#define GST_IS_HEST_SRC_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_HEST_SRC))

typedef struct _GstHestSrc {
  GstGLBaseSrc element;
  GstGLFramebuffer *fbo;
} GstHestSrc;

typedef struct _GstHestSrcClass {
  GstGLBaseSrcClass parent_class;
} GstHestSrcClass;

GType gst_hest_src_get_type(void);

G_END_DECLS

#endif

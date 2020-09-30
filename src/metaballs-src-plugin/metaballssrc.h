#ifndef _GST_METABALLS_SRC_H_
#define _GST_METABALLS_SRC_H_

#include <gst/gl/gl.h>
#include <gst/gst.h>

G_BEGIN_DECLS

#define GST_TYPE_METABALLS_SRC (gst_metaballs_src_get_type())
#define GST_METABALLS_SRC(obj)                                                                     \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_METABALLS_SRC, GstMetaballsSrc))
#define GST_METABALLS_SRC_CLASS(klass)                                                             \
  (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_METABALLS_SRC, GstMetaballsSrcClass))
#define GST_IS_METABALLS_SRC(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_METABALLS_SRC))
#define GST_IS_METABALLS_SRC_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_METABALLS_SRC))

typedef struct _GstMetaballsSrc {
  GstGLBaseSrc element;
  GstGLFramebuffer *fbo;
  GstGLShader *shader;
  guint vao;
  guint positionBuffer;
} GstMetaballsSrc;

typedef struct _GstMetaballsSrcClass {
  GstGLBaseSrcClass parent_class;
} GstMetaballsSrcClass;

GType gst_metaballs_src_get_type(void);

G_END_DECLS

#endif

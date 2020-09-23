#include "memory"
#include <gst/gst.h>

struct Deleter {
  void operator()(GstElement *p) { gst_object_unref(p); }
  void operator()(GstBus *p) { gst_object_unref(p); }
  void operator()(GMainLoop *p) { g_main_loop_unref(p); }
};

auto main_loop = std::unique_ptr<GMainLoop, Deleter>(g_main_loop_new(nullptr, false));

int main(int argc, char *argv[]) {
  /* Initialize GStreamer */
  gst_init(&argc, &argv);

  /* Build the pipeline */
  auto pipeline = std::unique_ptr<GstElement, Deleter>(
      gst_parse_launch("playbin "
                       "uri=https://www.freedesktop.org/software/gstreamer-sdk/"
                       "data/media/sintel_trailer-480p.webm",
                       nullptr));

  /* Start playing */
  gst_element_set_state(pipeline.get(), GST_STATE_PLAYING);

  auto bus = std::unique_ptr<GstBus, Deleter>(gst_element_get_bus(pipeline.get()));

  gst_bus_add_watch(
      bus.get(),
      [](GstBus *bus, GstMessage *msg, void *user_data) -> int {
        if (msg->type != GST_MESSAGE_ERROR && msg->type != GST_MESSAGE_EOS) {
          return true;
        }
        g_main_loop_quit(main_loop.get());
        return false;
      },
      main_loop.get());

  g_main_loop_run(main_loop.get());
  gst_element_set_state(pipeline.get(), GST_STATE_NULL);

  return 0;
}

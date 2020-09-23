#include "memory"
#include <gst/gst.h>

struct Deleter {
  // clang-format off
  void operator()(GstElement *p) { if (p) gst_object_unref(p); }
  void operator()(GstBus *p) { if (p) gst_object_unref(p); }
  void operator()(GMainLoop *p) { if (p) g_main_loop_unref(p); }
  // clang-format on
};

auto main_loop = std::unique_ptr<GMainLoop, Deleter>(g_main_loop_new(nullptr, false));

int main(int argc, char *argv[]) {
  GstElement *source, *sink, *filter;
  GstStateChangeReturn ret;

  /* Initialize GStreamer */
  gst_init(&argc, &argv);

  /* Create the elements */
  source = gst_element_factory_make("videotestsrc", "source");
  sink = gst_element_factory_make("autovideosink", "sink");
  filter = gst_element_factory_make("vertigotv", "filter");

  /* Create the empty pipeline */
  auto pipeline = std::unique_ptr<GstElement, Deleter>(gst_pipeline_new("test-pipeline"));

  if (!pipeline || !source || !sink) {
    g_printerr("Not all elements could be created.\n");
    return -1;
  }

  /* Build the pipeline */
  gst_bin_add_many(GST_BIN(pipeline.get()), source, filter, sink, NULL);

  if (gst_element_link_many(source, filter, sink, nullptr) != TRUE) {
    g_printerr("Elements could not be linked.\n");
    return -1;
  }

  /* Modify the source's properties */
  g_object_set(source, "pattern", 0, NULL);

  /* Start playing */
  ret = gst_element_set_state(pipeline.get(), GST_STATE_PLAYING);
  if (ret == GST_STATE_CHANGE_FAILURE) {
    g_printerr("Unable to set the pipeline to the playing state.\n");
    return -1;
  }

  /* Wait until error or EOS */
  gst_bus_add_watch(
      std::unique_ptr<GstBus, Deleter>(gst_element_get_bus(pipeline.get())).get(),
      [](GstBus *bus, GstMessage *msg, void *user_data) -> int {
        switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_ERROR: {
          GError *err;
          gchar *debug_info;
          gst_message_parse_error(msg, &err, &debug_info);
          g_printerr("Error received from element %s: %s\n", GST_OBJECT_NAME(msg->src),
                     err->message);
          g_printerr("Debugging information: %s\n", debug_info ? debug_info : "none");
          g_clear_error(&err);
          g_free(debug_info);
          g_main_loop_quit(main_loop.get());
          return false;
        }
        case GST_MESSAGE_EOS:
          g_print("End-Of-Stream reached.\n");
          g_main_loop_quit(main_loop.get());
          return false;
        default:
          return true;
        }
      },
      main_loop.get());

  g_main_loop_run(main_loop.get());

  /* Free resources */
  gst_element_set_state(pipeline.get(), GST_STATE_NULL);
  return 0;
}

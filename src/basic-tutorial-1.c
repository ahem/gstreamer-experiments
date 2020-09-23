#include <gst/gst.h>

GMainLoop *main_loop; /* GLib's Main Loop */

/* Process messages from GStreamer */
static gboolean handle_message(GstBus *bus, GstMessage *msg, void *user_data) {
  switch (GST_MESSAGE_TYPE(msg)) {
  case GST_MESSAGE_ERROR: {
    g_print("Error!!!\n");
    g_main_loop_quit(main_loop);
    return FALSE;
  }
  case GST_MESSAGE_EOS:
    g_print("End-Of-Stream reached.\n");
    g_main_loop_quit(main_loop);
    return FALSE;
  default:
    return TRUE;
  }
}

int main(int argc, char *argv[]) {
  GstElement *pipeline;
  GstBus *bus;

  /* Initialize GStreamer */
  gst_init(&argc, &argv);

  /* Build the pipeline */
  pipeline = gst_parse_launch(
      "playbin "
      "uri=https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm",
      NULL);

  /* Start playing */
  gst_element_set_state(pipeline, GST_STATE_PLAYING);

  /* Wait until error or EOS */
  bus = gst_element_get_bus(pipeline);
  gst_bus_add_watch(bus, (GstBusFunc)handle_message, NULL);

  main_loop = g_main_loop_new(NULL, FALSE);
  g_main_loop_run(main_loop);

  /* Free resources */
  g_main_loop_unref(main_loop);
  gst_object_unref(bus);
  gst_element_set_state(pipeline, GST_STATE_NULL);
  gst_object_unref(pipeline);
  return 0;
}

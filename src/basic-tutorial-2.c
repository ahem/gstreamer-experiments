#include <gst/gst.h>

GMainLoop *main_loop; /* GLib's Main Loop */

static gboolean handle_message(GstBus *bus, GstMessage *msg, void *user_data) {
  switch (GST_MESSAGE_TYPE(msg)) {
  case GST_MESSAGE_ERROR: {
    GError *err;
    gchar *debug_info;
    gst_message_parse_error(msg, &err, &debug_info);
    g_printerr("Error received from element %s: %s\n", GST_OBJECT_NAME(msg->src), err->message);
    g_printerr("Debugging information: %s\n", debug_info ? debug_info : "none");
    g_clear_error(&err);
    g_free(debug_info);
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
  GstElement *pipeline, *source, *sink, *filter;
  GstBus *bus;
  GstStateChangeReturn ret;

  /* Initialize GStreamer */
  gst_init(&argc, &argv);

  /* Create the elements */
  source = gst_element_factory_make("videotestsrc", "source");
  sink = gst_element_factory_make("autovideosink", "sink");
  filter = gst_element_factory_make("vertigotv", "filter");

  /* Create the empty pipeline */
  pipeline = gst_pipeline_new("test-pipeline");

  if (!pipeline || !source || !sink) {
    g_printerr("Not all elements could be created.\n");
    return -1;
  }

  /* Build the pipeline */
  gst_bin_add_many(GST_BIN(pipeline), source, filter, sink, NULL);

  if (gst_element_link(source, filter) != TRUE) {
    g_printerr("Elements (source, filter) could not be linked.\n");
    gst_object_unref(pipeline);
    return -1;
  }
  if (gst_element_link(filter, sink) != TRUE) {
    g_printerr("Elements (filter, sink) could not be linked.\n");
    gst_object_unref(pipeline);
    return -1;
  }

  /* Modify the source's properties */
  g_object_set(source, "pattern", 0, NULL);

  /* Start playing */
  ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
  if (ret == GST_STATE_CHANGE_FAILURE) {
    g_printerr("Unable to set the pipeline to the playing state.\n");
    gst_object_unref(pipeline);
    return -1;
  }

  /* Wait until error or EOS */
  bus = gst_element_get_bus(pipeline);
  gst_bus_add_watch(bus, (GstBusFunc)handle_message, NULL);

  main_loop = g_main_loop_new(NULL, FALSE);
  g_main_loop_run(main_loop);

  /* Free resources */
  gst_object_unref(bus);
  gst_element_set_state(pipeline, GST_STATE_NULL);
  gst_object_unref(pipeline);
  return 0;
}

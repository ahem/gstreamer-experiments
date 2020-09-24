#include <gst/gst.h>

GMainLoop *main_loop; /* GLib's Main Loop */

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
  GstElement *pipeline, *audio_source, *tee, *audio_queue, *audio_convert, *audio_resample,
      *audio_sink;
  GstElement *video_queue, *visual, *video_convert, *video_sink;
  GstBus *bus;
  GstPad *tee_audio_pad, *tee_video_pad;
  GstPad *queue_audio_pad, *queue_video_pad;

  /* Initialize GStreamer */
  gst_init(&argc, &argv);

  /* Create the elements */
  audio_source = gst_element_factory_make("audiotestsrc", "audio_source");
  tee = gst_element_factory_make("tee", "tee");
  audio_queue = gst_element_factory_make("queue", "audio_queue");
  audio_convert = gst_element_factory_make("audioconvert", "audio_convert");
  audio_resample = gst_element_factory_make("audioresample", "audio_resample");
  audio_sink = gst_element_factory_make("autoaudiosink", "audio_sink");
  video_queue = gst_element_factory_make("queue", "video_queue");
  visual = gst_element_factory_make("wavescope", "visual");
  video_convert = gst_element_factory_make("videoconvert", "csp");
  video_sink = gst_element_factory_make("autovideosink", "video_sink");

  /* Create the empty pipeline */
  pipeline = gst_pipeline_new("test-pipeline");

  if (!pipeline || !audio_source || !tee || !audio_queue || !audio_convert || !audio_resample ||
      !audio_sink || !video_queue || !visual || !video_convert || !video_sink) {
    g_printerr("Not all elements could be created."
               "Run with environment variable GDB_DEBUG=2 for more info\n");
    return -1;
  }

  /* Configure elements */
  g_object_set(audio_source, "freq", 215.0f, NULL);
  g_object_set(visual, "shader", 0, "style", 1, NULL);

  /* Link all elements that can be automatically linked because they have "Always" pads */
  gst_bin_add_many(GST_BIN(pipeline), audio_source, tee, audio_queue, audio_convert, audio_resample,
                   audio_sink, video_queue, visual, video_convert, video_sink, NULL);
  if (gst_element_link_many(audio_source, tee, NULL) != TRUE ||
      gst_element_link_many(audio_queue, audio_convert, audio_resample, audio_sink, NULL) != TRUE ||
      gst_element_link_many(video_queue, visual, video_convert, video_sink, NULL) != TRUE) {
    g_printerr("Elements could not be linked.\n");
    gst_object_unref(pipeline);
    return -1;
  }

  /* Manually link the Tee, which has "Request" pads */
  tee_audio_pad = gst_element_get_request_pad(tee, "src_%u");
  g_print("Obtained request pad %s for audio branch.\n", gst_pad_get_name(tee_audio_pad));
  queue_audio_pad = gst_element_get_static_pad(audio_queue, "sink");
  tee_video_pad = gst_element_get_request_pad(tee, "src_%u");
  g_print("Obtained request pad %s for video branch.\n", gst_pad_get_name(tee_video_pad));
  queue_video_pad = gst_element_get_static_pad(video_queue, "sink");
  if (gst_pad_link(tee_audio_pad, queue_audio_pad) != GST_PAD_LINK_OK ||
      gst_pad_link(tee_video_pad, queue_video_pad) != GST_PAD_LINK_OK) {
    g_printerr("Tee could not be linked.\n");
    gst_object_unref(pipeline);
    return -1;
  }
  gst_object_unref(queue_audio_pad);
  gst_object_unref(queue_video_pad);

  /* Start playing the pipeline */
  gst_element_set_state(pipeline, GST_STATE_PLAYING);

  /* Wait until error or EOS */
  bus = gst_element_get_bus(pipeline);
  gst_bus_add_watch(bus, (GstBusFunc)handle_message, NULL);

  main_loop = g_main_loop_new(NULL, FALSE);
  g_main_loop_run(main_loop);

  /* Release the request pads from the Tee, and unref them */
  gst_element_release_request_pad(tee, tee_audio_pad);
  gst_element_release_request_pad(tee, tee_video_pad);
  gst_object_unref(tee_audio_pad);
  gst_object_unref(tee_video_pad);

  /* Free resources */
  gst_object_unref(bus);
  gst_element_set_state(pipeline, GST_STATE_NULL);

  gst_object_unref(pipeline);
  return 0;
}

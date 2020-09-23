#include <gst/gst.h>

/* Structure to contain all our information, so we can pass it to callbacks */
typedef struct _CustomData {
  GstElement *pipeline;
  GstElement *source;
  GstElement *audioConvert;
  GstElement *audioResample;
  GstElement *audioSink;

  GstElement *videoConvert;
  GstElement *videoSink;

  GMainLoop *main_loop;
} CustomData;

/* Forward declarations */
static void pad_added_handler(GstElement *src, GstPad *pad, CustomData *data);
static gboolean message_handler(GstBus *bus, GstMessage *msg, CustomData *data);

int main(int argc, char *argv[]) {
  CustomData data;
  GstBus *bus;
  GstMessage *msg;
  GstStateChangeReturn ret;
  gboolean terminate = FALSE;

  /* Initialize GStreamer */
  gst_init(&argc, &argv);

  /* Create the elements */
  data.source = gst_element_factory_make("uridecodebin", "source");
  data.audioConvert = gst_element_factory_make("audioconvert", "audioConvert");
  data.audioResample = gst_element_factory_make("audioresample", "audioResample");
  data.audioSink = gst_element_factory_make("autoaudiosink", "audioSink");

  data.videoConvert = gst_element_factory_make("videoconvert", "videoConvert");
  data.videoSink = gst_element_factory_make("autovideosink", "videoSink");

  /* Create the empty pipeline */
  data.pipeline = gst_pipeline_new("test-pipeline");

  if (!data.pipeline || !data.source || !data.audioConvert || !data.audioResample ||
      !data.audioSink || !data.videoConvert || !data.videoSink) {
    g_printerr("Not all elements could be created.\n");
    return -1;
  }

  /* Build the pipeline. Note that we are NOT linking the source at this
   * point. We will do it later. */
  gst_bin_add_many(GST_BIN(data.pipeline), data.source, data.audioConvert, data.audioResample,
                   data.audioSink, data.videoConvert, data.videoSink, NULL);

  if (!gst_element_link_many(data.audioConvert, data.audioResample, data.audioSink, NULL)) {
    g_printerr("Audio elements could not be linked.\n");
    gst_object_unref(data.pipeline);
    return -1;
  }

  if (!gst_element_link_many(data.videoConvert, data.videoSink, NULL)) {
    g_printerr("Video elements could not be linked.\n");
    gst_object_unref(data.pipeline);
    return -1;
  }

  /* Set the URI to play */
  g_object_set(
      data.source, "uri",
      "https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm",
      NULL);

  /* Connect to the pad-added signal */
  g_signal_connect(data.source, "pad-added", G_CALLBACK(pad_added_handler), &data);

  /* Start playing */
  ret = gst_element_set_state(data.pipeline, GST_STATE_PLAYING);
  if (ret == GST_STATE_CHANGE_FAILURE) {
    g_printerr("Unable to set the pipeline to the playing state.\n");
    gst_object_unref(data.pipeline);
    return -1;
  }

  /* Listen to the bus */
  bus = gst_element_get_bus(data.pipeline);
  gst_bus_add_watch(bus, (GstBusFunc)message_handler, &data);

  /* create the main loop */
  data.main_loop = g_main_loop_new(NULL, FALSE);
  g_main_loop_run(data.main_loop);

  /* Free resources */
  g_main_loop_unref(data.main_loop);
  gst_object_unref(bus);
  gst_element_set_state(data.pipeline, GST_STATE_NULL);
  gst_object_unref(data.pipeline);
  return 0;
}

static void pad_added_handler(GstElement *src, GstPad *new_pad, CustomData *data) {
  g_print("Received new pad '%s' from '%s':\n", GST_PAD_NAME(new_pad), GST_ELEMENT_NAME(src));

  GstCaps *new_pad_caps = gst_pad_get_current_caps(new_pad);
  GstStructure *new_pad_struct = gst_caps_get_structure(new_pad_caps, 0);
  const gchar *new_pad_type = gst_structure_get_name(new_pad_struct);

  new_pad_caps = gst_pad_get_current_caps(new_pad);
  if (new_pad_caps != NULL) {
    new_pad_struct = gst_caps_get_structure(new_pad_caps, 0);
    new_pad_type = gst_structure_get_name(new_pad_struct);

    gboolean is_audio_pad = g_str_has_prefix(new_pad_type, "audio/x-raw");
    gboolean is_video_pad = g_str_has_prefix(new_pad_type, "video/x-raw");

    if (is_audio_pad || is_video_pad) {
      GstElement *element = is_audio_pad ? data->audioConvert : data->videoConvert;
      GstPad *sink_pad = gst_element_get_static_pad(element, "sink");

      if (!gst_pad_is_linked(sink_pad)) {
        if (gst_pad_link(new_pad, sink_pad) == GST_PAD_LINK_OK) {
          g_print("Link succeeded (type '%s').\n", new_pad_type);
        } else {
          g_print("Type is '%s' but link failed.\n", new_pad_type);
        }
      } else {
        g_print("We are already linked. Ignoring.\n");
      }

      gst_object_unref(sink_pad);
    }

    gst_caps_unref(new_pad_caps);
  }
}

static gboolean message_handler(GstBus *bus, GstMessage *msg, CustomData *data) {
  GError *err;
  gchar *debug_info;

  switch (GST_MESSAGE_TYPE(msg)) {
  case GST_MESSAGE_ERROR:
    gst_message_parse_error(msg, &err, &debug_info);
    g_printerr("Error received from element %s: %s\n", GST_OBJECT_NAME(msg->src), err->message);
    g_printerr("Debugging information: %s\n", debug_info ? debug_info : "none");
    g_clear_error(&err);
    g_free(debug_info);
    g_main_loop_quit(data->main_loop);
    return FALSE;
  case GST_MESSAGE_EOS:
    g_print("End-Of-Stream reached.\n");
    g_main_loop_quit(data->main_loop);
    return FALSE;
  case GST_MESSAGE_STATE_CHANGED:
    /* We are only interested in state-changed messages from the pipeline */
    if (GST_MESSAGE_SRC(msg) == GST_OBJECT(data->pipeline)) {
      GstState old_state, new_state, pending_state;
      gst_message_parse_state_changed(msg, &old_state, &new_state, &pending_state);
      g_print("Pipeline state changed from %s to %s:\n", gst_element_state_get_name(old_state),
              gst_element_state_get_name(new_state));
    }
    break;
  default:
    break;
  }
  return TRUE;
}

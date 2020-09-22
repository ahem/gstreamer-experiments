# GStreamer tutorials

## Making Basic Tutorials run on OSX

The basic tutorials use helper funtions to run the main loop and retrieve
messages from it (fx. `gst_bus_timed_pop_filtered`). This doesn't work on OSX.
The required change is to remove the call to `gst_bus_timed_pop_filtered` and
instead add real message handling with `gst_bus_add_watch`.

Before:

```
bus = gst_element_get_bus (pipeline);
msg =
  gst_bus_timed_pop_filtered (bus, GST_CLOCK_TIME_NONE,
  GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

/* Free resources */
  if (msg != NULL)
    gst_message_unref (msg);
  gst_object_unref (bus);
```

After

```
static gboolean handle_message (GstBus *bus, GstMessage *msg, void *user_data) {
  switch (GST_MESSAGE_TYPE (msg)) {
    case GST_MESSAGE_ERROR: {
      g_main_loop_quit (main_loop);
      break;
    }
    case GST_MESSAGE_EOS:
      g_main_loop_quit (main_loop);
      break;
    default:
      break;
  }
  return TRUE;
}

bus = gst_element_get_bus (pipeline);
gst_bus_add_watch (bus, (GstBusFunc)handle_message, NULL);

main_loop = g_main_loop_new (NULL, FALSE);
g_main_loop_run (main_loop);

/* Free resources */
g_main_loop_unref (main_loop);
gst_object_unref (bus);
```

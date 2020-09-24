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

/* playbin flags */
typedef enum {
  GST_PLAY_FLAG_VIS =
      (1 << 3) /* Enable rendering of visualizations when there is no video stream. */
} GstPlayFlags;

/* Return TRUE if this is a Visualization element */
static gboolean filter_vis_features(GstPluginFeature *feature, gpointer data) {
  GstElementFactory *factory;

  if (!GST_IS_ELEMENT_FACTORY(feature))
    return FALSE;
  factory = GST_ELEMENT_FACTORY(feature);
  if (!g_strrstr(gst_element_factory_get_klass(factory), "Visualization"))
    return FALSE;

  return TRUE;
}

int main(int argc, char *argv[]) {
  GstElement *pipeline, *vis_plugin;
  GstBus *bus;
  GList *list, *walk;
  GstElementFactory *selected_factory = NULL;
  guint flags;

  /* Initialize GStreamer */
  gst_init(&argc, &argv);

  /* Get a list of all visualization plugins */
  list = gst_registry_feature_filter(gst_registry_get(), filter_vis_features, FALSE, NULL);

  /* Print their names */
  g_print("Available visualization plugins:\n");
  for (walk = list; walk != NULL; walk = g_list_next(walk)) {
    const gchar *name;
    GstElementFactory *factory;

    factory = GST_ELEMENT_FACTORY(walk->data);
    name = gst_element_factory_get_longname(factory);
    g_print("  %s\n", name);

    if (selected_factory == NULL || g_str_has_prefix(name, "GOOM")) {
      selected_factory = factory;
    }
  }

  /* Don't use the factory if it's still empty */
  /* e.g. no visualization plugins found */
  if (!selected_factory) {
    g_print("No visualization plugins found!\n");
    return -1;
  }

  /* We have now selected a factory for the visualization element */
  g_print("Selected '%s'\n", gst_element_factory_get_longname(selected_factory));
  vis_plugin = gst_element_factory_create(selected_factory, NULL);
  if (!vis_plugin)
    return -1;

  /* Build the pipeline */
  pipeline = gst_parse_launch("playbin uri=http://radio.hbr1.com:19800/ambient.ogg", NULL);

  /* Set the visualization flag */
  g_object_get(pipeline, "flags", &flags, NULL);
  flags |= GST_PLAY_FLAG_VIS;
  g_object_set(pipeline, "flags", flags, NULL);

  /* set vis plugin for playbin */
  g_object_set(pipeline, "vis-plugin", vis_plugin, NULL);

  /* Start playing */
  gst_element_set_state(pipeline, GST_STATE_PLAYING);

  /* Wait until error or EOS */
  bus = gst_element_get_bus(pipeline);
  gst_bus_add_watch(bus, (GstBusFunc)handle_message, NULL);

  main_loop = g_main_loop_new(NULL, FALSE);
  g_main_loop_run(main_loop);

  /* Free resources */
  g_main_loop_unref(main_loop);
  gst_plugin_feature_list_free(list);
  gst_object_unref(bus);
  gst_element_set_state(pipeline, GST_STATE_NULL);
  gst_object_unref(pipeline);
  return 0;
}

#include <glibmm/main.h>
#include <gstreamermm.h>

Glib::RefPtr<Glib::MainLoop> mainloop;

bool message_handler(const Glib::RefPtr<Gst::Bus> &, const Glib::RefPtr<Gst::Message> &message);

int main(int argc, char **argv) {
  Gst::init(argc, argv);

  auto pipeline = Gst::Pipeline::create();

  auto source = Gst::ElementFactory::create_element("videotestsrc", "source");
  auto sink = Gst::ElementFactory::create_element("autovideosink", "sink");
  auto filter = Gst::ElementFactory::create_element("vertigotv", "filter");

  try {
    pipeline->add(source)->add(sink)->add(filter);
  } catch (std::runtime_error &ex) {
    g_error("exception while adding: %s\n", ex.what());
    return EXIT_FAILURE;
  }

  try {
    source->link(filter)->link(sink);
  } catch (std::runtime_error &ex) {
    g_error("exception while linking: %s\n", ex.what());
    return EXIT_FAILURE;
  }

  source->set_property("pattern", 0);

  pipeline->set_state(Gst::STATE_PLAYING);
  pipeline->get_bus()->add_watch(&message_handler);

  mainloop = Glib::MainLoop::create();
  mainloop->run();

  // cleanup
  pipeline->set_state(Gst::STATE_NULL);

  return EXIT_SUCCESS;
}

bool message_handler(const Glib::RefPtr<Gst::Bus> &, const Glib::RefPtr<Gst::Message> &message) {
  switch (message->get_message_type()) {
  case Gst::MESSAGE_ERROR: {
    auto error = Glib::RefPtr<Gst::MessageError>::cast_static(message);
    g_print("Error: %s\n", error ? error->parse_error().what().c_str() : "unknown");
    mainloop->quit();
    return false;
  }
  case Gst::MESSAGE_EOS:
    g_print("End of stream.\n");
    mainloop->quit();
    return false;
  default:
    return true;
  }
}

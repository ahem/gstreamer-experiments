#include <glibmm/main.h>
#include <gstreamermm.h>

Glib::RefPtr<Glib::MainLoop> mainloop;

bool message_handler(const Glib::RefPtr<Gst::Bus> &, const Glib::RefPtr<Gst::Message> &message);

int main(int argc, char **argv) {
  Gst::init(argc, argv);

  auto pipeline = Gst::Parse::launch(
      "playbin "
      "uri=https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm");

  pipeline->set_state(Gst::STATE_PLAYING);
  pipeline->get_bus()->add_watch(&message_handler);

  mainloop = Glib::MainLoop::create();
  mainloop->run();

  // cleanup
  pipeline->set_state(Gst::STATE_NULL);

  return EXIT_SUCCESS;
}

bool message_handler(const Glib::RefPtr<Gst::Bus> &, const Glib::RefPtr<Gst::Message> &message) {
  auto message_type = message->get_message_type();
  if (message_type == Gst::MESSAGE_EOS || message_type == Gst::MESSAGE_ERROR) {
    g_print(message_type == Gst::MESSAGE_EOS ? "End of stream\n" : "Error!\n");
    mainloop->quit();
    return false;
  }
  return true;
}

# Custom test plugin

## Notes

I had to clone gst-plugins-bad to get the `gst-element-maker` tool. I then created the skeleton files with:

```
PATH="/usr/local/opt/coreutils/libexec/gnubin:$PATH" ~/Projects/gst-plugins-bad/tools/gst-element-maker hest basesrc
```

The PATH stuff is because the script requires GNU coreutils.

This created `gstLhest.c`, `gstLhest.h`, `gstLhest.o` and `gstLhest.so`. I don't know what is up with the uppercase L.

The plugin is now discoverable by gstreamer:

```
GST_PLUGIN_PATH=$(pwd) gst-inspect-1.0 Lhest
```

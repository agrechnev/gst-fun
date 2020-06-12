#include <iostream>
#include <sstream>

#include <gtk/gtk.h>
#include <gst/gst.h>
#include <gst/video/videooverlay.h>

#include <gdk/gdk.h>

#if defined (GDK_WINDOWING_X11)

#include <gdk/gdkx.h>

#elif defined (GDK_WINDOWING_WIN32)
#include <gdk/gdkwin32.h>
#elif defined (GDK_WINDOWING_QUARTZ)
#include <gdk/gdkquartz.h>
#endif

struct GoblinData {
    /// THe pipeline
    GstElement *playBin = nullptr;
    GtkWidget *streamInfo = nullptr;
    GtkWidget *slider = nullptr;
    GstState state = GST_STATE_VOID_PENDING;
    gint64 duration = 0;
};

//======================================================================================================================
// GTK+ callbacks
//======================================================================================================================
static void cbPlay(GtkButton *button, GoblinData *data) {
    using namespace std;
//    cout << "PLAY !!!" << endl;
    gst_element_set_state(data->playBin, GST_STATE_PLAYING);
}

//======================================================================================================================
static void cbPause(GtkButton *button, GoblinData *data) {
    using namespace std;
//    cout << "PAUSE !!!" << endl;
    gst_element_set_state(data->playBin, GST_STATE_PAUSED);
}

//======================================================================================================================
static void cbStop(GtkButton *button, GoblinData *data) {
    using namespace std;
//    cout << "STOP !!!" << endl;
    gst_element_set_state(data->playBin, GST_STATE_READY);
}

//======================================================================================================================
static void cbDelEvent(GtkWidget *widget, GdkEvent *event, GoblinData *data) {
    using namespace std;
//    cout << "DEL EVENT !!!" << endl;
    cbStop(nullptr, data);
    gtk_main_quit();
}

//======================================================================================================================
static void cbRealize(GtkWidget *widget, GoblinData *data) {
    using namespace std;
//    cout << "REALIZE !!!" << endl;
    GdkWindow *window = gtk_widget_get_window(widget);
    guintptr handle;
    if (!gdk_window_ensure_native(window))
        throw runtime_error("Cannot create native window !");

    // Get handle
#if defined (GDK_WINDOWING_WIN32)
    handle = (guintptr)GDK_WINDOW_HWND (window);
#elif defined (GDK_WINDOWING_QUARTZ)
    handle = gdk_quartz_window_get_nsview (window);
#elif defined (GDK_WINDOWING_X11)
    handle = GDK_WINDOW_XID (window);
#endif

    gst_video_overlay_set_window_handle(GST_VIDEO_OVERLAY(data->playBin), handle);
}

//======================================================================================================================
static gboolean cbDraw(GtkWidget *widget, cairo_t *cr, GoblinData *data) {
    using namespace std;
    if (data->state == GST_STATE_PAUSED) {
        GtkAllocation allocation;
        gtk_widget_get_allocation(widget, &allocation);
        cairo_set_source_rgb(cr, 0, 0, 0);
        cairo_rectangle(cr, 0, 0, allocation.width, allocation.height);
        cairo_fill(cr);
    }
    return FALSE;
}
//======================================================================================================================
// Create the GTK+ UI
static void createUI(GoblinData *data) {
    GtkWidget *mainWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    g_signal_connect(G_OBJECT(mainWindow), "delete-event", G_CALLBACK(cbDelEvent), data);

    gtk_window_set_default_size(GTK_WINDOW(mainWindow), 800, 600);
    gtk_window_set_title(GTK_WINDOW(mainWindow), "Goblin window !");

    // Video panel
    GtkWidget *videoWindow = gtk_drawing_area_new();
    gtk_widget_set_double_buffered(videoWindow, FALSE);
    g_signal_connect(G_OBJECT(videoWindow), "realize", G_CALLBACK(cbRealize), data);
    g_signal_connect(G_OBJECT(videoWindow), "draw", G_CALLBACK(cbDraw), data);

    // Buttons
    GtkWidget *btnPlay = gtk_button_new_from_icon_name("media-playback-start", GTK_ICON_SIZE_LARGE_TOOLBAR);
    g_signal_connect(G_OBJECT(btnPlay), "clicked", G_CALLBACK(cbPlay), data);
    GtkWidget *btnPause = gtk_button_new_from_icon_name("media-playback-pause", GTK_ICON_SIZE_LARGE_TOOLBAR);
    g_signal_connect(G_OBJECT(btnPause), "clicked", G_CALLBACK(cbPause), data);
    GtkWidget *btnStop = gtk_button_new_from_icon_name("media-playback-stop", GTK_ICON_SIZE_LARGE_TOOLBAR);
    g_signal_connect(G_OBJECT(btnStop), "clicked", G_CALLBACK(cbStop), data);

    // Stream info
    data->streamInfo = gtk_text_view_new();

    // Slider
    data->slider = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
    gtk_scale_set_draw_value(GTK_SCALE(data->slider), TRUE);


    // Layout
    GtkWidget *boxVideo = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(boxVideo), videoWindow, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(boxVideo), data->streamInfo, FALSE, FALSE, 2);

    GtkWidget *boxControls = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(boxControls), btnPlay, FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(boxControls), btnPause, FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(boxControls), btnStop, FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(boxControls), data->slider, TRUE, TRUE, 2);

    GtkWidget *boxMain = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start(GTK_BOX(boxMain), boxVideo, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(boxMain), boxControls, FALSE, FALSE, 0);


    gtk_container_add(GTK_CONTAINER(mainWindow), boxMain);
    gtk_widget_show_all(mainWindow);
}

//======================================================================================================================
// This is called about every second or so
static gboolean refreshUI(GoblinData *data) {
    using namespace std;
//    cout << "Refresh " << endl;
    if (data->state < GST_STATE_PAUSED)
        return TRUE;

    if (data->duration == 0) {
        if (!gst_element_query_duration(data->playBin, GST_FORMAT_TIME, &data->duration)) {
            cerr << "Cannot query duration !" << endl;
        } else {
            gdouble d = gdouble(data->duration) / GST_SECOND;
            cout << "duration = " << d << endl;
            gtk_range_set_range(GTK_RANGE(data->slider), 0, d);
        }
    } else {
        // Update current pos
        gint64 current;
        if (gst_element_query_position(data->playBin, GST_FORMAT_TIME, &current)) {
            gtk_range_set_value(GTK_RANGE(data->slider), gdouble(current) / GST_SECOND);
        }
    }



    return TRUE;
}
//======================================================================================================================
static void analyzeStreams(GoblinData *data) {
    using namespace std;
//    cout << "analyzeStreams !!!" << endl;

    GtkTextBuffer *text = gtk_text_view_get_buffer(GTK_TEXT_VIEW(data->streamInfo));
    gtk_text_buffer_set_text(text, "", -1);

    // Read some props
    gint nVideo, nAudio, nText;
    g_object_get(data->playBin, "n-video", &nVideo, nullptr);
    g_object_get(data->playBin, "n-audio", &nAudio, nullptr);
    g_object_get(data->playBin, "n-text", &nText, nullptr);

//    cout << "nVideo = " << nVideo << " , nAudio = " << nAudio << " , nText = " << nText << endl;
    ostringstream oss;

    // Video
    for (int i = 0; i < nVideo; ++i) {
        GstTagList *tags;
        g_signal_emit_by_name(data->playBin, "get_video_tags", i, &tags);
        if (tags) {
            oss << "Video stream " << i << endl;
            gchar *str;
            gst_tag_list_get_string(tags, GST_TAG_VIDEO_CODEC, &str);
            oss << "codec = " << str << endl << endl;
            g_free(str);
        }
    }

    // Audio
    for (int i = 0; i < nAudio; ++i) {
        GstTagList *tags;
        g_signal_emit_by_name(data->playBin, "get_audio_tags", i, &tags);
        if (tags) {
            oss << "Audio stream " << i << endl;
            gchar *str;
            gst_tag_list_get_string(tags, GST_TAG_AUDIO_CODEC, &str);
            oss << "codec = " << str << endl;
            g_free(str);

            guint brate;
            if (gst_tag_list_get_uint(tags, GST_TAG_BITRATE, &brate)) {
                oss << "bitrate = " << brate << endl;
            }
            oss << endl;
        }
    }

    // No text, who cares !
    gtk_text_buffer_set_text(text, oss.str().c_str(), -1);
}
//======================================================================================================================
//  GST callbacks
//======================================================================================================================
static void cbApplication(GstBus *bus, GstMessage *msg, GoblinData *data) {

    if (g_strcmp0(gst_structure_get_name(gst_message_get_structure(msg)), "tags-changed") == 0)
        analyzeStreams(data);
}

//======================================================================================================================
static void cbStateChanged(GstBus *bus, GstMessage *msg, GoblinData * data){
    using namespace std;
    GstState sOld, sNew, sPen;
    if (GST_MESSAGE_SRC(msg) == GST_OBJECT(data->playBin)) {
        gst_message_parse_state_changed(msg, &sOld, &sNew, &sPen);
        cout << "State changed to " << gst_element_state_get_name(sNew) << endl;
        data->state = sNew;
    }
}
//======================================================================================================================
static void cbError(GstBus *bus, GstMessage *msg, GoblinData *data) {
    using namespace std;
    GError *err;
    gchar *deb;
    gst_message_parse_error(msg, &err, &deb);
    cout << "error = " << err->message << endl;
    cout << "debug_info = " << deb << endl;
    g_clear_error(&err);
    g_free(deb);
}
//======================================================================================================================
static void cbEOS(GstBus *bus, GstMessage *msg, GoblinData *data) {
    using namespace std;
    cout << "EOS reached !" << endl;
    gst_element_set_state(data->playBin, GST_STATE_READY);
}
//======================================================================================================================
static void cbTags(GstElement *playbin, gint stream, GoblinData *data) {
    using namespace std;
    gst_element_post_message(playbin,
                             gst_message_new_application(GST_OBJECT(playbin), gst_structure_new_empty("tags-changed")));
}

//======================================================================================================================
int main(int argc, char **argv) {
    using namespace std;
    cout << "GOBLINS WON !!!" << endl;
    GoblinData data;

    // Init GTK
    gtk_init(&argc, &argv);

    // Init gstreamer
    gst_init(&argc, &argv);

    // Create the gst pipeline
    data.playBin = gst_element_factory_make("playbin", "playbin");
    if (!data.playBin)
        throw runtime_error("Cannot create playbin !");
    g_object_set(data.playBin, "uri", "file:///home/seymour/Videos/tvoya.mp4", nullptr);
    g_signal_connect(G_OBJECT(data.playBin), "video-tags-changed", (GCallback) cbTags, &data);
    g_signal_connect(G_OBJECT(data.playBin), "audio-tags-changed", (GCallback) cbTags, &data);
    g_signal_connect(G_OBJECT(data.playBin), "text-tags-changed", (GCallback) cbTags, &data);

    // Create the UI
    createUI(&data);

    // Instruct GST bus to emit messages
    GstBus *bus = gst_element_get_bus(data.playBin);
    gst_bus_add_signal_watch(bus);
    g_signal_connect(G_OBJECT(bus), "message::application", G_CALLBACK(cbApplication), &data);
    g_signal_connect(G_OBJECT(bus), "message::state-changed", G_CALLBACK(cbStateChanged), &data);
    g_signal_connect(G_OBJECT(bus), "message::error", G_CALLBACK(cbError), &data);
    g_signal_connect(G_OBJECT(bus), "message::eos", G_CALLBACK(cbEOS), &data);

    // Start playing the video
    GstStateChangeReturn ret = gst_element_set_state(data.playBin, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE)
        throw runtime_error("Cannot start playing !");

    // Register UI refresh
    g_timeout_add_seconds(1, (GSourceFunc)refreshUI, &data);

    // GTK main loop
    gtk_main();

    return 0;
}
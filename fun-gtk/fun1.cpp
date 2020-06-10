#include <iostream>

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

struct GoblinData{
    /// THe pipeline
    GstElement *playBin;
};


//======================================================================================================================
static void cbPlay(GtkButton *button, GoblinData * data) {
    using namespace std;
    cout << "PLAY !!!" << endl;
    gst_element_set_state(data->playBin, GST_STATE_PLAYING);
}

//======================================================================================================================
static void cbPause(GtkButton *button, GoblinData * data) {
    using namespace std;
    cout << "PAUSE !!!" << endl;
    gst_element_set_state(data->playBin, GST_STATE_PAUSED);
}
//======================================================================================================================
static void cbStop(GtkButton *button, GoblinData * data) {
    using namespace std;
    cout << "STOP !!!" << endl;
    gst_element_set_state(data->playBin, GST_STATE_READY);
}
//======================================================================================================================
static void cbDelEvent(GtkWidget *widget, GdkEvent *event, GoblinData * data) {
    using namespace std;
    cout << "DEL EVENT !!!" << endl;
    cbStop(nullptr, data);
    gtk_main_quit();
}
//======================================================================================================================
static void cbRealize(GtkWidget *widget, GoblinData * data) {
    using namespace std;
    cout << "REALIZE !!!" << endl;
    GdkWindow * window = gtk_widget_get_window(widget);
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
// Create the GTK+ UI
static void createUI(GoblinData * data){
    GtkWidget *mainWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    g_signal_connect(G_OBJECT(mainWindow), "delete-event", G_CALLBACK(cbDelEvent), data);

    gtk_window_set_default_size(GTK_WINDOW(mainWindow), 800, 600);
    gtk_window_set_title(GTK_WINDOW(mainWindow), "Goblin window !");

    // Video panel
    GtkWidget * videoWindow = gtk_drawing_area_new();
    gtk_widget_set_double_buffered(videoWindow, FALSE);
    g_signal_connect(G_OBJECT(videoWindow), "realize", G_CALLBACK(cbRealize), data);

    // Buttons
    GtkWidget * btnPlay = gtk_button_new_from_icon_name("media-playback-start", GTK_ICON_SIZE_LARGE_TOOLBAR);
    g_signal_connect(G_OBJECT(btnPlay), "clicked", G_CALLBACK(cbPlay), data);
    GtkWidget * btnPause = gtk_button_new_from_icon_name("media-playback-pause", GTK_ICON_SIZE_LARGE_TOOLBAR);
    g_signal_connect(G_OBJECT(btnPause), "clicked", G_CALLBACK(cbPause), data);
    GtkWidget * btnStop = gtk_button_new_from_icon_name("media-playback-stop", GTK_ICON_SIZE_LARGE_TOOLBAR);
    g_signal_connect(G_OBJECT(btnStop), "clicked", G_CALLBACK(cbStop), data);

    // Layout
    GtkWidget *boxControls = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(boxControls), btnPlay, FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(boxControls), btnPause, FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(boxControls), btnStop, FALSE, FALSE, 2);

    GtkWidget *boxMain = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start(GTK_BOX(boxMain), videoWindow, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(boxMain), boxControls, FALSE, FALSE, 0);


    gtk_container_add(GTK_CONTAINER(mainWindow), boxMain);
    gtk_widget_show_all(mainWindow);
}
//======================================================================================================================
int main(int argc, char ** argv){
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
    g_object_set(data.playBin, "uri", "file:///home/seymour/Videos/tvoya.webm", NULL);

    // Create the UI
    createUI(&data);

    // Start playing the video
    GstStateChangeReturn ret = gst_element_set_state(data.playBin, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE)
        throw runtime_error("Cannot start playing !");

    // GTK main loop
    gtk_main();

    return 0;
}
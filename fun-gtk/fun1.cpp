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

//======================================================================================================================
static void cbQuit(GtkWidget *widget, GdkEvent *event, void * data) {
    using namespace std;
    cout << "QUIT !!!" << endl;
    gtk_main_quit();
}

//======================================================================================================================
// Create the GTK+ UI
static void createUI(){
    GtkWidget *mainWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    g_signal_connect(G_OBJECT(mainWindow), "delete-event", G_CALLBACK(cbQuit), nullptr);

    gtk_window_set_default_size(GTK_WINDOW(mainWindow), 800, 600);
    gtk_window_set_title(GTK_WINDOW(mainWindow), "Goblin window !");

    gtk_widget_show_all(mainWindow);
}
//======================================================================================================================
int main(int argc, char ** argv){
    using namespace std;
    cout << "GOBLINS WON !!!" << endl;

    // Init GTK
    gtk_init(&argc, &argv);

    createUI();

    gtk_main();

    return 0;
}
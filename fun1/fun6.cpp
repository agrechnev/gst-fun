//
// Created by IT-JIM 
//

#include <iostream>
#include <string>
#include <stdexcept>

#include <gst/gst.h>
#include <gst/pbutils/pbutils.h>



//======================================================================================================================
inline void myAssert(bool b, const std::string &s = "MYASSERT ERROR !") {
    if (!b)
        throw std::runtime_error(s);
}

//======================================================================================================================
struct GoblinData {
    GstDiscoverer *discoverer = nullptr;
    GMainLoop *loop = nullptr;
};
//======================================================================================================================
static void onDiscoveredCB(GstDiscoverer *discoverer, GstDiscovererInfo *info, GError *err, GoblinData *data) {
    using namespace std;
    cout << "==============================" << endl;
    cout << "onDiscoveredCB !" << endl;
    const char *uri = gst_discoverer_info_get_uri(info);
    GstDiscovererResult result = gst_discoverer_info_get_result(info);
    cout << "result=" << result << endl;
    cout << "uri=" << uri << endl;
    GstClockTime  dur = gst_discoverer_info_get_duration(info);
    cout << "duration = " << (dur * 1.0 / GST_SECOND) << " seconds" << endl;

    // Tags
    const GstTagList *tags = gst_discoverer_info_get_tags(info);
    cout << "Tags:" << endl;
//    gst_tag_list_foreach();
    cout << "==============================" << endl;
}
//======================================================================================================================
static void onFinishedCB(GstDiscoverer *discoverer, GoblinData *data) {
    using namespace std;
    cout << "onFinishedCB !" << endl;
    g_main_loop_quit(data->loop);
}
//======================================================================================================================
int main(int argc, char **argv) {
    using namespace std;
    cout << "fun6 !!!!" << endl;
    GoblinData data;
    string uri = "file:///home/seymour/Videos/tvoya.mp4";
    gst_init(&argc, &argv);

    cout << "Discovering " << uri << endl;

    // Init the discoverer
    GError *err = nullptr;
    data.discoverer = gst_discoverer_new(5 * GST_SECOND, &err);
    myAssert(data.discoverer);

    // Set callbacks
    g_signal_connect(data.discoverer, "discovered", G_CALLBACK(onDiscoveredCB), &data);
    g_signal_connect(data.discoverer, "finished", G_CALLBACK(onFinishedCB), &data);

    // Run discoverer
    gst_discoverer_start(data.discoverer);
    myAssert(gst_discoverer_discover_uri_async(data.discoverer, uri.c_str()));

    // Create and run the main loop
    data.loop = g_main_loop_new(nullptr, FALSE);
    g_main_loop_run(data.loop);

    gst_discoverer_stop(data.discoverer);
    g_object_unref(data.discoverer);
    g_main_loop_unref(data.loop);


    return 0;
}
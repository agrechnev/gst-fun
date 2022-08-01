//
// Created by seymour on 11.10.20.
//

#include <iostream>
#include <string>
#include <stdexcept>

#include <gst/gst.h>


//======================================================================================================================
inline void myAssert(bool b, const std::string &s = "MYASSERT ERROR !") {
    if (!b)
        throw std::runtime_error(s);
}

//======================================================================================================================
struct GoblinData {
    GstElement *pipeline;
    GstElement *source;
    GstElement *sink;
    GstElement *filt;
    GstElement *conv;
};
//======================================================================================================================
int main(int argc, char **argv) {
    using namespace std;
    cout << "gst-fun 0 !!!!" << endl;

//    string uri = "file:///home/seymour/Videos/tvoya.mp4";

    gst_init(&argc, &argv);

    // Build the pipeline
    GoblinData data;
    data.source = gst_element_factory_make("videotestsrc", "goblin_source");
    data.sink = gst_element_factory_make("autovideosink", "goblin_sink");
    data.filt = gst_element_factory_make("vertigotv", "goblin_filt");
    data.conv = gst_element_factory_make("videoconvert", "goblin_conv");

    data.pipeline = gst_pipeline_new("goblin_pipeline");
    myAssert(data.source && data.sink && data.filt && data.conv && data.pipeline, "Can't create elements !");
    gst_bin_add_many(GST_BIN(data.pipeline), data.source, data.filt, data.conv, data.sink, nullptr);
    myAssert(gst_element_link(data.source, data.filt), "Can't link 1");
    myAssert(gst_element_link(data.filt, data.conv), "Can't link 2");
    myAssert(gst_element_link(data.conv, data.sink), "Can't link 3");

    g_object_set(data.source, "pattern", 0, nullptr);

    int ret = gst_element_set_state(data.pipeline, GST_STATE_PLAYING);
    myAssert(ret != GST_STATE_CHANGE_FAILURE);

    // Wait
    GstBus * bus = gst_element_get_bus(data.pipeline);

    bool flagQuit = false;
    do {
//        GstMessage *msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, GstMessageType(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));
        GstMessage *msg = gst_bus_timed_pop(bus, GST_CLOCK_TIME_NONE);

        if (msg == nullptr)
            return 13;

        GstMessageType mType = GST_MESSAGE_TYPE(msg);
        cout << "BUS : mType = " << mType << "  ";

        switch (mType) {
            case GST_MESSAGE_ERROR:
                cout << "Error !" << endl;
                GError *err;
                gchar *dbg;
                gst_message_parse_error(msg, &err, &dbg);
                cout << " ERR = " << err->message << " FROM " << GST_OBJECT_NAME(msg->src) << endl;
                cout << "DBG = " << dbg << endl;
                g_clear_error(&err);
                g_free(dbg);
                flagQuit = true;
                break;
            case GST_MESSAGE_EOS:
                cout << "EOS !" << endl;
                flagQuit = true;
                break;
            case (GST_MESSAGE_STATE_CHANGED):
                cout << " State changed !" << endl;
                if (GST_MESSAGE_SRC(msg) == GST_OBJECT(data.pipeline)) {
                    GstState sOld, sNew, sPenging;
                    gst_message_parse_state_changed(msg, &sOld, &sNew, &sPenging);
                    cout << "Pipeline changed from " << gst_element_state_get_name(sOld) << " to " <<
                         gst_element_state_get_name(sNew) << endl;
                }
                break;
            case (GST_MESSAGE_STEP_START):
                cout << "STEP START !" << endl;
                break;
            case (GST_MESSAGE_STREAM_STATUS):
                cout << "STREAM STATUS !" << endl;
                break;
            case (GST_MESSAGE_ELEMENT):
                cout << "MESSAGE ELEMENT !" << endl;
                break;
            case (GST_MESSAGE_TAG):
                cout << "MESSAGE TAG !" << endl;
                break;
            default:
                cout << "Unknown type mType = " << mType << endl;
                break;
        }
        gst_message_unref(msg);
    } while (!flagQuit);

    gst_object_unref(bus);
    gst_element_set_state(data.pipeline, GST_STATE_NULL);
    gst_object_unref(data.pipeline);
    return 0;
}
//======================================================================================================================

//
// Created by seymour on 11.10.20.
//

#include <iostream>
#include <string>

#include <gst/gst.h>

//======================================================================================================================
int main(int argc, char **argv) {
    using namespace std;
    cout << "gst-fun 0 !!!!" << endl;

    string uri = "file:///home/seymour/Videos/tvoya.mp4";

    gst_init(&argc, &argv);
    string launch = "playbin uri=" + uri;
    GstElement *pipeline = gst_parse_launch(launch.c_str(), nullptr);
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    // Wait
    GstBus * bus = gst_element_get_bus(pipeline);

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
                if (GST_MESSAGE_SRC(msg) == GST_OBJECT(pipeline)) {
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
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    return 0;
}
//======================================================================================================================

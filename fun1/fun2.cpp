// By Oleksiy Grechnyev
// GST FUN 2

#include <iostream>
#include <string>
#include <gst/gst.h>

//======================================================================================================================
struct GoblinData {
    GstElement *playbin;
    bool flagQuit = false;
    bool playing = false;
    gint64 duration = -1;
    gboolean seekEnabled = 0;
    bool seekDone = false;
};


//======================================================================================================================
void handleMessage(GoblinData &data, GstMessage *msg) {
    using namespace std;


    GstMessageType mType = GST_MESSAGE_TYPE(msg);
    cout << "BUS : mType = " << mType;

    switch (mType) {
        case (GST_MESSAGE_ERROR):
            GError *err;
            gchar *dbg;
            gst_message_parse_error(msg, &err, &dbg);
            cout << " ERR = " << err->message << " FROM " << GST_OBJECT_NAME(msg->src) << endl;
            cout << "DBG = " << dbg << endl;
            g_clear_error(&err);
            g_free(dbg);
            data.flagQuit = true;
            break;
        case (GST_MESSAGE_EOS) :
            cout << " EOS !" << endl;
            data.flagQuit = true;
            break;
        case (GST_MESSAGE_STATE_CHANGED):
            cout << " State changed !" << endl;
            if (GST_MESSAGE_SRC(msg) == GST_OBJECT(data.playbin)) {
                GstState sOld, sNew, sPenging;
                gst_message_parse_state_changed(msg, &sOld, &sNew, &sPenging);
                cout << "Pipeline changed from " << gst_element_state_get_name(sOld) << " to " <<
                     gst_element_state_get_name(sNew) << endl;
                data.playing = (sNew == GST_STATE_PLAYING);

                if (data.playing) {
                    // Query if seek is possible
                    GstQuery *query = gst_query_new_seeking(GST_FORMAT_TIME);
                    if (gst_element_query(data.playbin, query)) {
                        gint64 t1, t2;
                        gst_query_parse_seeking(query, NULL, &data.seekEnabled, &t1, &t2);
                        if (data.seekEnabled)
                            cout << "SEEKING IS ENABLED FROM " << 1.0 * t1 / GST_SECOND << " TO "
                                 << 1.0 * t2 / GST_SECOND << endl;
                        else
                            cout << "SEEKING IS DISABLED" << endl;
                    } else {
                        cout << "SEEKING QUERY FAILED !!!" << endl;
                    }
                    gst_query_unref(query);
                }
            }
            break;
        case (GST_MESSAGE_STEP_START):
            cout << " STEP START !" << endl;
            break;
        case (GST_MESSAGE_STREAM_STATUS):
            cout << " STREAM STATUS !" << endl;
            break;
        case (GST_MESSAGE_ELEMENT):
            cout << " MESSAGE ELEMENT !" << endl;
            break;

        default:
            cout << endl;
    }
}

//======================================================================================================================
int main(int argc, char **argv) {
    using namespace std;

    cout << "GSTREAMER FUN 2 !!!" << endl;

    string uri = "file:///home/seymour/Videos/tvoya.mp4";
//    string uri = "https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm";


    gst_init(&argc, &argv);

    // Build pipeline
    GoblinData data;

    data.playbin = gst_element_factory_make("playbin", "goblin_playbin");

    if (!data.playbin)
        throw runtime_error("Canot create elements !");

    // Set URI
    g_object_set(data.playbin, "uri", uri.c_str(), nullptr);

    // Start playing
    GstStateChangeReturn ret = gst_element_set_state(data.playbin, GST_STATE_PLAYING);
    cout << "ret = " << ret << endl;

    // Wait until error/EOS
    GstBus *bus = gst_element_get_bus(data.playbin);
    do {

        GstMessage *msg = gst_bus_timed_pop(bus, 100 * GST_MSECOND);

        if (msg != nullptr) {
            handleMessage(data, msg);
        } else if (data.playing) {
            gint64 posCurrent = -1;
            if (!gst_element_query_position(data.playbin, GST_FORMAT_TIME, &posCurrent))
                cout << "CANNOT QUERY POSITION !!!" << endl;
            if (!gst_element_query_duration(data.playbin, GST_FORMAT_TIME, &data.duration))
                cout << "CANNOT QUERY DURATION !!!" << endl;

            // Nanosec -> double sec, GST_SECOND == 10^9
            cout << "POS = " << 1.0 * posCurrent / GST_SECOND << "/"
                 << 1.0 * data.duration / GST_SECOND << endl;

            // Perform seek for fun
            if (false && data.seekEnabled && !data.seekDone && posCurrent > 20 * GST_SECOND) {
                cout << "=====> SEEK !!!!!!!!!" << endl;
                gst_element_seek_simple(data.playbin, GST_FORMAT_TIME,
                                        GstSeekFlags(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT), 10 * GST_SECOND);
                data.seekDone = true;
            }
        } else {
            cout << "TICK !" << endl;
        }

    } while (!data.flagQuit);
    // Clean up
    gst_object_unref(bus);
    gst_element_set_state(data.playbin, GST_STATE_NULL);
    gst_object_unref(data.playbin);

    return 0;
}
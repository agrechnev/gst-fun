// By Oleksiy Grechnyev
// GST FUN1

#include <iostream>
#include <string>

#include <gst/gst.h>

//======================================================================================================================
struct GoblinData {
    GstElement *pipeline;
    GstElement *source;
    GstElement *convertAudio;
    GstElement *resampleAudio;
    GstElement *sinkAudio;
    GstElement *convertVideo;
    GstElement *sinkVideo;
};

//======================================================================================================================
static void padAddedCB(GstElement *src, GstPad *newPad, GoblinData *data) {
    using namespace std;
    cout << "PAD ADDED : " << GST_PAD_NAME(newPad) << " from " << GST_ELEMENT_NAME(src) << endl;
    GstCaps *newPadCaps = gst_pad_get_current_caps(newPad);
    GstStructure *newPadStruct = gst_caps_get_structure(newPadCaps, 0);
    const gchar *newPadType = gst_structure_get_name(newPadStruct);
    cout << "TYPE =" << newPadType << endl;

    if (g_str_has_prefix(newPadType, "audio/x-raw")) {
        cout << "AUDIO !!!" << endl;
        // Try to link
        GstPad *sinkPad = gst_element_get_static_pad(data->convertAudio, "sink");
        if (gst_pad_is_linked(sinkPad))
            cout << "Already linked !!!" << endl;
        else {
            GstPadLinkReturn ret = gst_pad_link(newPad, sinkPad);
            if (GST_PAD_LINK_FAILED(ret))
                cout << "LINK FAILED !!!" << endl;
            else
                cout << "LINK SUCCEEDED !!!" << endl;

        }
        gst_object_unref(sinkPad);
    } else if (g_str_has_prefix(newPadType, "video/x-raw")) {
        cout << "VIDEO !!!" << endl;
        // Try to link
        GstPad *sinkPad = gst_element_get_static_pad(data->convertVideo, "sink");
        if (gst_pad_is_linked(sinkPad))
            cout << "Already linked !!!" << endl;
        else {
            GstPadLinkReturn ret = gst_pad_link(newPad, sinkPad);
            if (GST_PAD_LINK_FAILED(ret))
                cout << "LINK FAILED !!!" << endl;
            else
                cout << "LINK SUCCEEDED !!!" << endl;
        }
        gst_object_unref(sinkPad);
    }

    if (newPadCaps)
        gst_caps_unref(newPadCaps);
}

//======================================================================================================================
int main(int argc, char **argv) {
    using namespace std;

    cout << "GSTREAMER FUN 1 !!!" << endl;

    string uri = "file:///home/seymour/Videos/tvoya.mp4";
//    string uri = "https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm";


    gst_init(&argc, &argv);

    // Build pipeline
    GoblinData data;

    data.source = gst_element_factory_make("uridecodebin", "goblin_source");
    data.convertAudio = gst_element_factory_make("audioconvert", "goblin_audio_convert");
    data.resampleAudio = gst_element_factory_make("audioresample", "goblin_audio_resample");
    data.sinkAudio = gst_element_factory_make("autoaudiosink", "goblin_audio_sink");
    data.convertVideo = gst_element_factory_make("videoconvert", "goblin_video_convert");
    data.sinkVideo = gst_element_factory_make("ximagesink", "goblin_video_sink");

    data.pipeline = gst_pipeline_new("goblin-pipeline");
    if (!data.source || !data.convertAudio || !data.resampleAudio || !data.sinkAudio ||
        !data.convertVideo || !data.sinkVideo || !data.pipeline)
        throw runtime_error("Canot create elements !");

    gst_bin_add_many(GST_BIN(data.pipeline), data.source,
                     data.convertAudio, data.resampleAudio, data.sinkAudio,
                     data.convertVideo, data.sinkVideo,
                     nullptr);
    if (!gst_element_link_many(data.convertAudio, data.resampleAudio, data.sinkAudio, nullptr))
        throw runtime_error("Cannot link 1");
    if (!gst_element_link_many(data.convertVideo, data.sinkVideo, nullptr))
        throw runtime_error("Cannot link 2");

    // Set URI
    g_object_set(data.source, "uri", uri.c_str(), nullptr);

    // Connect the signal
    g_signal_connect(data.source, "pad-added", G_CALLBACK(padAddedCB), &data);

    // Start playing
    GstStateChangeReturn ret = gst_element_set_state(data.pipeline, GST_STATE_PLAYING);
    cout << "ret = " << ret << endl;

    // Wait until error/EOS
    GstBus *bus = gst_element_get_bus(data.pipeline);
    bool flagQuit = false;
    do {
//        GstMessage *msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE,
//                                                     GstMessageType(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

        GstMessage *msg = gst_bus_timed_pop(bus, GST_CLOCK_TIME_NONE);

        if (msg == nullptr)
            return 13;

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
                flagQuit = true;
                break;
            case (GST_MESSAGE_EOS) :
                cout << " EOS !" << endl;
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

        gst_message_unref(msg);
    } while (!flagQuit);
    // Clean up
    gst_object_unref(bus);
    gst_element_set_state(data.pipeline, GST_STATE_NULL);
    gst_object_unref(data.pipeline);

    return 0;
}
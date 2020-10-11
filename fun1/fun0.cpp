//
// Created by seymour on 11.10.20.
//

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
static bool log_msg(GstMessage *msg) {
    using namespace std;
    bool fatal = false;
    if (!msg)
        return fatal;
    switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_ERROR:
            GError *err;
            gchar *debugInfo;
            gst_message_parse_error(msg, &err, &debugInfo);
            cout << "ERROR [" << GST_OBJECT_NAME(msg->src) << "] " << err->message << endl;
            if (debugInfo)
                cout << debugInfo << endl;
            fatal = true;
            break;
        case GST_MESSAGE_EOS:
            cout << "EOS" << endl;
            fatal = true;
            break;
        case GST_MESSAGE_STATE_CHANGED:
            GstState sOld, sNew, sPenging;
            gst_message_parse_state_changed(msg, &sOld, &sNew, &sPenging);
            cout << "STATE CHANGED [" << GST_OBJECT_NAME(msg->src) << "] from " << gst_element_state_get_name(sOld)
                 << " to " << gst_element_state_get_name(sNew) << endl;
            break;
        default:
            cout << "Who knows type = " << GST_MESSAGE_TYPE(msg) << endl;
            break;
    }
    return fatal;
}


//======================================================================================================================
static void padAddedCB(GstElement *src, GstPad *newPad, GoblinData *data) {
    using namespace std;
    GstCaps *newPadCaps = gst_pad_get_current_caps(newPad);
    GstStructure *newPadStruct = gst_caps_get_structure(newPadCaps, 0);
    const gchar *newPadType = gst_structure_get_name(newPadStruct);
    cout << "PAD ADDED " << GST_PAD_NAME(newPad) << " from " << GST_ELEMENT_NAME(src) << ", TYPE = " << newPadType
         << endl;

    if (g_str_has_prefix(newPadType, "audio/x-raw")) {
        cout << "AUDIO !!!" << endl;// Try to link
        GstPad *sinkPad = gst_element_get_static_pad(data->convertAudio, "sink");
        if (gst_pad_is_linked(sinkPad)) {
            cout << "Already linked !!!" << endl;
        } else {
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
        if (gst_pad_is_linked(sinkPad)) {
            cout << "Already linked !!!" << endl;
        } else {
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
    cout << "gst-fun 0 !!!!" << endl;

    string uri = "file:///home/seymour/Videos/tvoya.mp4";

    // Init gstreamer
    gst_init(&argc, &argv);

    // Create pipeline
    GoblinData data;
    data.source = gst_element_factory_make("uridecodebin", "goblin-source");
    data.convertAudio = gst_element_factory_make("audioconvert", "goblin-audio-convert");
    data.resampleAudio = gst_element_factory_make("audioresample", "goblin-audio-resample");
    data.sinkAudio = gst_element_factory_make("autoaudiosink", "goblin-audio-sink");
    data.convertVideo = gst_element_factory_make("videoconvert", "goblin-video-convert");
    data.sinkVideo = gst_element_factory_make("ximagesink", "goblin-video-sink");
    data.pipeline = gst_pipeline_new("goblin-pipeline");
    if (!data.pipeline || !data.source || !data.convertAudio || !data.resampleAudio || !data.sinkAudio ||
        !data.convertVideo || !data.sinkVideo)
        throw runtime_error("Cannot create stuff");

    gst_bin_add_many(GST_BIN(data.pipeline), data.source, data.convertAudio, data.resampleAudio, data.sinkAudio,
                     data.convertVideo, data.sinkVideo, nullptr);
    if (!gst_element_link_many(data.convertAudio, data.resampleAudio, data.sinkAudio, nullptr))
        throw runtime_error("Cannot link 1");
    if (!gst_element_link_many(data.convertVideo, data.sinkVideo, nullptr))
        throw runtime_error("Cannot link 2");

    // Set URI
    g_object_set(data.source, "uri", uri.c_str(), nullptr);

    // Set "pad added" callback
    g_signal_connect(data.source, "pad-added", G_CALLBACK(padAddedCB), &data);

    // Start playing
    if (gst_element_set_state(data.pipeline, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
        cout << "Cannot start pipeline" << endl;
        return 1;
    }

    // Wait until error or EOS
    GstBus *bus = gst_element_get_bus(data.pipeline);
    GstMessage *msg;

    for (;;) {
        msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE,
                                         static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS |
                                                                     GST_MESSAGE_STATE_CHANGED));
        if (log_msg(msg))
            break;
    }

    // Free resources
    if (msg != nullptr)
        gst_message_unref(msg);
    gst_object_unref(bus);
    gst_element_set_state(data.pipeline, GST_STATE_NULL);
    gst_object_unref(data.pipeline);
    return 0;
}
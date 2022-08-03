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
    GstElement *sinkVideo;
    GstElement *convVideo;
    GstElement *sinkAudio;
    GstElement *convAudio;
};
//======================================================================================================================
static void padAddedCB(GstElement *src, GstPad *newPad, GoblinData *data) {
    using namespace std;
    cout << "PAD ADDED : " << GST_PAD_NAME(newPad) << " from " << GST_ELEMENT_NAME(src) << endl;
    GstCaps *newPadCaps = gst_pad_get_current_caps(newPad);
    GstStructure *newPadStruct = gst_caps_get_structure(newPadCaps, 0);
    const gchar *newPadType = gst_structure_get_name(newPadStruct);
    cout << "TYPE = " << newPadType << endl;

    if (g_str_has_prefix(newPadType, "audio/x-raw")) {
        cout << "AUDIO !" << endl;
        GstPad *sinkPad = gst_element_get_static_pad(data->convAudio, "sink");
        myAssert(!gst_pad_is_linked(sinkPad), "Already linked !");
        GstPadLinkReturn ret = gst_pad_link(newPad, sinkPad);
        myAssert(!GST_PAD_LINK_FAILED(ret), "Link failed !");
        gst_object_unref(sinkPad);
    } else if (g_str_has_prefix(newPadType, "video/x-raw")) {
        cout << "VIDEO !" << endl;
        GstPad *sinkPad = gst_element_get_static_pad(data->convVideo, "sink");
        myAssert(!gst_pad_is_linked(sinkPad), "Already linked !");
        GstPadLinkReturn ret = gst_pad_link(newPad, sinkPad);
        myAssert(!GST_PAD_LINK_FAILED(ret), "Link failed !");
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

    gst_init(&argc, &argv);

    // Build the pipeline, not fully linked yet
    GoblinData data;
    data.source = gst_element_factory_make("uridecodebin", "goblin_source");
    data.convVideo = gst_element_factory_make("videoconvert", "goblin_video_conv");
    data.sinkVideo = gst_element_factory_make("autovideosink", "goblin_video_sink");
    data.convAudio = gst_element_factory_make("audioconvert", "goblin_audio_conv");
    data.sinkAudio = gst_element_factory_make("autoaudiosink", "goblin_audio_sink");

    data.pipeline = gst_pipeline_new("goblin_pipeline");
    myAssert(data.source && data.sinkVideo  && data.convVideo && data.convAudio && data.sinkAudio && data.pipeline, "Can't create elements !");
    gst_bin_add_many(GST_BIN(data.pipeline), data.source,  data.convVideo, data.sinkVideo, data.convAudio, data.sinkAudio, nullptr);
    myAssert(gst_element_link_many(data.convVideo, data.sinkVideo, nullptr), "Can't link video");
    myAssert(gst_element_link_many(data.convAudio, data.sinkAudio, nullptr), "Can't link audio");

    g_object_set(data.source, "uri", uri.c_str(), nullptr);

    // Connect signal
    g_signal_connect(data.source, "pad-added", G_CALLBACK(padAddedCB), &data);

    // PLay
    int ret = gst_element_set_state(data.pipeline, GST_STATE_PLAYING);
    myAssert(ret != GST_STATE_CHANGE_FAILURE);

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

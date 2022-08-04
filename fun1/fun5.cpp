//
// Created by IT-JIM 
//

#include <iostream>
#include <string>
#include <stdexcept>

#include <gst/gst.h>
#include <gst/audio/audio.h>

constexpr int SAMPLE_RATE = 44100;
constexpr int CHUNK_SIZE = 1024;


//======================================================================================================================
inline void myAssert(bool b, const std::string &s = "MYASSERT ERROR !") {
    if (!b)
        throw std::runtime_error(s);
}

//======================================================================================================================
struct GoblinData {
    //==== Elements
    GstElement *pipeline = nullptr;
    GstElement *source = nullptr;
    GstElement *tee = nullptr;
    GstElement *queueAudio = nullptr;
    GstElement *queueVideo = nullptr;
    GstElement *convertAudio1 = nullptr;
    GstElement *convertAudio2 = nullptr;
    GstElement *resampleAudio = nullptr;
    GstElement *sinkAudio = nullptr;
    GstElement *visual = nullptr;
    GstElement *convertVideo = nullptr;
    GstElement *sinkVideo = nullptr;

    //==== Generator data
    guint sourceId = 0;
    // Number of samples generated so far (for timestamp generation)
    guint64 numSamples = 0;
    gfloat a = 0, b = 1, c = 0, d = 1;

    //==== GLIB main loop
    GMainLoop *mainLoop = nullptr;
};

//======================================================================================================================
static gboolean printField(GQuark field, const GValue *value, gpointer pfx) {
    using namespace std;
    gchar *str = gst_value_serialize(value);
    cout << (char *) pfx << " " << g_quark_to_string(field) << " " << str << endl;
    g_free(str);
    return TRUE;
}

//======================================================================================================================
void printCaps(const GstCaps *caps, const std::string &pfx) {
    using namespace std;
    if (caps == nullptr)
        return;
    if (gst_caps_is_any(caps))
        cout << pfx << "ANY" << endl;
    else if (gst_caps_is_empty(caps))
        cout << pfx << "EMPTY" << endl;
    for (int i = 0; i < gst_caps_get_size(caps); ++i) {
        GstStructure *s = gst_caps_get_structure(caps, i);
        cout << pfx << gst_structure_get_name(s) << endl;
        gst_structure_foreach(s, &printField, (gpointer) pfx.c_str());
    }
}

//======================================================================================================================
static gboolean pushData(GoblinData *data) {
    using namespace std;
//    cout << "pushData" << endl;
    // Create a new empy buffer
    GstBuffer *buffer = gst_buffer_new_and_alloc(CHUNK_SIZE);

    // Set timestamp + duration
    gint nS = CHUNK_SIZE / 2;
            GST_BUFFER_TIMESTAMP(buffer) = gst_util_uint64_scale(data->numSamples, GST_SECOND, SAMPLE_RATE);
    GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale(nS, GST_SECOND, SAMPLE_RATE);

    // Generate some psychodelic waveforms
    GstMapInfo map;
    gst_buffer_map(buffer, &map, GST_MAP_WRITE);
    gint16 *raw = (gint16 *) map.data;
    data->c += data->d;
    data->d -= data->c / 1000;
    gfloat freq = 1100 + 1000 * data->d;
    for (int i = 0; i < nS; i++) {
        data->a += data->b;
        data->b -= data->a / freq;
        raw[i] = (gint16)(500 * data->a);
    }
    gst_buffer_unmap(buffer, &map);
    data->numSamples += nS;

    // Push the buffer to appsrc
    GstFlowReturn ret;
    g_signal_emit_by_name(data->source, "push-buffer", buffer, &ret);
    gst_buffer_unref(buffer);
    return (ret == GST_FLOW_OK) ? TRUE : FALSE;
}

//======================================================================================================================
static void startFeed(GstElement *source, guint size, GoblinData *data) {
    using namespace std;
    if (data->sourceId == 0) {
        cout << "startFeed" << endl;
        data->sourceId = g_idle_add((GSourceFunc) pushData, data);
    }
}

//======================================================================================================================
static void stopFeed(GstElement *source, GoblinData *data) {
    using namespace std;
    if (data->sourceId != 0) {
        cout << "stopFeed" << endl;
        g_source_remove(data->sourceId);
        data->sourceId = 0;
    }
}

//======================================================================================================================
static void errorCB(GstBus *bus, GstMessage *msg, GoblinData *data) {
    using namespace std;
    GError *err;
    gchar *dbg;
    gst_message_parse_error(msg, &err, &dbg);
    cout << " ERR = " << err->message << " FROM " << GST_OBJECT_NAME(msg->src) << endl;
    cout << "DBG = " << dbg << endl;
    g_clear_error(&err);
    g_free(dbg);
    g_main_loop_quit(data->mainLoop);
}
//======================================================================================================================
int main(int argc, char **argv) {
    using namespace std;
    cout << "fun5 !!!!" << endl;

    GoblinData data;
    gst_init(&argc, &argv);

    // Create pipeline
    data.source = gst_element_factory_make("appsrc", "goblin_source");
    data.tee = gst_element_factory_make("tee", "goblin_tee");
    data.queueAudio = gst_element_factory_make("queue", "goblin_queue_audio");
    data.queueVideo = gst_element_factory_make("queue", "goblin_queue_video");
    data.convertAudio1 = gst_element_factory_make("audioconvert", "goblin_convert_audio1");
    data.convertAudio2 = gst_element_factory_make("audioconvert", "goblin_convert_audio2");
    data.resampleAudio = gst_element_factory_make("audioresample", "goblin_resample_audio");
    data.sinkAudio = gst_element_factory_make("autoaudiosink", "goblin_sink_audio");
    data.visual = gst_element_factory_make("wavescope", "goblin_wavescope");
    data.convertVideo = gst_element_factory_make("videoconvert", "goblin_convert_video");
    data.sinkVideo = gst_element_factory_make("autovideosink", "goblin_sink_video");
    data.pipeline = gst_pipeline_new("goblin_pipeline");

    myAssert(data.source && data.tee && data.queueAudio && data.queueVideo);
    myAssert(data.convertAudio1 && data.resampleAudio && data.sinkAudio);
    myAssert(data.convertAudio2 && data.visual && data.convertVideo && data.sinkVideo && data.pipeline);

    // Configure visual
    g_object_set(data.visual, "shader", 0, "style", 1, nullptr);

    // Configure appsrc
    GstAudioInfo info;
    gst_audio_info_set_format(&info, GST_AUDIO_FORMAT_S16, SAMPLE_RATE, 1, nullptr);
    GstCaps *capsAudio = gst_audio_info_to_caps(&info);
//    printCaps(capsAudio, " ");
    g_object_set(data.source, "caps", capsAudio, "format", GST_FORMAT_TIME, nullptr);
    g_signal_connect(data.source, "need-data", G_CALLBACK(startFeed), &data);
    g_signal_connect(data.source, "enough-data", G_CALLBACK(stopFeed), &data);


    // Configure appsink
    gst_caps_unref(capsAudio);

    // Add elements and link what we can
    gst_bin_add_many(GST_BIN(data.pipeline),
                     data.source, data.tee, data.queueAudio, data.queueVideo,
                     data.convertAudio1, data.resampleAudio, data.sinkAudio,
                     data.convertAudio2, data.visual, data.convertVideo, data.sinkVideo,
                     nullptr);

    myAssert(gst_element_link_many(data.source, data.tee, nullptr));
    myAssert(gst_element_link_many(data.queueAudio, data.convertAudio1, data.resampleAudio, data.sinkAudio, nullptr));
    myAssert(gst_element_link_many(data.queueVideo, data.convertAudio2, data.visual, data.convertVideo, data.sinkVideo, nullptr));

    // Manually link tee
    GstPad *padTeeAudio = gst_element_request_pad_simple(data.tee, "src_%u");
    GstPad *padTeeVideo = gst_element_request_pad_simple(data.tee, "src_%u");
    GstPad *padQueueAudio = gst_element_get_static_pad(data.queueAudio, "sink");
    GstPad *padQueueVideo = gst_element_get_static_pad(data.queueVideo, "sink");
    myAssert(gst_pad_link(padTeeAudio, padQueueAudio) == GST_PAD_LINK_OK);
    myAssert(gst_pad_link(padTeeVideo, padQueueVideo) == GST_PAD_LINK_OK);
    gst_object_unref(padQueueAudio);
    gst_object_unref(padQueueVideo);

    // Error callback
    GstBus *bus = gst_element_get_bus(data.pipeline);
    gst_bus_add_signal_watch(bus);
    g_signal_connect(G_OBJECT(bus), "message::error", (GCallback)errorCB, &data);
    gst_object_unref(bus);

    // Play the pipeline
    gst_element_set_state(data.pipeline, GST_STATE_PLAYING);

    // Create and run GLIB main loop
    data.mainLoop = g_main_loop_new(nullptr, FALSE);
    g_main_loop_run(data.mainLoop);

    // Release the request pads from the Tee, and unref them
    gst_element_release_request_pad(data.tee, padTeeAudio);
    gst_element_release_request_pad(data.tee, padTeeVideo);
    gst_object_unref(padTeeAudio);
    gst_object_unref(padTeeVideo);


    gst_element_set_state(data.pipeline, GST_STATE_NULL);
    gst_object_unref(data.pipeline);

    return 0;
}

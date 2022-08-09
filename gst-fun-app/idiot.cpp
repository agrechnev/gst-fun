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
static GstFlowReturn newSample(GstElement *sink, GoblinData *data) {
    using namespace std;
    GstSample *sample = nullptr;
    g_signal_emit_by_name(sink, "pull-sample", &sample);
    if (sample) {
        cout << "*";
        cout.flush();
        return GST_FLOW_OK;
    } else
        return GST_FLOW_ERROR;
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
void printPadsCB(const GValue * item, gpointer userData) {
    using namespace std;
    GstElement *element = (GstElement *)userData;
    GstPad *pad = (GstPad *)g_value_get_object(item);
    myAssert(pad);
    cout << "PAD : " << gst_pad_get_name(pad) << endl;
    GstCaps *caps = gst_pad_get_current_caps(pad);
    char * str = gst_caps_to_string(caps);
    cout << str << endl;
    free(str);
}

void printPads(GstElement *element) {
    using namespace std;
    GstIterator *pad_iter = gst_element_iterate_pads(element);
    gst_iterator_foreach(pad_iter, printPadsCB, element);
    gst_iterator_free(pad_iter);

}
void diagnose(GstElement *element) {
    using namespace std;
    cout << "=====================================" << endl;
    cout << "DIAGNOSE element : " << gst_element_get_name(element) << endl;
    printPads(element);
    cout << "=====================================" << endl;
}
//======================================================================================================================
int main(int argc, char **argv) {
    using namespace std;
    cout << "fun5 !!!!" << endl;

    GoblinData data;
    gst_init(&argc, &argv);

    // Create pipeline
    string pipeStr = "appsrc name=goblin_src format=time caps=audio/x-raw,format=S16LE,rate=44100,channels=1,layout=interleaved ! audioconvert ! audioresample ! autoaudiosink";
    data.pipeline = gst_parse_launch(pipeStr.c_str(), nullptr);
    myAssert(data.pipeline);
    data.source = gst_bin_get_by_name(GST_BIN(data.pipeline), "goblin_src");
    myAssert(data.source);
    g_signal_connect(data.source, "need-data", G_CALLBACK(startFeed), &data);
    g_signal_connect(data.source, "enough-data", G_CALLBACK(stopFeed), &data);

    cout << "BEFORE :" << endl;
    diagnose(data.source);

//    GstAudioInfo info;
//    gst_audio_info_set_format(&info, GST_AUDIO_FORMAT_S16, SAMPLE_RATE, 1, nullptr);
//    GstCaps *capsAudio = gst_audio_info_to_caps(&info);
//    printCaps(capsAudio, " ");
//    g_object_set(data.source, "caps", capsAudio, "format", GST_FORMAT_TIME, nullptr);
//    gst_caps_unref(capsAudio);

//    printCaps(capsAudio, "");


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

    gst_element_set_state(data.pipeline, GST_STATE_NULL);
    gst_object_unref(data.pipeline);

    return 0;
}

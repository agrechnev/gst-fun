//
// Created by IT-JIM 
// AUDIO1: goblin and elf pipelines (like in FUN6), audio-only version
// We didn't do simpler examples, as this one worked out of the box ...

#include <iostream>
#include <string>
#include <stdexcept>
#include <thread>
#include <atomic>

#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <gst/app/gstappsrc.h>


//======================================================================================================================
inline void myAssert(bool b, const std::string &s = "MYASSERT ERROR !") {
    if (!b)
        throw std::runtime_error(s);
}

//======================================================================================================================
struct GoblinData {
    // Elements
    GstElement *goblinPipeline = nullptr;
    GstElement *goblinSinkA = nullptr;
    GstElement *elfPipeline = nullptr;
    GstElement *elfScrA = nullptr;

    // Misc
//    cv::Size2i size;
//    double fps;

    // Flags
    std::atomic_bool flagStop{false};
    std::atomic_bool flagRunA{false};
    std::atomic_bool flagElfStarted{false};
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
static bool busProcessMsg(GstElement *pipeline, GstMessage *msg, const std::string &prefix) {
    using namespace std;

    GstMessageType mType = GST_MESSAGE_TYPE(msg);
    cout << "[" << prefix << "] : mType = " << mType << endl;

    switch (mType) {
        case (GST_MESSAGE_ERROR):
            GError *err;
            gchar *dbg;
            gst_message_parse_error(msg, &err, &dbg);
            cout << " ERR = " << err->message << " FROM " << GST_OBJECT_NAME(msg->src) << endl;
            cout << "DBG = " << dbg << endl;
            g_clear_error(&err);
            g_free(dbg);
            return false;
            break;
        case (GST_MESSAGE_EOS) :
            cout << " EOS !" << endl;
            return false;
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
    return true;
}
//======================================================================================================================
// Run the message loop for one bus
void codeThreadBus(GstElement *pipeline, GoblinData &data, const std::string &prefix) {
    using namespace std;
    GstBus *bus = gst_element_get_bus(pipeline);

    while (!data.flagStop) {
        // We limit query to 200ms, so that flagStop can work
        // This is a precaution, as normally we quit on EOS anyway
        GstMessage *msg = gst_bus_timed_pop(bus, GST_MSECOND * 200);
        if (!msg)
            continue;
        bool res = busProcessMsg(pipeline, msg, prefix);
        gst_message_unref(msg);
        if (!res)
            break;
    }
    data.flagStop = true;
    gst_object_unref(bus);
}
//======================================================================================================================
static void startFeed(GstElement *source, guint size, GoblinData *data) {
    using namespace std;
    if (!data->flagRunA) {
        cout << "startFeed !" << endl;
        data->flagRunA = true;
    } else {
//        cout << "(start)" << endl;
    }
}

//======================================================================================================================
static void stopFeed(GstElement *source, GoblinData *data) {
    using namespace std;
    if (data->flagRunA) {
        cout << "stopFeed !" << endl;
        data->flagRunA = false;
    } else {
//        cout << "(stop)" << data->flagRun << " " << data << endl;
    }
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
/// Process audio
void codeThreadProcessA(GoblinData &data) {
    using namespace std;
    while (!data.flagStop) {
        // Wait if the ELF pipeline does not want data
        while (data.flagElfStarted && !data.flagRunA && !data.flagStop) {
            cout << "(wait)" << endl;
            this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        // Check again after waiting
        if (data.flagStop)
            break;

        // Pull the sample
        if (gst_app_sink_is_eos(GST_APP_SINK(data.goblinSinkA))) {
            cout << "EOS !" << endl;
            break;
        }

        GstSample *sample = gst_app_sink_pull_sample(GST_APP_SINK(data.goblinSinkA));
        if (sample == nullptr) {
            cout << "NO sample !" << endl;
            break;
        }

        // Get width and height
        GstCaps *caps = gst_sample_get_caps(sample);
        myAssert(caps != nullptr);
//        printCaps(caps, "");

//        GstStructure *s = gst_caps_get_structure(caps, 0);
//        assert(gst_structure_get_int(s, "width", &imW));

        if (!data.flagElfStarted) {
            // Set elf caps
            GstCaps *capsElf = gst_caps_copy(caps);
//            printCaps(capsElf, "");
//            exit(0);

            g_object_set(data.elfScrA, "caps", capsElf, nullptr);
            gst_caps_unref(capsElf);

            // Start the elf pipeline
            GstStateChangeReturn ret = gst_element_set_state(data.elfPipeline, GST_STATE_PLAYING);
            myAssert(ret != GST_STATE_CHANGE_FAILURE, "Can't play Elf !");
            data.flagElfStarted = true;
        }

//        cout << "sample !" << endl;
        // Process sample
        GstBuffer *bufferIn = gst_sample_get_buffer(sample);
        GstMapInfo mapIn;
        myAssert(gst_buffer_map(bufferIn, &mapIn, GST_MAP_READ));
//        cout << "size = " << map.size << " ==? " << imW*imH*3 << endl;

        // Timestamp
        uint64_t pts = bufferIn->pts;

        // Create the output bufer and send it to elfSrc
        int bufferSize = mapIn.size;
        cout << "bufferSize = " << mapIn.size << endl;
        GstBuffer *bufferOut = gst_buffer_new_and_alloc(bufferSize);
        GstMapInfo mapOut;
        gst_buffer_map(bufferOut, &mapOut, GST_MAP_WRITE);
        memcpy(mapOut.data, mapIn.data, bufferSize);
        gst_buffer_unmap(bufferIn, &mapIn);
        gst_buffer_unmap(bufferOut, &mapOut);
        bufferOut->pts = bufferIn->pts;
        bufferOut->duration = bufferIn->duration;
        GstFlowReturn ret = gst_app_src_push_buffer(GST_APP_SRC(data.elfScrA), bufferOut);

        gst_sample_unref(sample);
    }
    gst_app_src_end_of_stream(GST_APP_SRC(data.elfScrA));
    data.flagStop = true;
}
//======================================================================================================================
int main(int argc, char **argv) {
    using namespace std;
    cout << "audio1" << endl;

    string uri = "file:///home/seymour/Videos/tvoya.mp4";
//    string uri = "file:///home/seymour/w/7/voclarity_data/misc/obama.wav";
    gst_init(&argc, &argv);
    GoblinData data;

    // Create Goblin (input) pipeline
    string goblinPipeStr = "uridecodebin3 name=goblin_src uri=" + uri + " ! audioconvert ! audio/x-raw,format=S16LE,layout=interleaved ! appsink sync=false name=goblin_sink";
    data.goblinPipeline = gst_parse_launch(goblinPipeStr.c_str(), nullptr);
    myAssert(data.goblinPipeline);
    data.goblinSinkA = gst_bin_get_by_name(GST_BIN (data.goblinPipeline), "goblin_sink");
    myAssert(data.goblinSinkA);

    // Create Elf (output) pipeline
    string elfPipeStr = "appsrc name=elf_src format=time caps=audio/x-raw,format=S16LE,layout=interleaved ! audioconvert ! audioresample ! autoaudiosink";
    data.elfPipeline = gst_parse_launch(elfPipeStr.c_str(), nullptr);
    myAssert(data.elfPipeline);
    data.elfScrA = gst_bin_get_by_name(GST_BIN(data.elfPipeline), "elf_src");
    myAssert(data.elfScrA);
    g_signal_connect(data.elfScrA, "need-data", G_CALLBACK(startFeed), &data);
    g_signal_connect(data.elfScrA, "enough-data", G_CALLBACK(stopFeed), &data);

    // Play the Goblin pipeline
    GstStateChangeReturn ret = gst_element_set_state(data.goblinPipeline, GST_STATE_PLAYING);
    myAssert(ret != GST_STATE_CHANGE_FAILURE, "Can't play Goblin !");

    // Create and run the threads
    thread threadBusGoblin([&data]{
        codeThreadBus(data.goblinPipeline, data, "GOBLIN");
    });
    thread threadBusElf([&data]{
        codeThreadBus(data.elfPipeline, data, "ELF");
    });
    thread threadProcess([&data]{
        codeThreadProcessA(data);
    });
    threadBusGoblin.join();
    threadBusElf.join();
    threadProcess.join();

    // Stop and destroy pipelines
    gst_element_set_state(data.goblinPipeline, GST_STATE_NULL);
    gst_object_unref(data.goblinPipeline);

    return 0;
}
//======================================================================================================================

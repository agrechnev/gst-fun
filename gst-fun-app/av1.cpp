//
// Created by IT-JIM 
// AV1: Goblin + Elf pipelines, audio + video (basically fun6 + audio1 combined)

#include <iostream>
#include <string>
#include <stdexcept>
#include <thread>
#include <atomic>
#include <mutex>

#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <gst/app/gstappsrc.h>

#include <opencv2/opencv.hpp>


//======================================================================================================================
inline void myAssert(bool b, const std::string &s = "MYASSERT ERROR !") {
    if (!b)
        throw std::runtime_error(s);
}

//======================================================================================================================
struct GoblinData {
    // Elements
    GstElement *goblinPipeline = nullptr;
    GstElement *goblinSinkV = nullptr;
    GstElement *goblinSinkA = nullptr;
    GstElement *elfPipeline = nullptr;
    GstElement *elfSrcV = nullptr;
    GstElement *elfSrcA = nullptr;

    // Misc
//    cv::Size2i size;
//    double fps;

    // Flags
    std::atomic_bool flagStop{false};
    std::atomic_bool flagRunV{false};
    std::atomic_bool flagRunA{false};
    std::atomic_bool flagElfStarted{false};
    std::atomic_bool flagElfStartedA{false};
    std::atomic_bool flagElfStartedV{false};
    std::mutex mutexElfStart;
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
inline void checkErr(GError * err){
    using namespace std;
    if (err == nullptr)
        return;
    cerr << "checkErr : " << err->message << endl;
    exit(0);
}

//======================================================================================================================
static int busProcessMsg(GstElement *pipeline, GstMessage *msg, const std::string &prefix) {
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
            return 2;
            break;
        case (GST_MESSAGE_EOS) :
            cout << " EOS !" << endl;
            return 1;
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
    return 0;
}
//======================================================================================================================
// Run the message loop for one bus
void codeThreadBus(GstElement *pipeline, GoblinData &data, const std::string &prefix) {
    using namespace std;
    GstBus *bus = gst_element_get_bus(pipeline);

    int res;
    while (!data.flagStop) {
        // We limit query to 200ms, so that flagStop can work
        // This is a precaution, as normally we quit on EOS anyway
        // Update: if writing to a file, disable flagStop, EOS should work
        GstMessage *msg = gst_bus_timed_pop(bus, GST_MSECOND * 500);
        if (!msg)
            continue;
        res = busProcessMsg(pipeline, msg, prefix);
        gst_message_unref(msg);
        if (res)
            break;
    }
    if (res == 2) // Terminate on Error, not EOS !
        data.flagStop = true;
    gst_object_unref(bus);
    cout << "BUS THREAD FINISHED : " << prefix << endl;
}
//======================================================================================================================
static void startFeed(GstElement *source, guint size, GoblinData *data) {
    using namespace std;
    bool isV = false;
    if (source == data->elfSrcV)
        isV = true;
    else
        myAssert(source == data->elfSrcA);

    atomic_bool *f = isV ? &data->flagRunV : &data->flagRunA;
//    string prefix = isV ? "V : " : "A : ";
//    cout << prefix << "START " << f << endl;

    if (!(*f)) {
        string prefix = isV ? "V : " : "A : ";
        cout << prefix << "startFeed !" << endl;
        (*f) = true;
    } else {
//        string prefix = isV ? "V : " : "A : ";
//        cout << prefix << "(start) " << (*f) << endl;
    }
}
//======================================================================================================================
static void stopFeed(GstElement *source, GoblinData *data) {
    using namespace std;
    bool isV = false;
    if (source == data->elfSrcV)
        isV = true;
    else
        myAssert(source == data->elfSrcA);

    atomic_bool *f = isV ? &data->flagRunV : &data->flagRunA;
    if (*f) {
        string prefix = isV ? "V : " : "A : ";
        cout << prefix << "stopFeed !" << endl;
        (*f) = false;
    } else {
//        string prefix = isV ? "V : " : "A : ";
//        cout << prefix << "(stop) " << (*f) << endl;
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
/// Start the elf pipeline, thread-safe to avoid double start
void playElf(GoblinData &data) {
    using namespace std;
    lock_guard<mutex> lock(data.mutexElfStart);
    // We check again under mutex, the start code runs only once strictly !
    if (!data.flagElfStarted) {
        cout << "PLAYELF !!!! PLAYELF !!!! PLAYELF !!!! " << endl;
//        diagnose(data.elfSrcV);
        GstStateChangeReturn ret = gst_element_set_state(data.elfPipeline, GST_STATE_PLAYING);
        myAssert(ret != GST_STATE_CHANGE_FAILURE, "Can't play Elf !");
        data.flagElfStarted = true;
    }
}
//======================================================================================================================
/// Process video frames with openCV
void codeThreadProcessV(GoblinData &data) {
    using namespace std;
    using namespace cv;
    while (true || !data.flagStop) {
        // Wait if the ELF pipeline does not want data
//        cout << "V : flag = " << data.flagRunV << " : " << &(data.flagRunV) << endl;
        while (data.flagElfStarted && !data.flagRunV && !data.flagStop) {
//            cout << "V: (wait)" << endl;
            this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        // Check again after waiting
        if (data.flagStop)
            break;

        // Pull the sample
        if (gst_app_sink_is_eos(GST_APP_SINK(data.goblinSinkV))) {
            cout << "V: EOS !" << endl;
            break;
        }

        GstSample *sample = gst_app_sink_pull_sample(GST_APP_SINK(data.goblinSinkV));
        if (sample == nullptr) {
            cout << "V: NO sample !" << endl;
            break;
        }

        // Get width and height
        GstCaps *caps = gst_sample_get_caps(sample);
        myAssert(caps != nullptr);
//        printCaps(caps, "");

        GstStructure *s = gst_caps_get_structure(caps, 0);
        int imW, imH;
        myAssert(gst_structure_get_int(s, "width", &imW));
        myAssert(gst_structure_get_int(s, "height", &imH));
        int f1, f2;
        myAssert(gst_structure_get_fraction(s, "framerate", &f1, &f2));
        cout << "V: Sample: W = " << imW << ", H = " << imH << ", framerate = " << f1 << " / " << f2 << endl;

        if (!data.flagElfStartedV) {
            // Set elf caps video
            GstCaps *capsElf = gst_caps_copy(caps);
//            cout << "capsElf = " << endl;
//            printCaps(capsElf, "   ");
            g_object_set(data.elfSrcV, "caps", capsElf, nullptr);
            gst_caps_unref(capsElf);
            data.flagElfStartedV = true;
        }

        // Start the elf pipeline, thread-safe to avoid double start
        if (!data.flagElfStarted && data.flagElfStartedV && data.flagElfStartedA) {
            playElf(data);
        }

        // Process sample
        GstBuffer *bufferIn = gst_sample_get_buffer(sample);
        GstMapInfo mapIn;
        myAssert(gst_buffer_map(bufferIn, &mapIn, GST_MAP_READ));
        myAssert(mapIn.size == imW * imH * 3);
//        cout << "size = " << map.size << " ==? " << imW*imH*3 << endl;

        // Timestamp
        uint64_t pts = bufferIn->pts;

        // Modify the image
        // Clone to be safe, we don't want to modify the input buffer
        Mat frame = Mat(imH, imW, CV_8UC3, (void *) mapIn.data).clone();
        gst_buffer_unmap(bufferIn, &mapIn);
        // Modify the frame
        Mat frameMid(frame, Rect2i(imW/3, imH/3, imW/3, imH/3));
        bitwise_not(frameMid, frameMid);

        // Create the output bufer and send it to elfSrc
        int bufferSize = frame.cols * frame.rows * 3;
        GstBuffer *bufferOut = gst_buffer_new_and_alloc(bufferSize);
        GstMapInfo mapOut;
        gst_buffer_map(bufferOut, &mapOut, GST_MAP_WRITE);
        memcpy(mapOut.data, frame.data, bufferSize);
        gst_buffer_unmap(bufferOut, &mapOut);
        bufferOut->pts = pts;
//        bufferOut->duration = bufferIn->duration;
//        bufferOut->offset = bufferIn->offset;
        GstFlowReturn ret = gst_app_src_push_buffer(GST_APP_SRC(data.elfSrcV), bufferOut);

        gst_sample_unref(sample);
    }
    gst_app_src_end_of_stream(GST_APP_SRC(data.elfSrcV));
    cout << "PROCESS THREAD FINISHED : V" << endl;

//    data.flagStop = true;
}
//======================================================================================================================
/// Process audio
void codeThreadProcessA(GoblinData &data) {
    using namespace std;
    while (!data.flagStop) {
        // Wait if the ELF pipeline does not want data
//        cout << "A : flag = " << data.flagRunA << " : " << &(data.flagRunA) << endl;

        while (data.flagElfStarted && !data.flagRunA && !data.flagStop) {
//            cout << "A : (wait)" << endl;
            this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        // Check again after waiting
        if (data.flagStop)
            break;

        // Pull the sample
        if (gst_app_sink_is_eos(GST_APP_SINK(data.goblinSinkA))) {
            cout << "A : EOS !" << endl;
            break;
        }

        GstSample *sample = gst_app_sink_pull_sample(GST_APP_SINK(data.goblinSinkA));
        if (sample == nullptr) {
            cout << "A : NO sample !" << endl;
            break;
        }

        // Get width and height
        GstCaps *caps = gst_sample_get_caps(sample);
        myAssert(caps != nullptr);
//        printCaps(caps, "");

        if (!data.flagElfStartedA) {
            // Set elf caps video
            GstCaps *capsElf = gst_caps_copy(caps);
            g_object_set(data.elfSrcA, "caps", capsElf, nullptr);
            gst_caps_unref(capsElf);
            data.flagElfStartedA = true;
        }

        // Start the elf pipeline, thread-safe to avoid double start
        if (!data.flagElfStarted && data.flagElfStartedV && data.flagElfStartedA) {
            playElf(data);
        }

        // Process sample
        GstBuffer *bufferIn = gst_sample_get_buffer(sample);
        GstMapInfo mapIn;
        myAssert(gst_buffer_map(bufferIn, &mapIn, GST_MAP_READ));
//        cout << "A : sample ! bufferSize = "<< mapIn.size << endl;

        // Timestamp
        uint64_t pts = bufferIn->pts;

        // Create the output bufer and send it to elfSrc
        int bufferSize = mapIn.size;
        GstBuffer *bufferOut = gst_buffer_new_and_alloc(bufferSize);
        GstMapInfo mapOut;
        gst_buffer_map(bufferOut, &mapOut, GST_MAP_WRITE);
        memcpy(mapOut.data, mapIn.data, bufferSize);
        gst_buffer_unmap(bufferIn, &mapIn);
        gst_buffer_unmap(bufferOut, &mapOut);
        bufferOut->pts = bufferIn->pts;
        bufferOut->duration = bufferIn->duration;
        GstFlowReturn ret = gst_app_src_push_buffer(GST_APP_SRC(data.elfSrcA), bufferOut);

        gst_sample_unref(sample);
    }
    gst_app_src_end_of_stream(GST_APP_SRC(data.elfSrcA));
    cout << "PROCESS THREAD FINISHED : A" << endl;
//    data.flagStop = true;
}
//======================================================================================================================
int main(int argc, char **argv) {
    using namespace std;
    cout << "AV1" << endl;

    string uri = "file:///home/seymour/Videos/tvoya.mp4";
    gst_init(&argc, &argv);
    GoblinData data;

    // Create Goblin (input) pipeline
    string goblinPipeStr = "uridecodebin3 uri=" + uri + " name=u ! queue ! videoconvert ! appsink sync=false name=goblin_sink_v caps=video/x-raw,format=BGR " +
            "u. ! queue ! audioconvert ! appsink sync=false name=goblin_sink_a caps=audio/x-raw,format=S16LE,layout=interleaved";
    GError *err = nullptr;
    data.goblinPipeline = gst_parse_launch(goblinPipeStr.c_str(), &err);
    checkErr(err);
    myAssert(data.goblinPipeline);
    data.goblinSinkV = gst_bin_get_by_name(GST_BIN (data.goblinPipeline), "goblin_sink_v");
    myAssert(data.goblinSinkV);
    data.goblinSinkA = gst_bin_get_by_name(GST_BIN (data.goblinPipeline), "goblin_sink_a");
    myAssert(data.goblinSinkA);

    // Create Elf (output) pipeline
    string elfPipeStr = string("appsrc name=elf_src_v format=time caps=video/x-raw,format=BGR ! queue ! videoconvert ! autovideosink ") +
            "appsrc name=elf_src_a format=time caps=audio/x-raw,format=S16LE,layout=interleaved ! queue ! audioconvert ! audioresample ! autoaudiosink";


//    string elfPipeStr = string("appsrc name=elf_src_v format=time caps=video/x-raw,format=BGR ! queue ! videoconvert ! x264enc ! avimux name=m ! filesink location=out.avi ") +
//            "appsrc name=elf_src_a format=time caps=audio/x-raw,format=S16LE,layout=interleaved ! queue ! audioconvert ! audioresample ! avenc_aac ! m.";

    data.elfPipeline = gst_parse_launch(elfPipeStr.c_str(), &err);
    checkErr(err);
    myAssert(data.elfPipeline);
    data.elfSrcV = gst_bin_get_by_name(GST_BIN(data.elfPipeline), "elf_src_v");
    myAssert(data.elfSrcV);
    data.elfSrcA = gst_bin_get_by_name(GST_BIN(data.elfPipeline), "elf_src_a");
    myAssert(data.elfSrcA);
    g_signal_connect(data.elfSrcV, "need-data", G_CALLBACK(startFeed), &data);
    g_signal_connect(data.elfSrcV, "enough-data", G_CALLBACK(stopFeed), &data);
    g_signal_connect(data.elfSrcA, "need-data", G_CALLBACK(startFeed), &data);
    g_signal_connect(data.elfSrcA, "enough-data", G_CALLBACK(stopFeed), &data);


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
    thread threadProcessV([&data]{
        codeThreadProcessV(data);
    });
    thread threadProcessA([&data]{
        codeThreadProcessA(data);
    });
    threadBusGoblin.join();
    threadBusElf.join();
    threadProcessV.join();
    threadProcessA.join();

    // Stop and destroy pipelines
    gst_element_set_state(data.goblinPipeline, GST_STATE_NULL);
    gst_object_unref(data.goblinPipeline);

    return 0;
}
//======================================================================================================================

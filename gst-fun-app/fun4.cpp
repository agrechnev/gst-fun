//
// Created by IT-JIM 
// FUN4: appsrc example, create video with opencv, display with gstreamer, try to keep a correct FPS

#include <iostream>
#include <string>
#include <stdexcept>
#include <thread>
#include <atomic>

#include <gst/gst.h>
#include <gst/video/video.h>
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
    GstElement *pipeline;
    GstElement *source;
    GstElement *sinkVideo;
    GstElement *convVideo;

    // Misc
    cv::Size2i size;
    double fps;

    // Flags
    std::atomic_bool flagStop{false};
//    std::atomic_bool flagRun{false};
    bool flagRun = false;
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
static bool busCB(GstBus *bus, GstMessage *msg, void *data0) {
    using namespace std;

    GoblinData *data = (GoblinData *) data0;
    GstMessageType mType = GST_MESSAGE_TYPE(msg);
    cout << "BUS : mType = " << mType << endl;
    switch (mType) {
        case (GST_MESSAGE_ERROR):
            GError *err;
            gchar *dbg;
            gst_message_parse_error(msg, &err, &dbg);
            cout << " ERR = " << err->message << " FROM " << GST_OBJECT_NAME(msg->src) << endl;
            cout << "DBG = " << dbg << endl;
            g_clear_error(&err);
            g_free(dbg);
            data->flagStop = true;
            return false;
            break;
        case (GST_MESSAGE_EOS) :
            cout << " EOS !" << endl;
            data->flagStop = true;
            return false;
            break;
        case (GST_MESSAGE_STATE_CHANGED):
            cout << " State changed !" << endl;
            if (GST_MESSAGE_SRC(msg) == GST_OBJECT(data->pipeline)) {
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
cv::Size2i inspectFile(const std::string &fileName, double &fps) {
    using namespace std;
    using namespace cv;
    VideoCapture cap(fileName);
    myAssert(cap.isOpened(), "Cannot read video file " + fileName);
    fps = cap.get(CAP_PROP_FPS);
    Size2i size;
    size.width = (int) cap.get(CAP_PROP_FRAME_WIDTH);
    size.height = (int) cap.get(CAP_PROP_FRAME_HEIGHT);
    cout << "VIDEO : " << fileName << endl;
    cout << "FPS = " << fps << ", SIZE = " << size << endl;
    return size;
}

//======================================================================================================================
void codeThreadSrc(GoblinData &data, const std::string &fileName) {
    using namespace std;
    using namespace cv;
    VideoCapture cap(fileName);
    myAssert(cap.isOpened(), "Cannot read video file " + fileName);

    Mat frame;
    while (!data.flagStop) {
        if (!data.flagRun) {
            cout << "(wait)" << endl;
            this_thread::sleep_for(chrono::milliseconds(10));
            continue;
        }

        // Read frame
        cap.read(frame);
//        cout << "frame !" << endl;
        if (frame.empty()) {
            // EOF !
            break;
        }
        myAssert(frame.size() == data.size && frame.channels() == 3);

        // Copy data to buffer
        int bufferSize = frame.cols * frame.rows * 3;
        GstBuffer *buffer = gst_buffer_new_and_alloc(bufferSize);
        GstMapInfo map;
        gst_buffer_map(buffer, &map, GST_MAP_WRITE);
        memcpy(map.data, frame.data, bufferSize);
        gst_buffer_unmap(buffer, &map);


        // Send buffer to gstreamer
        GstFlowReturn ret = gst_app_src_push_buffer(GST_APP_SRC(data.source), buffer);
//        cout << "push ret = " << ret << endl;
//        gst_buffer_unref(buffer);

        // Note: framerate caps doesn't do shit, if e want realtime, we'll have to enforce it BY HAND
        this_thread::sleep_for(chrono::milliseconds(40));
    }
    gst_app_src_end_of_stream(GST_APP_SRC(data.source));
}

//======================================================================================================================
static void startFeed(GstElement *source, guint size, GoblinData *data) {
    using namespace std;
    if (!data->flagRun) {
        cout << "startFeed !" << endl;
        data->flagRun = true;
    } else {
//        cout << "(start)" << endl;
    }
}

//======================================================================================================================
static void stopFeed(GstElement *source, GoblinData *data) {
    using namespace std;
    if (data->flagRun) {
        cout << "stopFeed !" << endl;
        data->flagRun = false;
    } else {
//        cout << "(stop)" << data->flagRun << " " << data << endl;
    }
}

//======================================================================================================================
int main(int argc, char **argv) {
    using namespace std;
    cout << "fun4" << endl;

    string fileName = "/home/seymour/Videos/tvoya.mp4";
    gst_init(&argc, &argv);

    // Crete pipeline
    GoblinData data;
    data.source = gst_element_factory_make("appsrc", "goblin_source");
    data.convVideo = gst_element_factory_make("videoconvert", "goblin_convert");
    data.sinkVideo = gst_element_factory_make("autovideosink", "goblin_sink");
    data.pipeline = gst_pipeline_new("goblin_pipeline");
    myAssert(data.source && data.sinkVideo && data.convVideo && data.pipeline);

    // Examine the input file, determine width, height
    data.size = inspectFile(fileName, data.fps);

    // CONFIGURE SOURCE
    ostringstream oss;
    oss << "video/x-raw,format=BGR,width=" << data.size.width << ",height=" << data.size.height <<",framerate=24/5";
    cout << "CAPS : " << oss.str() << endl;
    GstCaps *capsVideo = gst_caps_from_string(oss.str().c_str());
    g_object_set(data.source, "caps", capsVideo, nullptr);
    gst_caps_unref(capsVideo);
    g_signal_connect(data.source, "need-data", G_CALLBACK(startFeed), &data);
    g_signal_connect(data.source, "enough-data", G_CALLBACK(stopFeed), &data);

    // Add + link
    gst_bin_add_many(GST_BIN(data.pipeline), data.source, data.convVideo, data.sinkVideo, nullptr);
    myAssert(gst_element_link_many(data.source, data.convVideo, data.sinkVideo, nullptr));

    // Play
    GstBus *bus = gst_element_get_bus(data.pipeline);
    GstStateChangeReturn ret = gst_element_set_state(data.pipeline, GST_STATE_PLAYING);
    myAssert(ret != GST_STATE_CHANGE_FAILURE, "Can't play !");


    // Start the appsrc thread (we could have used a main thread, just for fun)
    thread threadSrc([&data, &fileName] {
        codeThreadSrc(data, fileName);
    });

    // Run the bus
    for (;;) {
        GstMessage *msg = gst_bus_timed_pop(bus, GST_CLOCK_TIME_NONE);
        myAssert(msg);
        bool res = busCB(bus, msg, &data);
        gst_message_unref(msg);
        if (!res)
            break;
    }

    threadSrc.join();

    gst_element_set_state(data.pipeline, GST_STATE_NULL);
    gst_object_unref(bus);
    gst_object_unref(data.pipeline);
    return 0;
}

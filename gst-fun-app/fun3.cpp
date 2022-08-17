// FUN3: Use appsink with opencv to show video from a file (no sound)

#include <iostream>
#include <string>
#include <stdexcept>
#include <thread>

#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/app/gstappsink.h>

#include <opencv2/opencv.hpp>


//======================================================================================================================
inline void myAssert(bool b, const std::string &s = "MYASSERT ERROR !") {
    if (!b)
        throw std::runtime_error(s);
}

//======================================================================================================================
struct GoblinData {
    GstElement *pipeline;
    GstElement *sinkVideo;
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
static gboolean errorCB(GstBus *bus, GstMessage *msg, void *data) {
    using namespace std;

    if (GST_MESSAGE_TYPE (msg) == GST_MESSAGE_ERROR) {
        GError *err;
        gchar *dbg;
        gst_message_parse_error(msg, &err, &dbg);
        cout << " ERR = " << err->message << " FROM " << GST_OBJECT_NAME(msg->src) << endl;
        cout << "DBG = " << dbg << endl;
        g_clear_error(&err);
        g_free(dbg);
    }
    return TRUE;
}

//======================================================================================================================
int main(int argc, char **argv) {
    using namespace std;
    cout << "fun3" << endl;

    string uri = "file:///home/seymour/Videos/tvoya.mp4";

    GoblinData data;
    gst_init(&argc, &argv);

    // Set up the pipeline
    string pipeStr = "uridecodebin uri=" + uri + " ! videoconvert ! video/x-raw, format=BGR ! appsink name=testsink max-buffers=2";
    data.pipeline = gst_parse_launch(pipeStr.c_str(), nullptr);
    myAssert(data.pipeline, "Can't create pipeline !");
    data.sinkVideo = gst_bin_get_by_name(GST_BIN (data.pipeline), "testsink");
    myAssert(data.sinkVideo);

//    GstBus *bus = gst_element_get_bus(data.pipeline);
//    gst_bus_add_signal_watch(bus);
//    g_signal_connect(G_OBJECT(bus), "message::error", (GCallback)errorCB, &data);
//    gst_object_unref(bus);

    gst_bus_add_watch (GST_ELEMENT_BUS (data.pipeline), errorCB, &data);


    GstStateChangeReturn ret = gst_element_set_state(data.pipeline, GST_STATE_PLAYING);
    myAssert(ret != GST_STATE_CHANGE_FAILURE, "Can't play !");

    for (;;) {
        // Pull the sample
        if (gst_app_sink_is_eos(GST_APP_SINK(data.sinkVideo))) {
            cout << "EOS !" << endl;
            break;
        }

        GstSample *sample = gst_app_sink_pull_sample(GST_APP_SINK(data.sinkVideo));
        if (sample == nullptr) {
            cout << "NO sample !" << endl;
            break;
        }


        // Get width and height
        GstCaps *caps = gst_sample_get_caps(sample);
        myAssert(caps != nullptr);
        GstStructure *s = gst_caps_get_structure(caps, 0);
        int imW, imH;
        myAssert(gst_structure_get_int(s, "width", &imW));
        myAssert(gst_structure_get_int(s, "height", &imH));
        cout << "Sample: W = " << imW << ", H = " << imH << endl;

//        cout << "sample !" << endl;
        // Process sample
        GstBuffer *buffer = gst_sample_get_buffer(sample);
        GstMapInfo map;
        myAssert(gst_buffer_map(buffer, &map, GST_MAP_READ));
        myAssert(map.size == imW * imH * 3);
//        cout << "size = " << map.size << " ==? " << imW*imH*3 << endl;

        cv::Mat frame(imH, imW, CV_8UC3, (void *) map.data);
        cv::imshow("frame", frame);
        int key = cv::waitKey(1);

        gst_buffer_unmap(buffer, &map);
        gst_sample_unref(sample);
        if (27 == key)
            break;
    }

    gst_element_set_state(data.pipeline, GST_STATE_NULL);
    gst_object_unref(data.pipeline);

    return 0;
}
//======================================================================================================================
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
int main(int argc, char **argv) {
    using namespace std;
    cout << "fun1" << endl;

//    string uri = "file:///home/seymour/Videos/tvoya.mp4";

    GoblinData data;
    gst_init(&argc, &argv);

    string pipeStr = "videotestsrc pattern=18 ! video/x-raw, width=640, height=480, format=BGR ! appsink name=testsink max-buffers=2";
    data.pipeline = gst_parse_launch(pipeStr.c_str(), nullptr);
    data.sinkVideo = gst_bin_get_by_name(GST_BIN (data.pipeline), "testsink");

    gst_element_set_state(data.pipeline, GST_STATE_PLAYING);

    for (;;) {
        if (gst_app_sink_is_eos(GST_APP_SINK(data.sinkVideo))) {
            cout << "EOS !" << endl;
            break;
        }
        GstSample *sample = gst_app_sink_pull_sample(GST_APP_SINK(data.sinkVideo));
        if (sample == nullptr) {
            cout << "NO sample !" << endl;
            break;
        }
        cout << "sample !" << endl;
        // Process sample
        GstBuffer * buffer = gst_sample_get_buffer(sample);
        GstMapInfo map;
        myAssert(gst_buffer_map(buffer, &map, GST_MAP_READ));
        myAssert(map.size == 640*480*3);
//        cout << "size = " << map.size << " ==? " << 640*480*3 << endl;

        cv::Mat frame(480, 640, CV_8UC3, (void *)map.data);
        cv::imshow("frame", frame);
        cv::waitKey(1);

        gst_buffer_unmap(buffer, &map);
        gst_sample_unref(sample);
    }

    gst_element_set_state(data.pipeline, GST_STATE_NULL);
    gst_object_unref(data.pipeline);

    return 0;
}
//======================================================================================================================
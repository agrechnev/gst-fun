// FUN2: Use appsink with opencv to show video, minor improvements, create pipeline by hand

#include <iostream>
#include <sstream>
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
    GstElement *source;
    GstElement *sinkVideo;
    GstElement *convVideo;
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

    // Set up the pipeline
    data.source = gst_element_factory_make("videotestsrc", "goblin_source");
    data.sinkVideo = gst_element_factory_make("appsink", "goblin_sink");
    data.pipeline = gst_pipeline_new("goblin_pipeline");
    myAssert(data.source && data.sinkVideo && data.pipeline);
    gst_bin_add_many(GST_BIN(data.pipeline), data.source, data.sinkVideo, nullptr);
    myAssert(gst_element_link_many(data.source, data.sinkVideo, nullptr));


    // Set up source
    g_object_set(data.source, "pattern", 18, "is-live", true, nullptr);


    // Set up the appsink
//    GstVideoInfo info;
//    gst_video_info_set_format(&info, GST_VIDEO_FORMAT_BGR, 640, 480);
//    GstCaps *capsVideo = gst_video_info_to_caps(&info);

//    string capsStr = "video/x-raw,format=BGR,width=640,height=480,pixel-aspect-ratio=1/1";
    string capsStr = "video/x-raw,format=BGR,width=640,height=480";
    GstCaps *capsVideo = gst_caps_from_string(capsStr.c_str());

//    printCaps(capsVideo, " ");
//    exit(0);
    g_object_set(data.sinkVideo, "caps", capsVideo, nullptr);
    gst_caps_unref(capsVideo);


    gst_element_set_state(data.pipeline, GST_STATE_PLAYING);

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
//        cout << "W = " << imW << ", H = " << imH << endl;

//        cout << "sample !" << endl;
        // Process sample
        GstBuffer *buffer = gst_sample_get_buffer(sample);
        GstMapInfo map;
        myAssert(gst_buffer_map(buffer, &map, GST_MAP_READ));
        myAssert(map.size == imW * imH * 3);
//        cout << "size = " << map.size << " ==? " << 640*480*3 << endl;

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
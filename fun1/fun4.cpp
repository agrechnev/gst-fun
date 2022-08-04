//
// Created by IT-JIM 
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
    GstElement *tee;
    GstElement *queueAudio;
    GstElement *queueVideo;
    GstElement *convertAudio;
    GstElement *resampleAudio;
    GstElement *sinkAudio;
    GstElement *visual;
    GstElement *convertVideo;
    GstElement *sinkVideo;
};

//======================================================================================================================
int main(int argc, char **argv) {
    using namespace std;
    cout << "fun4 !!!!" << endl;

    GoblinData data;
    gst_init(&argc, &argv);

    // Create pipeline
    data.source = gst_element_factory_make("audiotestsrc", "goblin_source");
    data.tee = gst_element_factory_make("tee", "goblin_tee");
    data.queueAudio = gst_element_factory_make("queue", "goblin_queue_audio");
    data.queueVideo = gst_element_factory_make("queue", "goblin_queue_video");
    data.convertAudio = gst_element_factory_make("audioconvert", "goblin_convert_audio");
    data.resampleAudio = gst_element_factory_make("audioresample", "goblin_resample_audio");
    data.sinkAudio = gst_element_factory_make("autoaudiosink", "goblin_sink_audio");
    data.visual = gst_element_factory_make("wavescope", "goblin_wavescope");
    data.convertVideo = gst_element_factory_make("videoconvert", "goblin_convert_video");
    data.sinkVideo = gst_element_factory_make("autovideosink", "goblin_sink_video");
    data.pipeline = gst_pipeline_new("goblin_pipeline");

    myAssert(data.source && data.tee && data.queueAudio && data.queueVideo);
    myAssert(data.convertAudio && data.resampleAudio && data.sinkAudio);
    myAssert(data.visual && data.convertVideo && data.sinkVideo && data.pipeline);

    // Configure params
    g_object_set(data.source, "freq", 215.0f, nullptr);
    g_object_set(data.visual, "shader", 0, "style", 1, nullptr);

    // Add elements and link what we can
    gst_bin_add_many(GST_BIN(data.pipeline),
                     data.source, data.tee, data.queueAudio, data.queueVideo,
                     data.convertAudio, data.resampleAudio, data.sinkAudio,
                     data.visual, data.convertVideo, data.sinkVideo,
                     nullptr);

    myAssert(gst_element_link_many(data.source, data.tee, nullptr));
    myAssert(gst_element_link_many(data.queueAudio, data.convertAudio, data.resampleAudio, data.sinkAudio, nullptr));
    myAssert(gst_element_link_many(data.queueVideo, data.visual, data.convertVideo, data.sinkVideo, nullptr));

    // Manually link tee
    GstPad *padTeeAudio = gst_element_request_pad_simple(data.tee, "src_%u");
    GstPad *padTeeVideo = gst_element_request_pad_simple(data.tee, "src_%u");
    GstPad *padQueueAudio = gst_element_get_static_pad(data.queueAudio, "sink");
    GstPad *padQueueVideo = gst_element_get_static_pad(data.queueVideo, "sink");
    myAssert(gst_pad_link(padTeeAudio, padQueueAudio) == GST_PAD_LINK_OK);
    myAssert(gst_pad_link(padTeeVideo, padQueueVideo) == GST_PAD_LINK_OK);
    gst_object_unref(padQueueAudio);
    gst_object_unref(padQueueVideo);

    // Play the pipeline
    gst_element_set_state(data.pipeline, GST_STATE_PLAYING);
    GstBus *bus = gst_element_get_bus(data.pipeline);
    GstMessage *msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE,
                                                 GstMessageType(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

    // Release the request pads from the Tee, and unref them
    gst_element_release_request_pad(data.tee, padTeeAudio);
    gst_element_release_request_pad(data.tee, padTeeVideo);
    gst_object_unref(padTeeAudio);
    gst_object_unref(padTeeVideo);

    if (msg)
        gst_message_unref(msg);

    gst_object_unref(bus);
    gst_element_set_state(data.pipeline, GST_STATE_NULL);
    gst_object_unref(data.pipeline);

    return 0;
}

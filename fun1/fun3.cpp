//
// Created by IT-JIM 
// Fun with pad caps


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
static gboolean printField(GQuark field, const GValue *value, gpointer pfx) {
    using namespace std;
    gchar *str = gst_value_serialize(value);
    cout << (char *)pfx << " " << g_quark_to_string(field) << " " << str << endl;
    g_free(str);
    return TRUE;
}

//======================================================================================================================
void printCaps(const GstCaps *caps, const std::string & pfx) {
    using namespace std;
    if (caps == nullptr)
        return;
    if (gst_caps_is_any(caps))
        cout << pfx << "ANY" << endl;
    else if (gst_caps_is_empty(caps))
        cout << pfx << "EMPTY" << endl;
    for (int i = 0; i < gst_caps_get_size(caps); ++i) {
        GstStructure *s = gst_caps_get_structure(caps, i);
        cout << pfx <<  gst_structure_get_name(s) << endl;
        gst_structure_foreach(s, &printField, (gpointer)pfx.c_str());
    }
}

//======================================================================================================================
void printPadTemplates(GstElementFactory *factory) {
    using namespace std;
    cout << "===================================" << endl;
    cout << "Pad Templates for " << gst_element_factory_get_longname(factory) << ", N = "
         << gst_element_factory_get_num_pad_templates(factory) << endl;

    const GList *pads = gst_element_factory_get_static_pad_templates(factory);
    while (pads) {
        GstStaticPadTemplate *padTemplate = (GstStaticPadTemplate *) pads->data;
        pads = g_list_next(pads);

        if (padTemplate->direction == GST_PAD_SRC)
            cout << "SRC template : " << padTemplate->name_template << endl;
        else if (padTemplate->direction == GST_PAD_SINK)
            cout << "SINK template : " << padTemplate->name_template << endl;
        else
            cout << "UNKNOWN !! template : " << padTemplate->name_template << endl;

        if (padTemplate->presence == GST_PAD_ALWAYS)
            cout << "Availability: Always" << endl;
        else if (padTemplate->presence == GST_PAD_SOMETIMES)
            cout << "Availability: Sometimes" << endl;
        else if (padTemplate->presence == GST_PAD_REQUEST)
            cout << "Availability: On request" << endl;
        else
            cout << "Availability: UNKNOWN !!!" << endl;

        if (padTemplate->static_caps.string) {
            GstCaps *caps = gst_static_caps_get(&padTemplate->static_caps);
            cout << "Caps :" << endl;
            printCaps(caps, "    ");
            gst_caps_unref(caps);
        }
    }

}

//======================================================================================================================
int main(int argc, char **argv) {
    using namespace std;
    cout << "fun3 !!!!" << endl;

    gst_init(&argc, &argv);

    // Factories
    GstElementFactory *factSource = gst_element_factory_find("audiotestsrc");
    GstElementFactory *factSink = gst_element_factory_find("autoaudiosink");
    printPadTemplates(factSource);
    printPadTemplates(factSink);
    myAssert(factSource && factSink);

    // Elements
    GstElement *source = gst_element_factory_create(factSource, "goblin_source");
    GstElement *sink = gst_element_factory_create(factSink, "goblin_sink");
    myAssert(source && sink);

    // Pipeline
    GstElement *pipeline = gst_pipeline_new("gonlin_pipeline");
    gst_bin_add_many(GST_BIN(pipeline), source, sink, nullptr);
    myAssert(gst_element_link(source, sink));

    exit(0);

    //======================= Play
    int ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    myAssert(ret != GST_STATE_CHANGE_FAILURE);

    GstBus *bus = gst_element_get_bus(pipeline);
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
                if (GST_MESSAGE_SRC(msg) == GST_OBJECT(pipeline)) {
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
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    gst_object_unref(factSink);
    gst_object_unref(factSource);
    return 0;
}

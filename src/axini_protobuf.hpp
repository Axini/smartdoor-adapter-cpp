// Copyright 2023 Axini B.V. https://www.axini.com, see: LICENSE.txt.

#ifndef AXINI_PROTOBUF_HPP
#define AXINI_PROTOBUF_HPP

#include "pa_protobuf.hpp"

// This file provides several utility functions to create Protobuf messages.

using namespace PluginAdapter::Api;

namespace axini {
    long current_timestamp();

    std::string to_string(Message m);
    std::string to_string(Label l);
    std::string to_string(Label_Parameter p);
    std::string to_string(Label_Parameter_Value v);
    std::string to_string(Configuration c);
    std::string to_string(Configuration_Item item);

    Message message(Label label);
    Message message(Announcement announcement);
    Message message_error(std::string error_message);
    Message message_ready();

    Announcement announcement(std::string, std::vector<Label>, Configuration);

    Label label(Label, std::string physical_label, long timestamp);
    Label label(Label, std::string physical_label, long timestamp, long correlation_id);

    Label stimulus(std::string name);
    Label stimulus(std::string name, std::string channel);
    Label stimulus(std::string name, std::string channel, std::vector<Label_Parameter> parameters);

    Label response(std::string name);
    Label response(std::string name, std::string channel);
    Label response(std::string name, std::string channel, std::vector<Label_Parameter> parameters);

    Label_Parameter parameter(std::string name, Label_Parameter_Value value);

    Label_Parameter_Value parameter_value(std::string s);
    Label_Parameter_Value parameter_value(int ii);
    Label_Parameter_Value parameter_value(double dd);
    Label_Parameter_Value parameter_value(bool bb);
    Label_Parameter_Value parameter_value_date(long ll);
    Label_Parameter_Value parameter_value_time(long ll);

    std::string get_string_value_from(Configuration, std::string);
}

#endif // AXINI_PROTOBUF_HPP

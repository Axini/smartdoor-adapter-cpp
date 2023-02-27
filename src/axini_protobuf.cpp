// Copyright 2023 Axini B.V. https://www.axini.com, see: LICENSE.txt.

#include <string>
#include <sstream>
#include "axini_protobuf.hpp"

// Current time in nano seconds since EPOCH.
long axini::current_timestamp() {
    using namespace std::chrono;
    long duration_nsec = time_point_cast<nanoseconds>(system_clock::now())
                            .time_since_epoch().count();
    return duration_nsec;
}

std::string axini::to_string(Message msg) {
    if (msg.has_error())
        return "error";
    else if (msg.has_announcement())
        return "announcement";
    else if (msg.has_configuration())
        return to_string(msg.configuration());
    else if (msg.has_label())
        return to_string(msg.label());
    else if (msg.has_reset())
        return "reset";
    else if (msg.has_ready())
        return "ready";
    else
        return "?? unknown message !!";
}

std::string axini::to_string(Label label) {
    std::string direction = (label.type() == Label::STIMULUS) ? "?" : "!";
    std::string channel = label.channel();

    std::stringstream ps;
    bool first_param = true;
    for (const Label_Parameter& parameter : label.parameters()) {
        ps << (first_param ? "" : ", ") << to_string(parameter);
        first_param = false;
    }

    std::string parameters = (first_param ? "" : " (" + ps.str() + ")");

    std::stringstream s;
    s << direction << "[" << channel << "]" << label.label() << parameters;
    return s.str();
}

std::string axini::to_string(Label_Parameter param) {
    std::stringstream s;
    s << param.name() << ": " << to_string(param.value());
    return s.str();
}

// TODO: add code for date, time, array, struct and hash parameters.
// These are not needed for the SmartDoor SUT, though.
std::string axini::to_string(Label_Parameter_Value val) {
    if (val.has_string())
        return val.string();
    else if (val.has_integer())
        return std::to_string(val.integer());
    else if (val.has_decimal())
        return std::to_string(val.decimal());
    else if (val.has_boolean())
        return (val.boolean() ? "true" : "false");
    else {
        return "not yet implemented";
    }
}

std::string axini::to_string(Configuration config) {
    std::stringstream s;
    bool first_item = true;
    for (const Configuration_Item& item : config.items()) {
        s << (first_item ? "" : "\n") << to_string(item);
        first_item = false;
    }
    return s.str();
}

std::string axini::to_string(Configuration_Item item) {
    std::string key = item.key();
    std::string value;

    if (item.has_string())
        value = item.string();
    else if (item.has_integer())
        value = std::to_string(item.integer());
    else if (item.has_float_())
        value = std::to_string(item.float_());
    else if (item.has_boolean())
        value = std::to_string(item.boolean());
    else
        value = "?? unknown type";

    return key + " => " + value + " (" + item.description() + ")";
}

Message axini::message(Label label) {
    Label* label_ptr = new Label(label);
    Message message;
    message.set_allocated_label(label_ptr); // message now "owns" label_ptr
    return message;
}

Message axini::message(Announcement announcement) {
    Announcement* announcement_ptr = new Announcement(announcement);
    Message message;
    message.set_allocated_announcement(announcement_ptr);
    return message;
}

Message axini::message_error(std::string error_message) {
    Message_Error* error_ptr = new Message_Error;
    error_ptr->set_message(error_message);
    Message message;
    message.set_allocated_error(error_ptr);
    return message;
}

Message axini::message_ready() {
    Message_Ready* ready_ptr = new Message_Ready;
    Message message;
    message.set_allocated_ready(ready_ptr);
    return message;
}

Announcement axini::announcement(std::string name, std::vector<Label> labels,
                                Configuration configuration) {

    Configuration* configuration_ptr = new Configuration(configuration);

    Announcement announcement;
    announcement.set_name(name);
    announcement.set_allocated_configuration(configuration_ptr);

    // Copy labels to announcement.
    for (Label& label : labels) {
        Label* label_ptr = announcement.add_labels();
        *label_ptr = label;
    }

    return announcement;
}

Label axini::label(Label label, std::string physical_label, long timestamp) {
    Label new_label = label;
    new_label.set_physical_label(physical_label);
    new_label.set_timestamp(timestamp);
    return new_label;
}

Label axini::label(Label label, std::string physical_label, long timestamp,
                   long correlation_id) {
    Label new_label = label;
    new_label.set_physical_label(physical_label);
    new_label.set_timestamp(timestamp);
    new_label.set_correlation_id(correlation_id);
    return new_label;
}

Label axini::stimulus(std::string name) {
    Label label;
    label.set_type(Label::STIMULUS);
    label.set_label(name);
    return label;
}

Label axini::stimulus(std::string name, std::string channel) {
    Label label;
    label.set_type(Label::STIMULUS);
    label.set_label(name);
    label.set_channel(channel);
    return label;
}

Label axini::stimulus(std::string name,
                      std::string channel,
                      std::vector<Label_Parameter> parameters) {
    Label label;
    label.set_type(Label::STIMULUS);
    label.set_label(name);
    label.set_channel(channel);

    for (Label_Parameter& parameter : parameters) {
        Label_Parameter* parameter_ptr = label.add_parameters();
        *parameter_ptr = parameter;
    }

    return label;
}

Label axini::response(std::string name) {
    Label label;
    label.set_type(Label::RESPONSE);
    label.set_label(name);
    return label;
}

Label axini::response(std::string name, std::string channel) {
    Label label;
    label.set_type(Label::RESPONSE);
    label.set_label(name);
    label.set_channel(channel);
    return label;
}

Label axini::response(std::string name,
                      std::string channel,
                      std::vector<Label_Parameter> parameters) {

    Label label;
    label.set_type(Label::RESPONSE);
    label.set_label(name);
    label.set_channel(channel);

    for (Label_Parameter& parameter : parameters) {
        Label_Parameter* parameter_ptr = label.add_parameters();
        *parameter_ptr = parameter;
    }

    return label;
}

// ----- Label_Parameter

// TODO: add parameter_value methods for an array, struct and hash.
// These are not needed for the SmartDoor adapter, though.

Label_Parameter axini::parameter(std::string name, Label_Parameter_Value value) {
    Label_Parameter parameter;
    Label_Parameter_Value* value_ptr = new Label_Parameter_Value(value);
    parameter.set_name(name);
    parameter.set_allocated_value(value_ptr);
    return parameter;
}

Label_Parameter_Value axini::parameter_value(std::string ss) {
    Label_Parameter_Value value;
    value.set_string(ss);
    return value;
}

Label_Parameter_Value axini::parameter_value(int ii) {
    Label_Parameter_Value value;
    value.set_integer(ii);
    return value;
}

Label_Parameter_Value axini::parameter_value(double dd) {
    Label_Parameter_Value value;
    value.set_decimal(dd);
    return value;
}

Label_Parameter_Value axini::parameter_value(bool bb) {
    Label_Parameter_Value value;
    value.set_boolean(bb);
    return value;
}

// TODO: or should we use some Date representation here?
Label_Parameter_Value axini::parameter_value_date(long ll) {
    Label_Parameter_Value value;
    value.set_date(ll);
    return value;
}

// TODO: or should we use some Time representation here?
Label_Parameter_Value axini::parameter_value_time(long ll) {
    Label_Parameter_Value value;
    value.set_time(ll);
    return value;
}

std::string axini::get_string_value_from(Configuration configuration,
                                         std::string key) {
    for (int i = 0; i < configuration.items_size(); i++) {
        const Configuration_Item& item = configuration.items(i);
        if (item.key() == key) {
            return item.has_string() ? item.string() : "";
        }
    }
    return "";
}

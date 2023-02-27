// Copyright 2023 Axini B.V. https://www.axini.com, see: LICENSE.txt.

#include "spdlog/spdlog.h"
#include "adapter_core.hpp"
#include "handler.hpp"
#include "smartdoor_handler.hpp"
#include "smartdoor_connection.hpp"
#include "axini_protobuf.hpp"

// We use boost for to_lower and to_upper.
#include <boost/algorithm/string.hpp>

SmartDoorHandler::SmartDoorHandler()
    : smartdoor_connection_ptr(0) {
    set_configuration(default_configuration());
}

SmartDoorHandler::~SmartDoorHandler() {
    if (smartdoor_connection_ptr != 0) {
        delete smartdoor_connection_ptr;
    }
}

// Prepare to start testing.
void SmartDoorHandler::start() {
    spdlog::info("SmartDoorHandler::start");

    // Stop the "old" connection, we must use the "new" configuration.
    if (smartdoor_connection_ptr != 0) {
        stop();
    }

    Configuration config = get_configuration();
    std::string url = axini::get_string_value_from(config, "url");
    spdlog::info("SmartDoorHandler: trying to connect to SUT @ " + url);

    smartdoor_connection_ptr = new SmartDoorConnection(url);
    smartdoor_connection_ptr->register_handler(this);
    smartdoor_connection_ptr->connect();

    // TODO: add exception handling when things go wrong
    // e.g. invalid url, no connection can be made etc., see the Java version.
}

// Stop testing.
void SmartDoorHandler::stop() {
    spdlog::info("SmartDoorHandler::stop");
    if (smartdoor_connection_ptr != 0) {
        smartdoor_connection_ptr->close(1000, "Adapter is stopped");

        delete smartdoor_connection_ptr;
        smartdoor_connection_ptr = 0;
    }
}

// Prepare for the next test case.
void SmartDoorHandler::reset() {
    spdlog::info("SmartDoorHandler::reset");
    // Try to reuse the WebSocket connection to the SUT.
    if (smartdoor_connection_ptr != 0) {
        send_reset_to_sut();
        send_ready_to_amp();
    } else {
        stop();
        start();
    }
}

std::string SmartDoorHandler::stimulate(Label stimulus) {
    spdlog::info("SmartDoorHandler::stimulate: " + axini::to_string(stimulus));
    std::string sut_message = label_to_sut_message(stimulus);
    smartdoor_connection_ptr->send(sut_message);
    return sut_message;
}

void SmartDoorHandler::send_reset_to_sut() {
    spdlog::info("SmartDoorHandler::send_reset_to_sut");
    Configuration config = get_configuration();
    std::string manufacturer = axini::get_string_value_from(config, "manufacturer");
    std::string reset_string = RESET + ":" + manufacturer;
    smartdoor_connection_ptr->send(reset_string);
    spdlog::info("SmartDoorHandler: sent " + reset_string + " to SUT");
}

void SmartDoorHandler::send_response_to_amp(std::string message) {
    spdlog::info("SmartDoorHandler::send_response_to_amp");
    if (message != RESET_PERFORMED) {
        Label label = sut_message_to_label(message);
        long timestamp = axini::current_timestamp();
        std::string physical_label = message;
        adapter_core_ptr->send_response(label, physical_label, timestamp);
    }
}

Configuration SmartDoorHandler::default_configuration() {
    Configuration configuration;

    Configuration_Item* item_url = configuration.add_items();
    item_url->set_key("url");
    item_url->set_description("WebSocket URL of SmartDoor SUT");
    item_url->set_string(SMARTDOOR_URL);

    Configuration_Item* item_manufacturer = configuration.add_items();
    item_manufacturer->set_key("manufacturer");
    item_manufacturer->set_description("SmartDoor manufacturer to test");
    item_manufacturer->set_string(SMARTDOOR_MANUFACTURER);

    return configuration;
}

std::vector<Label> SmartDoorHandler::get_supported_labels() {
    std::vector<Label> labels;
    std::string channel_name = "door";

    std::string stimuli[] = {"open", "close"};
    std::string stimuli_passcode[] = {"lock", "unlock"};
    std::string responses[] = {
        "opened", "closed", "locked", "unlocked",
        "invalid_command", "invalid_passcode", "incorrect_passcode",
        "shut_off"
    };

    for (std::string label_name : stimuli) {
        labels.push_back(axini::stimulus(label_name, channel_name));
    }

    std::vector<Label_Parameter> parameters;
    Label_Parameter param = axini::parameter("passcode", axini::parameter_value(0));
    parameters.push_back(param);

    for (std::string label_name : stimuli_passcode) {
        labels.push_back(axini::stimulus(label_name, channel_name, parameters));
    }

    for (std::string label_name : responses) {
        labels.push_back(axini::response(label_name, channel_name));
    }

    // extra stimulus to reset the SUT
    std::vector<Label_Parameter> parameters_reset;
    parameters_reset.push_back(
        axini::parameter("manufacturer", axini::parameter_value("")));
    labels.push_back(axini::stimulus("reset", channel_name, parameters_reset));

    return labels;
}

// ----- Converters

// For the SmartDoor SUT the conversion between Protobuf Labels and
// SUT messages is simple (upper <-> lower). Hence, these converters
// can be part of the SmartDoorHandler. For practical SUTs, we typically
// introduce special classes for theses converters.

// Message to label converter.
Label SmartDoorHandler::sut_message_to_label(std::string message) {
    std::string response_message = boost::to_lower_copy(message);
    return axini::response(response_message, "door");
}

// Label to message converter.
std::string SmartDoorHandler::label_to_sut_message(Label stimulus) {
    std::string label_name = stimulus.label();
    std::string sut_message = boost::to_upper_copy(label_name);
    std::string result;

    if (label_name == "open" || label_name == "close") {
        result = sut_message;
    } else if (label_name == "lock" || label_name == "unlock") {
        const Label_Parameter param = stimulus.parameters(0);
        long passcode = param.value().integer();
        result = sut_message + ":" + std::to_string(passcode);
    } else if (label_name == "reset") {
        const Label_Parameter param = stimulus.parameters(0);
        std::string manufacturer = param.value().string();
        result = sut_message + ":" + manufacturer;
    } else {
        // This allows to send bad weather stimuli to the SUT.
        result = sut_message;
    }

    return result;
}

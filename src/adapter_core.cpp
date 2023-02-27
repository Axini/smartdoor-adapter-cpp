// Copyright 2023 Axini B.V. https://www.axini.com, see: LICENSE.txt.

#include "spdlog/spdlog.h"

#include "adapter_core.hpp"
#include "broker_connection.hpp"
#include "axini_protobuf.hpp"

AdapterCore::AdapterCore(std::string name, BrokerConnection* broker_connection_ptr,
                         Handler* handler_ptr) {
    this->adapter_name = name;
    this->broker_connection_ptr = broker_connection_ptr;
    this->handler_ptr = handler_ptr;
    this->state = DISCONNECTED;
}

AdapterCore::~AdapterCore() {
    // An AdapterCore does not "own" the BrokerConnection and does
    // not "own" the Handler, so we should *not* delete them.
}

void AdapterCore::start() {
    spdlog::info("AdapterCore::start");
    if (state == DISCONNECTED) {
        spdlog::info("AdapterCore: connecting to AMP's broker.");
        broker_connection_ptr->connect();
    } else {
        std::string message = "Adapter started while already connected.";
        spdlog::error(message);
        send_error(message);
    }
}

void AdapterCore::on_open() {
    spdlog::info("AdapterCore::on_open");

    if (state == DISCONNECTED) {
        state = CONNECTED;

        spdlog::info("AdapterCore: sending announcement to AMP");
        std::vector<Label> labels = handler_ptr->get_supported_labels();
        Configuration configuration = handler_ptr->get_configuration();
        Announcement announcement =
            axini::announcement(adapter_name, labels, configuration);
        Message message = axini::message(announcement);
        send_message(message);

        state = ANNOUNCED;

    } else {
        std::string message = "Connection openend while already connected";
        spdlog::error(message);
        send_error(message);
    }
}

// BrokerConnection: connection is closed.
// * stop the handler
void AdapterCore::on_close(int code, std::string reason) {
    state = DISCONNECTED;

    std::stringstream s;
    s << "AdapterCore: connection with AMP closed with code " << code
      << " and reason: " + reason + "."
      << ((code == 1006) ? " The server may not be reachable." : "");
    spdlog::info(s.str());

    // close the connection with the SUT.
    spdlog::info("AdapterCore: close the connection with the SUT.");
    handler_ptr->stop();

    // reconnect to AMP - keep the adapter alive.
    spdlog::info("AdapterCore: reconnecting to AMP.");
    start();
}

// Configuration received from AMP.
// * configure the handler,
// * start the handler,
// * send ready to AMP (should be done by handler).
void AdapterCore::on_configuration(Configuration configuration) {
    spdlog::info("AdapterCore::on_configuration");

    if (state == ANNOUNCED) {
        handler_ptr->set_configuration(configuration);
        state = CONFIGURED;

        spdlog::info("AdapterCore: connecting to the SUT.");
        handler_ptr->start();

        // The handler should call send_ready() as it knows when it is ready.

    } else {
        std::string message = (state == CONNECTED) ?
            "Configuration received from AMP while not yet announced." :
            "Configuration received from AMP while already configured.";
        spdlog::error(message);
        send_error(message);
    }
}

// Label (stimulus) received from AMP.
// * make handler offer the stimulus to the SUT,
// * acknowledge the actual stimulus to AMP.
// TODO: check that the label is indeed a stimulus.
void AdapterCore::on_label(Label label) {
    std::string label_name = label.label();
    spdlog::info("AdapterCore::on_label: " + label_name);

    if (state == READY) {
        spdlog::info("AdapterCore: forwarding label to Handler object");
        long correlation_id = label.correlation_id();
        std::string physical_label = handler_ptr->stimulate(label);
        long timestamp = axini::current_timestamp();
        send_stimulus(label, physical_label, timestamp, correlation_id);

    } else {
        std::string message = "AdapterCore: label received from AMP while *not* ready.";
        spdlog::error(message);
        send_error(message);
    }
}

// Reset message received from AMP.
// * reset the handler,
// * send ready to AMP (should be done by handler).
void AdapterCore::on_reset() {
    if (state == READY) {
        spdlog::info("AdapterCore: resetting the connection with the SUT.");
        handler_ptr->reset();
        // The handler should call send_ready() as it knows when it is ready.

    } else {
        std::string message = "AdapterCore: reset received from AMP while *not* ready.";
        spdlog::info(message);
        send_error(message);
    }
}

// Error message received from AMP.
// * close the connection to AMP
void AdapterCore::on_error(std::string message) {
    state = ERROR;
    std::string msg = "AdapterCore: error message received from AMP: " + message + ".";
    spdlog::error(msg);
    broker_connection_ptr->close(1000, message); // 1000 is normal closure...
}

void AdapterCore::handle_message(std::string msg) {
    spdlog::info("AdapterCore::handle_message");

    Message message;

    if (! message.ParseFromString(msg)) {
        spdlog::error("Error: could not parse the message");
        return; // TODO: should we throw an Exception?
    }

    if (message.has_configuration()) {
        spdlog::info("AdapterCore: configuration received from AMP");
        on_configuration(message.configuration());
    }

    else if (message.has_label()) {
        Label label = message.label();
        spdlog::info("AdapterCore: label received from AMP: " + axini::to_string(label));
        on_label(label);
    }

    else if (message.has_reset()) {
        spdlog::info("AdapterCore: 'Reset' received from AMP");
        on_reset();
    }

    else if (message.has_error()) {
        std::string error_msg = message.error().message();
        spdlog::info("AdapterCore: error received from AMP: " + error_msg);
        on_error(error_msg);
    }

    else if (message.has_announcement()) {
        spdlog::error("AdapterCore: message type 'Announcement' should not be sent by AMP");
    }

    else if (message.has_ready()) {
        spdlog::error("AdapterCore: message type 'Ready' should not be sent by AMP");
    }

    else {
        spdlog::error("AdapterCore: unexpected message type"); // should not get here
    }
}

// Send response to AMP (callback for Handler).
// TODO: check whether the label is indeed a response.
void AdapterCore::send_response(Label label, std::string physical_label,
                                long timestamp) {
    spdlog::info("AdapterCore::send_response (to AMP): " + axini::to_string(label));
    Label new_label = axini::label(label, physical_label, timestamp);
    Message message = axini::message(new_label);
    send_message(message);
}

// Send Ready to AMP
void AdapterCore::send_ready() {
    spdlog::info("AdapterCore::send_ready to AMP");
    send_message(axini::message_ready());
    state = READY;
}

void AdapterCore::send_message(Message message) {
    // spdlog::info("AdapterCore::send_message");
    std::string str;
    if (!message.SerializeToString(&str)) {
        spdlog::error("AdapterCore: failed to serialize ProtoBuf message.");
        return; // TODO: should we throw an exeption
    }
    broker_connection_ptr->send((void *) str.c_str(), message.ByteSizeLong());
}

// Acknowledge stimulus to AMP.
// TODO: check that the label is indeed a stimulus.
void AdapterCore::send_stimulus(Label label, std::string physical_label,
                                long timestamp, long correlation_id) {
    spdlog::info("AdapterCore::send_stimulus (back to AMP): " + axini::to_string(label));
    Label new_label = axini::label(label, physical_label, timestamp, correlation_id);
    Message message = axini::message(new_label);
    send_message(message);
}

// Send Error message to AMP (also callback for Handler).
void AdapterCore::send_error(std::string error_message) {
    spdlog::info("AdapterCore::send_error");
    Message message = axini::message_error(error_message);
    send_message(message);
    broker_connection_ptr->close(1000, error_message); // 1000 is normal closure
}

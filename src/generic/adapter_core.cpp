// Copyright 2023 Axini B.V. https://www.axini.com, see: LICENSE.txt.

#include <iostream>

#include "spdlog/spdlog.h"

#include "adapter_core.hpp"
#include "broker_connection.hpp"
#include "axini_protobuf.hpp"

using std::placeholders::_1;
AdapterCore::AdapterCore(std::string name, BrokerConnection* broker_connection_ptr,
                         Handler* handler_ptr)
    : qthread_to_amp(std::bind(&AdapterCore::send_message_to_amp, this, _1)),
      qthread_handle_message(std::bind(&AdapterCore::handle_message, this, _1))
{
    this->adapter_name = name;
    this->broker_connection_ptr = broker_connection_ptr;
    this->handler_ptr = handler_ptr;
    this->state = DISCONNECTED;
}

AdapterCore::~AdapterCore() {
    // An AdapterCore does not "own" the BrokerConnection and does
    // not "own" the Handler, so we should *not* delete them.
}

// Start the adapter which will open a connection with AMP.
void AdapterCore::start() {
    spdlog::info("AdapterCore::start");
    clear_qthread_queues();

    if (state == DISCONNECTED) {
        spdlog::info("AdapterCore: connecting to AMP's broker.");
        broker_connection_ptr->connect();
    } else {
        std::string message = "Adapter started while already connected.";
        spdlog::error(message);
        send_error(message);
    }
}

// Broker call back for when the connection is opened with AMP.
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
        queue_message_to_amp(message);

        state = ANNOUNCED;

    } else {
        std::string message = "Connection openend while already connected";
        spdlog::error(message);
        send_error(message);
    }
}

// BrokerConnection: connection with AMP has been closed. Try to reconnect.
// * stop the handler
void AdapterCore::on_close(int code, std::string reason) {
    state = DISCONNECTED;
    clear_qthread_queues();

    std::stringstream s;
    s << "AdapterCore: connection with AMP closed with code " << code
      << " and reason: " + reason + "."
      << ((code == 1006) ? " The server may not be reachable." : "");
    spdlog::info(s.str());

    // close the connection with the SUT.
    spdlog::info("AdapterCore: close the connection with the SUT.");
    handler_ptr->stop();

    // reconnect to AMP - keep the adapter alive.
    spdlog::info("AdapterCore: trying to reconnect to AMP.");
    start();
}

// Add the msg from AMP to the queue to be handled by the QThread.
void AdapterCore::handle_message_from_amp(std::string msg) {
    spdlog::debug("Adding message from AMP to the QThread to be handled: " + msg);
    qthread_handle_message.add(msg);
}

// Send response from the SUT to AMP (callback for Handler).
// TODO: check whether the label is indeed a response.
void AdapterCore::send_response(Label label, std::string physical_label,
                                long timestamp) {
    spdlog::info("AdapterCore::send_response (to AMP): " + axini::to_string(label));
    Label new_label = axini::label(label, physical_label, timestamp);
    Message message = axini::message(new_label);
    queue_message_to_amp(message);
}

// Send Ready to AMP.
void AdapterCore::send_ready() {
    spdlog::info("AdapterCore::send_ready - send ready to AMP");
    queue_message_to_amp(axini::message_ready());
    state = READY;
}

// Confirm a received stimulus by sending it back to AMP.
// TODO: check that the label is indeed a stimulus.
void AdapterCore::send_stimulus_confirmation(Label confirmation) {
    spdlog::info("AdapterCore::send_stimulus_confirmation (back to AMP): " +
                    axini::to_string(confirmation));
    Message message = axini::message(confirmation);
    queue_message_to_amp(message);
}

// Handle the message from AMP: parse the message and call the
// appropriate on_* method.
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
        std::string physical_label = handler_ptr->stimulate(label);

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
        clear_qthread_queues();
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

// Send Error message to AMP (also callback for Handler).
void AdapterCore::send_error(std::string error_message) {
    spdlog::info("AdapterCore::send_error");
    Message message = axini::message_error(error_message);
    queue_message_to_amp(message);
    broker_connection_ptr->close(1000, error_message); // 1000 is normal closure
}

// Adds message to the queue of pending messages to AMP.
void AdapterCore::queue_message_to_amp(Message message) {
    spdlog::debug("Adding message to the QThread which sends messages to AMP");
    qthread_to_amp.add(message);
}

// Send a Message to AMP.
void AdapterCore::send_message_to_amp(Message message) {
    spdlog::debug("AdapterCore::send_message_to_amp");
    std::string str;
    if (!message.SerializeToString(&str)) {
        spdlog::error("AdapterCore: failed to serialize ProtoBuf message.");
        return; // TODO: should we throw an exeption?
    }
    broker_connection_ptr->send((void *) str.c_str(), message.ByteSizeLong());
}

// Clear the queues of the QThread objects.
void AdapterCore::clear_qthread_queues() {
    spdlog::info("Clearing queues with pending messages.");
    qthread_to_amp.clear_queue();
    qthread_handle_message.clear_queue();
}

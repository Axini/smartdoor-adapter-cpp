// Copyright 2023 Axini B.V. https://www.axini.com, see: LICENSE.txt.

#include "spdlog/spdlog.h"

#include "handler.hpp"
#include "adapter_core.hpp"

Handler::Handler() {}
Handler::~Handler() {}

void Handler::send_ready_to_amp() {
    spdlog::info("Handler::send_ready_to_amp");
    adapter_core_ptr->send_ready();
}

void Handler::register_adapter_core(AdapterCore* adapter_core_ptr) {
    this->adapter_core_ptr = adapter_core_ptr;
}

void Handler::set_configuration(Configuration configuration) {
    this->configuration = configuration;
}

Configuration Handler::get_configuration() {
    return configuration;
}

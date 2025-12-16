#include "state_machine.hpp"

void StateMachine::init() {
    current = AppState::SLEEP;
}

void StateMachine::set_state(AppState new_state) {
    current = new_state;
}

void StateMachine::handle() {
    switch (current) {
    case AppState::SLEEP:
        // enter low power
        break;

    case AppState::LISTEN:
        // read I2S mic
        break;

    case AppState::PROCESS:
        // send to LLM / rule engine
        break;

    case AppState::MENU:
        // draw OLED UI
        break;

    case AppState::IDLE:
        // wait timeout
        break;
    }
}

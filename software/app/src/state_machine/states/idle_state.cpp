#include "../statemachine.hpp"
#include "menu_state.hpp"

extern ListeningState listeningState;

void ListeningState::onEnter(StateMachine&) {
    // Turn on display, mic, LED
}

void ListeningState::onEvent(StateMachine& sm, Event event) {
    if (event == Event::VOICE_DETECTED) {
        sm.transitionTo(&listeningState);
    }
}

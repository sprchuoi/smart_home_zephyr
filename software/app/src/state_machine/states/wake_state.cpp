#include "../StateMachine.hpp"
#include "WakeState.hpp"
#include "ListeningState.hpp"

extern ListeningState listeningState;

void WakeState::onEnter(StateMachine&) {
    // Turn on display, mic, LED
}

void WakeState::onEvent(StateMachine& sm, Event event) {
    if (event == Event::VOICE_DETECTED) {
        sm.transitionTo(&listeningState);
    }
}

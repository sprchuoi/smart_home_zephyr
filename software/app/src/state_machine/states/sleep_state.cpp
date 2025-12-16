#include "../StateMachine.hpp"
#include "SleepState.hpp"

void SleepState::onEnter(StateMachine&) {
    // Power down mic, display
}

void SleepState::onEvent(StateMachine& sm, Event event) {
    if (event == Event::WAKE_WORD) {
        sm.transitionTo(&wakeState);
    }
}

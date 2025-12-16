#pragma once
#include "state.hpp"

class StateMachine {
public:
    void init();
    void handle();
    void set_state(AppState new_state);

private:
    AppState current;
};

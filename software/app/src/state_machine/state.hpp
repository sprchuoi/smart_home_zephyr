#pragma once

// Implement a simple state machine for managing application states
/* ┌─────────┐
│  SLEEP  │◄──────────────┐
└────┬────┘               │
     │ Wake word / button │
     ▼                    │
┌─────────┐               │
│ LISTEN  │  (I2S mic)    │
└────┬────┘               │
     │ Command detected   │
     ▼                    │
┌─────────┐
│ PROCESS │  (LLM / rule)
└────┬────┘
     │
     ▼
┌─────────┐
│  MENU   │ (OLED / UI)
└────┬────┘
     │ timeout / exit
     ▼
┌─────────┐
│  IDLE   │
└────┬────┘
     │ inactivity
     ▼
┌─────────┐
│  SLEEP  │
└─────────┘
*/
enum class AppState {
    SLEEP,
    LISTEN,
    PROCESS,
    MENU,
    IDLE
};
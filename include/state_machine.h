#pragma once

// Steam detection temperature threshold
#define STEAM_TEMP_DETECTION 120

// Machine states
enum MachineState {
    STATE_INIT      = 0,
    STATE_COLDSTART = 10,
    STATE_PREREADY  = 19,
    STATE_READY     = 20,
    STATE_BREWING   = 30,
    STATE_POSTBREW  = 35,
    STATE_STEAM     = 40,
    STATE_CHILL     = 45
};

void checkMachineState();

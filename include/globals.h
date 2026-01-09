#pragma once

// Timing
extern unsigned long time_now;
extern unsigned long time_last;

// Temperature settings
extern double gTargetTemp;
extern double gOvershoot;

// PID parameters
extern double gP;
extern double gI;
extern double gD;
extern double gaP;
extern double gaI;
extern double gaD;

// Machine state (uses MachineState enum values from state_machine.h)
extern int machineState;

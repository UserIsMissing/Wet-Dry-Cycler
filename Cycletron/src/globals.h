// globals.h
#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduino.h>

// === State Machine ===
enum class SystemState
{
  IDLE,
  READY,
  REHYDRATING,
  HEATING,
  MIXING,
  REFILLING,
  EXTRACTING,
  LOGGING,
  PAUSED,
  ENDED,
  ERROR
};


// === Parameters set by frontend or recovery ===
extern float volumeAddedPerCycle;
extern float durationOfRehydration;
extern float syringeDiameter;
extern float desiredHeatingTemperature;
extern float durationOfHeating;
extern float durationOfMixing;

extern int numberOfCycles;
extern int syringeStepCount;

extern unsigned long heatingStartTime;
extern unsigned long mixingStartTime;

// === Runtime state tracking ===
extern bool heatingStarted;
extern bool mixingStarted;
extern bool refillingStarted;

extern int completedCycles;
extern int currentCycle;

extern float heatingProgressPercent;
extern float mixingProgressPercent;

#endif // GLOBALS_H
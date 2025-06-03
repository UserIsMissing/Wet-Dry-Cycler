// globals.h
#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduino.h>

// === State Machine ===
enum class SystemState
{
  VIAL_SETUP,
  WAITING,
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

extern bool shouldMoveForward; // Flag for back-and-forth movement
extern bool shouldMoveBack; // Flag for back-and-forth movement
extern bool movementForwardDone;
extern bool movementBackDone;

// Use globals from globals.h
extern float volumeAddedPerCycle;
extern float syringeDiameter;
extern float desiredHeatingTemperature;
extern float durationOfHeating;
extern float durationOfMixing;
extern int numberOfCycles;
extern int syringeStepCount;
extern unsigned long heatingStartTime;
extern unsigned long mixingStartTime;
extern bool heatingStarted;
extern bool mixingStarted;
extern int completedCycles;
extern int currentCycle;
extern float heatingProgressPercent;
extern float mixingProgressPercent;
extern int sampleZonesArray[3];
extern int sampleZoneCount;
extern SystemState currentState;
extern SystemState previousState;
extern void setState(SystemState newState);



#endif // GLOBALS_H
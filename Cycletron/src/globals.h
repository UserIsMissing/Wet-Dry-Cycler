// globals.h
#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduino.h>
#include <WebSocketsClient.h>
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


extern SystemState currentState;
extern SystemState previousState;
extern void setState(SystemState newState);


extern WebSocketsClient webSocket;


// === Parameters set by frontend or recovery basaed on wether it is a fresh setup or a recovery ===
extern float volumeAddedPerCycle;
extern float syringeDiameter;
extern float desiredHeatingTemperature;
extern float durationOfHeating;
extern float durationOfMixing;
extern int numberOfCycles;
extern int sampleZonesArray[3];
extern int sampleZoneCount;


//Globals variables used for recovery and updated with the frontend
// These are used to track the state of the system and the progress of operations
extern int syringeStepCount;
extern unsigned long heatingStartTime;
extern unsigned long mixingStartTime;
extern bool heatingStarted;
extern bool mixingStarted;
extern bool refillingStarted; // Flag to track if refilling has started
extern int completedCycles;
extern int currentCycle;
extern float heatingProgressPercent;
extern float mixingProgressPercent;

//flags used for back-and-forth movement in both vial setup and extraction
extern bool shouldMoveForward; // Flag for back-and-forth movement
extern bool shouldMoveBack; // Flag for back-and-forth movement
extern bool movementForwardDone;
extern bool movementBackDone;

// Add these to the extern declarations
extern float heatingDurationRemaining;
extern float mixingDurationRemaining;

typedef enum {
    ERROR_MOVEMENT_MAX_STEPS_FORWARD,
    ERROR_MOVEMENT_MAX_STEPS_BACKWARD,
    ERROR_SYRINGE_MAX_STEPS, // Add this for syringe overstep errors
    ERROR_DRV8825_FAULT, // Add this for DRV8825 fault pin error
    // Add more error types as needed
} SystemErrorType;



#endif // GLOBALS_H
#include "globals.h"

// State variables
SystemState currentState = SystemState::IDLE;
SystemState previousState = SystemState::IDLE;

// WebSocket client
WebSocketsClient webSocket;


// Parameters set by frontend
float volumeAddedPerCycle = 0;
float syringeDiameter = 0;
float desiredHeatingTemperature = 0;
float durationOfHeating = 0;
float durationOfMixing = 0;
int numberOfCycles = 0;
int sampleZonesArray[3] = {0};
int sampleZoneCount = 0;

// Runtime tracking variables
int syringeStepCount = 0;
unsigned long heatingStartTime = 0;
unsigned long mixingStartTime = 0;
bool heatingStarted = false;
bool mixingStarted = false;
bool refillingStarted = false;
int completedCycles = 0;
int currentCycle = 0;
float heatingProgressPercent = 0;
float mixingProgressPercent = 0;

// Movement flags
bool shouldMoveForward = false;
bool shouldMoveBack = false;
bool movementForwardDone = false;
bool movementBackDone = false;

// Duration tracking
float heatingDurationRemaining = 0;
float mixingDurationRemaining = 0;

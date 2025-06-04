// main.cpp
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

extern bool heatingStarted;
extern bool mixingStarted;
extern int completedCycles;
extern int currentCycle;
extern float heatingProgressPercent;
extern float mixingProgressPercent;

bool shouldMoveForward= false; // Initialize the flag to false
bool shouldMoveBack = false; // Initialize the flag to false

bool movementForwardDone = false;
bool movementBackDone = false;



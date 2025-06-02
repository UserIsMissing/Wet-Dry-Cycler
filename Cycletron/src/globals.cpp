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

extern int sampleZonesArray[3];
extern int sampleZoneCount;

volatile bool recoveryStateDirty = false;

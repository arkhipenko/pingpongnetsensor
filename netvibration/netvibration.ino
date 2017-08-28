#include <AvgFilter.h>
#include <TaskScheduler.h>

#define SOUND_PIN 4
#define VIB_PIN   1

#define STEP            5     // sensor reading interval, ms
#define MIN_TO_BEEP_AV  20    // min threshold to beep
#define MIN_TO_ARM_AV   0     // max threshold to keep silent 
#define DATA_SAMPLES    50    // number of data samples for average filter
#define NO_BEEP_TOUT    3     // seconds

long      vibData[DATA_SAMPLES];
avgFilter vib(DATA_SAMPLES, vibData);
long      avgVib;

void measureCallback();

Scheduler ts;
Task tMeasure(STEP, TASK_FOREVER, &measureCallback, &ts);
Task tDelay(NO_BEEP_TOUT * TASK_SECOND, TASK_ONCE, NULL, &ts); 


bool state = false; // false: listening, true: beep and wait to settle 

void measureCallback() {
  int v = digitalRead(VIB_PIN) == HIGH ? 100 : 0;
  avgVib = vib.value( (long) v);

  if (state) {
    if (!tDelay.isEnabled()) {
      if (avgVib <= MIN_TO_ARM_AV) {
        state = !state;
        vib.initialize();
      }
    }
  }
  else {
    if (avgVib >= MIN_TO_BEEP_AV) {
      tone(SOUND_PIN, 440, 1000);
      state = !state;
      tDelay.restartDelayed();

    }
  }
}

void setup() {
  // put your setup code here, to run once:

  pinMode(SOUND_PIN, OUTPUT);
  pinMode(VIB_PIN, INPUT);

  tone(SOUND_PIN, 220, 200);
  delay(250);
  tone(SOUND_PIN, 330, 200);
  delay(250);
  tone(SOUND_PIN, 440, 200);

  tMeasure.enable();
}

void loop() {
  // put your main code here, to run repeatedly:
  ts.execute();
}

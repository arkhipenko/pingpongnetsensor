#include <AvgFilter.h>
#include <TaskScheduler.h>
#include <EnableInterrupt.h>
#include <avr/sleep.h>
#include <avr/power.h>

#define SOUND_PIN 4
#define VIB_PIN   1

#define STEP            5     // sensor reading interval, ms
#define MIN_TO_BEEP_AV  20    // min threshold to beep
#define MIN_TO_ARM_AV   0     // max threshold to keep silent 
#define DATA_SAMPLES    50    // number of data samples for average filter
#define NO_BEEP_TOUT    3     // seconds - no repeat beeps for this period of time
#define SLEEP_TOUT      60    // seconds - put attiny85 into deep sleep after no vibration for this time

long      vibData[DATA_SAMPLES];
avgFilter vib(DATA_SAMPLES, vibData);
long      avgVib;

void measureCallback();
void timeoutCallback();

Scheduler ts;
Task tMeasure(STEP, TASK_FOREVER, &measureCallback, &ts);
Task tDelay(NO_BEEP_TOUT * TASK_SECOND, TASK_ONCE, NULL, &ts); 
Task tTimeout(SLEEP_TOUT * TASK_SECOND, TASK_ONCE, &timeoutCallback, &ts);


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
      tTimeout.restartDelayed();
    }
  }
}

void VibrationDetected() {
  tMeasure.enable();
  tTimeout.restartDelayed();
}

void timeoutCallback() {
  //   * The 5 different modes are:
  //   *     SLEEP_MODE_IDLE         -the least power savings
  //   *     SLEEP_MODE_ADC
  //   *     SLEEP_MODE_PWR_SAVE
  //   *     SLEEP_MODE_STANDBY
  //   *     SLEEP_MODE_PWR_DOWN     -the most power savings

  tone(SOUND_PIN, 110, 100);
  delay(100);
  
  ts.disableAll();
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  cli();
  sleep_enable();
  sleep_bod_disable();
  sei();
//  PCintPort::attachInterrupt(PIN_MOTION_ACTIVATION, &MotionDetected, FALLING);
  enableInterrupt(VIB_PIN, &VibrationDetected, RISING);
  sleep_cpu();

  // z..z..z..Z..Z..Z
  
  /* wake up here */
  sleep_disable();
}

void setup() {
  // put your setup code here, to run once:

  pinMode(SOUND_PIN, OUTPUT);
  pinMode(VIB_PIN, INPUT);

//  power_spi_disable();
//  power_twi_disable();
  power_adc_disable();

  tone(SOUND_PIN, 220, 200);
  delay(250);
  tone(SOUND_PIN, 330, 200);
  delay(250);
  tone(SOUND_PIN, 440, 200);

  tMeasure.enable();
  tTimeout.restartDelayed();
}

void loop() {
  // put your main code here, to run repeatedly:
  ts.execute();
}

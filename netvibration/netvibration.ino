/* -------------------------------------
  Faulty Ping Pong Serve detection 
  Based on ATTiny85 
   Code Version 1.0.1

  Change Log:
  2017-08-20
    v1.0.0 - initial release

  2017-08-30
    v1.0.1 - support for TM1650 based finetuning using Arduino Uno and 7 segment display
    
 ----------------------------------------*/

      
#include <AvgFilter.h>

// #define _TASK_TIMECRITICAL      // Enable monitoring scheduling overruns
 #define _TASK_SLEEP_ON_IDLE_RUN // Enable 1 ms SLEEP_IDLE powerdowns between tasks if no callback methods were invoked during the pass 
// #define _TASK_STATUS_REQUEST    // Compile with support for StatusRequest functionality - triggering tasks on status change events in addition to time only
// #define _TASK_WDT_IDS           // Compile with support for wdt control points and task ids
// #define _TASK_LTS_POINTER       // Compile with support for local task storage pointer
// #define _TASK_PRIORITY          // Support for layered scheduling priority
// #define _TASK_MICRO_RES         // Support for microsecond resolution
// #define _TASK_STD_FUNCTION      // Support for std::function (ESP8266 ONLY)
// #define _TASK_DEBUG             // Make all methods and variables public for debug purposes

#include <TaskScheduler.h>

#include <EnableInterrupt.h>
#include <avr/sleep.h>
#include <avr/power.h>

// #define _TEST_    // uncomment this to use with as a finetuning device, based on Arduino UNO and TM1650 7 segment display

#ifdef _TEST_
  #include <TM1650.h>
  #include <Wire.h>
  TM1650 panel;
#endif

#define SOUND_PIN 4
#define VIB_PIN   1

#define STEP            10     // sensor reading interval, ms
#define MIN_TO_BEEP_AV  30    // min threshold to beep
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
        avgVib = 0;
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

#ifdef _TEST_
  char line[5];
  snprintf(line, 5, "%04d", avgVib);
  panel.displayString(line);
#endif
}

void VibrationDetected() {
  tMeasure.enable();
  tTimeout.restartDelayed();

#ifdef _TEST_
  power_twi_enable();
  Wire.begin();
  panel.init();
  panel.displayOn();
#endif
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

#ifdef _TEST_
//  for some reason the panel never comes back up after deep sleep.
//  so I decided to never deep sleep when testing
  return;
#endif
  
  ts.disableAll();
  
#ifdef _TEST_
  panel.displayOff();
   // Disable digital input buffers on all ADC0-ADC5 pins
   DIDR0 = 0x3F; 
   // set I2C pin as input no pull up
   // this prevent current draw on I2C pins that
   // completly destroy our low power mode
 
   //Disable I2C interface so we can control the SDA and SCL pins directly
   TWCR &= ~(_BV(TWEN)); 
 
   // disable I2C module this allow us to control
   // SCA/SCL pins and reinitialize the I2C bus at wake up
   TWCR = 0;
   pinMode(SDA, INPUT);
   pinMode(SCL, INPUT);
   digitalWrite(SDA, LOW);
   digitalWrite(SCL, LOW);
#endif

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

#ifdef _TEST_
  Wire.begin(); //Join the bus as master

  panel.init();
  panel.displayOn();
  for (int i=0; i<5; i++) {
    panel.displayString("Ping");
    delay(500);
    panel.displayString("Pong");
    delay(500);
  }

#endif



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

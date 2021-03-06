// These define's must be placed at the beginning before #include "TimerInterrupt_Generic.h"
// _TIMERINTERRUPT_LOGLEVEL_ from 0 to 4
// Don't define _TIMERINTERRUPT_LOGLEVEL_ > 0. Only for special ISR debugging only. Can hang the system.
#define TIMER_INTERRUPT_DEBUG       0
#define _TIMERINTERRUPT_LOGLEVEL_   4

#include "ESP32_S2_TimerInterrupt.h"
#include <ESP32AnalogRead.h>
//C++ libraries are not native to arduino, included within ESPIDF
#include <deque> 

//Hardware Timer Defintions
#define TIMER0_INTERVAL_MS  1000
#define TIMER0_PERIOD_MS    10

//General PWM Configuration
#define PWM_FREQ 333 //Sets 333 Hz
#define PWM_RESOLUTION 8 //Sets 8 bit resolution
#define MOTOR_HOME 76 //30% duty cycle @8 bit resolution

//Defines PWM Pins
#define FINGER_THUMB_GPIO 42
#define FINGER_INDEX_GPIO 41
#define FINGER_MIDDLE_GPIO 40
#define FINGER_RING_GPIO 39
#define FINGER_PINKY_GPIO 38

//Defines PWM Channels
#define FINGER_THUMB_CHANNEL 0
#define FINGER_INDEX_CHANNEL 1
#define FINGER_MIDDLE_CHANNEL 2
#define FINGER_RING_CHANNEL 3
#define FINGER_PINKY_CHANNEL 4

//sets max deque size to 5 seconds, 500 samples [0,499]
#define MAX_DEQUE_SIZE 499

//Creates ADC objects
ESP32AnalogRead myoware1; //Myoware Sensor Inside Fore arm
ESP32AnalogRead myoware2; // Myoware Sensor Outside Fore arm

//Deque definitions
std::deque<double> myo1Deque;
std::deque<double> myo2Deque;

//used to to ensure vector size does not exceed 500 samples
int myo1DequeSize;
int myo2DequeSize;

//stores ADC reading 
volatile double myo1Volts;
volatile double myo2Volts;

//Timer Flags
volatile bool timer0 = false;

// Init ESP32 timer 0 and 1
ESP32Timer ITimer0(0);

//debugging for interrupt timer
volatile int timerDebug = 0;
bool IRAM_ATTR TimerHandler0(void * timerNo) { 
  timer0 = true;
  timerDebug = millis();
  myo1Volts = myoware1.readVoltage();
  myo2Volts = myoware2.readVoltage();

  return true;
}

void setup() {
  Serial.begin(115200);

  //Hardware TImer Configuration

  while (!Serial);

  delay(100);

  Serial.print(F("\nStarting Argument_None on ")); Serial.println(ARDUINO_BOARD);
  Serial.println(ESP32_S2_TIMER_INTERRUPT_VERSION);
  Serial.print(F("CPU Frequency = ")); Serial.print(F_CPU / 1000000); Serial.println(F(" MHz"));

  // Interval in microsecs
  if (ITimer0.attachInterruptInterval(TIMER0_INTERVAL_MS * TIMER0_PERIOD_MS, TimerHandler0)){
    Serial.print(F("Starting  ITimer0 OK, millis() = ")); Serial.println(millis());
  }else
    Serial.println(F("Can't set ITimer0. Select another Timer, freq. or timer"));

  //attatch ADC object to GPIO pins
  myoware1.attach(1); //Myoware 1 is attatched to GPIO 1
  myoware2.attach(2); //Myoware 2 is attatched to GPIO 2

  //Configures PWM Frequency and Resulution for all channels
   for (int channel = 0; channel < 5; channel++){
     ledcSetup(channel, PWM_FREQ, PWM_RESOLUTION);
  }

  //Attaches PWM pins to Motor Coltrol pins
  ledcAttachPin(FINGER_THUMB_GPIO, FINGER_THUMB_CHANNEL);
  ledcAttachPin(FINGER_INDEX_GPIO, FINGER_INDEX_CHANNEL);
  ledcAttachPin(FINGER_MIDDLE_GPIO, FINGER_MIDDLE_CHANNEL);
  ledcAttachPin(FINGER_RING_GPIO, FINGER_RING_CHANNEL);
  ledcAttachPin(FINGER_PINKY_GPIO, FINGER_PINKY_CHANNEL);

  //Initializes Motor Control to home position: 30%
  for (int channel = 0; channel < 5; channel++){
    ledcWrite(channel, MOTOR_HOME);
  }
}

void loop() {

  if (timer0){
    //reset timer flag
    timer0 = false;

    //adds latest reading to the front of the vector
    myo1Deque.push_front(double(myo1Volts));
    myo2Deque.push_front(double(myo2Volts));

    //matains Deque size of 500 elements,
    if ((myo1DequeSize = myo1Deque.size()) >= MAX_DEQUE_SIZE) 
      //myo1Vector.remove(myo1VectorSize - 1); //removes last element of myo1vector
      myo1Deque.pop_back();
    else if ((myo2DequeSize = myo1Deque.size()) >= MAX_DEQUE_SIZE)
      //myo2Vector.remove(myo2VectorSize - 1); //removes last element of myo2vector
     myo1Deque.pop_back();

    Serial.println(timerDebug);
    Serial.println(myo1Volts);
    Serial.println(myo2Volts);
  }
  
  //Tempoary drive for motors
  //if either myoware sensor reads above 1V then all motors are driven to max position
  if (myo1Volts > 1.0 || myo2Volts > 1.0){
    for (int channel = 0; channel < 5; channel++){
      ledcWrite(channel, 205); //205 is 80% duty cycles @8 bit resolution
    }
  }

}

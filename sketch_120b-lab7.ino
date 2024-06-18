/* 
  -=-=-=-=-=-=-=-=-=-=-=- theremin -=-=-=-=-=-=-=-=-=-=-=-
    .ino file for cs120bf23 final project
    authored by garett hammerle aka me

  notes:
    there is the weirdest most strange most mind-numbing bug with this project, quite often when the buzzer's frequency should be staying at a constant value (because the distance detected by the ultrasonic sensor is very high),
    it doesn't and starts jumping around erratically. this is not a problem with the distance returned by the ultrasonic sensor, it's something to do with my functions, or how the timing works out. i have a few hypotheses:
      1. my method of sensing distance using the ultrasonic sensor is flawed somehow
      2. my method of maintaining the current frequency is flawed
      3. the delays caused by the amount of time the rest of the project takes to run causes the ultrasonic sensor to inaccurately detect pulses (i.e., not in exactly 10us intervals) which 
        causes shorter-than-actual distances to be reported occasionally. solutions would be to further optimize the code when the distance is being maintained, or detect the distance in a different way altogether
    the most likely candidate is #3, but no fixes that i can think of have worked.
    in any case, it's not that crucial of an issue so i'm fine with the project in its current state. 

    there are no sharps/flats displayed on the 7-segment display because i ran out of time. instead the frequency is rounded up to the nearest natural (i.e., no sharp or flat on the note, just the "letter" itself) and that is displayed

    this theremin does not sound very good, and i'm aware of that. a way to make it sound less bad is to invest in a speaker and another ultrasonic sensor, in order to change not only the frequency of the note with one hand 
    but also it's volume with your other hand. this is how real theremins work and they actually sound good.
*/


#include "timer.h"

// states n periods
const int SENSOR_PERIOD = 1; // use the smallest period possible for the sensor
const int BUZZER_PERIOD = 5; // if this is too low or too high, the glitchy frequencies are more prevalent
const int SEVENSEG_PERIOD = 1; // has to be this low or else flickering occurs??
enum SENSOR_STATE {SENSOR_INIT, S_CALC_DISTANCE} sensorState = SENSOR_INIT;
enum BUZZER_STATE {BUZZER_INIT, S_TONE_ON, S_TONE_ON_BUTTON_PRESSED, S_TONE_OFF, S_TONE_OFF_BUTTON_PRESSED} buzzerState = BUZZER_INIT;
enum SEVENSEG_STATE {SEVENSEG_INIT, S_D1, S_D2} sevensegState = SEVENSEG_INIT;

int buzzerTimeElapsed = 0;
int sevensegTimeElapsed = 0;

// globals
double distance;
double duration;
int sensorCnt;
double currentFrequency;
double previousFrequency;
double targetFrequency;
const double HALF_STEP_CONSTANT = 1.05946; // 2^(1/12)
int buttonPressed;

// pin aliases
const int echoPin = 12;
const int trigPin = 13;
const int buzzerPin = A5;
const int digit1Pin = 11;
const int digit2Pin = 10;
const int segmentPins[7] = {7, 9, 5, 3, 2, 8, 6};
//                          A, B, C, D, E, F, G
const int dpPin = 4; // decimal point, we do not care
const int buttonPin = A4;



void setup() {
  currentFrequency = 0;
  previousFrequency = 0;
  pinMode(echoPin, INPUT);
  pinMode(trigPin, OUTPUT); 

  pinMode(buzzerPin, OUTPUT);

  pinMode(digit1Pin, OUTPUT);
  pinMode(digit2Pin, OUTPUT);
  for (auto i = 0; i < 7; i++) {
    pinMode(segmentPins[i], OUTPUT);
  }

  pinMode(buttonPin, INPUT);

  TimerSet(1); // tick every 1ms
  TimerOn();

  // serial statements for debugging purposes
  // Serial.begin(9600);
  // Serial.println(); // move silly squares up a line
}

void loop() {
  // constantly read button state
  buttonPressed = digitalRead(buttonPin);

  sensorTick();
  
  // low budget task scheduler
  if (buzzerTimeElapsed >= BUZZER_PERIOD) {
    buzzerTick();
    buzzerTimeElapsed = 0;
  }
  else buzzerTimeElapsed++;

  if (sevensegTimeElapsed >= SEVENSEG_PERIOD) {
    sevensegTick();
    sevensegTimeElapsed = 0;
  }
  else sevensegTimeElapsed++;

  while (!TimerFlag) {}
  TimerFlag = 0;
}

void sensorTick() {
  // transitions
  switch (sensorState) {
    case SENSOR_INIT:
      sensorState = S_CALC_DISTANCE;
    break;

    case S_CALC_DISTANCE:
      sensorState = S_CALC_DISTANCE;
    break;    
  }

  // actions
  switch (sensorState) {
    case SENSOR_INIT:
      // no actions...
    break;

    case S_CALC_DISTANCE:
      // use quick maths to calculate distance (speed of sound basically)
      digitalWrite(trigPin, HIGH);
      delayMicroseconds(10); // we are allowed to use this function :)
      digitalWrite(trigPin, LOW);
      // pulsed trigPin for 10us, meaning a soundwave was sent for 10us
      // read from echoPin, the duration it is high indicates how far away an object is, the longer it is high the further away the object is (in us)
      // speed of sound: 343m/s == 0.00034m/us == 0.34mm/us (i love the metric system). divide by 2 because the soundwaves make a round trip to the object and back, so the distance is half that
      // formula: speed = distance/time -> distance = time * speed
      duration = pulseIn(echoPin, HIGH);
      // get the distance in the unit of 10 mm, for precision in playing the tone reasons
      distance = (duration * 3.40)*0.5;
      // Serial.println(distance);
    break;
  }
}

void buzzerTick() {
  // transitions
  switch (buzzerState) {
    case BUZZER_INIT:
      buzzerState = S_TONE_OFF;
    break;

    // if button is pressed, transition to buttonpressed state and wait for it to be unpressed, then transition to the correct state for that button press
    case S_TONE_ON:
      if (buttonPressed == 1) buzzerState = S_TONE_ON_BUTTON_PRESSED;
      else if (buttonPressed == 0) buzzerState = S_TONE_ON;
    break;

    case S_TONE_ON_BUTTON_PRESSED:
      if (buttonPressed == 1) buzzerState = S_TONE_ON_BUTTON_PRESSED;
      else if (buttonPressed == 0) buzzerState = S_TONE_OFF;
    break;

    case S_TONE_OFF:
      if (buttonPressed == 1) buzzerState = S_TONE_OFF_BUTTON_PRESSED;
      else if (buttonPressed == 0) buzzerState = S_TONE_OFF;
    break;

    case S_TONE_OFF_BUTTON_PRESSED:
      if (buttonPressed == 1) buzzerState = S_TONE_OFF_BUTTON_PRESSED;
      else if (buttonPressed == 0) buzzerState = S_TONE_ON;
    break;
  }

  // actions
  switch (buzzerState) {
    case S_TONE_ON:
      // calc target frequency from current distance
      targetFrequency = mapDistToFreqContinuous(distance, targetFrequency);
      tone(buzzerPin, targetFrequency);
    break;

    case S_TONE_OFF:
      // if the buzzer is muted, still map the frequency (for the 7 segment display) but don't play any noise
      targetFrequency = mapDistToFreqContinuous(distance, targetFrequency);
      noTone(buzzerPin); // notone turns off the tone that was playing on the buzzer
    break;
  }
}

void sevensegTick() {
  // tran sitions
  switch (sevensegState) {
    case SEVENSEG_INIT:
      sevensegState = S_D1;
    break;

    case S_D1:
      sevensegState = S_D2;
    break;

    case S_D2:
      sevensegState = S_D1;
    break;
  }

  // actions!!
  switch (sevensegState) {
    case S_D1: // note octave
      digitalWrite(digit2Pin, HIGH); // turn off second digit
      displayNote(mapDistToFreqDiscrete(distance, targetFrequency), digit1Pin);
      digitalWrite(digit1Pin, LOW); // turn on first digit
    break;

    case S_D2: // note value
      digitalWrite(digit1Pin, HIGH); // turn off first digit
      displayNote(mapDistToFreqDiscrete(distance, targetFrequency), digit2Pin);
      digitalWrite(digit2Pin, LOW); // turn on second digit
  }
}

// mapDistToFreqDiscrete function, maps distance to the corresponding frequency for the 7 segment display to display. essentially rounds the value of mapDistToFreqContinuous(distance) to the nearest pure tone.
// input: raw distance value, current frequency.
// output: mapped frequency in hz. will be on a pure tone frequency value (i.e., exactly A4, C5, C2, B1 etc.).
double mapDistToFreqDiscrete(int d, double currentFrequency) {
  // max value for the distance that we update the frequency on is 45cm, anything more and we maintain the current frequency (that's why the current frequency is a parameter of this function)
  // essentially if there's nothing to the sensor closer than 45cm, keep the previous frequency (not the max)
  if (d <= 5000.00) {

    long halfSteps = map(d, 60.0, 5000.0, -12, 24);

    // really nice resource here: https://pages.mtu.edu/~suits/NoteFreqCalcs.html
    // ^ had the below formula:
    double retValDiscrete = constrain(440.0 * pow(HALF_STEP_CONSTANT, halfSteps), 220, 1760);

    return retValDiscrete;
  }
  else return currentFrequency;
}

// mapDistToFreqContinuous function, maps distance to the corresponding frequency for the buzzer to emit.
// input: raw distance value, current frequency.
// output: mapped frequency in hz. will be a continuous value in between pure tone frequencies (i believe it's called a microtone or something. it's just so that the buzzer tone smoothly moves up and down).
double mapDistToFreqContinuous(int d, double currentFrequency) {
  // max value for the distance that we update the frequency on is 45cm, anything more and we maintain the current frequency (that's why the current frequency is a parameter of this function)
  // essentially if there's nothing to the sensor closer than 45cm, keep the previous frequency (not the max)
  if (d <= 5000.00) {
    
    // continuous map (built in arduino map() function is for integers only).
    // range we are mapping is [30.0, 4500.0] -> [-24, 12], including decimals.
    double normalized = (d-60.0)/(5000.0-60.0);
    double halfStepsContinuous = normalized * 36 - 12;

    // really nice resource here: https://pages.mtu.edu/~suits/NoteFreqCalcs.html
    // ^ had the below formula:
    double retValContinuous = constrain(440.0 * pow(HALF_STEP_CONSTANT, halfStepsContinuous), 220, 1760);
    if (retValContinuous >= 1760) retValContinuous = 1760;

    return retValContinuous;
  }
  else return currentFrequency;
}

// displayNote function, displays specified note Value and Octave to the corresponding digit pin.
void displayNote(double frequency, int digitPin) {
  // turn off both digits
  digitalWrite(digit1Pin, HIGH);
  digitalWrite(digit2Pin, HIGH);

  // octave
  if (digitPin == digit1Pin) {
    // getting the octave is as simple as doing log2(freq) - 5 (every octave is a power of 2)
    int octave = floor(log10(frequency) / log10(2)) - 5;

    uint8_t encodeInt[10] = {
      // 0     1     2     3     4     5     6     7     8     9
      0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x67,
    };

    // write to segment pins
    for (int i = 0; i < 7; i++) {
      digitalWrite(segmentPins[i], encodeInt[octave] & (1 << i));
    }
  }

  // value
  else if (digitPin == digit2Pin) {
    int value = freqToValue(frequency);

    uint8_t encodeValue[7] = {
      // C     d     E     F     g     A     b
      0x39, 0x5e, 0x79, 0x71, 0x6f, 0x77, 0x7c,
    };

    // write to segment pins
    for (int i = 0; i < 7; i++) {
      digitalWrite(segmentPins[i], encodeValue[value] & (1 << i));
    }
  }

  // turn the correct digit back on
  digitalWrite(digitPin, LOW);
}

// frequencies of every pure tone on the second octave
const double OCTAVE2_FREQS[11] = {65.41, 69.30, 73.42, 77.78, 82.41, 87.31, 92.50, 98.00, 110.00, 116.54, 123.47};
// same as above, with no sharps (so that indices map to ENCODE_VALUE correctly)
const double OCTAVE2_FREQS_NATURAL[7] = {65.41, 73.42, 82.41, 87.31, 98.00, 110.00, 123.47}; 
const String ENCODE_VALUE[7] = {"C", "D", "E", "F", "G", "A", "B"};

// freqToValue function, using inputted frequency, output index corresponding to pure tone value that the frequency has. for example freqToValue(440.00) outputs A
int freqToValue(double freq) {
  double val = freq;
  // get the frequency in the range we care about (octave 2), dividing by 2 just moves down one octave. we are changing it to be in the second octave because those are the values we stored in the OCTAVE2_FREQS_NATURAL lookup table
  // could do 0 octave frequencies, but octave 2 is the lowest the buzzer goes in this project
  while (val >= 130.81 + 0.005) { // the 0.005 is to avoid some floating point tomfoolery
    val = val/2;
  }
  // find matching natural frequency, return the index
  // (if it's a sharp/flat note, returns the half step above)
  for (int i = 0; i < 7; i++) {
    if (val <= OCTAVE2_FREQS_NATURAL[i] + 0.005) { // the 0.005 is to avoid some floating point tomfoolery
      return i;
    }
  }
}


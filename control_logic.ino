/*
Control logic of 3D-pritend solution for affordable electrospinning device

Vojtěch Skoumal 9.10.2023
*/







// libraries
#include <LiquidCrystal.h>


// inicialize LCD display
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
// backlight control pin number
const int lcdBckLight = 10;
// interupt pin number
const int interruptPin = 2;  // button + 10 kohm resistor
// stepper motor control pins numbers
const int stepper_control_1 =  3;  // in1 driver pin
const int stepper_control_2 =  11;  // in2 driver pin
const int stepper_control_3 = 12;  // in3 driver pin
const int stepper_control_4 = 13;  // in4 driver pin
// collector speed detector analog pin
#define reflectance_analog_pin A1  // signal from collector rpm detector

// parameters constants
const int sample_lenght = 128;
const int reflectance_threshold = 500;  // for collector speed measurement - mean(average low, average high)
int samples[sample_lenght];

// variables
int dwell_time = 1;
int current_menu = 0;
int direction = 1;
int pumped = 0;
bool moving = false;
float collector_speed = 0;

// text placeholders
char *top_row[] = {"Status:         ", "Dwell Time:     ", "Collector speed:"};
char *bottom_row[] = {"Pumped:         ", "Approx Flow:    ", "                "};
char blank_row[] = "                ";

void setup() {
  Serial.begin(9600);
  Serial.println("start");
  // set pinMode for stepper motor cotrol pins
  pinMode(stepper_control_1, OUTPUT);
  pinMode(stepper_control_2, OUTPUT);
  pinMode(stepper_control_3, OUTPUT);
  pinMode(stepper_control_4, OUTPUT);
  // initialize display (16 x 2)
  lcd.begin(16, 2);
  digitalWrite(lcdBckLight, HIGH);
  // set pinMode for display backlight control
  pinMode(lcdBckLight, OUTPUT);
  // setup interupt pin
  pinMode(interruptPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(interruptPin), interup_func, RISING);
  Serial.begin(9600);
}

void loop() {
  if (moving) {
    if (direction == 1) {
      revolutionClockwise();
      pumped += 1;
      lcd.setCursor(8, 1);
      lcd.print(pumped * 59.7985 / 512); // volume pumped in microliters
    }
    else {
      revolutionCounterClockwise();
    }
  }
  else {
    // read from display buttons
    int analogData = analogRead(0);
    solve_display_button(analogData);
    // write text placeholders
    lcd.setCursor(0, 0);
    lcd.print(top_row[current_menu]);
    lcd.setCursor(0, 1);
    lcd.print(bottom_row[current_menu]);
    // write values
    set_values_text();
    delay(300);
  }
}
void solve_display_button(int analog) {
  // each display button is coded to its unique analog output value
  // thus, only one button can be pressed at a time
  if ((analog > 700) && (analog < 1024)); // no button is pressed
  else if (analog < 50) update_parameter(1);  // right
  else if ( (analog > 95) && (analog < 150) ) if (current_menu == 2) current_menu = 0; else current_menu += 1; // up
  else if ( (analog > 250) && (analog < 350) ) if (current_menu == 0) current_menu = 2; else current_menu -= 1;  // down
  else if ( (analog > 400) && (analog < 500) ) update_parameter(-1);  // left
  else if ( (analog > 600) && (analog < 750) ) update_parameter(0);  // select
  return;
}
void update_parameter(int change) {
  if (current_menu == 0) {
    // status menu - turning pump on with SELECT (0)
    // changing direction with left (-1) and right (1)
    if (change == 0) {
      moving = true;
    }
    else {
      direction = change;
    }
  }
  else if (current_menu == 1) {
    // dwell time - left (-1) and right (1) changes dwell time
    dwell_time += change;
    // dwell_time change resets pumped counter
    pumped = 0;
    // prevent dwell_times out of bounds
    if (dwell_time <= 0) {
      dwell_time = 1;
    }
  }
}
void set_values_text() {
  switch (current_menu) {
    case 0:
      // Status
      lcd.setCursor(8, 0);
      if (moving) {
        if (direction == 1) {
          lcd.print("Moving F");
        }
        else {
          lcd.print("Moving B");
        }
      }
      else {
        if (direction == 1) {
          lcd.print("Idle F");
        }
        else {
          lcd.print("Idle B");
        }
      }
      lcd.setCursor(8, 1);
      lcd.print(pumped * 59.7985 / 512); // volume pumped in microliters
      break;
    case 1:
      // Dwell time
      lcd.setCursor(12, 0);
      lcd.print(dwell_time);
      lcd.setCursor(12, 1);
      lcd.print(52.734375/dwell_time);
      break;
    case 2:
      // Collector speed
      measure_collecor_speed();
      lcd.setCursor(4, 1);
      lcd.print(collector_speed);
      break;
  }
  return;
}

void interup_func() {
  Serial.println("iterrupted");
  moving = false;
}

// stepper motor helper functions
// full revolution (360°) = 512 calls of revolutionClockwise - inner gearbox
void revolutionClockwise() {
  // extrusion
  step1();
  step2();
  step3();
  step4();
  step5();
  step6();
  step7();
  step8();
}
void revolutionCounterClockwise() {
  // unload
  step8();
  step7();
  step6();
  step5();
  step4();
  step3();
  step2();
  step1();
}
// individual steps of single revolution - 8 in total for single revolution of stepper motor (before inner gearbox conversion)
void step1(){
  digitalWrite(stepper_control_1, HIGH);
  digitalWrite(stepper_control_2, LOW);
  digitalWrite(stepper_control_3, LOW);
  digitalWrite(stepper_control_4, LOW);
  delay(dwell_time);
}
void step2(){
  digitalWrite(stepper_control_1, HIGH);
  digitalWrite(stepper_control_2, HIGH);
  digitalWrite(stepper_control_3, LOW);
  digitalWrite(stepper_control_4, LOW);
  delay(dwell_time);
}
void step3(){
  digitalWrite(stepper_control_1, LOW);
  digitalWrite(stepper_control_2, HIGH);
  digitalWrite(stepper_control_3, LOW);
  digitalWrite(stepper_control_4, LOW);
  delay(dwell_time);
}
void step4(){
  digitalWrite(stepper_control_1, LOW);
  digitalWrite(stepper_control_2, HIGH);
  digitalWrite(stepper_control_3, HIGH);
  digitalWrite(stepper_control_4, LOW);
  delay(dwell_time);
}
void step5(){
  digitalWrite(stepper_control_1, LOW);
  digitalWrite(stepper_control_2, LOW);
  digitalWrite(stepper_control_3, HIGH);
  digitalWrite(stepper_control_4, LOW);
  delay(dwell_time);
}
void step6(){
  digitalWrite(stepper_control_1, LOW);
  digitalWrite(stepper_control_2, LOW);
  digitalWrite(stepper_control_3, HIGH);
  digitalWrite(stepper_control_4, HIGH);
  delay(dwell_time);
}
void step7(){
  digitalWrite(stepper_control_1, LOW);
  digitalWrite(stepper_control_2, LOW);
  digitalWrite(stepper_control_3, LOW);
  digitalWrite(stepper_control_4, HIGH);
  delay(dwell_time);
}
void step8(){
  digitalWrite(stepper_control_1, HIGH);
  digitalWrite(stepper_control_2, LOW);
  digitalWrite(stepper_control_3, LOW);
  digitalWrite(stepper_control_4, HIGH);
  delay(dwell_time); 
}
void measure_collecor_speed() {
  int peaks = 0;
  int avg_counts_between_peaks = 0;
  int t_counts_between_peaks = 0;
  int to_be_ignored = 10;
  unsigned long start;
  start = millis();
  for (byte i = 0; i < sample_lenght; i++) {
    samples[i] = analogRead(reflectance_analog_pin);
    delay(1);  // to be removed for high RPMs
  }
  float time_per_sample = (millis() - start) * 1000 / sample_lenght;  // factor 1000 solves problem of small numbers - result in krpm not rpm
  Serial.println(time_per_sample);
  for (byte i = 0; i < sample_lenght; i++) {
    t_counts_between_peaks += 1;
    if (samples[i] >= reflectance_threshold) {
      if (t_counts_between_peaks >= to_be_ignored) {
        if (peaks == 0) {
          peaks = 1;
          t_counts_between_peaks = 0;
        }
        else {
          avg_counts_between_peaks = (avg_counts_between_peaks * (peaks - 1) + t_counts_between_peaks) / peaks;
          peaks += 1;
        }
      }
    }
  }
  // avg_counts_between_peaks * time_per_sample gives time per collector revolution in miliseconds
  // 1 over time per collector revolution in seconds gives revolutions per second
  // times 60 to get rpm (actualy in krpm because of the 1000 in time_per_sample calculation)
  collector_speed = 60 / (avg_counts_between_peaks * time_per_sample * 0.001);
}
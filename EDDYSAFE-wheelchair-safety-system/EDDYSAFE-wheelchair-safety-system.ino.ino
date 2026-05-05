#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>
#include <Adafruit_NeoPixel.h>

// --- Rocker Switch Configuration ---
const int switchPin = 2; // Middle pin to Pin 2, outer pin to GND

// --- LED Strip Configuration ---
#define PIN_A      6   
#define PIN_B      7   
#define LED_COUNT 60   
Adafruit_NeoPixel stripA(LED_COUNT, PIN_A, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel stripB(LED_COUNT, PIN_B, NEO_GRB + NEO_KHZ800);

// --- Ultrasonic Sensors ---
const int trigPin1 = 4;
const int echoPin1 = 3;
const int trigPin2 = 8; 
const int echoPin2 = 9;

// --- Accelerometer & Relay Configuration ---
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);
const int relayPin = 11;           
const float TRIGGER_ON = 10.0;     
const float TRIGGER_OFF = 8.5;     
bool magnetState = false;          

void setup() {
  Serial.begin(115200);

  // Initialize Switch
  pinMode(switchPin, INPUT_PULLUP);

  // Initialize LEDs
  stripA.begin();
  stripB.begin();
  stripA.setBrightness(50);
  stripB.setBrightness(50);
  clearStrips();

  // Initialize Ultrasonic Pins
  pinMode(trigPin1, OUTPUT);
  pinMode(echoPin1, INPUT);
  pinMode(trigPin2, OUTPUT);
  pinMode(echoPin2, INPUT);

  // Initialize Relay
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);

  // Initialize ADXL345
  if(!accel.begin()) {
    Serial.println("ADXL345 not found!");
    while(1);
  }
  accel.setRange(ADXL345_RANGE_16_G);

  Serial.println("System Fully Integrated - Active");
}

void loop() {
  // --- 1. MASTER SWITCH CHECK ---
  bool ledsEnabled = (digitalRead(switchPin) == LOW);

  // --- 2. ACCELEROMETER / RELAY LOGIC (Independent of Switch) ---
  sensors_event_t event; 
  accel.getEvent(&event);
  
  float x = event.acceleration.x;
  float y = event.acceleration.y;
  float z = event.acceleration.z;

  float xy_magnitude = sqrt(x * x + y * y);
  float slope = atan2(xy_magnitude, z) * 180 / PI;

  if (slope >= TRIGGER_ON && !magnetState) {
    digitalWrite(relayPin, HIGH); 
    magnetState = true;
    Serial.println(">>> MAGNET ON <<<");
  } 
  else if (slope < TRIGGER_OFF && magnetState) {
    digitalWrite(relayPin, LOW);
    magnetState = false;
    Serial.println(">>> MAGNET OFF <<<");
  }

  // --- 3. ULTRASONIC / LED LOGIC ---
  long dist1 = readDistance(trigPin1, echoPin1);
  delay(35); // Small delay to prevent sensor cross-talk
  long dist2 = readDistance(trigPin2, echoPin2);

  long validDist1 = (dist1 == 0) ? 999 : dist1;
  long validDist2 = (dist2 == 0) ? 999 : dist2;
  long shortestReading = min(validDist1, validDist2);

  if (ledsEnabled) {
    // UPDATED THRESHOLDS: Red < 100cm, Yellow 100-200cm
    if (shortestReading < 50) {
      updateStrips(stripA.Color(255, 0, 0)); // Red
    } 
    else if (shortestReading >= 50 && shortestReading <= 200) {
      updateStrips(stripA.Color(255, 255, 0)); // Yellow
    } 
    else if (shortestReading > 200) {
      // Clear ONLY when object is beyond 200cm
      clearStrips();
    }
  } else {
    // If switch is off, kill the lights immediately
    clearStrips();
  }

  delay(50); 
}

// Function to handle the trigger/echo math
long readDistance(int trig, int echo) {
  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);

  long duration = pulseIn(echo, HIGH, 30000); 
  return duration * 0.034 / 2;
}

// Helper: Sets both strips to a solid color
void updateStrips(uint32_t color) {
  stripA.fill(color, 0, LED_COUNT);
  stripB.fill(color, 0, LED_COUNT);
  stripA.show();
  stripB.show();
}

// Helper: Turns all LEDs off
void clearStrips() {
  stripA.clear();
  stripB.clear();
  stripA.show();
  stripB.show();
}
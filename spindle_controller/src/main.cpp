#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>

// ---- OLED pins ----
#define SDA_PIN 5
#define SCL_PIN 6
U8G2_SSD1306_72X40_ER_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

// ---- Tachometric sensor pin ----
#define PIN_SENSOR 7
// ---- Probe relay pin -- set to 1 when spindle is on ----
#define PROBE_OUT 8
// ---- Copy signal from PWM_IN pin ----
#define PWM_OUT 9
// ---- Input with PWM signal for direct spindle control ----
#define PWM_IN 10

// ---- Frequency to send data ----
#define SERIAL_FREQ 250 // ms

// ---- RPM limits ----
#define MAX_RPM 65000
#define MIN_PULSE_INTERVAL (60000000UL / MAX_RPM)  // ~2400µs at 25000 RPM

// Volatile variables for interrupt handling
volatile unsigned long pulseCount = 0;
volatile unsigned long lastPulseTime = 0;
volatile unsigned long pulseInterval = 0;
volatile bool newPulse = false;

// RPM calculation variables
float currentRPM = 0.0;
unsigned long lastSerialUpdate = 0;
unsigned long lastDisplayUpdate = 0;
unsigned long totalPulses = 0;

// Spinlock for critical sections (ESP32-C3)
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

// Interrupt Service Routine for optical sensor (must be in IRAM for ESP32)
void IRAM_ATTR sensorISR() {
    unsigned long currentTime = micros();
    unsigned long interval = currentTime - lastPulseTime;

    // Debounce: ignore pulses faster than MAX_RPM allows (noise filtering)
    if (lastPulseTime > 0 && interval >= MIN_PULSE_INTERVAL) {
        pulseInterval = interval;
        newPulse = true;
        pulseCount++;
    } else if (lastPulseTime == 0) {
        // First pulse - just record the time
        pulseCount++;
    }

    lastPulseTime = currentTime;
}

// ISR for PWM pass-through: forward PWM_IN state directly to PWM_OUT
void IRAM_ATTR pwmPassthroughISR() {
    digitalWrite(PWM_OUT, digitalRead(PWM_IN));
}

// Calculate RPM from pulse interval (call from within critical section)
float calculateRPM(unsigned long interval) {
    if (interval == 0) {
        return 0.0;
    }

    // RPM = (60 seconds * 1,000,000 microseconds) / pulse_interval_in_microseconds
    // Since we have 1 pulse per revolution
    float rpm = 60000000.0 / (float)interval;

    // Clamp to maximum expected RPM
    if (rpm > MAX_RPM) {
        rpm = MAX_RPM;
    }

    return rpm;
}

// Send statistics to serial port
void sendStatistics() {
    Serial.print("RPM: ");
    Serial.print(currentRPM, 2);
    Serial.print(" | Pulses: ");
    Serial.print(totalPulses);
    Serial.print(" | Interval(us): ");
    Serial.println(pulseInterval);
}


// ---- Display helper ----
void drawOLEDMessage(int rpm, const char* status) {
  u8g2.clearBuffer();
  u8g2.setDrawColor(1);

  // Format RPM as 5-digit string with leading zeros (clamped to MAX_RPM)
  char rpmStr[6];
  snprintf(rpmStr, sizeof(rpmStr), "%05d", rpm > MAX_RPM ? MAX_RPM : rpm);

  // Large font for RPM - u8g2_font_logisoso16_tn is 16px tall numeric font
  // Each digit is ~11px wide, 5 digits = ~55px, centered in 72px = offset ~8
  u8g2.setFont(u8g2_font_logisoso16_tn);
  int rpmWidth = u8g2.getStrWidth(rpmStr);
  int rpmX = (72 - rpmWidth) / 2;
  u8g2.drawStr(rpmX, 20, rpmStr);

  // Small font for status message below
  u8g2.setFont(u8g2_font_5x8_tr);
  int statusWidth = u8g2.getStrWidth(status);
  int statusX = (72 - statusWidth) / 2;
  u8g2.drawStr(statusX, 38, status);

  u8g2.sendBuffer();
}

void setup() {
  // Initialize serial communication
  Serial.begin(115200);

  // Initialize I2C and OLED
  Wire.begin(SDA_PIN, SCL_PIN);
  u8g2.begin();

  drawOLEDMessage(0, "Starting...");

  // Configure sensor pin as input with pullup
  pinMode(PIN_SENSOR, INPUT_PULLUP);

  // Configure PWM pass-through pins
  pinMode(PWM_IN, INPUT);
  pinMode(PWM_OUT, OUTPUT);
  digitalWrite(PWM_OUT, LOW);

  // Configure probe output pin
  pinMode(PROBE_OUT, OUTPUT);
  digitalWrite(PROBE_OUT, LOW);

  // Attach interrupt - FALLING edge for optical sensor
  attachInterrupt(digitalPinToInterrupt(PIN_SENSOR), sensorISR, FALLING);

  // Attach interrupt for PWM pass-through - trigger on any edge change
  attachInterrupt(digitalPinToInterrupt(PWM_IN), pwmPassthroughISR, CHANGE);

  Serial.println("Tachometer initialized");
  Serial.println("Waiting for pulses...");

  lastSerialUpdate = millis();
  lastDisplayUpdate = millis();
}

void loop() {
  // Snapshot volatile variables under critical section
  unsigned long intervalSnapshot;
  unsigned long lastPulseSnapshot;
  bool hasNewPulse;

  portENTER_CRITICAL(&mux);
  hasNewPulse = newPulse;
  intervalSnapshot = pulseInterval;
  lastPulseSnapshot = lastPulseTime;
  totalPulses = pulseCount;
  newPulse = false;
  portEXIT_CRITICAL(&mux);

  // Update RPM if new pulse received
  if (hasNewPulse) {
    currentRPM = calculateRPM(intervalSnapshot);
  }

  // Check if no pulses received for 1 second -> motor stopped
  unsigned long timeSinceLastPulse = micros() - lastPulseSnapshot;
  if (timeSinceLastPulse > 1000000 && currentRPM > 0) {
    currentRPM = 0.0;
  }

  // Update PROBE_OUT: HIGH when spindle is running, LOW when stopped
  digitalWrite(PROBE_OUT, currentRPM > 0 ? HIGH : LOW);

  // Send statistics to serial at SERIAL_FREQ interval
  unsigned long currentTime = millis();
  if (currentTime - lastSerialUpdate >= SERIAL_FREQ) {
    sendStatistics();
    lastSerialUpdate = currentTime;
  }

  // Update OLED display at 10Hz (100ms)
  if (currentTime - lastDisplayUpdate >= 100) {
    const char* status = (currentRPM > 0) ? "Running" : "Stopped";
    drawOLEDMessage((int)currentRPM, status);
    lastDisplayUpdate = currentTime;
  }
}

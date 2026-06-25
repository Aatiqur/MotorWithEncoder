// ===================================
// DAKE MOTOR - ROTATION + RPM
// SNT MAG 18A2 V2 ENCODER
// Rotating: FRONT OUTPUT SHAFT
// CPR: 1200 (after gearbox)
// ===================================

#define ENCODER_A 5
#define ENCODER_B 18

#define PPR 18                  // 1200 / 4
#define QUADRATURE 4             // 4x quadrature decoding
#define CPR (PPR * QUADRATURE)   // = 1200 counts per revolution

volatile long pulses = 0;
volatile uint8_t lastState = 0;

// ===================================
// QUADRATURE DECODE
// ===================================
inline int8_t decodeQuadrature(uint8_t last, uint8_t current) {
  if ((last == 0b00 && current == 0b01) ||
      (last == 0b01 && current == 0b11) ||
      (last == 0b11 && current == 0b10) ||
      (last == 0b10 && current == 0b00)) return +1;
  if ((last == 0b00 && current == 0b10) ||
      (last == 0b10 && current == 0b11) ||
      (last == 0b11 && current == 0b01) ||
      (last == 0b01 && current == 0b00)) return -1;
  return 0;
}

// ===================================
// ENCODER TASK - CORE 0
// ===================================
void EncoderTask(void *parameter) {
  lastState = (digitalRead(ENCODER_A) << 1) | digitalRead(ENCODER_B);
  while (true) {
    uint8_t currentState = (digitalRead(ENCODER_A) << 1) | digitalRead(ENCODER_B);
    int8_t dir = decodeQuadrature(lastState, currentState);
    pulses += dir;
    lastState = currentState;
    vTaskDelay(1);
  }
}

// ===================================
// SETUP
// ===================================
void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(ENCODER_A, INPUT);
  pinMode(ENCODER_B, INPUT);

  Serial.println("==============================");
  Serial.println("   DAKE MOTOR ENCODER");
  Serial.println("   SNT MAG 18A2 V2");
  Serial.println("   CPR : 1200");
  Serial.println("   Send r to reset");
  Serial.println("==============================");

  xTaskCreatePinnedToCore(
    EncoderTask,
    "EncoderTask",
    4096,
    NULL,
    2,
    NULL,
    0
  );
}

// ===================================
// MAIN LOOP
// ===================================
void loop() {
  static long lastPulses = 0;
  static long rpmPulses = 0;
  static unsigned long lastTime = 0;

  unsigned long currentTime = millis();
  unsigned long elapsed = currentTime - lastTime;

  // Print on every count change
  if (pulses != lastPulses) {
    float totalRotations = (float)pulses / CPR;

    Serial.print("Pulses: ");
    Serial.print(pulses);
    Serial.print("  |  Rotations: ");
    Serial.print(totalRotations, 2);
    Serial.print("  |  Dir: ");
    Serial.println(pulses > lastPulses ? "CW  -->" : "<-- CCW");

    lastPulses = pulses;
  }

  // RPM every 500ms
  if (elapsed >= 500) {
    long deltaPulses = pulses - rpmPulses;
    float rpm = ((float)deltaPulses / CPR) / ((float)elapsed / 60000.0);

    Serial.print(">>> RPM: ");
    Serial.print(abs(rpm), 1);
    Serial.print("  |  Dir: ");
    Serial.println(deltaPulses >= 0 ? "CW  -->" : "<-- CCW");

    rpmPulses = pulses;
    lastTime = currentTime;
  }

  // Reset
  if (Serial.available() && Serial.read() == 'r') {
    pulses = 0;
    lastPulses = 0;
    rpmPulses = 0;
    Serial.println("==============================");
    Serial.println("        RESET TO ZERO");
    Serial.println("==============================");
  }

  delay(10);
}
#include <math.h>

const int trigPin = 9;
const int echoPin = 10;

// Settings
const unsigned long interval = 50;     // ms between measurements
const int sampleCount = 20;            // Number of samples per batch
const float minDistance = 2.0;         // cm (minimum valid reading)
const float maxDistance = 400.0;       // cm (maximum valid reading)

float distances[sampleCount];
int sampleIndex = 0;
unsigned long lastMillis = 0;

void setup() {
  Serial.begin(9600);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  // Initialize distances to zero
  for (int i = 0; i < sampleCount; i++) {
    distances[i] = 0;
  }
}

void loop() {
  unsigned long now = millis();
  if (now - lastMillis >= interval) {
    lastMillis = now;

    float d = getDistanceCM();

    // Store only valid readings, else store 0
    if (d >= minDistance && d <= maxDistance) {
      distances[sampleIndex] = d;
    } else {
      distances[sampleIndex] = 0;
    }

    sampleIndex++;

    // Once enough samples, compute and send RMS
    if (sampleIndex >= sampleCount) {
      float sumSquares = 0;
      int validCount = 0;
      for (int i = 0; i < sampleCount; i++) {
        if (distances[i] > 0) {
          sumSquares += distances[i] * distances[i];
          validCount++;
        }
      }

      float rms = 0;
      if (validCount > 0) {
        rms = sqrt(sumSquares / validCount);
      }

      // Output only the RMS value (easy for Python to read)
      Serial.println(rms);

      sampleIndex = 0; // Reset for next batch
    }
  }
}

float getDistanceCM() {
  // Send 10us pulse
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 30000); // Timeout 30 ms
  if (duration == 0) return 0;
  return (duration * 0.0343) / 2.0;
}

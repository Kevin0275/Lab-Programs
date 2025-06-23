// Define pins
const int trigPin = 9;
const int echoPin = 10;

// Constants
const unsigned long interval = 50; // ms
const int sampleCount = 20; // number of samples for RMS (e.g. 1 second worth if 50ms interval)
const float minDistance = 2.0;  // cm - adjust as needed
const float maxDistance = 400.0; // cm - adjust as needed

// Storage for samples
float distances[sampleCount];
int currentIndex = 0;

unsigned long lastMillis = 0;

void setup() {
  Serial.begin(9600);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // Initialize array
  for (int i = 0; i < sampleCount; i++) {
    distances[i] = 0;
  }
}

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - lastMillis >= interval) {
    lastMillis = currentMillis;

    float distance = measureDistance();

    // Check if value is reasonable
    if (distance >= minDistance && distance <= maxDistance) {
      distances[currentIndex] = distance;
    } else {
      distances[currentIndex] = 0; // Treat as zero for RMS; you can choose to skip instead
    }

    currentIndex++;

    if (currentIndex >= sampleCount) {
      currentIndex = 0;

      // Compute RMS
      float sumSquares = 0;
      int validSamples = 0;
      
      for (int i = 0; i < sampleCount; i++) {
        if (distances[i] > 0) {
          sumSquares += distances[i] * distances[i];
          validSamples++;
        }
      }

      float rms = 0;
      if (validSamples > 0) {
        rms = sqrt(sumSquares / validSamples);
      }

      // Output the RMS distance
      Serial.print("RMS Distance: ");
      Serial.print(rms);
      Serial.println(" cm");
    }
  }
}

float measureDistance() {
  // Clear trigger
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  // 10us trigger pulse
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Read echo pulse duration
  long duration = pulseIn(echoPin, HIGH, 30000); // timeout at 30 ms (~5 m max)

  if (duration == 0) {
    return 0; // Timeout → invalid
  }

  // Calculate distance
  float distance = (duration * 0.0343) / 2;

  return distance;
}

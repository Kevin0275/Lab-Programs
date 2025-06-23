// Define pins
const int trigPin = 9;  // Connect T (transmitter) to pin 9
const int echoPin = 10; // Connect R (receiver) to pin 10

// Variables to store duration and distance
long duration;
float distance;

void setup() {
  // Initialize serial communication for output
  Serial.begin(9600);
  
  // Configure pins
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
}

void loop() {
  // Ensure trigger pin is low
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  // Send a 10 microsecond pulse to trigger the ultrasonic burst
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Read the echo pin for duration of return pulse
  duration = pulseIn(echoPin, HIGH);

  // Calculate distance: speed of sound = 343 m/s
  // duration is round trip; divide by 2; convert microseconds to seconds
  // distance = (duration / 2) * (speed of sound)
  // distance = (duration / 2) * 0.0343 cm/microsecond
  distance = (duration * 0.0343) / 2;

  // Output the distance
  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  // Small delay before next measurement
  delay(500);
}

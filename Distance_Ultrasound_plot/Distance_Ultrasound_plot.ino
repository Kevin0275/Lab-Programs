const int trigPin = 9;
const int echoPin = 10;

long duration;
float distanceCm;

void setup() {
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  Serial.begin(9600);
}

void loop() {
  // Send 10 microsecond pulse to TRIG
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Read duration of ECHO pulse
  duration = pulseIn(echoPin, HIGH);

  // Calculate distance (sound speed = 343 m/s)
  distanceCm = duration * 0.0343 / 2;

  // Display result
  Serial.print("Distance: ");
  Serial.print(distanceCm);
  Serial.println(" cm");

  delay(200);
}

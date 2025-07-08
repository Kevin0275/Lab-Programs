#define TRIG_PIN 9
#define ECHO_PIN 10

void setup() {
  Serial.begin(9600);           // Start Serial at 9600 baud
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
}

void loop() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH);
  float distance = duration * 0.034 / 2;  // Convert to cm

  Serial.println(distance);     // Send distance in cm
  delay(100);                   // Send every 100ms
}

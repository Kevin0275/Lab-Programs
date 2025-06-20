// PWM LED Sine Wave Fader
const int ledPin = 9;       // PWM pin connected to LED
float angle = 0.0;          // Radians for sine function
const float increment = 0.1;  // Change in angle per loop
const int delayTime = 10;     // Delay in ms per step

void setup() {
  pinMode(ledPin, OUTPUT);
}

void loop() {
  // Calculate sine-based brightness: map sin() from [-1,1] to [0,255]
  float rawSine = sin(angle);               // -1 to 1
  int pwmValue = int((rawSine + 1) * 127.5); // 0 to 255

  analogWrite(ledPin, pwmValue); // Write PWM value to LED

  angle += increment;            // Increment angle
  if (angle > TWO_PI) {          // Reset after 2π for smooth repeat
    angle = 0;
  }

  delay(delayTime);              // Small delay for visible smoothness
}

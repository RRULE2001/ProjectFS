#define BAUDRATE 9600
#define PIN2 2

void setup() {
  Serial.begin(BAUDRATE);
  pinMode(PIN2, INPUT); // Sets PIN to input
  digitalWrite(PIN2, HIGH); // Sets PIN to have pull up
  attachInterrupt (digitalPinToInterrupt(PIN2), interruptPin2, FALLING);
}

void interruptPin2(uint8_t pin) {
  Serial.println("PIN2");
}

void loop() {
  //Serial.println(" ");
  delay(1000);
}

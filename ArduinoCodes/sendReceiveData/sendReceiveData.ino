/*
 * Example For sending and receiving data with SerialPort
 */

#define BAUD 9600
#define LED LED_BUILTIN


void setup() {
  Serial.begin(BAUD);
  pinMode(LED, OUTPUT);
}

void loop() {
  if (Serial.available() > 0) {
    String receivedString = Serial.readStringUntil('\n');
    
    Serial.print("Hello, world!\n");
    Serial.print(receivedString);
    Serial.print("\n");
    
    if (receivedString.equals("ON")) digitalWrite(LED, LOW);
    else if (receivedString.equals("OFF")) digitalWrite(LED, HIGH);
  }
  delay(30);
}

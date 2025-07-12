#define LED_BUILTIN 2

unsigned long previousMillis = 0;
const long interval = 2000;  // 2 second interval

void setup() {
  // start serial port at 9600 bps:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  
  
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)
  
  while (Serial.available() <= 0) { // wait until incoming connection sends something
    Serial.print('.');
    delay(300);
  }

  digitalWrite(LED_BUILTIN, LOW);   // turn the LED off by making the voltage LOW
}

void loop() {
  unsigned long currentMillis = millis();
  
  // Send "hello world" every 2 seconds (non-blocking)
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    Serial.println("Hello world!");
  }
  
  // Echo back any received data
  if (Serial.available() > 0) {
    Serial.write(Serial.read());
  }
}

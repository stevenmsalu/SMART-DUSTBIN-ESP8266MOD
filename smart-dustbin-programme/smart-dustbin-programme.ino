
#define BUZZER_PIN D4

void setup() {
  Serial.begin(115200);

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  Serial.println();
  Serial.println("================================");
  Serial.println(" SMART DUSTBIN - BUZZER TEST");
  Serial.println("================================");

  delay(1000);
}

void loop() {

  // TEST 1: Steady HIGH signal
  Serial.println("TEST 1: Active buzzer test");

  digitalWrite(BUZZER_PIN, HIGH);
  delay(1000);

  digitalWrite(BUZZER_PIN, LOW);
  delay(1000);

  // TEST 2: 2kHz frequency
  Serial.println("TEST 2: Passive buzzer test");

  tone(BUZZER_PIN, 2000);
  delay(1500);

  noTone(BUZZER_PIN);
  digitalWrite(BUZZER_PIN, LOW);

  delay(1500);

  // TEST 3: Repeated beeps
  Serial.println("TEST 3: Repeated beep pattern");

  for (int i = 0; i < 3; i++) {
    tone(BUZZER_PIN, 2000);
    delay(250);

    noTone(BUZZER_PIN);
    digitalWrite(BUZZER_PIN, LOW);
    delay(250);
  }

  delay(2000);
}

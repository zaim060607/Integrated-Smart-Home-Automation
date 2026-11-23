#include <Servo.h>
#include <math.h>

const byte TEMP = A0, LDR = A1, PIR = 2, BELL = 3, MODE = 4;
const byte COOL = 5, LIGHT = 6, ALERT = 7, WELCOME = 8;
const byte NORMAL = 9, ARMED = 10, BUZZER = 11, DOOR = 12;
Servo door;
bool armed = false, cooling = false, doorOpen = false;
bool bellLast = HIGH, modeLast = HIGH;
unsigned long bellChange = 0, modeChange = 0, openAt = 0;
unsigned long sampledAt = 0, reportedAt = 0;
float temperature = 24;
int ldr = 0;
bool motion = false, dark = false;

void setup() {
  Serial.begin(9600);
  pinMode(PIR, INPUT);
  pinMode(BELL, INPUT_PULLUP);
  pinMode(MODE, INPUT_PULLUP);
  for (byte p = COOL; p <= BUZZER; p++) pinMode(p, OUTPUT);
  door.attach(DOOR);
  door.write(0);
  Serial.println("SMART HOME SIMULATION READY");
}

void loop() {
  unsigned long now = millis();
  bool m = digitalRead(MODE);
  if (m != modeLast) modeChange = now;
  static bool modeStable = HIGH;
  if ((now - modeChange) > 40 && m != modeStable) {
    modeStable = m;
    if (m == LOW) {
      armed = !armed;
      Serial.println(armed ? "SECURITY MODE ARMED" : "NORMAL MODE");
    }
  }
  modeLast = m;

  bool b = digitalRead(BELL);
  if (b != bellLast) bellChange = now;
  static bool bellStable = HIGH;
  if ((now - bellChange) > 40 && b != bellStable) {
    bellStable = b;
    if (b == LOW) {
      doorOpen = true;
      openAt = now;
      door.write(90);
      Serial.println("DOOR OPEN");
    }
  }
  bellLast = b;
  if (doorOpen && now - openAt >= 5000) {
    doorOpen = false;
    door.write(0);
    Serial.println("DOOR CLOSED");
  }
  digitalWrite(WELCOME, doorOpen);

  if (now - sampledAt >= 200) {
    sampledAt = now;
    int raw = analogRead(TEMP);
    if (raw > 0 && raw < 1023) {
      temperature = 1.0 / (log(1.0 / (1023.0 / raw - 1.0)) / 3950.0 + 1.0 / 298.15) - 273.15;
    }
    if (temperature >= 30) cooling = true;
    if (temperature <= 28) cooling = false;
    digitalWrite(COOL, cooling);

    // Wokwi LDR module: higher analog reading means DARKER.
    ldr = analogRead(LDR);
    dark = ldr > 550;
    digitalWrite(LIGHT, dark);
    motion = digitalRead(PIR) == HIGH;
  }

  digitalWrite(NORMAL, !armed);
  digitalWrite(ARMED, armed);
  digitalWrite(ALERT, motion && (!armed || (now / 250) % 2 == 0));
  if (armed && motion) {
    if ((now / 250) % 2 == 0) tone(BUZZER, 1400);
    else noTone(BUZZER);
  } else if (cooling && (now % 4000 < 150)) {
    tone(BUZZER, 900);
  } else {
    noTone(BUZZER);
  }

  if (now - reportedAt >= 1000) {
    reportedAt = now;
    Serial.print("Temp C: "); Serial.print(temperature, 1);
    Serial.print(" | LDR: "); Serial.print(ldr);
    Serial.print(" | Motion: "); Serial.print(motion ? "YES" : "NO");
    Serial.print(" | Cooling: "); Serial.print(cooling ? "ON" : "OFF");
    Serial.print(" | Room Light: "); Serial.print(dark ? "ON" : "OFF");
    Serial.print(" | Mode: "); Serial.println(armed ? "ARMED" : "NORMAL");
  }
}

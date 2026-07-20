#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <string.h>

// Dacă LCD-ul nu afișează nimic, încearcă adresa 0x3F.
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Pini
const byte buzzerPin = 8;
const byte keyPin = 7;

// Buzzerul activ are oscilator intern: este pornit/oprit digital.
const byte wpm = 15;

// Durata punctului în milisecunde
const unsigned int dotTime = 1200 / wpm;

// Sub acest prag: punct.
// Peste acest prag: linie.
const unsigned int dotDashLimit = dotTime * 2;

// Pauza după care caracterul introdus este considerat terminat
const unsigned int characterTimeout = dotTime * 5;

// Timp maxim pentru începerea răspunsului
const unsigned long answerTimeout = 10000UL;

// Lungimea maximă a unui caracter Morse
const byte maxMorseLength = 5;

const char characters[] =
  "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

// Numărul de caractere disponibile pentru exerciții.
const byte characterCount = sizeof(characters) - 1;

const char* morseCodes[] = {
  // A-Z
  ".-",    // A
  "-...",  // B
  "-.-.",  // C
  "-..",   // D
  ".",     // E
  "..-.",  // F
  "--.",   // G
  "....",  // H
  "..",    // I
  ".---",  // J
  "-.-",   // K
  ".-..",  // L
  "--",    // M
  "-.",    // N
  "---",   // O
  ".--.",  // P
  "--.-",  // Q
  ".-.",   // R
  "...",   // S
  "-",     // T
  "..-",   // U
  "...-",  // V
  ".--",   // W
  "-..-",  // X
  "-.--",  // Y
  "--..",  // Z

  // 0-9
  "-----",  // 0
  ".----",  // 1
  "..---",  // 2
  "...--",  // 3
  "....-",  // 4
  ".....",  // 5
  "-....",  // 6
  "--...",  // 7
  "---..",  // 8
  "----."   // 9
};

void soundOn() {
  digitalWrite(buzzerPin, HIGH);
}

void soundOff() {
  digitalWrite(buzzerPin, LOW);
}

void playDot() {
  soundOn();
  delay(dotTime);
  soundOff();
  delay(dotTime);
}

void playDash() {
  soundOn();
  delay(dotTime * 3);
  soundOff();
  delay(dotTime);
}

void playMorse(const char* code) {
  for (byte i = 0; code[i] != '\0'; i++) {
    if (code[i] == '.') {
      playDot();
    } else if (code[i] == '-') {
      playDash();
    }
  }

  soundOff();
}

void waitForKeyRelease() {
  soundOff();

  while (digitalRead(keyPin) == LOW) {
    delay(5);
  }

  delay(30);
}

// Testul începe numai după o apăsare intenționată a cheii.
void waitForTestStart() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Apasa butonul");
  lcd.setCursor(0, 1);
  lcd.print("pentru start");

  while (digitalRead(keyPin) == HIGH) {
    delay(5);
  }

  waitForKeyRelease();
}

void clearSecondLine() {
  lcd.setCursor(0, 1);
  lcd.print("                ");
  lcd.setCursor(0, 1);
}

bool readUserMorse(char* receivedCode) {
  byte position = 0;
  bool started = false;

  unsigned long waitingStart = millis();
  unsigned long lastReleaseTime = millis();

  receivedCode[0] = '\0';

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Reproduce:");
  clearSecondLine();

  waitForKeyRelease();

  while (true) {
    if (digitalRead(keyPin) == LOW) {
      started = true;

      unsigned long pressStart = millis();

      soundOn();

      while (digitalRead(keyPin) == LOW) {
        delay(1);
      }

      soundOff();

      unsigned long pressDuration = millis() - pressStart;

      // Ignoră impulsurile extrem de scurte provocate de contacte
      if (pressDuration >= 15) {
        if (position < maxMorseLength) {
          if (pressDuration < dotDashLimit) {
            receivedCode[position] = '.';
          } else {
            receivedCode[position] = '-';
          }

          position++;
          receivedCode[position] = '\0';

          clearSecondLine();
          lcd.print(receivedCode);
        }

        lastReleaseTime = millis();
      }

      delay(20);
    }

    if (started && millis() - lastReleaseTime >= characterTimeout) {
      return true;
    }

    if (!started && millis() - waitingStart >= answerTimeout) {
      return false;
    }
  }
}

void correctSound() {
  soundOn();
  delay(100);
  soundOff();

  delay(80);

  soundOn();
  delay(150);
  soundOff();
}

void errorSound() {
  soundOn();
  delay(400);
  soundOff();
}

void showResult(
  char targetCharacter,
  const char* targetCode,
  const char* receivedCode) {
  lcd.clear();

  if (strcmp(targetCode, receivedCode) == 0) {
    lcd.setCursor(0, 0);
    lcd.print("CORECT!");

    lcd.setCursor(0, 1);
    lcd.print(targetCharacter);
    lcd.print(" = ");
    lcd.print(targetCode);

    correctSound();
  } else {
    lcd.setCursor(0, 0);
    lcd.print("GRESIT: ");
    lcd.print(receivedCode);

    lcd.setCursor(0, 1);
    lcd.print(targetCharacter);
    lcd.print(" = ");
    lcd.print(targetCode);

    errorSound();
  }

  delay(3000);
}

void setup() {
  pinMode(buzzerPin, OUTPUT);
  pinMode(keyPin, INPUT_PULLUP);

  Wire.begin();

  lcd.begin();
  lcd.backlight();

  randomSeed(analogRead(A0));

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("CW Trainer");

  lcd.setCursor(0, 1);
  lcd.print(wpm);
  lcd.print(" WPM");

  delay(2500);

  waitForTestStart();
}

void loop() {
  char receivedCode[maxMorseLength + 1];

  // Alege aleatoriu o literă sau o cifră disponibilă.
  int index = random(0, characterCount);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Asculta...");

  lcd.setCursor(0, 1);
  lcd.print(characters[index]);
  lcd.print(" = ");
  lcd.print(morseCodes[index]);

  delay(800);

  // Arduino demonstrează caracterul.
  playMorse(morseCodes[index]);

  delay(dotTime * 4);

  // Utilizatorul reproduce caracterul cu cheia CW.
  bool entered = readUserMorse(receivedCode);

  if (entered) {
    showResult(
      characters[index],
      morseCodes[index],
      receivedCode);
  } else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Timp expirat");

    lcd.setCursor(0, 1);
    lcd.print(characters[index]);
    lcd.print(" = ");
    lcd.print(morseCodes[index]);

    errorSound();
    delay(3000);
  }
}

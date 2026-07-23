#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <string.h>
#include <stdio.h>

// LCD 1602 I2C existent; incearca 0x3F daca modulul nu raspunde la 0x27.
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Arduino Nano (ATmega328P): A4/A5 sunt rezervati pentru LCD/I2C, iar D8
// pentru buzzer. D7 este cheia/butonul original si nu este reutilizat.
constexpr byte buzzerPin = 8;
constexpr byte keyPin = 7;  // Buton existent, activ LOW, INPUT_PULLUP.
// Calibrarea pe dispozitiv a aratat ca orientarea fizica nu corespunde
// etichetelor VRx/VRy: A0 se modifica sus/jos, iar A1 stanga/dreapta.
constexpr byte PIN_JOYSTICK_AXIS_1 = A0;
constexpr byte PIN_JOYSTICK_AXIS_2 = A1;
constexpr byte PIN_JOYSTICK_SW = 2;
constexpr byte PIN_STRAIGHT_KEY = 3;
constexpr byte PIN_PADDLE_DIT = 4;
constexpr byte PIN_PADDLE_DAH = 5;

constexpr int DEFAULT_WPM = 15;
constexpr int MIN_WPM = 5;
constexpr int MAX_WPM = 50;
constexpr int WPM_STEP = 1;
constexpr byte MAIN_MENU_ITEM_COUNT = 4;
// Nano are ADC pe 10 biti (0..1023); aceste praguri sunt usor de calibrat.
constexpr int JOYSTICK_LOW_THRESHOLD = 300;
constexpr int JOYSTICK_HIGH_THRESHOLD = 700;
// Valorile brute verificate pentru montajul curent: A0 mare = SUS, A0 mic = JOS,
// A1 mare = DREAPTA si A1 mic = STANGA. Nu deduce directia din numele VRx/VRy.
constexpr bool JOYSTICK_DIAGNOSTICS = true;
constexpr unsigned long JOYSTICK_INITIAL_REPEAT_MS = 350;
constexpr unsigned long JOYSTICK_REPEAT_MS = 140;
constexpr unsigned long JOYSTICK_DEBOUNCE_MS = 25;
constexpr unsigned long JOYSTICK_LONG_PRESS_MS = 800;
constexpr unsigned long KEY_DEBOUNCE_MS = 12;
constexpr unsigned long answerTimeout = 10000UL;

struct MorseTiming {
  unsigned long dit;
  unsigned long dah;
  unsigned long elementGap;
  unsigned long characterGap;
  unsigned long wordGap;
};

enum class TrainingMode { Letters, Numbers, Phrases };
enum class AppState { MainMenu, TrainingSelection, WpmSettings, Training };
enum class JoystickEvent { None, Up, Down, Left, Right, ShortPress, LongPress };
enum class OutputPhase { Idle, Tone, Gap };
enum class TrainingPhase { Choose, Preview, Playing, WaitAnswer, Result };

int currentWpm = DEFAULT_WPM;
MorseTiming timing;
TrainingMode trainingMode = TrainingMode::Letters;
AppState appState = AppState::MainMenu;
TrainingPhase trainingPhase = TrainingPhase::Choose;
byte menuIndex = 0;
byte selectionIndex = 0;
bool displayDirty = true;

const char characters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
const char *const morseCodes[] = {
  ".-", "-...", "-.-.", "-..", ".", "..-.", "--.", "....", "..", ".---",
  "-.-", ".-..", "--", "-.", "---", ".--.", "--.-", ".-.", "...", "-",
  "..-", "...-", ".--", "-..-", "-.--", "--..",
  "-----", ".----", "..---", "...--", "....-", ".....", "-....", "--...", "---..", "----."
};
constexpr byte LETTER_COUNT = 26;
constexpr byte CHARACTER_COUNT = sizeof(characters) - 1;
// Frazele sunt stocate in flash ca date constante; extinde lista fara RAM dinamic.
const char phrases[][17] = { "CQ CQ", "TEST ONE", "HELLO WORLD", "73 DE YO6LPG" };
constexpr byte PHRASE_COUNT = sizeof(phrases) / sizeof(phrases[0]);

// Motorul comun pentru demonstratie si paddle. Nu foloseste delay().
OutputPhase outputPhase = OutputPhase::Idle;
const char *outputCode = nullptr;
byte outputPosition = 0;
unsigned long outputDeadline = 0;
bool outputFinished = false;

char targetText[17];
char targetCode[110];
bool answerStarted = false;
bool previousExistingKeyDown = false;
unsigned long existingKeyChangedAt = 0;
unsigned long existingKeyDownAt = 0;
unsigned long answerStartedAt = 0;
unsigned long phaseStartedAt = 0;
bool answerCorrect = false;

void acceptAnswerSignal(char signal);
// Pozitia urmatorului semnal care trebuie introdus. Separatorii din targetCode
// ('/' intre litere si '|' intre cuvinte) nu necesita o apasare a cheii.
byte expectedPosition = 0;
bool answerHasError = false;
unsigned long answerErrorShownAt = 0;

bool previousStraightDown = false;
unsigned long straightChangedAt = 0;
bool paddleLastWasDit = false;

bool previousJoystickButtonDown = false;
unsigned long joystickButtonChangedAt = 0;
unsigned long joystickButtonDownAt = 0;
bool joystickLongReported = false;
JoystickEvent heldDirection = JoystickEvent::None;
unsigned long directionChangedAt = 0;
unsigned long directionRepeatAt = 0;
int lastJoystickAxis1 = 0;
int lastJoystickAxis2 = 0;

void recalculateMorseTiming() {
  timing.dit = 1200UL / currentWpm;
  timing.dah = timing.dit * 3UL;
  timing.elementGap = timing.dit;
  timing.characterGap = timing.dit * 3UL;
  timing.wordGap = timing.dit * 7UL;
}

const char *modeName(TrainingMode mode) {
  if (mode == TrainingMode::Letters) return "Litere";
  if (mode == TrainingMode::Numbers) return "Cifre";
  return "Fraze";
}

void soundOn() { digitalWrite(buzzerPin, HIGH); }
void soundOff() { digitalWrite(buzzerPin, LOW); }

void startMorseOutput(const char *code) {
  outputCode = code;
  outputPosition = 0;
  outputPhase = OutputPhase::Idle;
  outputFinished = false;
}

bool morseOutputBusy() { return outputCode != nullptr; }

void updateMorseOutput() {
  if (!outputCode) return;
  unsigned long now = millis();
  if (outputPhase == OutputPhase::Idle) {
    if (outputCode[outputPosition] == '\0') {
      outputCode = nullptr;
      outputFinished = true;
      soundOff();
      return;
    }
    // Separators are generated by buildPhraseMorse(): the normal element gap
    // has already elapsed, so add the remaining units for a character/word.
    if (outputCode[outputPosition] == '/' || outputCode[outputPosition] == '|') {
      soundOff();
      outputDeadline = now + (outputCode[outputPosition] == '/' ?
        timing.characterGap - timing.elementGap : timing.wordGap - timing.elementGap);
      outputPhase = OutputPhase::Gap;
      return;
    }
    soundOn();
    outputDeadline = now + (outputCode[outputPosition] == '.' ? timing.dit : timing.dah);
    outputPhase = OutputPhase::Tone;
  } else if (now >= outputDeadline) {
    if (outputPhase == OutputPhase::Tone) {
      soundOff();
      outputDeadline = now + timing.elementGap;
      outputPhase = OutputPhase::Gap;
    } else {
      ++outputPosition;
      outputPhase = OutputPhase::Idle;
    }
  }
}

const char *morseForCharacter(char character) {
  if (character >= 'A' && character <= 'Z') return morseCodes[character - 'A'];
  if (character >= '0' && character <= '9') return morseCodes[LETTER_COUNT + character - '0'];
  return nullptr;
}

void buildPhraseMorse(const char *phrase, char *destination, size_t destinationSize) {
  size_t used = 0;
  for (byte i = 0; phrase[i] != '\0' && used + 1 < destinationSize; ++i) {
    if (phrase[i] == ' ') {
      destination[used++] = '|';
      continue;
    }
    const char *code = morseForCharacter(phrase[i]);
    if (!code) continue;
    for (byte j = 0; code[j] != '\0' && used + 1 < destinationSize; ++j) destination[used++] = code[j];
    if (phrase[i + 1] != '\0' && phrase[i + 1] != ' ' && used + 1 < destinationSize) destination[used++] = '/';
  }
  destination[used] = '\0';
}

JoystickEvent directionFromJoystick() {
  // Citeste ambele canale o singura data, pentru ca diagnosticul si decodorul
  // sa descrie exact aceeasi pozitie a manetei.
  lastJoystickAxis1 = analogRead(PIN_JOYSTICK_AXIS_1);
  lastJoystickAxis2 = analogRead(PIN_JOYSTICK_AXIS_2);

  // A0 este axa fizica verticala in acest montaj; ea nu ajunge la adjustWpm().
  if (lastJoystickAxis1 > JOYSTICK_HIGH_THRESHOLD) return JoystickEvent::Up;
  if (lastJoystickAxis1 < JOYSTICK_LOW_THRESHOLD) return JoystickEvent::Down;
  // A1 este axa fizica orizontala. Valoarea mare este dreapta fizica.
  if (lastJoystickAxis2 < JOYSTICK_LOW_THRESHOLD) return JoystickEvent::Left;
  if (lastJoystickAxis2 > JOYSTICK_HIGH_THRESHOLD) return JoystickEvent::Right;
  return JoystickEvent::None;
}

const char *joystickEventName(JoystickEvent event) {
  switch (event) {
    case JoystickEvent::Up: return "UP";
    case JoystickEvent::Down: return "DOWN";
    case JoystickEvent::Left: return "LEFT";
    case JoystickEvent::Right: return "RIGHT";
    default: return "NONE";
  }
}

void logWpmJoystickDiagnostic(JoystickEvent event, int wpmBefore, int wpmAfter) {
  if (!JOYSTICK_DIAGNOSTICS) return;
  Serial.print(F("Joystick raw: axis1=")); Serial.print(lastJoystickAxis1);
  Serial.print(F(", axis2=")); Serial.println(lastJoystickAxis2);
  Serial.print(F("Detected physical direction: ")); Serial.println(joystickEventName(event));
  Serial.print(F("WPM before: ")); Serial.println(wpmBefore);
  Serial.print(F("WPM after: ")); Serial.println(wpmAfter);
}

JoystickEvent updateJoystick() {
  unsigned long now = millis();
  bool down = digitalRead(PIN_JOYSTICK_SW) == LOW;
  if (down != previousJoystickButtonDown && now - joystickButtonChangedAt >= JOYSTICK_DEBOUNCE_MS) {
    previousJoystickButtonDown = down;
    joystickButtonChangedAt = now;
    if (down) { joystickButtonDownAt = now; joystickLongReported = false; }
    else if (!joystickLongReported) return JoystickEvent::ShortPress;
  }
  if (down && !joystickLongReported && now - joystickButtonDownAt >= JOYSTICK_LONG_PRESS_MS) {
    joystickLongReported = true;
    return JoystickEvent::LongPress;
  }
  JoystickEvent direction = directionFromJoystick();
  if (direction != heldDirection) {
    heldDirection = direction;
    directionChangedAt = now;
    directionRepeatAt = now + JOYSTICK_INITIAL_REPEAT_MS;
    return direction;
  }
  if (direction != JoystickEvent::None && now >= directionRepeatAt) {
    directionRepeatAt = now + JOYSTICK_REPEAT_MS;
    return direction;
  }
  return JoystickEvent::None;
}

void printPadded(const char *line) {
  lcd.print(line);
  byte length = strlen(line);
  while (length++ < 16) lcd.print(' ');
}

void renderMenu() {
  if (!displayDirty) return;
  lcd.clear();
  if (appState == AppState::MainMenu) {
    const char *items[] = { "Training", "Viteza WPM", "Start Training", "Inapoi" };
    lcd.setCursor(0, 0); lcd.print('>'); lcd.print(items[menuIndex]);
    lcd.setCursor(0, 1);
    if (menuIndex == 0) { lcd.print(modeName(trainingMode)); }
    else if (menuIndex == 1) { lcd.print(currentWpm); lcd.print(" WPM"); }
    else if (menuIndex == 2) lcd.print("Buton D7 = start");
    else lcd.print("Apasa lung: sus");
  } else if (appState == AppState::TrainingSelection) {
    lcd.setCursor(0, 0); lcd.print("Training:");
    lcd.setCursor(0, 1); lcd.print('>'); lcd.print(modeName(static_cast<TrainingMode>(selectionIndex)));
  } else if (appState == AppState::WpmSettings) {
    lcd.setCursor(0, 0); lcd.print("Viteza WPM");
    lcd.setCursor(0, 1); lcd.print("< "); lcd.print(currentWpm); lcd.print(" WPM >");
  }
  displayDirty = false;
}

void startTraining() {
  appState = AppState::Training;
  trainingPhase = TrainingPhase::Choose;
  outputCode = nullptr;
  soundOff();
  displayDirty = true;
}

void adjustWpm(JoystickEvent event) {
  int requestedWpm = currentWpm;
  // Reglarea vitezei accepta exclusiv evenimentele produse de axa orizontala.
  // Up si Down nu pot modifica WPM in acest ecran.
  switch (event) {
    case JoystickEvent::Right:
      requestedWpm += WPM_STEP;
      break;
    case JoystickEvent::Left:
      requestedWpm -= WPM_STEP;
      break;
    default:
      return;
  }

  // Clamp the requested speed so a changed step size can never cross a limit.
  requestedWpm = constrain(requestedWpm, MIN_WPM, MAX_WPM);
  if (requestedWpm == currentWpm) return;

  currentWpm = requestedWpm;
  recalculateMorseTiming();
  displayDirty = true;
}

void updateMenu(JoystickEvent event) {
  if (appState == AppState::Training) return;
  if (appState == AppState::MainMenu) {
    // Meniul principal se parcurge exclusiv pe axa orizontala: dreapta pentru
    // urmatorul element si stanga pentru cel anterior. Butonul SW confirma
    // elementul afisat, astfel incat dreapta nu poate deschide accidental un
    // ecran de setari.
    if (event == JoystickEvent::Left && menuIndex > 0) { --menuIndex; displayDirty = true; }
    if (event == JoystickEvent::Right && menuIndex < MAIN_MENU_ITEM_COUNT - 1) { ++menuIndex; displayDirty = true; }
    if (event == JoystickEvent::ShortPress) {
      if (menuIndex == 0) { appState = AppState::TrainingSelection; selectionIndex = static_cast<byte>(trainingMode); displayDirty = true; }
      else if (menuIndex == 1) { appState = AppState::WpmSettings; displayDirty = true; }
      else if (menuIndex == 2) startTraining();
    }
  } else if (appState == AppState::TrainingSelection) {
    if ((event == JoystickEvent::Up || event == JoystickEvent::Left) && selectionIndex > 0) { --selectionIndex; displayDirty = true; }
    if ((event == JoystickEvent::Down || event == JoystickEvent::Right) && selectionIndex < 2) { ++selectionIndex; displayDirty = true; }
    if (event == JoystickEvent::ShortPress) { trainingMode = static_cast<TrainingMode>(selectionIndex); appState = AppState::MainMenu; displayDirty = true; }
    if (event == JoystickEvent::LongPress) { appState = AppState::MainMenu; displayDirty = true; }
  } else if (appState == AppState::WpmSettings) {
    // In setarile WPM, doar directia orizontala modifica viteza:
    // dreapta creste, iar stanga scade.
    int wpmBefore = currentWpm;
    adjustWpm(event);
    if (event == JoystickEvent::Up || event == JoystickEvent::Down ||
        event == JoystickEvent::Left || event == JoystickEvent::Right) {
      logWpmJoystickDiagnostic(event, wpmBefore, currentWpm);
    }
    if (event == JoystickEvent::ShortPress || event == JoystickEvent::LongPress) { appState = AppState::MainMenu; displayDirty = true; }
  }
}

// D7 pastreaza semantica originala: LOW inseamna apasat, tine sidetone-ul
// activ pe durata apasarii si masoara punct/linie in timpul raspunsului.
void updateExistingKeyButton() {
  unsigned long now = millis();
  bool down = digitalRead(keyPin) == LOW;
  if (down != previousExistingKeyDown && now - existingKeyChangedAt >= KEY_DEBOUNCE_MS) {
    previousExistingKeyDown = down;
    existingKeyChangedAt = now;
    if (down) {
      existingKeyDownAt = now;
      if (appState == AppState::MainMenu) startTraining();
      if (appState == AppState::Training && trainingPhase == TrainingPhase::WaitAnswer) {
        answerStarted = true;
        soundOn();
      }
    } else if (appState == AppState::Training && trainingPhase == TrainingPhase::WaitAnswer) {
      soundOff();
      unsigned long pressDuration = now - existingKeyDownAt;
      if (pressDuration >= 15) acceptAnswerSignal(pressDuration < timing.dit * 2UL ? '.' : '-');
    }
  }
}

// Straight key este independent de D7 si are sidetone imediat, cu filtrare scurta.
void updateStraightKey() {
  unsigned long now = millis();
  bool down = digitalRead(PIN_STRAIGHT_KEY) == LOW;
  if (down != previousStraightDown && now - straightChangedAt >= KEY_DEBOUNCE_MS) {
    previousStraightDown = down;
    straightChangedAt = now;
    if (appState != AppState::Training || trainingPhase != TrainingPhase::WaitAnswer) {
      if (down) soundOn(); else if (!morseOutputBusy()) soundOff();
    }
  }
}

// Keyer simplu non-iambic: la ambele padele selecteaza alternativ DIT/DAH.
// Este delimitat aici pentru extindere ulterioara cu memoria padelelor/Mode A/B.
void updatePaddle() {
  if (morseOutputBusy() || (appState == AppState::Training && trainingPhase == TrainingPhase::WaitAnswer)) return;
  bool dit = digitalRead(PIN_PADDLE_DIT) == LOW;
  bool dah = digitalRead(PIN_PADDLE_DAH) == LOW;
  if (!dit && !dah) return;
  bool sendDit = dit;
  if (dit && dah) sendDit = paddleLastWasDit ? false : true;
  else if (dah) sendDit = false;
  paddleLastWasDit = sendDit;
  startMorseOutput(sendDit ? "." : "-");
}

void chooseTrainingTarget() {
  if (trainingMode == TrainingMode::Phrases) {
    strncpy(targetText, phrases[random(PHRASE_COUNT)], sizeof(targetText));
    targetText[sizeof(targetText) - 1] = '\0';
    buildPhraseMorse(targetText, targetCode, sizeof(targetCode));
  } else {
    byte first = trainingMode == TrainingMode::Letters ? 0 : LETTER_COUNT;
    byte count = trainingMode == TrainingMode::Letters ? LETTER_COUNT : CHARACTER_COUNT - LETTER_COUNT;
    byte index = first + random(count);
    targetText[0] = characters[index]; targetText[1] = '\0';
    strncpy(targetCode, morseCodes[index], sizeof(targetCode));
  }
}

void showTrainingLine(const char *top, const char *bottom) {
  lcd.clear(); lcd.setCursor(0, 0); printPadded(top); lcd.setCursor(0, 1); printPadded(bottom);
}

// Construieste o fereastra de maximum 16 coloane din modelul Morse. Fiecare
// semnal este delimitat de spatii, iar cel care urmeaza este incadrat in [];
// pentru fraze, '/' separa literele iar '//' separa cuvintele.
void formatMorseModel(char *destination, size_t destinationSize) {
  destination[0] = '\0';
  size_t used = 0;
  byte start = expectedPosition;
  // Arata si cateva elemente deja introduse, ca utilizatorul sa poata urmari
  // progresul, fara a ascunde urmatorul element pe LCD-ul de 16 coloane.
  while (start > 0 && expectedPosition - start < 3) --start;
  for (byte i = start; targetCode[i] != '\0'; ++i) {
    char token[6];
    if (targetCode[i] == '|') strcpy(token, " // ");
    else if (targetCode[i] == '/') strcpy(token, " / ");
    else if (i == expectedPosition) {
      token[0] = '['; token[1] = targetCode[i]; token[2] = ']'; token[3] = ' '; token[4] = '\0';
    } else {
      token[0] = targetCode[i]; token[1] = ' '; token[2] = '\0';
    }
    size_t tokenLength = strlen(token);
    if (used + tokenLength >= destinationSize) break;
    strcpy(destination + used, token);
    used += tokenLength;
  }
}

void skipMorseSeparators() {
  while (targetCode[expectedPosition] == '/' || targetCode[expectedPosition] == '|') ++expectedPosition;
}

void showAnswerPrompt() {
  char targetLine[17];
  char modelLine[17];
  snprintf(targetLine, sizeof(targetLine), "T:%s", targetText);
  formatMorseModel(modelLine, sizeof(modelLine));
  showTrainingLine(targetLine, modelLine);
}

void acceptAnswerSignal(char signal) {
  skipMorseSeparators();
  answerStarted = true;
  if (targetCode[expectedPosition] == signal) {
    ++expectedPosition;
    skipMorseSeparators();
    answerHasError = false;
    if (targetCode[expectedPosition] == '\0') {
      answerCorrect = true;
      showTrainingLine("CORECT!", "Secventa completa");
      trainingPhase = TrainingPhase::Result;
      phaseStartedAt = millis();
    } else {
      showAnswerPrompt();
    }
  } else {
    // Nu avansam pozitia: acelasi punct sau linie ramane evidentiat si poate
    // fi incercat din nou.
    answerHasError = true;
    answerErrorShownAt = millis();
    showTrainingLine("GRESIT! Incearca", "din nou");
  }
}

void updateTraining(JoystickEvent event) {
  if (appState != AppState::Training) return;
  unsigned long now = millis();
  if (event == JoystickEvent::LongPress) { outputCode = nullptr; soundOff(); appState = AppState::MainMenu; displayDirty = true; return; }
  if (trainingPhase == TrainingPhase::Choose) {
    chooseTrainingTarget();
    char line[17]; snprintf(line, sizeof(line), "%s %s", targetText, targetCode);
    showTrainingLine("Asculta...", line);
    phaseStartedAt = now; trainingPhase = TrainingPhase::Preview;
  } else if (trainingPhase == TrainingPhase::Preview && now - phaseStartedAt >= 800) {
    // Frazele raman extensibile; caracterele sunt demonstrate cu codul lor Morse.
    startMorseOutput(targetCode); trainingPhase = TrainingPhase::Playing;
  } else if (trainingPhase == TrainingPhase::Playing && outputFinished) {
    outputFinished = false; phaseStartedAt = now; trainingPhase = TrainingPhase::WaitAnswer;
    expectedPosition = 0; answerStarted = false; answerHasError = false; answerStartedAt = now;
    showAnswerPrompt();
  } else if (trainingPhase == TrainingPhase::WaitAnswer) {
    if (answerHasError && now - answerErrorShownAt >= 1200) {
      answerHasError = false;
      showAnswerPrompt();
    } else if (!answerStarted && now - answerStartedAt >= answerTimeout) {
      showTrainingLine("Timp expirat", "Incearca din nou"); trainingPhase = TrainingPhase::Result; phaseStartedAt = now;
    }
  } else if (trainingPhase == TrainingPhase::Result && now - phaseStartedAt >= 3000) {
    trainingPhase = TrainingPhase::Choose;
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(buzzerPin, OUTPUT);
  pinMode(keyPin, INPUT_PULLUP);
  pinMode(PIN_JOYSTICK_SW, INPUT_PULLUP);
  pinMode(PIN_STRAIGHT_KEY, INPUT_PULLUP);
  pinMode(PIN_PADDLE_DIT, INPUT_PULLUP);
  pinMode(PIN_PADDLE_DAH, INPUT_PULLUP);
  recalculateMorseTiming();
  Wire.begin(); lcd.begin(); lcd.backlight();
  randomSeed(analogRead(A2));  // A2 ramane liber dupa alocarea joystick-ului pe A0/A1.
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("   CW Trainer");
  lcd.setCursor(0, 1); lcd.print("     YO6LPG");
  delay(2000);
  displayDirty = true;
}

void loop() {
  JoystickEvent event = updateJoystick();
  updateExistingKeyButton();
  updateStraightKey();
  updatePaddle();
  updateMenu(event);
  updateTraining(event);
  updateMorseOutput();
  renderMenu();
}

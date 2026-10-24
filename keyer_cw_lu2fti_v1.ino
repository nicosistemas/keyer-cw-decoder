/*
  KEYER IÁMBICO LU2FTI + DECODIFICADOR CW para ESP32-C3 Mini
  Por defecto: 20 WPM, Modo Iámbico B, con menú ajustado y sin saltos dobles.
*/

#include <Wire.h>
#include <U8g2lib.h>
#include <EEPROM.h>

// ==========================================
//   CONFIGURACIÓN DE PINES
// ==========================================
#define PIN_DIT         2
#define PIN_DAH         3
#define PIN_COMMAND     0
#define PIN_SIDETONE    1
#define PIN_KEY_OUT     6

#define I2C_SDA         5
#define I2C_SCL         4

// ==========================================
//   DISPLAY
// ==========================================
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// ==========================================
//   CONFIGURACIÓN (Por defecto: 20 WPM, Modo B -> false)
// ==========================================
struct KeyerSettings {
  uint8_t speedWPM;
  bool iambicModeA; // true = Modo A, false = Modo B
  bool swapPaddles;
  bool sidetoneEnabled;
  uint16_t sidetoneFreq;
};

KeyerSettings settings = {20, false, false, true, 600};

// Variables de control de menú
bool inMenu = false;
uint8_t menuOption = 0; // 0: Velocidad, 1: Modo Iámbico

// ==========================================
//   TABLA MORSE
// ==========================================
struct MorseChar {
  char character;
  uint8_t length;
  uint32_t code;
};

const MorseChar morseTable[] PROGMEM = {
  {'A',2,0b01},{'B',4,0b1000},{'C',4,0b1010},{'D',3,0b100},{'E',1,0b0},
  {'F',4,0b0010},{'G',3,0b110},{'H',4,0b0000},{'I',2,0b00},{'J',4,0b0111},
  {'K',3,0b101},{'L',4,0b0100},{'M',2,0b11},{'N',2,0b10},{'O',3,0b111},
  {'P',4,0b0110},{'Q',4,0b1101},{'R',3,0b010},{'S',3,0b000},{'T',1,0b1},
  {'U',3,0b001},{'V',4,0b0001},{'W',3,0b011},{'X',4,0b1001},{'Y',4,0b1011},
  {'Z',4,0b1100},{'0',5,0b11111},{'1',5,0b01111},{'2',5,0b00111},{'3',5,0b00011},
  {'4',5,0b00001},{'5',5,0b00000},{'6',5,0b10000},{'7',5,0b11000},{'8',5,0b11100},
  {'9',5,0b11110},{'.',6,0b010101},{',',6,0b110011},{'?',6,0b001100},{'!',6,0b101011},
  {';',6,0b101010},{':',6,0b111000},{'\'',6,0b011110},{'"',6,0b010110},{'-',6,0b100001},
  {'/',5,0b10010},{'=',5,0b10001},{'(',5,0b10110},{')',6,0b101101},{'@',6,0b110110},
  {'_',6,0b100110},{'+',5,0b01010},{'&',5,0b01000}
};

// ==========================================
//   VARIABLES
// ==========================================
#define MAX_TEXT_LENGTH 200
char textBuffer[MAX_TEXT_LENGTH];
uint16_t bufferStart = 0, bufferEnd = 0;
bool bufferFull = false;

uint32_t currentCode = 0;
uint8_t currentLength = 0;
unsigned long lastChangeTime = 0;
bool isKeyDown = false;
unsigned long keyDownStart = 0;

unsigned long lastKeyActivityTime = 0;
bool espacioAgregadoPorPausa = false;
const unsigned long PAUSA_ESPACIO_MS = 1000;

enum Element { ELEM_NONE, ELEM_DIT, ELEM_DAH };
Element currentElem = ELEM_NONE;
Element lastElemSent = ELEM_NONE;

enum KeyerState { K_IDLE, K_ELEMENT, K_SPACE };
KeyerState keyerState = K_IDLE;
unsigned long stateTimer = 0;

bool txActive = false;
bool lastBtnState = HIGH;
bool lastMenuBtnState = HIGH;
#define MAX_PANTALLA_CHARS 80
int caracteresEnPantalla = 0;

// Declaraciones previas
void updateDisplay();
void updateMenuDisplay();

unsigned long getDitDuration() { return 1200UL / settings.speedWPM; }
unsigned long getDahDuration() { return getDitDuration() * 3; }
unsigned long getInterLetterGap() { return getDitDuration() * 3; }
unsigned long getInterWordGap() { return getDitDuration() * 7; }

// ==========================================
//   LECTURA CON ANTIRREBOTE (DEBOUNCE)
// ==========================================
bool isDitPressed() {
  static bool stableState = HIGH;
  static bool lastRawState = HIGH;
  static unsigned long lastDebounceTime = 0;
  
  bool raw = digitalRead(PIN_DIT);
  if (raw != lastRawState) {
    lastDebounceTime = millis();
    lastRawState = raw;
  }
  if ((millis() - lastDebounceTime) > 10) {
    stableState = raw;
  }
  bool pressed = (stableState == LOW);
  if (pressed) { lastKeyActivityTime = millis(); espacioAgregadoPorPausa = false; }
  return pressed;
}

bool isDahPressed() {
  static bool stableState = HIGH;
  static bool lastRawState = HIGH;
  static unsigned long lastDebounceTime = 0;
  
  bool raw = digitalRead(PIN_DAH);
  if (raw != lastRawState) {
    lastDebounceTime = millis();
    lastRawState = raw;
  }
  if ((millis() - lastDebounceTime) > 10) {
    stableState = raw;
  }
  bool pressed = (stableState == LOW);
  if (pressed) { lastKeyActivityTime = millis(); espacioAgregadoPorPausa = false; }
  return pressed;
}

void addCharToBuffer(char c) {
  if (bufferFull) {
    textBuffer[bufferEnd] = c;
    bufferEnd = (bufferEnd + 1) % MAX_TEXT_LENGTH;
    bufferStart = bufferEnd;
    caracteresEnPantalla = MAX_PANTALLA_CHARS;
  } else {
    textBuffer[bufferEnd] = c;
    bufferEnd = (bufferEnd + 1) % MAX_TEXT_LENGTH;
    if (bufferEnd == bufferStart) bufferFull = true;
    caracteresEnPantalla++;
  }
  updateDisplay();
}

void addSpaceToBuffer() {
  if (bufferEnd != bufferStart || !bufferFull) addCharToBuffer(' ');
}

void clearBuffer() {
  bufferStart = 0; bufferEnd = 0; bufferFull = false;
  caracteresEnPantalla = 0;
  memset(textBuffer, 0, MAX_TEXT_LENGTH);
  updateDisplay();
}

void updateDisplay() {
  if (inMenu) return;
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_6x10_tf);
    uint16_t idx = bufferStart;
    for (int line = 0; line < 4; line++) {
      char lineBuffer[21] = {0};
      int col = 0;
      while (col < 20 && (idx != bufferEnd || bufferFull)) {
        char c = textBuffer[idx];
        if (c == '\0') c = ' ';
        lineBuffer[col++] = c;
        idx = (idx + 1) % MAX_TEXT_LENGTH;
        if (idx == bufferStart && !bufferFull) break;
      }
      u8g2.drawStr(0, 12 + (line * 12), lineBuffer);
    }
    char statusLine[25];
    sprintf(statusLine, "%dWPM %s KEYER LU2FTI", settings.speedWPM,
            settings.iambicModeA ? "IAM-A" : "IAM-B");
    u8g2.setFont(u8g2_font_5x7_tf);
    u8g2.drawStr(0, 58, statusLine);
  } while (u8g2.nextPage());
}

char decodeMorse(uint32_t code, uint8_t length) {
  int num = sizeof(morseTable) / sizeof(morseTable[0]);
  for (int i = 0; i < num; i++) {
    MorseChar mc;
    memcpy_P(&mc, &morseTable[i], sizeof(mc));
    if (mc.length == length && mc.code == code) return mc.character;
  }
  return '?';
}

void verificarPausaLarga() {
  if (!txActive && !isKeyDown) {
    if ((millis() - lastKeyActivityTime) >= PAUSA_ESPACIO_MS && !espacioAgregadoPorPausa) {
      addSpaceToBuffer();
      espacioAgregadoPorPausa = true;
    }
  } else {
    espacioAgregadoPorPausa = false;
  }
}

void verificarAutoClear() {
  if (caracteresEnPantalla >= MAX_PANTALLA_CHARS) {
    clearBuffer();
    const char* msg = ">CLEAR<";
    for (const char* p = msg; *p; p++) addCharToBuffer(*p);
    if (settings.sidetoneEnabled) {
      tone(PIN_SIDETONE, 1000); delay(60); noTone(PIN_SIDETONE);
      delay(40);
      tone(PIN_SIDETONE, 1200); delay(60); noTone(PIN_SIDETONE);
    }
  }
}

void processDecoder() {
  bool currentKeyState = digitalRead(PIN_KEY_OUT);
  unsigned long now = millis();
  
  if (currentKeyState && !isKeyDown) {
    isKeyDown = true; keyDownStart = now; lastKeyActivityTime = now;
    if (currentLength > 0 && (now - lastChangeTime) > getInterLetterGap()) {
      char c = decodeMorse(currentCode, currentLength);
      if (c != '\0') addCharToBuffer(c);
      currentCode = 0; currentLength = 0;
    }
  } else if (!currentKeyState && isKeyDown) {
    isKeyDown = false;
    unsigned long duration = now - keyDownStart;
    unsigned long ditDur = getDitDuration();
    lastKeyActivityTime = now;
    if (duration <= ditDur * 1.5) {
      currentCode = (currentCode << 1) | 0;
    } else if (duration <= ditDur * 3.5) {
      currentCode = (currentCode << 1) | 1;
    } else {
      currentCode = 0; currentLength = 0;
    }
    if (duration <= ditDur * 3.5) { currentLength++; if (currentLength > 12) currentLength = 12; }
    lastChangeTime = now;
  } else if (!isKeyDown && currentLength > 0) {
    if ((now - lastChangeTime) >= getInterLetterGap()) {
      char c = decodeMorse(currentCode, currentLength);
      if (c != '\0') addCharToBuffer(c);
      currentCode = 0; currentLength = 0; lastChangeTime = now;
    }
    if ((now - lastChangeTime) >= getInterWordGap()) {
      if (bufferEnd == 0 || textBuffer[(bufferEnd - 1 + MAX_TEXT_LENGTH) % MAX_TEXT_LENGTH] != ' ') {
        addSpaceToBuffer();
      }
      lastChangeTime = now;
    }
  }
  verificarPausaLarga();
  verificarAutoClear();
}

void startTransmit() {
  if (!txActive) {
    txActive = true;
    digitalWrite(PIN_KEY_OUT, HIGH);
    if (settings.sidetoneEnabled) tone(PIN_SIDETONE, settings.sidetoneFreq);
  }
}

void stopTransmit() {
  if (txActive) {
    txActive = false;
    digitalWrite(PIN_KEY_OUT, LOW);
    if (settings.sidetoneEnabled) noTone(PIN_SIDETONE);
  }
}

// ==========================================
//   MOTOR IAMBICO PRECISO
// ==========================================
void processKeyer() {
  unsigned long now = millis();

  bool dit_pressed = isDitPressed();
  bool dah_pressed = isDahPressed();

  if (settings.swapPaddles) {
    bool temp = dit_pressed;
    dit_pressed = dah_pressed;
    dah_pressed = temp;
  }

  static bool dit_memory = false;
  static bool dah_memory = false;

  switch (keyerState) {
    case K_IDLE:
      dit_memory = false;
      dah_memory = false;

      if (dit_pressed && dah_pressed) {
        currentElem = (lastElemSent == ELEM_DIT) ? ELEM_DAH : ELEM_DIT;
      } else if (dit_pressed) {
        currentElem = ELEM_DIT;
      } else if (dah_pressed) {
        currentElem = ELEM_DAH;
      }

      if (dit_pressed || dah_pressed) {
        lastElemSent = currentElem;
        startTransmit();
        stateTimer = now + (currentElem == ELEM_DIT ? getDitDuration() : getDahDuration());
        keyerState = K_ELEMENT;
      }
      break;

    case K_ELEMENT:
      if (dit_pressed) dit_memory = true;
      if (dah_pressed) dah_memory = true;

      if (now >= stateTimer) {
        stopTransmit();
        stateTimer = now + getDitDuration();
        keyerState = K_SPACE;
      }
      break;

    case K_SPACE:
      if (dit_pressed) dit_memory = true;
      if (dah_pressed) dah_memory = true;

      if (now >= stateTimer) {
        if (!dit_pressed) dit_memory = false;
        if (!dah_pressed) dah_memory = false;

        bool next_dit = dit_pressed || (settings.iambicModeA ? dit_pressed : dit_memory);
        bool next_dah = dah_pressed || (settings.iambicModeA ? dah_pressed : dah_memory);

        if (next_dit && next_dah) {
          currentElem = (lastElemSent == ELEM_DIT) ? ELEM_DAH : ELEM_DIT;
        } else if (next_dit) {
          currentElem = ELEM_DIT;
        } else if (next_dah) {
          currentElem = ELEM_DAH;
        }

        if (next_dit || next_dah) {
          if (currentElem == ELEM_DIT) dit_memory = false;
          else dah_memory = false;

          lastElemSent = currentElem;
          startTransmit();
          stateTimer = now + (currentElem == ELEM_DIT ? getDitDuration() : getDahDuration());
          keyerState = K_ELEMENT;
        } else {
          keyerState = K_IDLE;
        }
      }
      break;
  }
}

// ==========================================
//   GESTIÓN DEL MENÚ INTERACTIVO (Con Cooldown)
// ==========================================
void updateMenuDisplay() {
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_ncenB08_tf);
    u8g2.drawStr(0, 12, "-- MENU KEYER --");
    u8g2.setFont(u8g2_font_6x10_tf);
    
    char line1[30], line2[30];
    if (menuOption == 0) {
      sprintf(line1, "> 1. Vel: %d WPM", settings.speedWPM);
      sprintf(line2, "  2. Modo: %s", settings.iambicModeA ? "IAM-A" : "IAM-B");
    } else {
      sprintf(line1, "  1. Vel: %d WPM", settings.speedWPM);
      sprintf(line2, "> 2. Modo: %s", settings.iambicModeA ? "IAM-A" : "IAM-B");
    }
    u8g2.drawStr(0, 32, line1);
    u8g2.drawStr(0, 48, line2);
    u8g2.setFont(u8g2_font_5x7_tf);
    u8g2.drawStr(0, 62, "Punto-/Raya+/Boton:Sig");
  } while (u8g2.nextPage());
}

void processMenu() {
  static unsigned long lastMenuAction = 0;

  // Cooldown de 250ms para que cada toque en el menú mueva de a uno exactamente
  if (millis() - lastMenuAction > 250) {
    if (digitalRead(PIN_DIT) == LOW) {
      if (menuOption == 0) {
        if (settings.speedWPM > 5) settings.speedWPM--;
      } else {
        settings.iambicModeA = true;
      }
      lastMenuAction = millis();
      updateMenuDisplay();
    } else if (digitalRead(PIN_DAH) == LOW) {
      if (menuOption == 0) {
        if (settings.speedWPM < 50) settings.speedWPM++;
      } else {
        settings.iambicModeA = false;
      }
      lastMenuAction = millis();
      updateMenuDisplay();
    }
  }

  bool btnState = digitalRead(PIN_COMMAND);
  if (lastMenuBtnState == HIGH && btnState == LOW) {
    delay(30); // Antirrebote de botón
    if (digitalRead(PIN_COMMAND) == LOW) {
      unsigned long pressStart = millis();
      while (digitalRead(PIN_COMMAND) == LOW) delay(10);
      unsigned long duracion = millis() - pressStart;
      
      if (duracion < 800) {
        menuOption++;
        if (menuOption > 1) {
          EEPROM.begin(sizeof(settings) + 50);
          EEPROM.put(50, settings);
          EEPROM.commit();
          EEPROM.end();
          
          inMenu = false;
          clearBuffer();
          const char* msg = ">SAVED<";
          for (const char* p = msg; *p; p++) addCharToBuffer(*p);
          if (settings.sidetoneEnabled) {
            tone(PIN_SIDETONE, 1200); delay(100); noTone(PIN_SIDETONE);
          }
        } else {
          updateMenuDisplay();
        }
      }
    }
  }
  lastMenuBtnState = btnState;
}

void processCommand() {
  bool btnState = digitalRead(PIN_COMMAND);
  if (lastBtnState == HIGH && btnState == LOW) {
    unsigned long pressStart = millis();
    while (digitalRead(PIN_COMMAND) == LOW) delay(10);
    unsigned long duracion = millis() - pressStart;
    if (duracion < 1000) {
      clearBuffer();
      if (settings.sidetoneEnabled) { tone(PIN_SIDETONE, 800); delay(80); noTone(PIN_SIDETONE); }
    } else {
      inMenu = true;
      menuOption = 0;
      updateMenuDisplay();
      if (settings.sidetoneEnabled) { 
        tone(PIN_SIDETONE, 1000); delay(100); noTone(PIN_SIDETONE); 
      }
    }
  }
  lastBtnState = btnState;
}

void showSplash() {
  u8g2.firstPage();
  do {
    // Título principal (Fuente mediana B10)
    u8g2.setFont(u8g2_font_ncenB10_tf);
    u8g2.drawStr(22, 16, "KEYER CW");

    // Subtítulo
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(34, 31, "by LU2FTI");

    // URL separada en 2 líneas usando una fuente pequeña (5x7)
    u8g2.setFont(u8g2_font_5x7_tf);
    u8g2.drawStr(12, 47, "github.com/nicosistemas");
  } while (u8g2.nextPage());
  delay(3000);
}

void setup() {
  pinMode(PIN_DIT, INPUT_PULLUP);
  pinMode(PIN_DAH, INPUT_PULLUP);
  pinMode(PIN_COMMAND, INPUT_PULLUP);
  pinMode(PIN_SIDETONE, OUTPUT);
  pinMode(PIN_KEY_OUT, OUTPUT);
  digitalWrite(PIN_KEY_OUT, LOW);
  
  EEPROM.begin(sizeof(settings) + 50);
  EEPROM.get(50, settings);
  if (settings.speedWPM < 5 || settings.speedWPM > 50) {
    settings = {20, false, false, true, 600};
    EEPROM.put(50, settings);
    EEPROM.commit();
  }
  EEPROM.end();
  
  Wire.begin(I2C_SDA, I2C_SCL);
  u8g2.begin();
  u8g2.setContrast(255);
  
  showSplash();
  
  lastKeyActivityTime = millis();
  clearBuffer();
  const char* inicio = ">LU2FTI READY<";
  for (const char* p = inicio; *p; p++) addCharToBuffer(*p);
  
  if (settings.sidetoneEnabled) { tone(PIN_SIDETONE, 600); delay(200); noTone(PIN_SIDETONE); }
}

void loop() {
  if (inMenu) {
    processMenu();
  } else {
    processKeyer();
    processDecoder();
    processCommand();
  }
  delay(1);
}

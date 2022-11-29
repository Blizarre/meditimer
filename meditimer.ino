#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library.
// On an arduino UNO:       A4(SDA), A5(SCL)
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO:   2(SDA),  3(SCL), ...
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

int PIN_INCTIME = 3;
int PIN_RESTART = 2;
int DEBOUNCE_COUNT = 2000;

unsigned char val = 5;
unsigned int acc_addtime = 0;
unsigned int acc_restart = 0;

long int start_time = 0;
bool meditationEnded = true;

void setup() {
  Serial.begin(9600);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Don't proceed, loop forever
  }

  pinMode(PIN_INCTIME, INPUT_PULLUP);
  pinMode(PIN_RESTART, INPUT_PULLUP);

  // Clear the buffer
  display.clearDisplay();
  display.display();

  display.setTextSize(2);      // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE, SSD1306_BLACK); // Draw white text
  display.cp437(true);         // Use full 256 char 'Code Page 437' font

  start_time = seconds();
  updateScreen();

  while (true) {
    loop();
  }
}

long int seconds() {
  return millis() / 1000;
}

void updateScreen() {
  display.clearDisplay();
  display.setCursor(0, 0);     // Start at top-left corner
  display.write('0' + (val / 100) % 10);
  display.write('0' + (val / 10 ) % 10) ;
  display.write('0' + (val      ) % 10);

  display.setCursor(50, 0);
  if (meditationEnded) {
    display.write("END  ");
  } else {
    display.write("START");
  }

  display.display();
}

void flashScreen() {
    display.fillRect(0, 0, display.width(), display.height(), SSD1306_WHITE);
    display.display();

    delay(300);

    display.clearDisplay();
    display.display();

    delay(200);

    display.fillRect(0, 0, display.width(), display.height(), SSD1306_WHITE);
    display.display();
    delay(300);

    display.clearDisplay();
    display.display();
}

void startOfMeditation() {
  if (meditationEnded) {
    meditationEnded = false;
    updateScreen();
  }
}

void endOfMeditation() {
  if (! meditationEnded) {
    meditationEnded = true;
    flashScreen();
    updateScreen();
  }
}

bool debounce(uint8_t pin, unsigned int &acc) {
  if (digitalRead(pin)) {
    if (acc == DEBOUNCE_COUNT) {
      acc ++;
      return true;
    }
    if (acc < DEBOUNCE_COUNT) {
      acc ++;
    }
  } else {
    acc = 0;
  }
  return false;
}

void loop() {
  int newval = val;
  if (debounce(PIN_INCTIME, acc_addtime)) {
      newval += 5;
  }

  if(debounce(PIN_RESTART, acc_restart)) {
      start_time = seconds() / 1000;
  }

  // Rollover after 45 minutes
  if (newval > 45) {
    newval = 5;
  }

  if (newval != val) {
    val = newval;
    updateScreen();
  }

  if (seconds() > start_time + ((long int)val) * 60) {
    endOfMeditation();
  } else {
    startOfMeditation();
  }

}

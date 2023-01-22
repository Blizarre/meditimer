#include "TM1637Display.h"

int PIN_INCTIME = 3;
int PIN_RESTART = 2;
unsigned int DEBOUNCE_COUNT = 2000;

unsigned char timer_duration = 5;
unsigned int acc_addtime = 0;
unsigned int acc_restart = 0;

typedef unsigned long int time;

time start_time = 0;
bool meditationEnded = true;


class SingleLED {
  u8 pwm_value;
  u8 pin_negative;
  u8 pin_positive;
  bool current_value;

  public:

  SingleLED(u8 pwm_value, u8 pin_positive, u8 pin_negative) {
    this->pwm_value = pwm_value;
    this->pin_negative = pin_negative;
    this->pin_positive = pin_positive;
    this->current_value = false;
  }

  void setup() {
    pinMode(this->pin_positive, OUTPUT);
    pinMode(this->pin_negative, OUTPUT);
    digitalWrite(this->pin_positive, 0);
    digitalWrite(this->pin_negative, 0);
  }

  void on() {
    if (this->current_value == false) {
      Serial.println(F("[SingleLED] Turning on"));
      analogWrite(this->pin_positive, this->pwm_value);
      this->current_value = true;
    }
  }

  void off() {
    if (this->current_value == true) {
      Serial.println(F("[SingleLED] Turning off"));
      digitalWrite(this->pin_positive, 0);
      this->current_value = false;
    }
  }
};


class DisplayNumbers {
private:
  enum DisplayType {
    None,
    Value,
    Done,
  };

  const uint8_t SEG_DONE[4] = {
    SEG_B | SEG_C | SEG_D | SEG_E | SEG_G,           // d
    SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,   // O
    SEG_C | SEG_E | SEG_G,                           // n
    SEG_A | SEG_D | SEG_E | SEG_F | SEG_G            // E
    };

  int value;
  TM1637Display* display;
  DisplayType type;

  void refresh() {
    if (this->display == nullptr) {
      Serial.println(F("[DisplayNumbers] Display unavailable, ignoring refresh"));
      return;
    }

    switch(this->type) {
      case None:
        this->display->clear();
        break;
      case Done:
        this->display->setSegments(SEG_DONE);
        break;
      case Value:
        display->showNumberDec(this->value);
        break;
      default:
        Serial.println(F("ERROR: DisplayNumbers] Unknown display type"));
        Serial.println(this->type);
    }
  }
public:

  DisplayNumbers(u8 dio_pin, u8 clk_pin) {
    this->display = new TM1637Display(clk_pin, dio_pin);
    this->value = 0;
    this->type = None;
  }

  ~DisplayNumbers() {
    delete(this->display);
  }

  void setup() {
    Serial.println(F("[DisplayNumbers] Setup"));
    this->display->setBrightness(0);
    refresh();
  }

  void displayValue(int value) {
    if (this->type != Value || this->value != value) {
      Serial.println(F("[DisplayNumbers] -> Value"));
      Serial.println(value);
      this->type = Value;
      this->value = value;
      this->refresh();
    }
  }

  void displayDone() {
    if (this->type != Done) {
      Serial.println(F("[DisplayNumbers] -> DONE"));
      this->type = Done;
      this->refresh();
    }
  }

  void displayNothing() {
    if (this->type != None) {
      Serial.println(F("[DisplayNumbers] -> None"));
      this->type = None;
      this->refresh();
    }
  }

};


SingleLED endOfMeditationLed = SingleLED(8, 5, 6);
DisplayNumbers numbers = DisplayNumbers(8, 9);


void setup() {
  Serial.begin(115200);
  Serial.println(F("Meditimer Startup"));

  pinMode(PIN_INCTIME, INPUT_PULLUP);
  pinMode(PIN_RESTART, INPUT_PULLUP);

  endOfMeditationLed.setup();
  numbers.setup();

  start_time = seconds();

  Serial.println(F("Meditimer Loop"));
  while (true) {
    loop();
  }
}

time seconds() {
  return millis() / 1000;
}

void refreshScreen() {
  if (meditationEnded) {
    endOfMeditationLed.on();
    numbers.displayDone();
  } else {
    endOfMeditationLed.off();
    numbers.displayValue(timer_duration);
  }
}

void flashScreen() {
  endOfMeditationLed.off();
  numbers.displayNothing();
  delay(100);
  endOfMeditationLed.on();
  numbers.displayValue(8888);
  delay(300);
  numbers.displayNothing();
  endOfMeditationLed.off();
  refreshScreen();
}

void startOfMeditation() {
  if (meditationEnded) {
    meditationEnded = false;
  }
}

void endOfMeditation() {
  if (! meditationEnded) {
    meditationEnded = true;
    flashScreen();
  }
}

bool debounce(uint8_t pin, unsigned int &acc) {
  if (!digitalRead(pin)) {
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
  if (debounce(PIN_INCTIME, acc_addtime)) {
      Serial.println("[Loop] Increase time");
      timer_duration += 5;
  }

  if(debounce(PIN_RESTART, acc_restart)) {
      Serial.println("[Loop] Restart timer");
      start_time = seconds();
      flashScreen();
  }

  // Rollover after 45 minutes
  if (timer_duration > 45ul) {
    timer_duration = 5;
  }

  if (seconds() > start_time + ((time)timer_duration) * 60ul) {
    endOfMeditation();
  } else {
    startOfMeditation();
  }

  refreshScreen();

}

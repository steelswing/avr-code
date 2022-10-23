#include "Arduino.h" // инклюд тонны говна
#include "pins_arduino.h"
#include "Button.h"

#define BUT_ADD 3
#define BUT_MENU 4
#define BUT_REDUCE 5

#define SEG_A 25
#define SEG_B 24
#define SEG_C 23
#define SEG_D 22
#define SEG_E 21
#define SEG_F 20
#define SEG_G 19

#define SEG_ID_1 18
#define SEG_ID_2 17
#define SEG_ID_3 16
#define SEG_ID_4 15

#define MAX_SEG_ID 4

#define MODE_DEFAULT 0
#define MODE_EDIT_HOURS 1
#define MODE_EDIT_MINS 2
#define MAX_MODE MODE_EDIT_MINS

void resetMode();
void show(byte seg, byte mask);
void checkTime();
void updateTime();
void onAddClicked(uint8_t state);
void onMenuClicked(uint8_t state);
void onReduceClicked(uint8_t state);

const byte segIds[MAX_SEG_ID] = {
    SEG_ID_1, SEG_ID_2, SEG_ID_3, SEG_ID_4,
};

const byte numberSegments[10] = {
    0b1111110, // 0
    0b0110000, // 1
    0b1101101, // 2
    0b1111001, // 3
    0b0110011, // 4
    0b1011011, // 5
    0b1011111, // 6
    0b1110000, // 7
    0b1111111, // 8
    0b1111011, // 9
};

Button* buttonAdd;
Button* buttonMenu;
Button* buttonReduce;

uint8_t currentMode = MODE_DEFAULT;

bool blink = true;
uint64_t timer3 = 0;
uint64_t timer2 = 0;
uint64_t timer1 = 0;
uint64_t time = 0;
uint8_t id = 0;

int hours = 00;
int mins = 00;
int seconds = 0;
int autoresetTimer = 0;


bool stopTick = false;

void setup() {
    for (int i = 0; i < 8; i++) {
        pinMode(SEG_G + i, OUTPUT);
    }
    for (int i = 0; i < 4; i++) {
        pinMode(SEG_ID_4 + i, OUTPUT);
    }
    buttonAdd = new Button(BUT_ADD, &onAddClicked);
    buttonMenu = new Button(BUT_MENU, &onMenuClicked);
    buttonReduce = new Button(BUT_REDUCE, &onReduceClicked);
}

void onAddClicked(uint8_t state) {
    if (currentMode == MODE_DEFAULT) {
        return;
    }
    stopTick = true;
    if (currentMode == MODE_EDIT_HOURS) {
        seconds = 0;
        if (state == BUTTON_STATE_CLICK) {
            hours++;
        }
        if (state == BUTTON_STATE_DOWN) {
            hours += 10;
        }
    }
    if (currentMode == MODE_EDIT_MINS) {
        seconds = 0;
        if (state == BUTTON_STATE_CLICK) {
            mins++;
        }
        if (state == BUTTON_STATE_DOWN) {
            mins += 10;
        }
    }
    checkTime();

    autoresetTimer = 5;
}

void onReduceClicked(uint8_t state) {
    if (currentMode == MODE_DEFAULT) {
        return;
    }
    stopTick = true;

    if (currentMode == MODE_EDIT_HOURS) {
        seconds = 0;
        if (state == BUTTON_STATE_CLICK) {
            hours--;
        }
        if (state == BUTTON_STATE_DOWN) {
            hours -= 10;
        }
    }
    if (currentMode == MODE_EDIT_MINS) {
        seconds = 0;
        if (state == BUTTON_STATE_CLICK) {
            mins--;
        }
        if (state == BUTTON_STATE_DOWN) {
            mins -= 10;
        }
    }
    checkTime();
    autoresetTimer = 5;
}

void onMenuClicked(uint8_t state) {
    if (state == BUTTON_STATE_CLICK) {
        currentMode++;
        autoresetTimer = 5;
        if (currentMode > MAX_MODE) {
            currentMode = MODE_DEFAULT;
        }
        if (currentMode == MODE_DEFAULT) {
            resetMode();
        }
    }
}

void updateTime() {
    if (millis() - time >= 1000) {
        time = millis();
        seconds++;
        checkTime();
    }
}

void checkTime() {
    if (seconds >= 60) {
        seconds = 0;
        mins++;
    }
    if (mins >= 60) {
        mins = 0;
        hours++;
    }
    if (hours >= 24) {
        hours = 0;
    }

    if (seconds < 0) {
        seconds = 0;
    }
    if (mins < 0) {
        mins = 0;
    }
    if (hours < 0) {
        hours = 0;
    }
}

void loop() {
    if (!stopTick) {
        updateTime();
    }
    if (currentMode != MODE_DEFAULT) {
        if (millis() - timer2 > 1000) {
            timer2 = millis();
            if (autoresetTimer > 0) {
                autoresetTimer--;
            } else {
                resetMode();
            }
        }
    }
    // turn off segments
    for (int i = 0; i < 8; i++) {
        digitalWrite(i + SEG_G, 0);
    }
    buttonAdd->update();
    buttonMenu->update();
    buttonReduce->update();

    byte mask1 = numberSegments[mins / 10];
    byte mask2 = numberSegments[mins % 10];

    byte mask3 = numberSegments[hours / 10];
    byte mask4 = numberSegments[hours % 10];

    if (currentMode != MODE_DEFAULT) {
        if (millis() - timer3 >= 100) {
            timer3 = millis();
            blink = !blink;
        }
        if (blink) {
            if (currentMode == MODE_EDIT_MINS) {
                mask1 = 0b0000000;
                mask2 = 0b0000000;
            }
            if (currentMode == MODE_EDIT_HOURS) {
                mask3 = 0b0000000;
                mask4 = 0b0000000;
            }
        }
    }


    // dynamic indication
    if (micros() - timer1 >= 3111) {
        timer1 = micros();
        id++;
        if (id > 3) {
            id = 0;
        }
    }
    if (id == 0) {
        show(SEG_ID_1, mask2);
    } else if (id == 1) {
        show(SEG_ID_2, mask1);
    } else if (id == 2) {
        show(SEG_ID_3, mask4);
    } else if (id == 3) {
        show(SEG_ID_4, mask3);
    }
}

void resetMode() {
    stopTick = false;
    currentMode = MODE_DEFAULT;
}

void show(byte seg, byte mask) {
    for (int i = 0; i < MAX_SEG_ID; i++) {
        digitalWrite(segIds[i], LOW);
    }
    digitalWrite(seg, HIGH);
    for (int i = 0; i < 8; i++) {
        digitalWrite(i + SEG_G, bitRead(mask, i));
    }
}

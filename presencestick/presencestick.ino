// https://github.com/adafruit/Adafruit_NeoPixel
#include <Adafruit_NeoPixel.h>
// https://github.com/thomasfredericks/Metro-Arduino-Wiring
#include <Metro.h>

#define LEDPIN 1
#define SW1PIN 4 // start rainbow mode on switch pressed
#define SW2PIN 0 // off switch for color timer

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(1, LEDPIN, NEO_RGB + NEO_KHZ800);

const uint32_t HANDCLAP_INTERVAL = 300;
const uint8_t RAINBOW_WAIT = 6;

Metro handclapBlink = Metro(HANDCLAP_INTERVAL);
Metro rainbowWait = Metro(RAINBOW_WAIT);
Metro colorTimerBlink = Metro(2000);
Metro cmdBlink = Metro(500);
// colorTimer満了後、赤点灯状態で、時々赤点滅して注意をうながす
Metro reminderWait = Metro(4000);
Metro reminderBlink = Metro(50);

enum mode_t {
    MODE_NONE,
    MODE_BLINKING,
    MODE_HANDCLAP,
    MODE_RAINBOW,
    MODE_COLORTIMER,
    MODE_REMINDERWAIT,
    MODE_REMINDERBLINK,
};
mode_t mode = MODE_NONE;
uint8_t ledState = 0;
uint32_t currentColor = 0;
uint32_t now;

uint16_t rainbowidx = 0;

const uint32_t HANDCLAPMS = 3000;
const int SWON = LOW;
const int SWOFF = HIGH;
uint32_t handclaptm = 0;
uint32_t prevColor = 0;
mode_t prevMode = MODE_NONE;

// color timer
uint32_t colortimertm = 0;
uint8_t colortimerstate = 0;
#if 1
const uint32_t COLORTIMERMS_END = 15 * 60 * 1000L; // 15[min] in [ms]
const uint32_t COLORTIMERMS_RED =  5 * 60 * 1000L;
#else
const uint32_t COLORTIMERMS_END = 5 * 60 * 1000L; // 5[min] in [ms]
const uint32_t COLORTIMERMS_RED = 1 * 60 * 1000L;
#endif

// reminder blink
const uint32_t REMINDER_DUR = 500;
uint32_t remindertm = 0;

void colorAll(uint32_t c) {
    for (uint16_t i = 0; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, c);
    }
    strip.show();
    ledState = (c != 0);
}

void setColor(uint8_t r, uint8_t g, uint8_t b)
{
    currentColor = strip.Color(r, g, b);
    colorAll(currentColor);
}

void offLed(void)
{
    mode = MODE_NONE;
    setColor(0, 0, 0);
}

enum {
    PARSER_DELAY = 1,
    PARSER_RED,
    PARSER_GREEN,
    PARSER_BLUE,
    PARSER_END,
};

void parseMessage(char letter)
{
    static uint8_t current_token;
    static uint32_t data;
    static uint8_t r;
    static uint8_t g;
    static uint8_t b;

    switch (letter) {
    case 'n': // set RGB color immediately 'n'r,g,b. ex: n255,0,0.
        data = 0;
        r = g = b = 0;
        current_token = PARSER_RED;
        break;
    case 'i': // blink 'i't. ex: i300.
        data = 0;
        current_token = PARSER_DELAY;
        break;
    case 'h': // handclap
        beginHandclap();
        break;
    case 'a': // rainbow
        beginRainbow();
        break;
    case 't': // colortimer
        beginColorTimer();
        break;
    case 'e': // reminder wait
        beginReminderWait();
        break;
    case 'j': // blink off 'j'
        mode = MODE_NONE;
        colorAll(currentColor);
        break;
    case 'o': // off
        offLed();
        break;
    case 'w': // white
        setColor(255, 255, 255);
        break;
    case 'y': // yellow
        setColor(255, 255, 0);
        break;
    case 'c': // cyan
        setColor(0, 255, 255);
        break;
    case 'm': // magenta
        setColor(255, 0, 255);
        break;
    case 'r': // red
        setColor(255, 0, 0);
        break;
    case 'g': // green
        setColor(0, 255, 0);
        break;
    case 'b': // blue
        setColor(0, 0, 255);
        break;
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        if (current_token == PARSER_RED
                || current_token == PARSER_GREEN
                || current_token == PARSER_BLUE
                || current_token == PARSER_DELAY) {
            data *= 10;
            data += (letter - '0');
        }
        break;
    case ',':
        switch (current_token) {
        case PARSER_RED:
            r = data;
            current_token = PARSER_GREEN;
            break;
        case PARSER_GREEN:
            g = data;
            current_token = PARSER_BLUE;
            break;
        default:
            break;
        }
        data = 0;
        break;
    case '.':
    default:
        switch (current_token) {
        case PARSER_DELAY:
            if (data > 0) {
                cmdBlink.interval(data);
            }
            cmdBlink.reset();
            mode = MODE_BLINKING;
            break;
        case PARSER_BLUE:
            b = data;
            setColor(r, g, b);
            break;
        default:
            break;
        }
        current_token = PARSER_END;
        data = 0;
        r = g = b = 0;
        break;
    }
}

void setup()
{
    Serial.begin(115200);
    strip.begin();
    strip.show(); // Initialize all pixels to 'off'
    pinMode(SW1PIN, INPUT_PULLUP);
    pinMode(SW2PIN, INPUT_PULLUP);
    Mouse.begin();
}

void beginHandclap(void)
{
    handclaptm = now; // extend handclapLoop()
    if (mode != MODE_HANDCLAP) {
        prevMode = mode;
        mode = MODE_HANDCLAP;
        handclapBlink.reset();
        prevColor = currentColor;
        setColor(0, 0, 255);
    }
}

void endHandclap(void)
{
    currentColor = prevColor;
    mode = prevMode;
    colorAll(currentColor);
}

void handclapLoop(void)
{
    if (now - handclaptm > HANDCLAPMS) {
        endHandclap();
        ledModeLoop();
        return;
    }
    if (handclapBlink.check()) {
        if (ledState == 0) {
            colorAll(currentColor);
        } else {
            colorAll(0);
        }
    }
}

void beginRainbow(void)
{
    mode = MODE_RAINBOW;
    rainbowWait.reset();
}

// from Adafruit_NeoPixel/examples/strandtest/strandtest.ino
// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos)
{
    if (WheelPos < 85) {
        return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
    } else if (WheelPos < 170) {
        WheelPos -= 85;
        return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
    } else {
        WheelPos -= 170;
        return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
    }
}

void rainbowLoop(void)
{
    if (rainbowWait.check()) {
        rainbowidx = (rainbowidx >= 255) ? 0 : ++rainbowidx;
        colorAll(Wheel(rainbowidx & 255));
    }
}

void beginColorTimer(void)
{
    if (mode != MODE_COLORTIMER) {
        colortimertm = now;
        colortimerstate = 0;
        mode = MODE_COLORTIMER;
        colorTimerBlink.reset();
        //setColor(255, 255, 0); // yellow
    }
}

void colorTimerLoop(void)
{
    static const int BLINK_MS_BEGIN = 2000;
    static const int BLINK_MS_END = 10;
    // on:off = 2000ms:2000ms@(lefttm=END) -> 10ms:10ms@(lefttm=0)
    // y1=ax1+b, y2=ax2+b
    // x1=BLINK_MS_BEGIN, x2=0, y1=BLINK_MS_BEGIN, y2=BLINK_MS_END
    static const float B = BLINK_MS_END;
    static const float A = (float)(BLINK_MS_BEGIN - BLINK_MS_END)/COLORTIMERMS_END;
    static uint32_t oncolor, offcolor;
    uint32_t spent = now - colortimertm;
    uint32_t lefttm = COLORTIMERMS_END - spent;
    if (spent >= COLORTIMERMS_END) { // end color timer mode -> keep red on
        //setColor(255, 0, 0);
        setColor(20, 0, 0);
        beginReminderWait();
        return;
    } else if (lefttm <= COLORTIMERMS_RED) {
        //oncolor = strip.Color(255, 0, 0);
        oncolor = strip.Color(20, 0, 0);
        //offcolor = strip.Color(255, 255, 0); // yellow
        offcolor = strip.Color(20, 20, 0);
    } else {
        //oncolor = strip.Color(255, 255, 0); // yellow
        oncolor = strip.Color(20, 20, 0);
        offcolor = 0;
    }
    if (colorTimerBlink.check()) {
        uint32_t interval = lefttm * A + B;
        /*
        Serial.print(lefttm);
        Serial.print(", ");
        Serial.println(interval);
        */
        if (colortimerstate == 0) {
            colortimerstate = 1;
            colorAll(oncolor);
        } else {
            colortimerstate = 0;
            colorAll(offcolor);
        }
        colorTimerBlink.interval(interval);
    }
}

void beginReminderWait(void)
{
    mode = MODE_REMINDERWAIT;
    reminderWait.reset();
}

void reminderWaitLoop(void)
{
    if (reminderWait.check()) {
        beginReminderBlink();
    }
}

void beginReminderBlink(void)
{
    remindertm = now;
    mode = MODE_REMINDERBLINK;
    reminderBlink.reset();
}

void reminderBlinkLoop(void)
{
    if (now - remindertm > REMINDER_DUR) {
        endReminderBlink();
        ledModeLoop();
        return;
    }
    if (reminderBlink.check()) {
        if (ledState == 0) {
            colorAll(currentColor);
        } else {
            colorAll(0);
        }
    }
}

void endReminderBlink(void)
{
    colorAll(currentColor);
    beginReminderWait();
}

static void ledBlinkLoop(void)
{
    // blink LED
    if (cmdBlink.check()) {
        if (ledState == 0) {
            colorAll(currentColor);
        } else {
            colorAll(0);
        }
    }
}

static void mouseLoop()
{
    // move mouse to avoid screen saver
    static uint32_t mprevms = 0;
    const uint32_t PCLOCKMS = 540000; // 9 [min]
    if (now - mprevms > PCLOCKMS) {
        mprevms = now;
        Mouse.move(1, 0, 0);
        //Mouse.end();
        //Serial.write("M");
    }
}

void ledModeLoop()
{
    switch (mode) {
    case MODE_COLORTIMER:
        colorTimerLoop();
        break;
    case MODE_HANDCLAP:
        handclapLoop();
        break;
    case MODE_RAINBOW:
        rainbowLoop();
        break;
    case MODE_BLINKING:
        ledBlinkLoop();
        break;
    case MODE_REMINDERWAIT:
        reminderWaitLoop();
        break;
    case MODE_REMINDERBLINK:
        reminderBlinkLoop();
    case MODE_NONE:
    default:
        break;
    }
}

void loop()
{
    now = millis();
    ledModeLoop();
    mouseLoop();
    if (digitalRead(SW2PIN) == SWON) {
        offLed();
    }
    if (digitalRead(SW1PIN) == SWON) {
        beginRainbow();
    }
    while (Serial.available()) {
        char letter = Serial.read();
        parseMessage(letter);
    }
}

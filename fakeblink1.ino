#include <Adafruit_NeoPixel.h>
#include <Metro.h>

#define LEDPIN 7

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(1, LEDPIN, NEO_GRB + NEO_KHZ800);

const uint32_t HANDCLAP_INTERVAL = 300;

Metro handclapBlink = Metro(HANDCLAP_INTERVAL);
Metro colorTimerBlink = Metro(3000);
Metro cmdBlink = Metro(500);

enum mode_t {
    MODE_NONE,
    MODE_BLINKING,
    MODE_HANDCLAP,
    MODE_COLORTIMER,
};
mode_t mode = MODE_NONE;
uint8_t ledState = 0;
uint32_t currentColor = 0;
uint32_t now;

// blink LED in a duration after switch pressed
#define SWPIN 4
const uint32_t HANDCLAPMS = 3000;
const int SWON = LOW;
const int SWOFF = HIGH;
uint32_t handclaptm = 0;
uint32_t prevColor = 0;
mode_t prevMode = MODE_NONE;

// color timer
uint32_t colortimertm = 0;
uint8_t colortimerstate = 0;
const uint32_t COLORTIMERMS_END = 15 * 60 * 1000L; // 15[min] in [ms]
const uint32_t COLORTIMERMS_RED =  5 * 60 * 1000L;

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
    case 'n': // set RGB color immediately 'n'r,g,b
        data = 0;
        r = g = b = 0;
        current_token = PARSER_RED;
        break;
    case 'i': // blink 'i't
        data = 0;
        current_token = PARSER_DELAY;
        break;
    case 'h': // handclap
        beginHandclap();
        break;
    case 't': // colortimer
        beginColorTimer();
        break;
    case 'j': // blink off 'j'
        mode = MODE_NONE;
        colorAll(currentColor);
        break;
    case 'o': // off
        mode = MODE_NONE;
        setColor(0, 0, 0);
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
    pinMode(SWPIN, INPUT_PULLUP);
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
        if (mode == MODE_BLINKING) {
            ledBlinkLoop();
        }
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
    // on:off = 0ms:1000ms(0:1)@lefttm~END -> 1000ms:0ms(1:0)@lefttm~RED
    // y=ax+b, 0=900000a+b, 1000=300000a+b -> b=1000-300000a,600000a=-1000
    // a=-1000/600000,b=1000-300000a
    static const float A1ON = -1000.0/600000;
    static const float B1ON = 1000 - 300000 * A1ON;
    // 1000=900000a+b, 0=300000a+b -> b=-300000a,600000a=1000
    static const float A1OFF = 1000.0/600000;
    static const float B1OFF = -300000 * A1OFF;
    // on:off = 1ms:1000ms@lefttm~RED -> 1000ms:0ms(1:0)@lefttm~0
    // 1=300000a+b, 1000=0a+b -> b=1000,a=-999/300000
    static const float A2ON = -999.0/300000;
    static const float B2ON = 1000;
    // 1000=300000a+b, 0=0a+b -> b=0,a=1000/300000
    static const float A2OFF = 1000.0/300000;
    static const float B2OFF = 0;
    static uint32_t oncolor, offcolor;
    static uint32_t oninterval, offinterval;
    uint32_t spent = now - colortimertm;
    uint32_t lefttm = COLORTIMERMS_END - spent;
    if (spent >= COLORTIMERMS_END) { // end color timer mode -> keep red on
        setColor(255, 0, 0);
        mode = MODE_NONE;
        return;
    } else if (lefttm <= COLORTIMERMS_RED) {
	colortimerstate = !ledState; // now, yellow indicates 'off'
        oncolor = strip.Color(255, 0, 0);
        offcolor = strip.Color(255, 255, 0); // yellow
        oninterval  = lefttm * A2ON  + B2ON;
        offinterval = lefttm * A2OFF + B2OFF;
    } else {
        oncolor = strip.Color(255, 255, 0); // yellow
        offcolor = 0;
        oninterval  = lefttm * A1ON  + B1ON;
        offinterval = lefttm * A1OFF + B1OFF;
    }
    if (colorTimerBlink.check()) {
        Serial.print(lefttm); // 897000
        Serial.print(", on ");
        Serial.print(oninterval);
        Serial.print(", off ");
        Serial.println(offinterval);
        if (colortimerstate == 0) {
	    colortimerstate = 1;
            colorAll(oncolor);
            colorTimerBlink.interval(oninterval);
        } else {
	    colortimerstate = 0;
            colorAll(offcolor);
            colorTimerBlink.interval(offinterval);
        }
    }
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

void loop()
{
    now = millis();
    switch (mode) {
    case MODE_COLORTIMER:
        colorTimerLoop();
        break;
    case MODE_HANDCLAP:
        handclapLoop();
        break;
    case MODE_BLINKING:
        ledBlinkLoop();
        break;
    case MODE_NONE:
    default:
        break;
    }
    mouseLoop();
    int val = digitalRead(SWPIN);
    if (val == SWON) {
        beginHandclap();
    }
    while (Serial.available()) {
        char letter = Serial.read();
        parseMessage(letter);
    }
}

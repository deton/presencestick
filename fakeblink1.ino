#include <Adafruit_NeoPixel.h>

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

uint32_t now;
uint8_t isBlinking = 0;
uint8_t ledState = 0;
uint32_t currentColor = 0;
uint32_t interval = 500;

// blink LED in a duration after switch pressed
#define SWPIN 4
const uint32_t SWEFFECTMS = 3000;
const int SWON = LOW;
const int SWOFF = HIGH;
uint8_t isSwEffect = 0;
uint32_t swprevms = 0;
uint32_t prevColor = 0;
uint8_t prevBlinking = 0;
uint32_t prevInterval = 0;

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
        interval = HANDCLAP_INTERVAL;
        isBlinking = 1;
        break;
    case 'j': // blink off 'j'
        isBlinking = 0;
        colorAll(currentColor);
        break;
    case 'o': // off
        isBlinking = 0;
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
                interval = data;
            }
            isBlinking = 1;
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

static void ledLoop(void)
{
    static uint32_t prevms = 0;

    if (isSwEffect && now - swprevms > SWEFFECTMS) {
        isBlinking = prevBlinking;
        currentColor = prevColor;
        interval = prevInterval;
        colorAll(currentColor);
        isSwEffect = 0;
    }

    // blink LED
    if (isBlinking && now - prevms > interval) {
        prevms = now;
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
    ledLoop();
    mouseLoop();
    int val = digitalRead(SWPIN);
    if (!isSwEffect && val == SWON) {
        prevColor = currentColor;
        prevBlinking = isBlinking;
        prevInterval = interval;
        interval = HANDCLAP_INTERVAL;
        isBlinking = 1;
        setColor(0, 0, 255);
        swprevms = now;
        isSwEffect = 1;
    }
    while (Serial.available()) {
        char letter = Serial.read();
        parseMessage(letter);
    }
}

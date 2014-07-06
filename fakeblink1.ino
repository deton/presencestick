#include <Adafruit_NeoPixel.h>

#define PIN 7

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(1, PIN, NEO_GRB + NEO_KHZ800);

uint8_t isBlinking = 0;
uint32_t currentColor = 0;
uint32_t interval = 500;

void colorAll(uint32_t c) {
    for (uint16_t i = 0; i < strip.numPixels(); i++) {
	strip.setPixelColor(i, c);
    }
    strip.show();
}

void handleMessage(uint8_t cmd, uint32_t delay, uint8_t r, uint8_t g, uint8_t b)
{
    currentColor = strip.Color(r, g, b);
    switch (cmd) {
    case 'n': // set RGB color immediately
	isBlinking = 0;
	colorAll(currentColor);
	break;
    case 'b': // blink
	isBlinking = 1;
	interval = delay;
	colorAll(currentColor);
	break;
    default:
	break;
    }
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
    static uint8_t cmd;
    static uint32_t t;
    static uint8_t r;
    static uint8_t g;
    static uint8_t b;

    switch (letter) {
    case 'n': // set RGB color immediately 'n'r,g,b.
	cmd = letter;
	t = data = 0;
	r = g = b = 0;
	current_token = PARSER_RED;
	break;
    case 'b': // blink RGB color 'b't,r,g,b.
	cmd = letter;
	t = data = 0;
	r = g = b = 0;
	current_token = PARSER_DELAY;
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
	case PARSER_DELAY:
	    t = data;
	    current_token = PARSER_RED;
	    break;
	case PARSER_RED:
	    r = data;
	    current_token = PARSER_GREEN;
	    break;
	case PARSER_GREEN:
	    g = data;
	    current_token = PARSER_BLUE;
	    break;
	case PARSER_BLUE:
	    b = data;
	    current_token = PARSER_END;
	    break;
	default:
	    break;
	}
	data = 0;
	break;
    case '.':
	if (current_token == PARSER_BLUE) {
	    b = data;
	}
	handleMessage(cmd, t, r, g, b);
	break;
    default:
	break;
    }
}

void setup()
{
    Serial.begin(115200);
    strip.begin();
    strip.show(); // Initialize all pixels to 'off'
}

static void ledLoop(void)
{
    static uint8_t ledState = 0;
    static uint32_t prevms = 0;
    uint32_t now = millis();
    if (isBlinking && now - prevms > interval) {
	prevms = now;
	if (ledState == 0) {
	    colorAll(currentColor);
	    ledState = 1;
	} else {
	    colorAll(0);
	    ledState = 0;
	}
    }
}

void loop()
{
    ledLoop();
    while (Serial.available()) {
	char letter = Serial.read();
	parseMessage(letter);
    }
}

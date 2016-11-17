/*
 *
 * This example is configured for a Atmega32 at 16MHz
 */ 

#include <util/delay.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "light_ws2812.h"

#define MAXPIX 50
#define COLORLENGTH MAXPIX/2
#define FADE 256/COLORLENGTH
#define RAINBOW_COLORS 12
#define INITIAL_COLORS 8
#define COLORS MAXPIX

#define SUB_STEPS_BITS 8
#define CALC_PRECISION uint32_t
#define OFFSET_PRECISION uint16_t

struct cRGB rainbowColors[RAINBOW_COLORS];
struct cRGB initialColors[INITIAL_COLORS];
struct cRGB colors[COLORS];
struct cRGB led[MAXPIX];

#define DDR_BTN 	DDRD
#define PIN_BTN 	PIND
#define PORT_BTN 	PORTD
#define BTN_1 	(1 << 5)
#define BTN_2 	(1 << 6)

#define STYLES 	4



static OFFSET_PRECISION offset = 0;
static OFFSET_PRECISION offsetLength = 0xFF;
static OFFSET_PRECISION speed = 16;

static uint8_t step;
static uint8_t prevStep;
static OFFSET_PRECISION c1, c2;


static uint8_t shades[] = {32, 255, 128, 64, 32, 16, 8, 0, 0};

static uint32_t randomSeed;

uint8_t random() {
//	randomSeed = randomSeed * 37 + 73 * (randomSeed >> 12) + 997;
//	return randomSeed >> 4;
	if (randomSeed == 0) {
		randomSeed = 997;
	}
	randomSeed ^= randomSeed>>11;
	randomSeed ^= randomSeed<<7 & 0x9D2C5680;
	randomSeed ^= randomSeed<<15 & 0xEFC60000;
	randomSeed ^= randomSeed>>18;
	return randomSeed;
}

void drawBetween(uint8_t i, struct cRGB oldColor, struct cRGB newColor) {
	led[i].r = (((CALC_PRECISION) newColor.r) * c1
			+ ((CALC_PRECISION) oldColor.r) * c2
//			+ random()
			) >> SUB_STEPS_BITS;
	led[i].g = (((CALC_PRECISION) newColor.g) * c1
			+ ((CALC_PRECISION) oldColor.g) * c2
//			+ random()
			) >> SUB_STEPS_BITS;
	led[i].b = (((CALC_PRECISION) newColor.b) * c1
			+ ((CALC_PRECISION) oldColor.b) * c2
//			+ random()
			) >> SUB_STEPS_BITS;
}

void randomizeColor(uint8_t i) {
	uint8_t color = random() & (INITIAL_COLORS - 1);
	if (color < INITIAL_COLORS - 1) {
		colors[i] = initialColors[color];
	} else {
		uint8_t brightness;
		do {
			colors[i].r = random() & 0xFF;
			colors[i].g = random() & 0xFF;
			colors[i].b = random() & 0xFF;
			brightness = (colors[i].r >> 2) + (colors[i].g >> 2) + (colors[i].b >> 2);
			colors[i].r += ((colors[i].r >> 2) * 3 - brightness) << 1;
			colors[i].g += ((colors[i].g >> 2) * 3 - brightness) << 1;
			colors[i].b += ((colors[i].b >> 2) * 3 - brightness) << 1;
		} while (brightness < 64 || brightness >= 128);
	}
}

struct cRGB shade(struct cRGB color, uint8_t shade) {
	struct cRGB shaded;
	shaded.r = (uint16_t) color.r * shade >> 8;
	shaded.g = (uint16_t) color.g * shade >> 8;
	shaded.b = (uint16_t) color.b * shade >> 8;
	return shaded;
}

inline struct cRGB rgb(uint8_t r, uint8_t g, uint8_t b) {
	struct cRGB shaded;
	shaded.r = r;
	shaded.g = g;
	shaded.b = b;
	return shaded;
}

int main(void)
{

	DDRB |= _BV(ws2812_pin);

	PORT_BTN |= BTN_1 | BTN_2;

	// prescale timer to 1/1024th the clock rate
	TCCR0 |= (1<<CS02) | (1<<CS00);

	// ADC
	ADMUX |= (1<<REFS0) | (0<<ADLAR);
	ADCSRA |= (1<<ADEN);


    //Rainbow colors
	rainbowColors[0] = rgb(255, 0, 0);//red
    rainbowColors[1] = rgb(255, 60, 0);//orange
    rainbowColors[2] = rgb(200, 100, 0);//yellow
    rainbowColors[3] = rgb(0, 255, 0);//green
    rainbowColors[4] = rgb(0, 120, 120);//light blue (türkis)
    rainbowColors[5] = rgb(0, 0, 255);//blue
    rainbowColors[6] = rgb(80, 0, 120);//violet
    rainbowColors[7] = rgb(40, 40, 40);
    rainbowColors[8] = rgb(0, 0, 0);
    rainbowColors[9] = rgb(0, 0, 0);
    rainbowColors[10] = rgb(0, 0, 0);
    rainbowColors[11] = rgb(0, 0, 0);

    initialColors[0] = rgb(255, 0, 0);//red
    initialColors[1] = rgb(255, 60, 0);//orange
    initialColors[2] = rgb(200, 100, 0);//yellow
    initialColors[3] = rgb(0, 255, 0);//green
    initialColors[4] = rgb(0, 120, 120);//light blue (türkis)
    initialColors[5] = rgb(0, 0, 255);//blue
    initialColors[6] = rgb(80, 0, 120);//violet
//    initialColors[7] = rgb(220,20,60);//crimson

//    initialColors[0] = rgb(255, 0, 0);//red
//    initialColors[1] = rgb(220,20,60);//crimson
//    initialColors[2] = rgb(0, 255, 0);//green
//    initialColors[3] = rgb(173,255,47);//greenyellow
//    initialColors[4] = rgb(0, 0, 255);//blue
//    initialColors[5] = rgb(32,178,170);//lightseagreen
//    initialColors[6] = rgb(255,215,0);//gold
//    initialColors[7] = rgb(139,69,19);//saddlebrown

//    initialColors[7] = rgb(139, 69, 19);//saddlebrown
//    initialColors[8] = rgb(188, 143, 143);//rosybrown
//    initialColors[9] = rgb(255,215,0);//gold
//    initialColors[10] = rgb(220,20,60);//crimson
//    initialColors[11] = rgb(128,0,0);//maroon
//    initialColors[12] = rgb(75,0,130);//indigo
//    initialColors[13] = rgb(173,255,47);//greenyellow
//    initialColors[14] = rgb(107,142,35);//olivedrab
//    initialColors[15] = rgb(255,228,181);//moccasin


	uint8_t i = 0;

	for (i = 0; i < MAXPIX; i++) {
		randomizeColor(i);
	}

    uint8_t style = 0;
    uint8_t direction = 0;
    uint8_t prevButtons = 0;

    offset = 0;
    offsetLength = 0xFF;
    speed = 16;

    uint16_t speedADC = 0;

	while (1) {

		// wait for next frame, 50fps on 8mhz
		do {
			ADCSRA |= (1<<ADSC);
			while (ADCSRA & (1<<ADSC)) {};
			speedADC -= speedADC >> 6;
			speedADC += ADC;
		} while (TCNT0 < 156);
		TCNT0 = 0;


		// process buttons
		uint8_t buttons = ~PIN_BTN;
		uint8_t buttonsPressed = buttons & ~prevButtons;

		if (buttonsPressed & BTN_1) {
			style++;
			if (style >= STYLES) {
				style = 0;
			}
			offset = 0;
			offsetLength = 0xFF;
			direction = 0;
		}
		if (buttonsPressed & BTN_2) {
			direction = (direction + 1) & 3;
		}


		// calculate next frame
		if (direction == 0) {
			offset += ((speedADC >> 8) * speed >> 5) + 1;
		} else if (direction == 2) {
			offset += offsetLength - (((speedADC >> 8) * speed >> 5) + 1);
		}

		while (offset >= offsetLength) {
			offset -= offsetLength;
		}
		step = offset >> SUB_STEPS_BITS;
		c1 = offset & ((1 << SUB_STEPS_BITS) - 1);
		c2 = (1 << SUB_STEPS_BITS) - c1;

		// calculate leds depending on style
		if (style == 0) {

			// Running lights
		    speed = 16;
			offsetLength = 64 << SUB_STEPS_BITS;

			uint8_t col, j;
			for (i = 0; i < MAXPIX; i++) {
				j = (step + i) & 7;
				col = (step + i) >> 3;
				col++;
				if (col >= (64 >> 3)) {
					col -= (64 >> 3);
				}

				drawBetween(MAXPIX - 1 - i, shade(colors[col], shades[j]), shade(colors[col], shades[j + 1]));
			}
			uint8_t nextCol = (col + 1) & 7;
			if (direction) {
				// reverse
				col = (nextCol + 1) & 7;
			}
			do {
				randomizeColor(nextCol);
			} while ((colors[col].r & 0x80) == (colors[nextCol].r & 0x80) &&
					(colors[col].g & 0x80) == (colors[nextCol].g & 0x80) &&
					(colors[col].b & 0x80) == (colors[nextCol].b & 0x80));
		} else if (style == 1) {

			// Rainbow
		    speed = 16;
			offsetLength = RAINBOW_COLORS << SUB_STEPS_BITS;

			for (i = 0; i < MAXPIX; i++) {
				uint8_t prev = step;
				step++;
				if (step >= RAINBOW_COLORS) {
					step = 0;
				}

				drawBetween(MAXPIX - 1 - i, rainbowColors[prev], rainbowColors[step]);
			}

		} else if (style == 2) {

			// Same color
		    speed = 1;
			offsetLength = 7 << SUB_STEPS_BITS;

			uint8_t next = step + 1;

			if (next >= 7) {
				next = 0;
			}

			for (i = 0; i < MAXPIX; i++) {
				drawBetween(i, rainbowColors[step], rainbowColors[next]);
			}

		} else {

			// Randomized
		    speed = 8;
			offsetLength = 2 << SUB_STEPS_BITS;

			if (step != prevStep) {
				for (i = 0; i < COLORS >> 1; i++) {
					colors[i] = colors[i + (COLORS >> 1)];
				}
				for (i = COLORS >> 1; i < MAXPIX; i++) {
					if (direction == 2 || (i & 1) == (step & 1)) {
						randomizeColor(i);
					}
				}
			}
			uint8_t col = 0;
			for (i = 0; i < MAXPIX; i++) {
				col++;
				if (col >= COLORS >> 1) {
					col = 0;
				}
				if (direction & 2) {
					drawBetween(i, colors[col + (COLORS >> 1)], colors[col]);
				} else {
					drawBetween(i, colors[col], colors[col + (COLORS >> 1)]);
				}
			}
		}

//		led[0].r = blink;
//		led[0].g = blink;
//		led[0].b = blink;
//		blink = ~blink;

		ws2812_sendarray((uint8_t *) led, MAXPIX * 3);

		prevButtons = buttons;
		prevStep = step;
	}
}

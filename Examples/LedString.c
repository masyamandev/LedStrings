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
#define RAINBOW_COLORS 10
#define COLORS MAXPIX

#define SUB_STEPS_BITS 8
#define CALC_PRECISION uint32_t
#define OFFSET_PRECISION uint16_t

struct cRGB rainbowColors[RAINBOW_COLORS];
struct cRGB colors[COLORS];
struct cRGB led[MAXPIX];

#define DDR_BTN 	DDRD
#define PIN_BTN 	PIND
#define PORT_BTN 	PORTD
#define BTN_1 	(1 << 5)
#define BTN_2 	(1 << 6)

#define STYLES 	3



static OFFSET_PRECISION offset = 0;
static OFFSET_PRECISION offsetLength = 0xFF;
static OFFSET_PRECISION speed = 16;

static uint8_t step;
static OFFSET_PRECISION c1, c2;

static uint16_t randomSeed;

uint8_t random() {
	randomSeed = randomSeed * 37 + 73 * (randomSeed >> 12) + 997;
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
	colors[i] = rainbowColors[random() & 7];
}

struct cRGB shade(struct cRGB color, uint8_t shade) {
	struct cRGB shaded;
	shaded.r = (uint16_t) color.r * shade >> 8;
	shaded.g = (uint16_t) color.g * shade >> 8;
	shaded.b = (uint16_t) color.b * shade >> 8;
	return shaded;
}

int main(void)
{

	DDRB |= _BV(ws2812_pin);

	PORT_BTN |= BTN_1 | BTN_2;
		
    //Rainbow colors
    rainbowColors[0].r=255; rainbowColors[0].g=000; rainbowColors[0].b=000;//red
    rainbowColors[1].r=255; rainbowColors[1].g=60;  rainbowColors[1].b=000;//orange
    rainbowColors[2].r=200; rainbowColors[2].g=100; rainbowColors[2].b=000;//yellow
    rainbowColors[3].r=000; rainbowColors[3].g=255; rainbowColors[3].b=000;//green
    rainbowColors[4].r=000; rainbowColors[4].g=120; rainbowColors[4].b=120;//light blue (türkis)
    rainbowColors[5].r=000; rainbowColors[5].g=000; rainbowColors[5].b=255;//blue
    rainbowColors[6].r=80;  rainbowColors[6].g=000; rainbowColors[6].b=120;//violet
//    rainbowColors[7].r=20;  rainbowColors[7].g=000; rainbowColors[7].b=30;
    rainbowColors[7].r=40;  rainbowColors[7].g=40;  rainbowColors[7].b=40;
    rainbowColors[8].r=000; rainbowColors[8].g=000; rainbowColors[8].b=000;
    rainbowColors[9].r=000; rainbowColors[9].g=000; rainbowColors[9].b=000;

	uint8_t i = 0;

	for (i = 0; i < MAXPIX; i++) {
		randomizeColor(i);
	}

    uint8_t style = 0;
    uint8_t onPause = 0;
    uint8_t prevButtons = 0;

    offset = 0;
    offsetLength = 0xFF;
    speed = 16;

	while (1) {
		uint8_t buttons = ~PIN_BTN;
		uint8_t buttonsPressed = buttons & ~prevButtons;

		if (buttonsPressed & BTN_1) {
			style++;
			if (style >= STYLES) {
				style = 0;
			}
			offset = 0;
			offsetLength = 0xFF;
		}
		if (buttonsPressed & BTN_2) {
			onPause = !onPause;
		}

		if (onPause) {
			_delay_ms(10);
			prevButtons = buttons;
			continue;
		}

		offset += speed;
		while (offset >= offsetLength) {
			offset -= offsetLength;
		}
		step = offset >> SUB_STEPS_BITS;
		c1 = offset & ((1 << SUB_STEPS_BITS) - 1);
		c2 = (1 << SUB_STEPS_BITS) - c1;

		if (style == 0) {

			// Running lights
		    speed = 16;
			offsetLength = 64 << SUB_STEPS_BITS;

			uint8_t shades[] = {32, 255, 128, 64, 32, 16, 8, 0, 0};

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
			do {
				randomizeColor(nextCol);
			} while (colors[col].r == colors[nextCol].r && colors[col].g == colors[nextCol].g && colors[col].b == colors[nextCol].b);
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
			offsetLength = 8 << SUB_STEPS_BITS;

			uint8_t next = step + 1;

			if (next >= 8) {
				next = 0;
			}

			for (i = 0; i < MAXPIX; i++) {
				drawBetween(i, rainbowColors[step], rainbowColors[next]);
			}

		} else {
//			uint8_t i = 0, j = offset;
//			for (i = 0; i < MAXPIX; i++) {
//				led[i] = rainbowColors[j];
//				j++;
//				if (j >= RAINBOW_COLORS) {
//					j = 0;
//				}
//			}
//			offset++;
//			if (offset >= RAINBOW_COLORS) {
//				offset = 0;
//			}
		}


		_delay_ms(10);
		ws2812_sendarray((uint8_t *) led, MAXPIX * 3);

		prevButtons = buttons;
	}
}

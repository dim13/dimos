/* $Id$ */
/* public domain */

#include <inttypes.h>

void
hsv(uint8_t *r, uint8_t *g, uint8_t *b, uint16_t h, uint8_t s, uint8_t v)
{
	uint8_t i, f, p, q, t;

	i = h / 60;
	f = h % 60;
        p = v - ((uint16_t)v * s) / 255;
        q = v - ((uint32_t)v * s * f) / (255 * 59);
        t = v - ((uint32_t)v * s * (59 - f)) / (255 * 59);

	switch (i % 6) {
	case 0: *r = v; *g = t; *b = p; break;
	case 1: *r = q; *g = v; *b = p; break;
	case 2: *r = p; *g = v; *b = t; break;
	case 3: *r = p; *g = q; *b = v; break;
	case 4: *r = t; *g = p; *b = v; break;
	case 5: *r = v; *g = p; *b = q; break;
	}
}

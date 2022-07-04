/* Render a lovely interference pattern between sin in x & y
 *
 * 4BPP at 320x240, for MR project test.
 *
 * Copyright 2016-2022 Matt Evans
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <inttypes.h>
#include "lookuptables.h"


#define WIDTH	320
#define HEIGHT	240

static int ang = 1;

/* The SIN/COS stuff is :14 bit fixed point, i.e. 1 is 0x7fff and -1 is 0xffff.
 * bit [15] is sign; to normalise to O(1), >>14.
 */
void draw_plasma(uint32_t *fb)
{
	uint32_t pixword = 0;

	int x, y;
	int x_theta;
	int x_thetaB;

	int y_theta = (98-(ang*11)) << 14;
	int y_thetaB = (27+(ang*9)) << 14;


	int mag = 2;
	int x_theta_inc = SIN(ang) * mag;
	int y_theta_inc = COS(ang) * mag;
	// Another pattern, but not in fixed-point.
	int x_thetaB_inc = 3;
	int y_thetaB_inc = 4;

	ang++;

	for (y = 0; y < HEIGHT; y++)
	{
		x_theta = (ang*3)+21;
		y_theta += y_theta_inc;

		x_thetaB = 99-(ang*7);
		y_thetaB += y_thetaB_inc;

		for (x = 0; x < WIDTH; x++)
		{
			unsigned int c,p;

			x_theta += x_theta_inc;
			x_thetaB += x_thetaB_inc;

			/* This 4+sin.. i.e., ranged 0-8. */
			c = 0x20000 + SIN(x_theta >> 14)
				+ SIN(y_theta >> 14)
				+ SIN(x_thetaB)
				+ COS(y_thetaB)
				;

			p = (c >> 14) & 0xf;
			pixword >>= 4;
			pixword |= p << 28;

			// Framebuffer is little-endian!
			// byte-reversed store is easiest & fast here.
			if ((x & 0x7) == 0x7) {
				__asm__ __volatile__("stwbrx	%0, 0, %1\n"
						     : : "r"(pixword), "r"(fb) : "memory");
				fb++;
			}
		}
		y_theta++;
	}
}

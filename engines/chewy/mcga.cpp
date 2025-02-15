/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "common/system.h"
#include "graphics/palette.h"
#include "graphics/surface.h"
#include "chewy/chewy.h"
#include "chewy/globals.h"
#include "chewy/main.h"
#include "chewy/mcga.h"

namespace Chewy {

#define SCREEN_S _G(currentScreen)
#define SCREEN _G(currentScreen).getPixels()
#define VGA_COLOR_TRANS(x) ((x) * 255 / 63)

void init_mcga() {
	_G(currentScreen) = (byte *)g_screen->getPixels();
	_G(spriteWidth) = 0;
}

byte *get_dispoff() {
	return SCREEN;
}

void setScummVMPalette(const byte *palette, uint start, uint count) {
	byte tempPal[PALETTE_SIZE];
	byte *dest = &tempPal[0];

	for (uint i = 0; i < count * 3; ++i, ++palette, ++dest)
		*dest = VGA_COLOR_TRANS(*palette);

	g_system->getPaletteManager()->setPalette(tempPal, start, count);
}

void set_palette(const byte *palette) {
	setScummVMPalette(palette, 0, PALETTE_COUNT);
}

void rastercol(int16 color, int16 r, int16 g, int16 b) {
	byte rgb[3];
	rgb[0] = r;
	rgb[1] = g;
	rgb[2] = b;

	setScummVMPalette(&rgb[0], color, 1);
}

void setPartialPalette(const byte *palette, int16 startCol, int16 nr) {
	setScummVMPalette(palette + startCol * 3, startCol, nr);
}

void clear_mcga() {
	if (SCREEN == (byte *)g_screen->getPixels())
		g_screen->clear();
	else
		Common::fill(SCREEN, SCREEN + SCREEN_WIDTH * SCREEN_HEIGHT, 0);
}

uint8 getpix(int16 x, int16 y) {
	return *(byte *)SCREEN_S.getBasePtr(x, y);
}

void line_mcga(int16 x1, int16 y1, int16 x2, int16 y2, int16 color) {
	return SCREEN_S.drawLine(x1, y1, x2, y2, color);
}

void mem2mcga(const byte *ptr) {
	byte *destP = (byte *)g_screen->getPixels();
	Common::copy(ptr + 4, ptr + 4 + (SCREEN_WIDTH * SCREEN_HEIGHT), destP);
	g_screen->markAllDirty();
}

void map_spr_2screen(const byte *sptr, int16 x, int16 y) {
	const int width = *((const int16 *)sptr);
	sptr += 4 + y * width + x;
	byte *destP = SCREEN;

	for (int row = 0; row < SCREEN_HEIGHT;
			++row, sptr += width, destP += SCREEN_WIDTH) {
		Common::copy(sptr, sptr + SCREEN_WIDTH, destP);
	}
}

void spr_save_mcga(byte *sptr, int16 x, int16 y, int16 width, int16 height, int16 scrWidth) {
	*((int16 *)sptr) = width;
	sptr += 2;
	*((int16 *)sptr) = height;
	sptr += 2;

	int pitch;
	byte *scrP;
	if (scrWidth == 0) {
		scrP = SCREEN + y * SCREEN_WIDTH + x;
		pitch = SCREEN_WIDTH;
	} else {
		scrP = SCREEN + y * scrWidth + x;
		pitch = scrWidth;
	}

	if (width >= 1 && height >= 1) {
		for (int row = 0; row < height; ++row) {
			Common::copy(scrP, scrP + width, sptr);
			scrP += pitch;
			sptr += width;
		}
	}
}

void spr_set_mcga(const byte *sptr, int16 x, int16 y, int16 scrWidth) {
	const int width = *((const int16 *)sptr);
	sptr += 2;
	const int height = *((const int16 *)sptr);
	sptr += 2;

	if (width >= 1 && height >= 1) {
		int pitch;
		byte *scrP;
		if (scrWidth == 0) {
			scrP = SCREEN + y * SCREEN_WIDTH + x;
			pitch = SCREEN_WIDTH;
		} else {
			scrP = SCREEN + y * scrWidth + x;
			pitch = scrWidth;
		}

		for (int row = 0; row < height; ++row) {
			Common::copy(sptr, sptr + width, scrP);
			scrP += pitch;
		}
	}
}

static bool mspr_set_mcga_clip(int x, int y, int pitch, int &width, int &height, const byte *&srcP, byte *&destP) {
	if (y < _G(clipy1)) {
		int yDiff = ABS(_G(clipy1) - y);
		height -= yDiff;
		srcP += yDiff * width;
		y = _G(clipy1);
	}
	if (height < 1)
		return false;

	if (x < _G(clipx1)) {
		int xDiff = ABS(_G(clipx1) - x);
		width -= xDiff;
		srcP += xDiff;
		x = _G(clipx1);
	}
	if (width < 1)
		return false;

	int x2 = x + width;
	if (x2 > _G(clipx2)) {
		int xDiff = x2 - _G(clipx2);
		width -= xDiff;
	}
	if (width <= 1)
		return false;

	int y2 = y + height;
	if (y2 > _G(clipy2)) {
		int yDiff = y2 - _G(clipy2);
		height -= yDiff;
	}
	if (height < 1)
		return false;

	destP = SCREEN + pitch * y + x;
	return true;
}

void mspr_set_mcga(byte *sptr, int16 x, int16 y, int16 scrWidth) {
	if (!sptr)
		return;

	byte *destP;
	int width = *((const int16 *)sptr);
	sptr += 2;
	int height = *((const int16 *)sptr);
	sptr += 2;
	const byte *srcP = sptr;
	_G(spriteWidth) = width;

	if (!(height >= 1 && width >= 4))
		return;

	int pitch = scrWidth ? scrWidth : SCREEN_WIDTH;
	if (!mspr_set_mcga_clip(x, y, pitch, width, height, srcP, destP))
		return;
	int destPitchRemainder = pitch - width;
	int srcPitchRemainder = _G(spriteWidth) - width;

	for (int row = 0; row < height; ++row,
			srcP += srcPitchRemainder, destP += destPitchRemainder) {
		for (int col = 0; col < width; ++col, ++srcP, ++destP) {
			if (*srcP != 0)
				*destP = *srcP;
		}
	}
}

void vors() {
	_G(gcurx) += _G(fontMgr)->getFont()->getDataWidth();
}

void putcxy(int16 x, int16 y, unsigned char c, int16 fgCol, int16 bgCol, int16 scrWidth) {
	ChewyFont *font = _G(fontMgr)->getFont();
	Graphics::Surface *textSurface = font->getLine(Common::String(c));
	byte *data = (byte *)textSurface->getPixels();

	for (int curX = 0; curX < textSurface->pitch; curX++) {
		for (int curY = 0; curY < textSurface->h; curY++) {
			if (curX + x < 320 && curY + y < 200) {
				byte *src = data + (curY * textSurface->pitch) + curX;
				byte *dst = (byte *)_G(currentScreen).getBasePtr(curX + x, curY + y);
				if (*src != 0xFF)
					*dst = fgCol;
				else if (bgCol < 0xFF)
					*dst = bgCol;
			}
		}
	}

	g_screen->addDirtyRect(Common::Rect(
		x, y, x + textSurface->pitch, y + textSurface->h));

	textSurface->free();
	delete textSurface;
}

void putz(unsigned char c, int16 fgCol, int16 bgCol, int16 scrWidth) {
	putcxy(_G(gcurx), _G(gcury), c, fgCol, bgCol, scrWidth);
}

namespace Zoom {

static int spriteHeight;
static int spriteDeltaX1, spriteDeltaX2;
static int spriteDeltaY1, spriteDeltaY2;
static int spriteXVal1, spriteXVal2;
static int spriteYVal1, spriteYVal2;

static void setXVals() {
	if (spriteDeltaX2 == 0) {
		spriteXVal1 = 0;
		spriteXVal2 = 1;
	} else {
		spriteXVal1 = _G(spriteWidth) / spriteDeltaX2;
		spriteXVal2 = 1000 * (_G(spriteWidth) % spriteDeltaX2);
		if (spriteDeltaX2)
			spriteXVal2 /= spriteDeltaX2;
	}
}

static void setYVals() {
	if (spriteDeltaY2 == 0) {
		spriteYVal1 = 0;
		spriteYVal2 = 1;
	} else {
		spriteYVal1 = spriteHeight / spriteDeltaY2;
		spriteYVal2 = 1000 * (spriteHeight % spriteDeltaY2);
		spriteYVal2 /= spriteDeltaY2;
	}
}

void clip(byte *&source, byte *&dest, int16 &x, int16 &y) {
	if (y < _G(clipy1)) {
		int yCount = _G(clipy1) - y;
		spriteDeltaY2 -= yCount;

		--yCount;
		if (yCount >= 1) {
			for (int yc = 0, countY = spriteYVal2; yc < yCount; ++yc) {
				source += _G(spriteWidth) * spriteYVal1;
				dest += SCREEN_WIDTH;

				while (countY > 1000) {
					countY -= 1000;
					source += _G(spriteWidth);
				}
			}
		}
	}

	if (spriteDeltaY2 <= 0) {
		source = nullptr;
		return;
	}

	if (x < _G(clipx1)) {
		int xCount = _G(clipx1) - x;
		spriteDeltaX2 -= xCount;
		dest += xCount;

		--xCount;
		if (xCount >= 1) {
			for (int xc = 0, countX = spriteXVal2; xc < xCount; ++xc) {
				source += spriteXVal1;
				while (countX >= 1000) {
					countX -= 1000;
					++source;
				}
			}
		}
	}

	if (spriteDeltaX2 > 0) {
		int x2 = x + spriteDeltaX2;
		if (x2 >= _G(clipx2)) {
			spriteDeltaX2 -= x2 - _G(clipx2);
		}

		if (spriteDeltaY2 > 0) {
			int y2 = y + spriteDeltaY2;
			if (y2 >= _G(clipy2)) {
				spriteDeltaY2 -= y2 - _G(clipy2);
			}
			if (spriteDeltaY2 <= 0)
				source = nullptr;
		} else {
			source = nullptr;
		}
	} else {
		source = nullptr;
	}
}

void zoom_set(byte *source, int16 x, int16 y, int16 xDiff, int16 yDiff, int16 scrWidth) {
	_G(spriteWidth) = ((int16 *)source)[0];
	spriteHeight = ((int16 *)source)[1];
	source += 4;

	spriteDeltaX1 = xDiff;
	spriteDeltaX2 = _G(spriteWidth) + xDiff;
	spriteDeltaY1 = yDiff;
	spriteDeltaY2 = spriteHeight + yDiff;

	setXVals();
	setYVals();

	byte *scrP;
	if (scrWidth == 0) {
		scrP = SCREEN + y * SCREEN_WIDTH + x;
	} else {
		scrP = SCREEN + y * scrWidth + x;
	}

	clip(source, scrP, x, y);

	if (source) {
		for (int yc = spriteDeltaY2, countY = spriteYVal2; yc > 0; --yc) {
			byte *srcLine = source;
			byte *scrLine = scrP;

			for (int xc = spriteDeltaX2, countX = spriteXVal2; xc > 0; --xc) {
				if (*source)
					*scrP = *source;

				++scrP;
				source += spriteXVal1;
				countX += spriteXVal2;
				while (countX > 1000) {
					countX -= 1000;
					++source;
				}
			}

			source = srcLine;
			scrP = scrLine + SCREEN_WIDTH;

			for (int ySkip = 0; ySkip < spriteYVal1; ++ySkip) {
				source += _G(spriteWidth);
			}

			countY += spriteYVal2;
			while (countY > 1000) {
				countY -= 1000;
				source += _G(spriteWidth);
			}
		}
	}
}

} // namespace Zoom

void zoom_set(byte *source, int16 x, int16 y, int16 xDiff, int16 yDiff, int16 scrWidth) {
	Zoom::zoom_set(source, x, y, xDiff, yDiff, scrWidth);
}

} // namespace Chewy

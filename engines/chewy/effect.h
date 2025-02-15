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

#ifndef CHEWY_EFFECT_H
#define CHEWY_EFFECT_H

namespace Chewy {

enum BlendMode {
	BLEND_NONE = 0,
	BLEND1 = 1,
	BLEND2 = 2,
	BLEND3 = 3,
	BLEND4 = 4
};

class Effect {
public:
	Effect();
	~Effect();

	void blende1(byte *memPtr, byte *screen,
		byte *palette, int16 frames, uint8 mode, int16 color);
	void border(byte *screen, int16 val1, uint8 mode, int16 color);

	void rnd_blende(byte *rnd_speicher, byte *sram_speicher,
		byte *screen, byte *palette, int16 col, int16 skip_line);

};

} // namespace Chewy

#endif

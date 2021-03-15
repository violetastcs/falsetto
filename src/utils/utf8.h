/*
 *  This file is part of Falsetto.
 *
 *  Falsetto is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Falsetto is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Falsetto.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <iso646.h>
#include <wchar.h>
#include <ctype.h>

// Get the size of the next UTF-8 codepoint in bytes
uint8_t utf8_char_size(uint8_t const* c) {
	const bool vals[] = {
		(*c & 0b10000000) == 0b00000000,
		(*c & 0b11100000) == 0b11000000,
		(*c & 0b11110000) == 0b11100000,
		(*c & 0b11111000) == 0b11110000,
	};

	uint8_t out = 0;

	for (uint8_t i = 0; i < 4; ++i)
		vals[i] and (out = i);

	return out + 1;
}

// Extract a codepoint
wint_t utf8_to_int(uint8_t const* c) {
	int32_t out = *c;

	switch (utf8_char_size(c)) {
		case 1: return out;
		case 2: return ((out & 31) << 6)  | (c[1]  & 63);
		case 3: return ((out & 15) << 12) | ((c[1] & 63) << 6)  | (c[2]  & 63);
		case 4: return ((out & 7) << 18)  | ((c[1] & 63) << 12) | ((c[2] & 63) << 6) | (c[3] & 63);
	}

	return 0;
}

// Check if a codepoint is whitespace
bool utf8_is_whitespace(wint_t c) {
	return isspace(c)
		or (c >= 0x0009 and c <= 0x000d)
		or c == 0x0020
		or c == 0x0085
		or c == 0x00a0
		or c == 0x1680
		or (c >= 0x2000 && c <= 0x200a)
		or c == 0x2028
		or c == 0x2029
		or c == 0x202f
		or c == 0x205f
		or c == 0x3000 
	;
}

/*
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
 */

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sysexits.h>

#include "../lib/base64.c"


/**
 * 
 */
void en_decode_test(char *s) {

	printf("SOURCE: %s\n", s);

	struct base64_encoding_buffer *encbuf = base64_create_encoding_buffer(75);
	base64_encode_chunk(encbuf, s, strlen(s)+1);
	printf("ENCODED: %s\n", encbuf->s);

	struct base64_decoding_buffer *decbuf = base64_create_decoding_buffer();
	base64_decode_chunk(decbuf, encbuf->s, strlen(encbuf->s));
	printf("DECODED: %s\n", decbuf->s);
	
	printf("\n\n----------------------\n");
	
	free(encbuf);
	free(decbuf);

}

/**
 * 
 */
int main(int argc, char *argv[]) {

	en_decode_test("Hello World");
	en_decode_test("Hello World!");
	en_decode_test("Hello World!!");
	en_decode_test("Hello World!!!");
	en_decode_test("Hello World!!!!");

}



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
	base64_encode_chunk(encbuf, (unsigned char*)s, strlen(s));
	printf("ENCODED: %s\n", encbuf->s);

	struct base64_decoding_buffer *decbuf = base64_create_decoding_buffer();
	base64_decode_chunk(decbuf, encbuf->s, strlen((char *)encbuf->s));
	printf("DECODED: %s\n", decbuf->s);

	printf("\n\n----------------------\n");

	base64_free_encoding_buffer(encbuf);
	base64_free_decoding_buffer(decbuf);

}

/**
 * 
 */
int main(int argc, char *argv[]) {

	// check strings
	en_decode_test("Hello World");
	en_decode_test("Hello World!");
	en_decode_test("Hello World!!");
	en_decode_test("Hello World!!!");
	en_decode_test("Hello World!!!!");

	// check binary data
	unsigned char bin[255];
	for( unsigned char i=0 ; i < 255 ; i++ ) {
		bin[i] = i;
		printf("%d ", bin[i]);
	}
	printf("\n");

	struct base64_encoding_buffer *encbuf = base64_create_encoding_buffer(75);
	base64_encode_chunk(encbuf, bin, 255);
	printf("ENCODED:\n%s\n", encbuf->s);

	struct base64_decoding_buffer *decbuf = base64_create_decoding_buffer();
	base64_decode_chunk(decbuf, encbuf->s, strlen((char *)encbuf->s));
	printf("DECODED:\n");
	for( unsigned char i=0 ; i < 255 ; i++ ) {
		printf("%d ", decbuf->s[i]);
	}
	printf("\n\n");
	
	base64_free_encoding_buffer(encbuf);
	base64_free_decoding_buffer(decbuf);

	// check encoding in chunks
	struct base64_encoding_buffer *encbuf2 = base64_create_encoding_buffer(75);
	
	int chunk_size = 96;
	int offset = 0;
	while( offset < 255 ) {
		printf("ENCODE CHUNK, size=%d\n", chunk_size);
		base64_encode_chunk(encbuf2, bin + offset, offset + chunk_size > 255 ? 255 - offset : chunk_size);
		offset += chunk_size;
		printf("ENCODED CHUNK:\n%s\n", encbuf2->s);
	}
	printf("ENCODED:\n%s\n", encbuf2->s);
	
	base64_free_encoding_buffer(encbuf2);

}



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

#include "../lib/qp.c"


/**
 * 
 */
void en_decode_test(char *s) {

	printf("SOURCE: %s\n", s);

	struct qp_encoding_buffer *encbuf = qp_create_encoding_buffer(75);
	qp_encode_chunk(encbuf, (unsigned char*)s, strlen(s));
	printf("ENCODED: %s\n", encbuf->s);

	struct qp_decoding_buffer *decbuf = qp_create_decoding_buffer();
	qp_decode_chunk(decbuf, encbuf->s, strlen((char *)encbuf->s));
	printf("DECODED: %s\n", decbuf->s);

	printf("\n\n----------------------\n");

	qp_free_encoding_buffer(encbuf);
	qp_free_decoding_buffer(decbuf);

}

/**
 * 
 */
int main(int argc, char *argv[]) {

	// check strings
	en_decode_test("Hello World = !");

	// check multipe chunks
	char* s = ".-===================-.\n";
	struct qp_encoding_buffer *encbuf = qp_create_encoding_buffer(75);
	qp_encode_chunk(encbuf, (unsigned char*)s, strlen(s));
	qp_encode_chunk(encbuf, (unsigned char*)s, strlen(s));
	printf("ENCODED:\n%s\n", encbuf->s);
	qp_free_encoding_buffer(encbuf);

	char* qps = "=3D0=3D1=3D2=3D3=3D4=3D5=3D6=3D7=3D8=3D9\n";
	struct qp_decoding_buffer *decbuf = qp_create_decoding_buffer();
	qp_decode_chunk(decbuf, (unsigned char*)qps, strlen(qps));
	qp_decode_chunk(decbuf, (unsigned char*)qps, strlen(qps));
	printf("DECODED:\n\"%s\"\n", decbuf->s);
	qp_free_decoding_buffer(decbuf);

}



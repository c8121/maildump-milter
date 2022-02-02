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

/*
 * Author: christian c8121 de
 */

/**
 * Caller must free result
 */
char* iso_to_utf(char *s) {

	size_t len = strlen(s);

	unsigned char *in = (unsigned char*)s;
	unsigned char *out = malloc(len * 2 +1);

	unsigned char *end = in + len;
	unsigned char *p = in;
	unsigned char *o = out;
	while( p < end ) {
		if (*p < 128)
			*o++ = *p++;
		else {
			*o++ = 0xc2+(*p > 0xbf);
			*o++ = (*p++ & 0x3f) + 0x80;
		}
	}
	*o = '\0';

	return (char *)out;
}

/**
 * Return 1 = valid, 0 = invalid
 */
int check_invalid_utf8_seq(char *s, char replace) {

	if( s == NULL )
		return 1;

	int result = 1;

	size_t len = strlen(s);
	
	unsigned char *in = (unsigned char*)s;
	
	unsigned char *end = in + len;
	unsigned char *p = in;
	int code_length;
	while( p < end ) {

		if( *p > 0x7F ) {

			if( *p >= 0xC2 && *p <= 0xDF ) {
				code_length = 2;
			} else if ( *p >= 0xE0 && *p <= 0xEF ) {
				code_length = 3;
			} else if ( *p >= 0xF0 && *p <= 0xF4 ) {
				code_length = 4;
			} else {
				//invalid first byte
				if( replace != 0 )
					*p = '?';
				code_length = 1;
				result = 0;
			}

			if( code_length > 1 ) {
				if( p + code_length >= end ) {
					//incomplete byte sequence
					if( replace != 0 )
						*p = '?';
					code_length = 1;
					result = 0;
				} else {
					int valid = 1;
					for( int i=1 ; i < code_length ; i++ ) {
						if( (*(p+i) & 0xC0) != 0x80 ) {
							//invalid continuation bytes: bit 7 should be set, bit 6 should be unset (b10xxxxxx)
							valid = 0;
						}
					}
					if( !valid ) {
						if( replace != 0 )
							*p = '?';
						code_length = 1;
						result = 0;
					}
				}
			}

			p += code_length;

		} else {
			p++;
		}
	}

	return result;
}

/**
 * Caller must free result
 */
char *charset_validate_string(char *s) {

	char *result;

	if( check_invalid_utf8_seq(s, 0) == 0 ) {

		result = iso_to_utf(s);
		//If converting did not work out?
		if( check_invalid_utf8_seq(result, '?') == 0 )
			fprintf(stderr, "Failed to convert ISO to UTF\n");	

	} else {

		result = malloc(strlen(s)+1);
		strcpy(result, s);

	}

	return result;
}

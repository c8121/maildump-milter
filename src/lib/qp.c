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

#define HEX "0123456789ABCDEF"

/**
 * 
 */
int __hexval(int c) {
	if ('0' <= c && c <= '9') 
		return c - '0';
	return 10 + c - 'A';
}

/**
 * Caller is responsible for freeing the returned buffer.
 */
char* qp_decode(const char *body)
{
	char *out = malloc(strlen(body)+1);

	int p = 0;
	int o = 0;
	while( body[p] != '\0' && p <= strlen(body) ) {

		if (body[p] != '=')
			out[o++] = body[p++];
		else if (body[p+1] == '\r' && body[p+2] == '\n')
			p += 3;
		else if (body[p+1] == '\n')
			p += 2;
		else if (!strchr(HEX, body[p+1]))
			out[o++] = body[p++];
		else if (!strchr(HEX, body[p+2]))
			out[o++] = body[p++];
		else {
			out[o++] = 16 * __hexval(body[p+1]) + __hexval(body[p+2]); 
			p += 3;
		}
	}
	out[o++] = '\0';

	return out;
}

char* qp_encode(const unsigned char *body, size_t len) {

	size_t out_len = len * 1.2 + 6;
	char *out = malloc(out_len);
	
	int li = 0;
	int o = 0;
	for( int i=0 ; i<len ; i++ ) {
		
		if( o >= out_len - 3 ) {
			out_len += 100;
			char *tmp = malloc(out_len);
			strcpy(tmp, out);
			free(out);
			out = tmp;
		}
		
		if( body[i] == '\n' ) {
			//Single <LF> to <CR><LF>
			out[o++] = '\r';
			out[o++] = '\n';
		} else if( i < (len-1) && body[i] == '\r' && body[i+1] == '\n' ) {
			//<CR><LF> as it is
			out[o++] = '\r';
			out[o++] = '\n';
			i++;
		} else if ( body[i] == '\r' ) {
			//Single <CR> to <CR><LF>
			out[o++] = '\r';
			out[o++] = '\n';
		} else if( (body[i] >= 32 && body[i] <= 60) || (body[i] >= 62 && body[i] <= 126) ) {
			//Allowed chars
			out[o++] = body[i];
			li++;
		} else {
			//Encoded chars
			if( li >= 70 ) {
				//Max line length reached
				sprintf(out+o, "=\r\n");
				o += 3;
				li = 0;
			}
			sprintf(out+o, "=%02X", body[i]);
			o += 3;
			li += 3;
		}

		if( body[i] == '\r' || body[i] == '\n' ) {
			//New line just started, reset line counter
			li = 0;
		} else if( li >= 73 ) {
			//Max line length reached
			sprintf(out+o, "=\r\n");
			o += 3;
			li = 0;
		}
	}
	out[o++] = '\0';
	
	return out;
}
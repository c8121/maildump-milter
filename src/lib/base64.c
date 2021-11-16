/*
 * Heavily inspired by & copied from mail-transcode.c from Bert Bos <bert@w3.org>
 * https://www.w3.org/Tools/Mail-Transcode/mail-transcode.c
 * 
 */

/*
 * Author: christian c8121 de
 */

#define BASE64 "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"

struct base64_encoding_buffer {
	char *s;
	int max_line_length; //must be set before first call of base64_encode_chunk
	int line_index; //position in current line, should be initialized with 0
};

struct base64_decoding_buffer {
	unsigned char *s;
	size_t len;
	char *encoded; //Data which has not been encoded so far (input length was not devidable by 4 or not padded) 
};

/**
 * 
 */
void base64_encode_chunk(struct base64_encoding_buffer *buf, unsigned char *s, size_t len) {

	if( buf->s != NULL ) {
		size_t last = strlen(buf->s)-1;
		while( buf->s[last] == '\r' || buf->s[last] == '\n' )
			last--;
		if( buf->s[last] == '=' ) {
			fprintf(stderr, "Cannot append chunk, buffer was padded (please take care to choose a length devidable by 3 when appending chunks; only last chunk can have different size\n)");
			exit(EX_IOERR);
		}
	}

	size_t out_len = len * 4 / 3 + 4;
	out_len += out_len / buf->max_line_length *2; //CRLF
	out_len++; // nul
	unsigned char *out = malloc(out_len);

	unsigned char *p = s;
	unsigned char *o = out;

	while( p < s + len ) {
		*o++ = BASE64[*p >> 2];
		*o++ = BASE64[((*p & 0x03) << 4) | (*(p+1) >> 4)];
		*o++ = BASE64[((*(p+1) & 0x0F) << 2) | (*(p+2) >> 6)];
		*o++ = BASE64[*(p+2) & 0x3F];

		p += 3;
		buf->line_index += 4;
		if( buf->line_index >= buf->max_line_length ) {
			*o++ = '\r';
			*o++ = '\n';
			buf->line_index = 0;
		}
	}

	if (!*p) {
		/* No padding needed */
	} else if (!*(p+1)) {
		*o++ = BASE64[*p >> 2];
		*o++ = BASE64[((*p & 0x03) << 4)];
		*o++ = '=';
		*o++ = '=';
		*o++ = '\r';
		*o++ = '\n';
	} else {
		*o++ = BASE64[*p >> 2];
		*o++ = BASE64[((*p & 0x03) << 4) | (*(p+1) >> 4)];
		*o++ = BASE64[((*(p+1) & 0x0F) << 2)];
		*o++ = '=';
		*o++ = '\r';
		*o++ = '\n';
	}
	*o++ = '\0';


	if( buf->s == NULL ) {
		buf->s = malloc(strlen(out)+1);
		strcpy(buf->s, out);
	} else {
		unsigned char *tmp = malloc(strlen(buf->s)+strlen(out)+1);
		strcpy(tmp, buf->s);
		strcat(tmp, out);
		free(buf->s);
		buf->s = tmp;
	}
}

/**
 * 
 */
unsigned int __base64_val(char c)
{
	if ('A' <= c && c <= 'Z') 
		return c - 'A';
	if ('a' <= c && c <= 'z') 
		return 26 + c - 'a';
	if ('0' <= c && c <= '9') 
		return 52 + c - '0';
	if (c == '+') 
		return 62;
	if (c == '/') 
		return 63;
	return 64;
}

/**
 * 
 */
void base64_decode_chunk(struct base64_decoding_buffer *buf, unsigned char *s, size_t len)
{
	unsigned char *in;

	size_t bi;
	if( buf->encoded == NULL ) {
		buf->encoded = malloc(len  + 1);
		bi = 0;
	} else {
		bi = strlen(buf->encoded);
		unsigned char *tmp = malloc(bi + len + 1);
		strcpy(tmp, buf->encoded);
		free(buf->encoded);
		buf->encoded = tmp;
	}

	for( size_t i=0 ; i < len ; i++ ) {
		//Copy valid chars and padding only
		if (s[i]=='=' || (__base64_val(s[i])) < 64) {
			buf->encoded[bi++] = s[i];
		}
	}
	buf->encoded[bi] = '\0';

	size_t enc_len = strlen(buf->encoded);
	if( (enc_len % 4 != 0) && buf->encoded[enc_len-1] != '=' ) {
		return;
	}

	unsigned char *out = malloc(enc_len);

	unsigned int c, v;
	int have_bits;

	unsigned char *p = buf->encoded;
	unsigned char *o = out;

	for (have_bits = 0; *p && *p != '='; p++) {
		
		v = __base64_val(*p);

		switch (have_bits) {
		case 0: 
			c = v; 
			have_bits = 6; 
			break;

		case 6: 
			*o++ = (c << 2) | (v >> 4); 
			c = v & 0x0F; 
			have_bits = 4; 
			break;

		case 4: 
			*o++ = (c << 4) | (v >> 2); 
			c = v & 0x03; 
			have_bits = 2; 
			break;

		case 2: 
			*o++ = (c << 6) | v; 
			c = 0; 
			have_bits = 0; 
			break;
		}

	}
	
	size_t out_len = o - out;
	if( buf->s == NULL ) {
		buf->s = malloc(out_len);
		buf->len = out_len;
		memcpy(buf->s, out, out_len);
	} else {
		unsigned char *tmp = malloc(buf->len + out_len);
		memcpy(tmp, buf->s, buf->len);
		memcpy(tmp + buf->len, out, out_len);
		free(buf->s);
		buf->s = tmp;
		buf->len += out_len;
	}
	
	free(buf->encoded);
	buf->encoded = NULL;
}


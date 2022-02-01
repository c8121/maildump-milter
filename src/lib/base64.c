/*
 * Heavily inspired by & copied from mail-transcode.c from Bert Bos <bert@w3.org>
 * https://www.w3.org/Tools/Mail-Transcode/mail-transcode.c
 * 
 */

/*
 * Author: christian c8121 de
 */

#define BASE64 "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"

#define BASE64_MAX_MALLOC_SIZE 4206592 //4MB

struct base64_encoding_buffer {
	unsigned char *s;
	size_t encoded_len;
	int max_line_length; //must be set before first call of base64_encode_chunk
	int line_index; //position in current line, should be initialized with 0
};

struct base64_decoding_buffer {
	unsigned char *s;
	size_t len;
	unsigned char *encoded; //Data which has not been encoded so far (input length was not devidable by 4 or not padded) 
	size_t encoded_len;
	size_t encoded_avail;
};

/**
 * 
 */
struct base64_encoding_buffer* base64_create_encoding_buffer(int max_line_length) {
	struct base64_encoding_buffer *buf = malloc(sizeof(struct base64_encoding_buffer));
	buf->s = NULL;
	buf->encoded_len = 0;
	buf->max_line_length = max_line_length;
	buf->line_index = 0;

	return buf;
}

/**
 * 
 */
void base64_free_encoding_buffer(struct base64_encoding_buffer *buf) {
	
	if( buf->s != NULL )
		free(buf->s);
	
	free(buf);
}

/**
 * 
 */
size_t base64_encoded_size(size_t len) {

	size_t b64_len = len * 4 / 3;

	size_t pad = b64_len % 4;
	if( pad > 0 )
		b64_len += 4 - b64_len % 4;

	return b64_len;
}


/**
 * 
 */
void base64_encode_chunk(struct base64_encoding_buffer *buf, unsigned char *s, size_t len) {

	if( buf->s != NULL ) {
		size_t last = buf->encoded_len;
		while( buf->s[last] == '\r' || buf->s[last] == '\n' )
			last--;
		if( buf->s[last] == '=' ) {
			fprintf(stderr, "Cannot append chunk, buffer was padded (please take care to choose a length devidable by 3 when appending chunks; only last chunk can have different size)\n");
			exit(EX_IOERR);
		}
	}

	size_t out_len = base64_encoded_size(len);
	out_len += out_len / buf->max_line_length * 2; //CRLF
	out_len += 3; // CRLF+nul
	unsigned char *out = malloc(out_len);

	unsigned char *p = s;
	unsigned char *o = out;
	unsigned char *end = s + len;

	unsigned char last;
	int byte_idx = 0;

	while( p < end ) {

		switch( byte_idx ) {

		case 0:
			*o++ = BASE64[(*p >> 2) & 0x3F];
			buf->line_index++;
			break;
		case 1:
			*o++ = BASE64[((last & 0x3) << 4) | ((*p >> 4) & 0xF)];
			buf->line_index++;
			break;
		case 2:
			*o++ = BASE64[((last & 0xF) << 2) | ((*p >> 6) & 0x3)];
			*o++ = BASE64[*p & 0x3F];
			buf->line_index += 2;
			break;
		}

		last = *p++;
		byte_idx = byte_idx < 2 ? byte_idx+1 : 0;

		if( buf->line_index >= buf->max_line_length ) {
			*o++ = '\r';
			*o++ = '\n';
			buf->line_index = 0;
		}
	}

	switch (byte_idx) {
	case 1:
		*o++ = BASE64[(last & 0x3) << 4];
		*o++ = '=';
		*o++ = '=';
		break;
	case 2:
		*o++ = BASE64[(last & 0xF) << 2];
		*o++ = '=';
		break;
	}

	*o = '\0';

	out_len = o - out;
	if( buf->s == NULL ) {
		buf->encoded_len = out_len;
		buf->s = malloc(out_len + 1);
		memcpy(buf->s, out, out_len + 1);
	} else {
		unsigned char *tmp = malloc(buf->encoded_len + out_len + 1);
		memcpy(tmp, buf->s, buf->encoded_len);
		memcpy(tmp + buf->encoded_len, out, out_len + 1);
		free(buf->s);
		buf->encoded_len += out_len;
		buf->s = tmp;
	}
}

/**
 * 
 */
unsigned int __base64_val(unsigned char c)
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
struct base64_decoding_buffer* base64_create_decoding_buffer() {
	struct base64_decoding_buffer *buf = malloc(sizeof(struct base64_decoding_buffer));
	buf->s = NULL;
	buf->len = 0;
	buf->encoded = NULL;
	buf->encoded_len = 0;

	return buf;
}

/**
 * 
 */
void base64_free_decoding_buffer(struct base64_decoding_buffer *buf) {
	
	if( buf->s != NULL )
		free(buf->s);
	
	if( buf->encoded != NULL )
		free(buf->encoded);
	
	free(buf);
}

/**
 * Append only, do not decode yet
 * base64_decode_chunk(...) must follow
 */
void base64_append_chunk(struct base64_decoding_buffer *buf, unsigned char *s, size_t len)
{
	if( buf->encoded == NULL ) {
		size_t malloc_size = len;
		buf->encoded = malloc(malloc_size);
		buf->encoded_len = 0;
		buf->encoded_avail = malloc_size;
	} else {
		if( buf->encoded_avail < buf->encoded_len + len ) {
			size_t malloc_size = buf->encoded_len * 2;
			malloc_size += len;
			if( malloc_size > BASE64_MAX_MALLOC_SIZE ) {
				malloc_size = BASE64_MAX_MALLOC_SIZE;
			}
			unsigned char *tmp = malloc(buf->encoded_avail + malloc_size);
			memcpy(tmp, buf->encoded, buf->encoded_len);
			free(buf->encoded);
			buf->encoded = tmp;
			buf->encoded_avail += malloc_size;
		}
	}

	unsigned char *p = s;
	unsigned char *o = buf->encoded + buf->encoded_len;
	unsigned char *n = s + len;
	size_t valid_cnt = 0;
	while( p < n ) {
		//Copy valid chars and padding only (ignore CRLF)
		if (*p == '=' || __base64_val(*p) < 64) {
			*o++ = *p;
			valid_cnt++;
		}
		p++;
	}

	buf->encoded_len += valid_cnt;
}

/**
 * 
 */
void base64_decode_chunk(struct base64_decoding_buffer *buf, unsigned char *s, size_t len)
{
	if( s != NULL ) {
		base64_append_chunk(buf, s, len);
	}

	unsigned char *out = malloc(buf->encoded_len);

	unsigned int c, v;
	int have_bits;

	unsigned char *p = buf->encoded;
	unsigned char *o = out;

	for (have_bits = 0; *p != '=' && (p - buf->encoded) < buf->encoded_len ; p++) {

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


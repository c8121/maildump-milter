/*
 * Heavily inspired by & copied from mail-transcode.c from Bert Bos <bert@w3.org>
 * See https://www.w3.org/Tools/Mail-Transcode/mail-transcode.c
 * 
 */

/*
 * Author: christian c8121 de
 */

#ifndef MM_QP
#define MM_QP

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define HEX "0123456789ABCDEF"

#define QP_MAX_MALLOC_SIZE 4096

struct qp_encoding_buffer {
	unsigned char *s;
	size_t encoded_len;
	int max_line_length; //must be set before first call of qp_encode_chunk
	int line_index; //position in current line, should be initialized with 0
};

struct qp_decoding_buffer {
	unsigned char *s;
	size_t len;
	size_t len_avail;
};


/**
 * 
 */
struct qp_encoding_buffer* qp_create_encoding_buffer(int max_line_length) {

	struct qp_encoding_buffer *buf = malloc(sizeof(struct qp_encoding_buffer));
	buf->s = NULL;
	buf->encoded_len = 0;
	buf->max_line_length = max_line_length;
	buf->line_index = 0;

	return buf;
}

/**
 * 
 */
void qp_free_encoding_buffer(struct qp_encoding_buffer *buf) {
	
	if( buf->s != NULL )
		free(buf->s);
	
	free(buf);
}


/**
 * 
 */
void qp_encode_chunk(struct qp_encoding_buffer *buf, unsigned char *s, size_t len) {

	size_t out_len = len * 3 + 6;
	unsigned char *out = malloc(out_len);

	unsigned char *p = s;
	unsigned char *o = out;
	unsigned char *n = p + len;

	while( p < n ) {

		if (buf->line_index >= buf->max_line_length && *(p) != '\r' && *(p) != '\n') {
			*o++ = '=';
			*o++ = '\r';
			*o++ = '\n';
			buf->line_index = 0;
		}

		if (*(p) == '\r' || *(p) == '\n') {
			*o++ = *p++;
			buf->line_index = 0;
		} else if (*(p) < 32 || *(p) == 61 || *(p) > 126){
			if(buf->line_index >= buf->max_line_length -3 ) {
				*o++ = '=';
				*o++ = '\r';
				*o++ = '\n';
				buf->line_index = 0;
			}
			char tmp[4];
			sprintf(tmp, "=%02X", *p);
			for( int i=0 ; i < 3 ; i++)
				*o++ = tmp[i];
			p++;
			buf->line_index += 3;
		} else {
			*o++ = *p++;
			buf->line_index++;
		}

	}
	*o++ = '\0';

	out_len = o - out - 1;

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
int __hexval(int c) {
	if ('0' <= c && c <= '9') 
		return c - '0';
	return 10 + c - 'A';
}


/**
 * 
 */
struct qp_decoding_buffer* qp_create_decoding_buffer() {

	struct qp_decoding_buffer *buf = malloc(sizeof(struct qp_decoding_buffer));
	buf->s = NULL;
	buf->len = 0;
	buf->len_avail = 0;

	return buf;
}

/**
 * 
 */
void qp_free_decoding_buffer(struct qp_decoding_buffer *buf) {
	
	if( buf->s != NULL )
		free(buf->s);
	
	free(buf);
}


/**
 * 
 */
void qp_decode_chunk(struct qp_decoding_buffer *buf, unsigned char *s, size_t len)
{
	unsigned char *out = malloc(len + 1); //nul will be added for output

	unsigned char *p = s;
	unsigned char *o = out;
	unsigned char *n = p + len;
	while( p < n ) {

		if (*p != '=')
			*o++ = *p++;
		else if (*(p+1) == '\r' && *(p+2)== '\n')
			p += 3;
		else if (*(p+1) == '\n')
			p += 2;
		else if (!strchr(HEX, *(p+1)))
			*o++ = *p++;
		else if (!strchr(HEX, *(p+2)))
			*o++ = *p++;
		else {
			*o++ = 16 * __hexval(*(p+1)) + __hexval(*(p+2)); 
			p += 3;
		}
	}
	*o++ = '\0';

	size_t out_len = o - out - 1;
	
	if( buf->s == NULL ) {
		size_t malloc_size = out_len * 4;
		buf->s = malloc(malloc_size);
		memcpy(buf->s, out, out_len);
		buf->len = out_len;
		buf->len_avail = malloc_size;
	} else {
		if( buf->len_avail < buf->len + out_len + 1 ) {
			size_t malloc_size = buf->len * 2;
			if( malloc_size > QP_MAX_MALLOC_SIZE ) {
				malloc_size = QP_MAX_MALLOC_SIZE;
			}
			unsigned char *tmp = malloc(buf->len_avail + malloc_size + out_len + 1);
			memcpy(tmp, buf->s, buf->len);
			free(buf->s);
			buf->s = tmp;
			buf->len_avail += malloc_size + out_len + 1;
		}

		memcpy(buf->s + buf->len, out, out_len);
		buf->len += out_len;
	}

	free(out);
}

#endif //MM_QP
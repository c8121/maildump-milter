/*
 * Heavily inspired by & copied from mail-transcode.c from Bert Bos <bert@w3.org>
 * See https://www.w3.org/Tools/Mail-Transcode/mail-transcode.c
 * 
 */

/*
 * Author: christian c8121 de
 */

#define HEX "0123456789ABCDEF"

struct qp_encoding_buffer {
	char *s;
	int max_line_length; //must be set before first call of qp_encode_chunk
	int line_index; //position in current line, should be initialized with 0
};

struct qp_decoding_buffer {
	char *s;
};

/**
 * 
 */
void qp_encode_chunk(struct qp_encoding_buffer *buf, unsigned char *s) {

	size_t len = strlen(s);
	size_t out_len = len * 3 + 6;
	unsigned char *out = malloc(out_len);

	unsigned char *p = s;
	unsigned char *o = out;
	unsigned char *n = p + len;

	while( *p && p < n ) {

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
			unsigned char tmp[4];
			sprintf(tmp, "=%02X", (unsigned char)*p);
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

	if( buf->s == NULL ) {
		buf->s = malloc(strlen(out)+1);
		strcpy(buf->s, out);
	} else {
		unsigned char tmp[strlen(buf->s)+strlen(out)+1];
		strcpy(tmp, buf->s);
		strcat(tmp, out);
		free(buf->s);
		buf->s = malloc(sizeof(tmp));
		strcpy(buf->s, tmp);
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
void qp_decode_chunk(struct qp_decoding_buffer *buf, unsigned char *s)
{
	size_t len = strlen(s);
	unsigned char *out = malloc(len+1);

	unsigned char *p = s;
	unsigned char *o = out;
	unsigned char *n = p + len;
	while( *p != '\0' && p < n ) {

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

	if( buf->s == NULL ) {
		buf->s = malloc(strlen(out)+1);
		strcpy(buf->s, out);
	} else {
		unsigned char tmp[strlen(buf->s)+strlen(out)+1];
		strcpy(tmp, buf->s);
		strcat(tmp, out);
		free(buf->s);
		buf->s = malloc(sizeof(tmp));
		strcpy(buf->s, tmp);
	}
}


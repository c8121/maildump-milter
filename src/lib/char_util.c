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

#define CHAR_BUFFER_INITIAL_SIZE 4096
#define CHAR_BUFFER_MAX_MALLOC_SIZE 409600

struct char_buffer {
	char *s;
	size_t len;
	size_t avail;
};


/**
 * @param cb can be NULL
 * 
 * Caller must free result (use cb_free(*cb))
 */
struct char_buffer *strappend(struct char_buffer *cb, char *s, size_t len) {
	
	struct char_buffer *buffer;
	
	if( *s && len < 1 ) //If size is omitted, determine size
		len = strlen(s);
	
	if( cb == NULL ) {
		
		buffer = malloc(sizeof(struct char_buffer));
		
		size_t malloc_size = CHAR_BUFFER_INITIAL_SIZE;
		if( malloc_size < (len +1) )
			malloc_size = len + 1;
		
		buffer->s = malloc(CHAR_BUFFER_INITIAL_SIZE);
		buffer->len = 0;
		buffer->avail = CHAR_BUFFER_INITIAL_SIZE;
		
	} else {
		buffer= cb;
	}
	
	
	if( buffer->avail < (buffer->len + len +1) ) {
		
		size_t malloc_size = (buffer->avail + len) * 4;
		if( malloc_size > CHAR_BUFFER_MAX_MALLOC_SIZE )
			malloc_size = CHAR_BUFFER_MAX_MALLOC_SIZE;
			
		if( malloc_size < (buffer->len + len +1) )
			malloc_size = buffer->len + len + 1;
			
		char *tmp = malloc(malloc_size);
		memcpy(tmp, buffer->s, buffer->len);
		free(buffer->s);
		buffer->s = tmp;
		
		buffer->avail = malloc_size;
	}
	
	memcpy(buffer->s + buffer->len, s, len);
	buffer->len += len;
	*(buffer->s + buffer->len) = '\0';
	
	return buffer;
}

/**
 * 
 */
void cb_free(struct char_buffer *cb) {
	if( cb->s != NULL )
		free(cb->s);
	free(cb);
}



struct char_list_item {
	char *s;
	struct char_list_item *next;
};

/**
 * 
 */
struct char_list_item *__cl_append(struct char_list_item *prev, char *s) {

	struct char_list_item *result = malloc(sizeof(struct char_list_item));
	if( s != NULL ) {
		result->s = malloc(strlen(s)+1);
		strcpy(result->s, s);
	} else {
		result->s = NULL;
	}

	result->next = NULL;
	if( prev != NULL ) {
		prev->next = result;
	}

	return result;
}

/**
 * 
 */
void __cl_free(struct char_list_item *cb) {

	struct char_list_item *curr = cb;
	struct char_list_item *cb_free;
	while( curr != NULL ) {
		cb_free = curr;
		curr = curr->next;

		if (cb_free->s != NULL)
			free(cb_free->s);
		free(cb_free);
	}
}

/**
 * 
 */
char *__cl_string(struct char_list_item *cb) {

	size_t size = 0;
	struct char_list_item *curr = cb;
	while( curr != NULL ) {

		if( curr->s != NULL ) {
			size += strlen(curr->s);
		}

		curr = curr->next;
	}


	size_t offset = 0;
	char *result = malloc(size + 1);
	curr = cb;
	while( curr != NULL ) {

		if( curr->s != NULL ) {
			strcpy(result + offset, curr->s);
			offset += strlen(curr->s);
		}

		curr = curr->next;
	}

	return result;
}

/**
 * 
 */
char* strreplace(char *s, char *f, char *r) {

	if( !s || !f )
		return NULL;

	struct char_list_item *result = NULL;
	struct char_list_item *curr = NULL;

	size_t f_len = strlen(f);
	size_t r_len = r != NULL ? strlen(r) : 0;

	char *last = s;
	char *p = s;
	while( (p = strstr(p, f)) ) {

		char tmp[p - last + r_len + 1];
		strncpy(tmp, last, p - last);
		if( r != NULL )
			strcpy(tmp + (p - last), r);
		else
			tmp[p - last] = '\0';

		if( result == NULL ) {
			result = __cl_append(NULL, tmp);
			curr = result;
		} else {
			curr = __cl_append(curr, tmp);
		}

		p += f_len;
		last = p;
	}

	if( result == NULL ) {
		result = __cl_append(NULL, last);
	} else {
		__cl_append(curr, last);
	}

	char *sresult = __cl_string(result);
	__cl_free(result);

	return sresult;
}

/**
 * Convinient function:
 * Free's s after replacement, allows repeated calls:
 *    s = strreplace_free(s, f, r);
 *    s = strreplace_free(s, f, r);
 */
char* strreplace_free(char *s, char *f, char *r) {

	char *result = strreplace(s, f, r);
	free(s);

	return result;
}

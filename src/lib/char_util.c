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

struct char_buffer {
	char *s;
	struct char_buffer *next;
};

/**
 * 
 */
struct char_buffer *__cb_append(struct char_buffer *prev, char *s) {

	struct char_buffer *result = malloc(sizeof(struct char_buffer));
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
void __cb_free(struct char_buffer *cb) {

	struct char_buffer *curr = cb;
	struct char_buffer *cb_free;
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
char *__cb_string(struct char_buffer *cb) {

	size_t size = 0;
	struct char_buffer *curr = cb;
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

	struct char_buffer *result = NULL;
	struct char_buffer *curr = NULL;

	size_t f_len = strlen(f);
	size_t r_len = strlen(r);

	char *last = s;
	char *p = s;
	while( p = strstr(p, f) ) {

		char tmp[p - last + r_len + 1];
		strncpy(tmp, last, p - last);
		strcpy(tmp + (p - last), r);

		if( result == NULL ) {
			result = __cb_append(NULL, tmp);
			curr = result;
		} else {
			curr = __cb_append(curr, tmp);
		}

		p += f_len;
		last = p;
	}

	if( result == NULL ) {
		result = __cb_append(NULL, last);
	} else {
		__cb_append(curr, last);
	}

	char *sresult = __cb_string(result);
	__cb_free(result);

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

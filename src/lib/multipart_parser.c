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

#ifndef MM_MULTIPART_PARSER
#define MM_MULTIPART_PARSER

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <mailutils/mime.h>
#include <ctype.h>


/**
 * 
 */
int is_empty_line(char *s) {
	if( s[0] == '\r' || s[0] == '\n' || s[0] == '\0' ) {
		return 1;
	} else {
		return 0;
	}
}

/**
 * 
 */
int __is_whitespace(char c) {
	switch(c) {
	case '\0':
	case '\r':
	case '\n':
	case ' ':
		return 1;
	default:
		return 0;
	}
}

/**
 * 
 */
char* __last_non_whitespace(char *s) {
	char *result = s + strlen(s);
	while( __is_whitespace(result[0]) ) {
		result -= 1;
		if( result < s ) {
			return NULL;
		}
	}
	return result;
}


/**
 * Caller must free result
 */
char* get_header_value(char *name, struct message_line *part) {

	char *result = NULL;

	struct message_line *curr = part;
	while( curr != NULL ) {

		char *value = NULL;

		if( strncasecmp(name, curr->s, strlen(name)) == 0 ) {
			//Begin of header
			value = strstr(curr->s, ":");
		} else if ( result != NULL ) {
			if(curr->s[0] == ' ' || curr->s[0] == '\t')
				value = curr->s; //Next line of header
			else
				break; //Begin of next header
		}

		if( value != NULL ) {
			//LTrim
			while( value[0] == ':' || value[0] == ' ' || value[0] == '\t' || value[0] == '\r' || value[0] == '\n' )
				value++;
			if( result == NULL ) {
				result = malloc(strlen(value)+1);
				strcpy(result, value);
			} else {
				char *tmp = malloc(strlen(result)+strlen(value)+1);
				strcpy(tmp, result);
				strcat(tmp, value);
				free(result);
				result = tmp;
			}
		}

		if( is_empty_line(curr->s) )
			break;

		curr = curr->next;
	}

	if( result != NULL ) {
		//RTrim
		char *p = result + strlen(result) -1;
		for( ; p > result && (*p == '\r' || *p == '\n') ; p-- )
			*p = '\0';
	}

	return result;
}

/**
 * @param free_header_value If true, then given header_value will bre free'd
 * 
 * Caller must free result
 */
char* decode_header_value(char *header_value, int remove_newline, int free_header_value) {

	if( header_value == NULL )
		return NULL;

	char *result;
	mu_rfc2047_decode("utf-8", header_value, &result);
	if( result == NULL ) {
		result = malloc(strlen(header_value)+1);
		strcpy(result, header_value);
	}

	if( free_header_value )
		free(header_value);

	if( remove_newline ) {
		char *p = result;
		char *o = result;
		while( *p ) {
			if( *p == '\r' || *p == '\n' ) {
				p++;
			} else {
				*o++ = *p++;
			}
		}
		*o = '\0';
	}

	return result;

}

/**
 * Extract first mail address out of From-, To, ...-Header
 * 
 * Caller must free result
 */
char *extract_address(char *header_value) {

	if( header_value == NULL )
		return NULL;

	char *result = malloc(strlen(header_value) + 1);
	strcpy(result, header_value);

	char *p = strchr(header_value, '<');
	if( p != NULL ) {
		char *e = strchr(p, '>');
		if( e != NULL ) {
			strcpy(result, p+1);
			result[e - p - 1] = '\0';
		}
	}

	//Find spaces, use token containing @
	p = result;
	while( p < (result + strlen(result)) ) {
		
		char *e = strchr(p, ' ');
		if( e == NULL ) 
			e = p + strlen(p);

		char *at = strchr(p, '@');
		if( at != NULL && at < e ) {
			strcpy(result, p);
			result[e - p] = '\0';
			break;
		}
		
		p = e+1;
	}
	
	//trim
	p = result;
	char *o = result;
	while( *p ) {
		if( strchr(" \t\r\n,;", *p) )
			p++;
		else
			*o++ = tolower(*p++); 
	}
	*o++ = '\0';

	return result;
}

/**
 * Caller must free result
 */
char* get_header_attribute(char *name, char *header_value) {

	size_t len = strlen(name);
	char find[len+2];
	strcpy(find, name);
	find[len] = '=';
	find[len+1] = '\0';

	char *p = strcasestr(header_value, find);
	if( p == NULL ) {
		return NULL;
	}

	p += strlen(find);
	char *result = malloc(strlen(p)+1);

	//Copy chars without white space and quotes
	char *o = result;
	int end_reached = 0;
	while( *p && !end_reached ) {
		switch( *p ) {
		case ' ':
		case '\r':
		case '\n':
		case '\t':
		case '"':
		case '\'':
			//Ignore
			break;

		case ';':
			//End of value
			end_reached = 1;
			break;

		default:
			*o++ = *p;
		}
		p++;
	}
	*o++ = '\0';

	return result;
}

/**
 * 
 */
void find_parts(struct message_line *message, void (*handle_part_function)(void *start, void *end), int verbose) {

	struct message_line *part_begin = NULL;

	char curr_header_name[MAX_LINE_LENGTH];
	curr_header_name[0] = '\0';

	char curr_boundary[MAX_LINE_LENGTH+3];
	curr_boundary[0] = '\0';

	int reading_headers = 1;

	struct message_line *curr = message;
	while( curr != NULL ) {

		if( reading_headers == 1 ) {

			if( verbose > 0 )
				printf("%i HEADER> %s", curr->line_number, curr->s);

			if( is_empty_line(curr->s) ) {

				if( verbose > 0 )
					printf("%i END-OF-HEADERS\n\n", curr->line_number);
				reading_headers = 0;

			} else {

				if(curr->s[0] != ' ' && curr->s[0] != '\t') {
					char *p = strchr(curr->s, ':');
					if( p != NULL ) {
						strcpy(curr_header_name, curr->s);
						curr_header_name[p - curr->s] = '\0';
						//printf("%i HEADER-NAME> \"%s\"\n", curr->line_number, curr_header_name);
					}
				}

				if( strcasecmp(curr_header_name, "Content-Type") == 0 ) {
					char *p = strcasestr(curr->s, "boundary");
					if( p != NULL ) {
						p = strstr(p, "=");
						if( p != NULL ) {
							p++;
							char *e = __last_non_whitespace(p);
							if( e != NULL ) {
								if( e[0] == ';' )
									e--;
								if( p[0] == '"' && e[0] == '"' ) {
									p++;
									e--;
								}
								strncpy(curr_boundary, p, strlen(p));
								curr_boundary[e-p+1] = '\0';

								if( verbose > 0 )
									printf("%i *BOUNDARY> %s\n", curr->line_number, curr_boundary);
							}
						}
					}
				}
			}
		} else {

			if( curr_boundary[0] != '\0' ) {

				char *p = strstr(curr->s+2, curr_boundary);
				if( p != NULL ) {

					if( part_begin != NULL ) {

						if( verbose > 0 )
							printf("%i BEGIN WITH> (%i) %s", curr->line_number, part_begin->line_number, part_begin->s);

						find_parts(part_begin, handle_part_function, verbose);
					}

					char *e = curr->s +2 + strlen(curr_boundary);
					if( strlen(e) > 1 && e[0] == '-' && e[1] == '-' ) {

						if( verbose > 0 )
							printf("%i *END> %s\n", curr->line_number, curr_boundary);

						curr_boundary[0] = '\0';

						(*handle_part_function)(part_begin, curr);

					} else if(part_begin == NULL) {

						if( verbose > 0 )
							printf("%i *FIRST> %s\n", curr->line_number, curr_boundary);

						part_begin = curr->next;
					} else {

						if( verbose > 0 )	
							printf("%i *NEXT> %s\n", curr->line_number, curr_boundary);

						(*handle_part_function)(part_begin, curr);

						part_begin = curr->next;
					}
				}
			}
		}

		curr = curr->next;
	}
}

#endif //MM_MULTIPART_PARSER
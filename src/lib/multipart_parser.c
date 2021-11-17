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
 * 
 */
char* get_header_value(char *name, struct message_line *part) {

	char *result = NULL;

	struct message_line *curr = part;
	while( curr != NULL ) {

		char *value = NULL;

		if( strncasecmp(name, curr->s, strlen(name)) == 0 ) {
			//Begin of header
			value = strstr(curr->s, ":");
			if( value != NULL ) {
				//LTrim
				while( value[0] == ':' || value[0] == ' ' )
					value++;
			}
		} else if ( result != NULL ) {
			if(curr->s[0] == ' ' || curr->s[0] == '\t')
				value = curr->s; //Next line of header
			else
				break; //Begin of next header
		}

		if( value != NULL ) {
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

		if( curr->s[0] == '\r' || curr->s[0] == '\n' || curr->s[0] == '\0' )
			break;

		curr = (struct message_line*)curr->list.next;
	}

	return result;
}

/**
 * 
 */
char* get_header_attribute(char *name, char *header_value) {

	char find[strlen(name)+2];
	strcpy(find, name);
	find[strlen(name)] = '=';
	find[strlen(name)+1] = '\0';

	char *p = strcasestr(header_value, find);
	if( p == NULL ) {
		return NULL;
	}

	char *result = malloc(strlen(p));

	//Copy chars without white space and quotes
	int o = 0;
	while( p[0] != '\0' ) {
		switch(p[0]) {
		case ' ':
		case '\r':
		case '\n':
		case '\t':
		case '"':
		case '\'':
			//Ignore
			break;
		default:
			result[o++] = p[0];
		}
		p++;
	}
	result[o++] = '\0';

	return result;
}

/**
 * 
 */
void find_parts(struct message_line *message, void (*handle_part_function)(void *start, void *end), int verbose) {

	struct message_line *part_begin = NULL;
	char curr_boundary[MAX_LINE_LENGTH+3];
	curr_boundary[0] = '\0';

	int reading_headers = 1;

	struct message_line *curr = message;
	while( curr != NULL ) {

		if( reading_headers == 1 ) {

			if( verbose > 0 )
				printf("%i HEADER> %s", curr->line_number, curr->s);

			if( curr->s[0] == '\r' || curr->s[0] == '\n' || curr->s[0] == '\0' ) {

				if( verbose > 0 )
					printf("%i END-OF-HEADERS\n\n", curr->line_number);
				reading_headers = 0;

			} else {

				char *p = strstr(curr->s, "boundary");
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

						part_begin = (struct message_line*)curr->list.next;
					} else {

						if( verbose > 0 )	
							printf("%i *NEXT> %s\n", curr->line_number, curr_boundary);

						(*handle_part_function)(part_begin, curr);

						part_begin = (struct message_line*)curr->list.next;
					}
				}
			}
		}

		curr = (struct message_line*)curr->list.next;
	}
}
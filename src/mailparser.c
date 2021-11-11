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


/// -------------- WIP: JUST A TESTING UTIL AT THE MOMENT -------------------- //

#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sysexits.h>
#include <sys/stat.h>

#include "../lib/sntools/src/lib/linked_items.c"

#define MAX_LINE_LENGTH 1024

struct message_line {
	struct linked_item list;
	char s[MAX_LINE_LENGTH];
};

/**
 * 
 */
int is_whitespace(char c) {
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
char* last_non_whitespace(char *s) {
	char *result = s + strlen(s);
	while( is_whitespace(result[0]) ) {
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
void find_parts(struct message_line *message) {

	struct message_line *part_begin = NULL;
	char curr_boundary[MAX_LINE_LENGTH+3];
	curr_boundary[0] = '\0';

	int reading_headers = 1;
	int offset = 0;

	struct message_line *curr = message;
	while( curr != NULL ) {
		
		if( reading_headers == 1 ) {
			printf("%i HEADER> %s", offset, curr->s);
			if( curr->s[0] == '\r' || curr->s[0] == '\n' || curr->s[0] == '\0' ) {
				printf("%i END-OF-HEADERS\n\n", offset);
				reading_headers = 0;
			}
		}
		
		if( curr_boundary[0] != '\0' ) {
			char *p = strstr(curr->s+2, curr_boundary);
			if( p != NULL ) {

				if( part_begin != NULL )
					find_parts(part_begin);

				char *e = curr->s +2 + strlen(curr_boundary);
				if( strlen(e) > 1 && e[0] == '-' && e[1] == '-' ) {
					printf("%i *END> %s\n", offset, curr_boundary);
					curr_boundary[0] = '\0';
				} else if(part_begin == NULL) {
					printf("%i *FIRST> %s\n", offset, curr_boundary);
					part_begin = (struct message_line*)curr->list.next;
				} else {
					printf("%i *NEXT> %s\n", offset, curr_boundary);
					part_begin = (struct message_line*)curr->list.next;
				}
			}
		} else {
			char *p = strstr(curr->s, "boundary");
			if( p != NULL ) {
				p = strstr(p, "=");
				if( p != NULL ) {
					p++;
					char *e = last_non_whitespace(p);
					if( e != NULL ) {
						if( e[0] == ';' )
							e--;
						if( p[0] == '"' && e[0] == '"' ) {
							p++;
							e--;
						}
						strncpy(curr_boundary, p, strlen(p));
						curr_boundary[e-p+1] = '\0';
						printf("%i *BOUNDARY> %s\n", offset, curr_boundary);
					}
				}
			}
		}

		curr = (struct message_line*)curr->list.next;
		offset++;
	}

	printf("%i\n", offset);
}

/**
 * 
 */
int main(int argc, char *argv[]) {

	if( argc < 2 ) {
		fprintf(stderr, "Missing arguments\n");
		exit(EX_USAGE);
	}

	char *message_file = argv[1];
	struct stat file_stat;
	if( stat(message_file, &file_stat) != 0 ) {
		fprintf(stderr, "File not found: %s\n", message_file);
		exit(EX_IOERR);
	}

	FILE *fp = fopen(message_file, "r");
	if( fp == NULL ) {
		fprintf(stderr, "Failed to open file: %s\n", message_file);
		exit(EX_IOERR);
	}

	struct message_line *message = NULL;
	struct message_line *curr_line = NULL; 
	char line[MAX_LINE_LENGTH];
	while(fgets(line, sizeof(line), fp)) {

		if( message == NULL ) {
			message = linked_item_create(NULL, sizeof(struct message_line));
			curr_line = message;
		} else {
			curr_line = linked_item_create(curr_line, sizeof(struct message_line));
		}

		strcpy(curr_line->s, line);
	}

	fclose(fp);

	if( message != NULL ) {
		find_parts(message);
	} else {
		fprintf(stderr, "Ignore empty file\n");
	}
}




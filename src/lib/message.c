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

#ifndef MM_MESSAGE
#define MM_MESSAGE

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define MAX_LINE_LENGTH 1024


/**
 * Represents one line from a message file
 */
struct message_line {
	char *s;
	int line_number;
	struct message_line *prev;
	struct message_line *next;
};

/**
 * 
 */
void message_line_set_s(struct message_line *line, char *s) {

	if( line->s != NULL ) {
		free(line->s);
	}

	if( s != NULL ) {
		size_t size = strlen(s) + 1;
		line->s = malloc(size);
		strcpy(line->s, s);
	} else {
		line->s = NULL;
	}

}

/**
 * Caller must free result using message_line_free(...)
 */
void* message_line_create(struct message_line *prev, char *s) {

	struct message_line *line = malloc(sizeof(struct message_line));
	line->s = NULL;
	line->line_number = 0;

	line->prev = NULL;
	line->next = NULL;

	if( prev != NULL ) {
		if( prev->next != NULL ) {
			line->next = prev->next;
			prev->next->prev = line;
		}
		prev->next = line;
		line->prev = prev;
	}

	message_line_set_s(line, s);

	return line;
}

/**
 * 
 */
void message_line_free(struct message_line *start) {

	struct message_line *curr = start;
	struct message_line *free_item;
	while( curr != NULL ) {

		free_item = curr;
		curr = curr->next;

		if( free_item->s != NULL )
			free(free_item->s);

		free(free_item);
	}
}

/**
 * Caller must free result using message_line_free(...)
 */
struct message_line *read_message(char *filename) {

	FILE *fp = fopen(filename, "r");
	if( fp == NULL ) {
		fprintf(stderr, "Failed to open file: %s\n", filename);
		return NULL;
	}

	struct message_line *message = NULL;
	struct message_line *curr_line = NULL; 
	char line[MAX_LINE_LENGTH];
	int line_number = 0;
	while(fgets(line, sizeof(line), fp)) {

		if( message == NULL ) {
			message = message_line_create(NULL, line);
			curr_line = message;
		} else {
			curr_line = message_line_create(curr_line, line);
		}

		curr_line->line_number = ++line_number;
	}

	fclose(fp);

	return message;
}

#endif //MM_MESSAGE
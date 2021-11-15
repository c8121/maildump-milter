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

#define MAX_LINE_LENGTH 1024


/**
 * Represents one line from a message file
 */
struct message_line {
	struct linked_item list;
	char *s;
	int line_number;
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
 * 
 */
void* message_line_create(struct message_line *prev, char *s) {

	struct message_line *line = linked_item_create(prev, sizeof(struct message_line));

	message_line_set_s(line, s);

	return line;
}

/**
 * 
 */
void __message_line_free_item(struct message_line *l) {
	if( l->s != NULL ) {
		free(l->s);
	}
}

/**
 * 
 */
void message_line_free_item(void *l) {
	__message_line_free_item(l);
}

/**
 * 
 */
void message_line_free(struct message_line *start) {
	linked_item_free(start, &message_line_free_item);
}
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

#define _GNU_SOURCE //to enable strcasestr(...)

#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sysexits.h>
#include <sys/stat.h>

#include "../lib/sntools/src/lib/linked_items.c"
#include "../lib/jouni-malinen/base64.c"
#include "./lib/qp.c"

#define MAX_LINE_LENGTH 1024

struct message_line {
	struct linked_item list;
	char s[MAX_LINE_LENGTH];
};

struct file_description {
	char content_type[255];
	char filename[255];
	char encoding[255];
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
void qp_decode_save(struct message_line *start, struct message_line *end, FILE *fp) {

	int in_header = 1;
	struct message_line *content_start = NULL;
	int out_size = 0;

	struct message_line *curr = start;
	while( curr != NULL && curr != end ) {

		if( in_header == 1 ) {
			if( curr->s[0] == '\r' || curr->s[0] == '\n' || curr->s[0] == '\0' ) {
				in_header = 0;
			}
		} else {
			if( content_start == NULL)
				content_start = curr;
			out_size += strlen(curr->s);
		}

		curr = (struct message_line*)curr->list.next;
	}

	if( content_start != NULL ) {
		char *out = malloc(out_size + 1);
		int p=0;
		curr = content_start;
		while( curr != NULL && curr != end ) {

			for( int i=0 ; i < strlen(curr->s) ; i++ )
				out[p++] = curr->s[i];

			curr = (struct message_line*)curr->list.next;
		}
		out[p++] = '\0';

		char *decoded = qp_decode(out);
		fwrite(decoded, 1, strlen(decoded), fp);

		free(decoded);
		free(out);
	}

}

/**
 * 
 */
void base64_decode_save(struct message_line *start, struct message_line *end, FILE *fp) {

	int in_header = 1;

	struct message_line *curr = start;
	while( curr != NULL && curr != end ) {

		if( in_header == 1 ) {
			if( curr->s[0] == '\r' || curr->s[0] == '\n' || curr->s[0] == '\0' ) {
				in_header = 0;
			}
		} else {

			size_t out_len = 0;
			char *out = base64_decode(curr->s, strlen(curr->s), &out_len);
			fwrite(out, 1, out_len, fp);
			free(out);
		}

		curr = (struct message_line*)curr->list.next;
	}
}

/**
 * 
 */
void undecoded_save(struct message_line *start, struct message_line *end, FILE *fp) {

	int in_header = 1;

	struct message_line *curr = start;
	while( curr != NULL && curr != end ) {

		if( in_header == 1 ) {
			printf("HEADER FROM PART WITHOUT ENCODING> %s", curr->s);
			if( curr->s[0] == '\r' || curr->s[0] == '\n' || curr->s[0] == '\0' ) {
				in_header = 0;
			}
		} else {
			fwrite(curr->s, 1, strlen(curr->s), fp);
		}

		curr = (struct message_line*)curr->list.next;
	}

}

/**
 * 
 */
void save_part(struct message_line *start, struct message_line *end, struct file_description *fd) {

	printf("Saving part to: %s\n", fd->filename);
	FILE *fp = fopen(fd->filename, "w");

	printf("ENCODING: %s\n", fd->encoding);

	if( strcasestr(fd->encoding, "quoted-printable") != NULL ) {
		qp_decode_save(start, end, fp);
	} else if( strcasestr(fd->encoding, "base64") != NULL ) {
		base64_decode_save(start, end, fp);
	} else {
		undecoded_save(start, end, fp);
	}

	fclose(fp);
}

/**
 * 
 */
char* get_header_value(char *name, struct message_line *part) {

	struct message_line *curr = part;
	while( curr != NULL ) {

		if( strncasecmp(name, curr->s, strlen(name)) == 0 ) {
			char *p = strstr(curr->s, ":");
			if( p != NULL ) {
				//LTrim
				while( p[0] == ':' || p[0] == ' ' )
					p++;
				//RTrim
				for( char *i=p+strlen(p)-1 ; (i[0] == ' ' || i[0] == '\n' || i[0] == '\r') && i >= p ; i-- )
					i[0] = '\0';
				return p;
			}
		}

		if( curr->s[0] == '\r' || curr->s[0] == '\n' || curr->s[0] == '\0' )
			return NULL;

		curr = (struct message_line*)curr->list.next;
	}

	return NULL;
}

/**
 * 
 */
struct file_description* get_file_description(struct message_line *part) {

	struct file_description *fd = malloc(sizeof(struct file_description));

	char *encoding = get_header_value("Content-Transfer-Encoding", part);
	if( encoding != NULL ) {
		strcpy(fd->encoding, encoding);
	} else {
		fd->encoding[0] = '\0';
	}

	char ext[6];
	char *content_type = get_header_value("Content-Type", part);
	if( content_type != NULL ) {
		strcpy(fd->content_type, content_type);

		if( strcasestr(content_type, "multipart/") != NULL ) {
			//Don't save multipart-container.
			free(fd);
			return NULL;
		}

		if( strcasestr(content_type, "text/plain") != NULL ) {
			strcpy(ext, "txt");
		} else if( strcasestr(content_type, "text/html") != NULL ) {
			strcpy(ext, "html");
		} else if( strcasestr(content_type, "application/pdf") != NULL ) {
			strcpy(ext, "pdf");
		} else if( strcasestr(content_type, "image/jpg") != NULL || strcasestr(content_type, "image/jpeg") != NULL ) {
			strcpy(ext, "jpg");
		} else if( strcasestr(content_type, "image/gif") != NULL ) {
			strcpy(ext, "gif");
		} else if( strcasestr(content_type, "image/png") != NULL ) {
			strcpy(ext, "png");
		} else {
			strcpy(ext, "bin");
		}
	} else {
		fd->content_type[0] = '\0';
		strcpy(ext, "bin");
	}

	sprintf(fd->filename, "/tmp/message-part.%s", ext);

	return fd;
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
					struct file_description *fd = get_file_description(part_begin);
					if( fd != NULL ) {
						save_part(part_begin, curr, fd);
						free(fd);
					}
				} else if(part_begin == NULL) {
					printf("%i *FIRST> %s\n", offset, curr_boundary);
					part_begin = (struct message_line*)curr->list.next;
				} else {
					printf("%i *NEXT> %s\n", offset, curr_boundary);
					struct file_description *fd = get_file_description(part_begin);
					if( fd != NULL ) {
						save_part(part_begin, curr, fd);
						free(fd);
					}
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
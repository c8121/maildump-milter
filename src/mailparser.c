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
#include "./lib/message.c"
#include "../lib/jouni-malinen/base64.c"
#include "./lib/qp.c"

#include "./lib/multipart_parser.c"

struct file_description {
	char content_type[255];
	char filename[255];
	char encoding[255];
};

char *output_dir = "/tmp";
int show_result_filename_only = 0;


int last_file_num = 0;

/**
 * Read command line arguments and configure application
 */
void configure(int argc, char *argv[]) {

	const char *options = "f";
	int c;

	while ((c = getopt(argc, argv, options)) != -1) {
		switch(c) {

		case 'f':
			show_result_filename_only = 1;
			break;
		}
	}
}

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
			if( is_empty_line(curr->s) ) {
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
			if( is_empty_line(curr->s) ) {
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

			if( show_result_filename_only != 1 )
				printf("%i PART WITHOUT ENCODING> %s", curr->line_number, curr->s);

			if( is_empty_line(curr->s) ) {
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

	if( show_result_filename_only != 1 )
		printf("    Saving part to: %s\n", fd->filename);

	FILE *fp = fopen(fd->filename, "w");
	if( fp == NULL ) {
		fprintf(stderr, "Failed to create file: %s\n", fd->filename);
		return;
	}

	if( show_result_filename_only != 1 )
		printf("    Encoding: %s\n", fd->encoding);

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
 * Replace content by file reference
 */
void replace_content(struct message_line *start, struct message_line *end, struct file_description *fd) {

	struct message_line *content_start = NULL;

	int in_header = 1;

	struct message_line *curr = start;
	while( curr != NULL && curr != end && content_start == NULL) {

		if( in_header == 1 ) {
			if( is_empty_line(curr->s) ) {
				in_header = 0;
			}
		} else {
			content_start = curr;
		}

		curr = (struct message_line*)curr->list.next;
	}

	if( content_start != NULL ) {

		char *ref_format = "{{REF((%s))}}\r\n";
		char reference[strlen(ref_format)+strlen(fd->filename)];
		sprintf(reference, ref_format, fd->filename);
		message_line_set_s(content_start, reference);

		struct message_line *start_free = (struct message_line*)content_start->list.next;
		struct message_line *end_free = (struct message_line*)end->list.prev;
		end_free->list.next = NULL;

		content_start->list.next = (void*)end;
		end->list.prev = (void*)content_start;

		message_line_free(start_free);
	}
}

/**
 * 
 */
void save_message(struct message_line *start) {

	char filename[1024];
	sprintf(filename, "%s/message", output_dir);

	FILE *fp = fopen(filename, "w");
	if( fp == NULL ) {
		fprintf(stderr, "Failed to create file: %s\n", filename);
		return;
	}

	struct message_line *curr = start;
	while( curr != NULL ) {
		fwrite(curr->s, 1, strlen(curr->s), fp);
		curr = (struct message_line*)curr->list.next;
	}

	fclose(fp);

	if( show_result_filename_only != 1 )
		printf("Saved parsed message: %s\n", filename);
	else
		printf("%s\n", filename);
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

		} else if( strcasestr(content_type, "application/") != NULL ) {
			strcpy(ext, "bin");
			char *name = get_header_attribute("name", content_type);
			if( name != NULL ) {
				char *p = strrchr(name, '.');
				if( p != NULL )
					strncpy(ext, p+1, 5);
			}
			free(name);
		} else {
			strcpy(ext, "bin");
		}

	} else {
		fd->content_type[0] = '\0';
		strcpy(ext, "bin");
	}

	sprintf(fd->filename, "%s/message-part.%i.%s", output_dir, ++last_file_num, ext);

	return fd;
}

/**
 * 
 */
void export_part_content(void *start, void *end) {

	struct file_description *fd = get_file_description(start);
	if( fd != NULL ) {
		save_part(start, end, fd);
		replace_content(start, end, fd);
		free(fd);
	}

}

/**
 * 
 */
int main(int argc, char *argv[]) {

	configure(argc, argv);

	if (argc - optind +1 < 2) {
		fprintf(stderr, "Missing arguments\n");
		exit(EX_USAGE);
	}

	char *message_file = argv[optind];
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

	if( message != NULL ) {
		find_parts(message, &export_part_content, show_result_filename_only == 1 ? 0 : 1);
		save_message(message);
	} else {
		fprintf(stderr, "Ignore empty file\n");
	}
}
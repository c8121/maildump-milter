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

// Counterpart to mailparser: Rebiuld a message file which was parsed by mailparser

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
#include "./lib/base64.c"
#include "./lib/qp.c"

#include "./lib/multipart_parser.c"


int show_result_filename_only = 0;

/**
 * 
 */
void usage() {
	printf("Usage: mailassembler [-f] <parsed file> <output file>\n");
}

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
void replace_base64_content(struct message_line *ref, FILE *fp) {

	struct base64_encoding_buffer *buf = malloc(sizeof(struct base64_encoding_buffer));
	buf->s = NULL;
	buf->max_line_length = 75;
	buf->line_index = 0;

	int chunk_size = 4002; //must be devidable by 3
	unsigned char chunk[chunk_size];
	int r;
	while(r = fread(chunk, 1, chunk_size, fp)) {

		base64_encode_chunk(buf, chunk, r);
	}

	if( buf->s != NULL ) {
		message_line_set_s(ref, buf->s);
		free(buf->s);
	}
	free(buf);
}

/**
 * 
 */
void replace_qp_content(struct message_line *ref, FILE *fp) {

	struct qp_encoding_buffer *buf = malloc(sizeof(struct qp_encoding_buffer));
	buf->s = NULL;
	buf->max_line_length = 75;
	buf->line_index = 0;

	unsigned char line[4096];
	while(fgets(line, sizeof(line), fp)) {

		qp_encode_chunk(buf, line);
	}

	if( buf->s != NULL ) {
		message_line_set_s(ref, buf->s);
		free(buf->s);
	}
	free(buf);
}

/**
 * 
 */
void replace_unencoded_content(struct message_line *ref, FILE *fp) {

	struct message_line *append = NULL;

	char line[MAX_LINE_LENGTH];
	while(fgets(line, sizeof(line), fp)) {

		append = append == NULL ? ref : message_line_create(append, NULL);
		message_line_set_s(append, line);

	}
}

/**
 * Replace reference by file content
 */
void replace_content(struct message_line *ref, char *encoding, char *filename) {

	struct stat file_stat;
	if( stat(filename, &file_stat) != 0 ) {
		fprintf(stderr, "File not found: %s\n", filename);
		exit(EX_IOERR);
	}

	FILE *fp = fopen(filename, "rb");
	if( fp == NULL ) {
		fprintf(stderr, "Failed to open file: %s\n", filename);
		exit(EX_IOERR);
	}

	if( show_result_filename_only != 1 ) {
		printf("    Loading part from: %s\n", filename);
		printf("    Encoding: %s\n", encoding);
	}

	if( strcasestr(encoding, "quoted-printable") != NULL ) {
		replace_qp_content(ref, fp);
	} else if( strcasestr(encoding, "base64") != NULL ) {
		replace_base64_content(ref, fp);
	} else {
		replace_unencoded_content(ref, fp);
	}

	fclose(fp);
}


/**
 * 
 */
void find_file_references(void *start, void *end) {

	char *encoding = get_header_value("Content-Transfer-Encoding", start);

	struct message_line *curr = start;
	while( curr != NULL && curr != end ) {

		if( strncmp("{{REF((", curr->s, 7) == 0 ) {
			char *e = strstr(curr->s, "))}}");
			char *s = curr->s + 7;
			char filename[e-s+1];
			strncpy(filename, s, e-s);
			filename[e-s] = '\0';
			
			replace_content(curr, encoding, filename);
		}

		curr = (struct message_line*)curr->list.next;
	}
}

/**
 * 
 */
void save_message(struct message_line *start, char *filename) {

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
int main(int argc, char *argv[]) {

	configure(argc, argv);

	if (argc - optind +1 < 3) {
		fprintf(stderr, "Missing arguments\n");
		usage();
		exit(EX_USAGE);
	}

	char *message_file = argv[optind];
	struct stat file_stat;
	if( stat(message_file, &file_stat) != 0 ) {
		fprintf(stderr, "File not found: %s\n", message_file);
		exit(EX_IOERR);
	}

	char *destination_file = argv[optind + 1];
	if( stat(destination_file, &file_stat) == 0 ) {
		fprintf(stderr, "Destination file already exists: %s\n", destination_file);
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
		find_parts(message, &find_file_references, show_result_filename_only == 1 ? 0 : 1);
		save_message(message, destination_file);
	} else {
		fprintf(stderr, "Ignore empty file\n");
	}
}
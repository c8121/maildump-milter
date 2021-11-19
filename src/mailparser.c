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
#include "./lib/base64.c"
#include "./lib/qp.c"

#include "./lib/char_util.c"
#include "./lib/multipart_parser.c"

struct file_description {

	char content_type[255];
	char original_filename[4096];	//original file from header
	char filename_suffix[512];		//suffix determindes from content-type or from original file name (txt, pdf, html...)
	char encoding[255];				//transfer encoding (base64, quoted-printable, 7bit)

	char filename[4096];			//local filename where part will be stored
	char text_filename[4096];		//local filename where text content (concerted by cat-* utils) will be stored
};

char *output_dir = "/tmp";
char *message_output_filename = "message";
char *part_output_filename_prefix = "message-part";
char *part_text_filename_prefix = "message-part-text";

int show_result_filename_only = 0;

int create_text_files = 0;
char *create_text_files_from = "html pdf doc";
char *cat_program_tpl = "%s/cat-%s \"{{input_file}}\" > \"{{output_file}}\"";

char *working_dir = NULL;
int last_file_num = 0;

/**
 * 
 */
void usage() {
	printf("Usage: mailparser [-q] [-t] [-f <output message filename/path>] [-p <output filename prefix for parts>] [-x <output filename prefix for texts>] <file>\n");
	printf("\n");
	printf("Options:\n");
	printf("    -q              Quiet: No output besides the filename of message file.\n");
	printf("\n");
	printf("    -f <file name>  Name of the output file.\n");
	printf("                    If the name contains a directory path, this dirctory path will be used to store files.\n");
	printf("\n");
	printf("    -p <prefix>     File name prefix to use for message part files.\n");
	printf("\n");
	printf("    -t              Create text files from parts containing converted text content of parts.\n");
	printf("\n");
	printf("    -x <prefix>     File name prefix to use for message text files.\n");
	printf("\n");
}

/**
 * Read command line arguments and configure application
 */
void configure(int argc, char *argv[]) {

	const char *options = "qtx:f:p:";
	int c;

	while ((c = getopt(argc, argv, options)) != -1) {
		switch(c) {

		case 'q':
			show_result_filename_only = 1;
			break;


		case 'f':
			message_output_filename = optarg;
			char *p;
			if( (p = strrchr(message_output_filename, '/')) != NULL ) {
				message_output_filename = p+1;
				output_dir = malloc(p-optarg+1);
				strncpy(output_dir, optarg, p-optarg);
			}
			break;

		case 't':
			create_text_files = 1;
			break;

		case 'p':
			part_output_filename_prefix = optarg;
			break;

		case 'x':
			part_text_filename_prefix = optarg;
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
	struct qp_decoding_buffer *buf = malloc(sizeof(struct qp_decoding_buffer));
	buf->s = NULL;
	buf->len = 0;

	struct message_line *curr = start;
	while( curr != NULL && curr != end ) {

		if( in_header == 1 ) {
			if( is_empty_line(curr->s) ) {
				in_header = 0;
			}
		} else {
			qp_decode_chunk(buf, curr->s);
		}

		curr = (struct message_line*)curr->list.next;
	}

	fwrite(buf->s, 1, strlen(buf->s), fp);

	if( buf->s != NULL)
		free(buf->s);
	free(buf);

}

/**
 * 
 */
void base64_decode_save(struct message_line *start, struct message_line *end, FILE *fp) {

	int in_header = 1;
	struct base64_decoding_buffer *buf = malloc(sizeof(struct base64_decoding_buffer));
	buf->s = NULL;
	buf->len = 0;
	buf->encoded = NULL;
	buf->encoded_len = 0;

	struct message_line *curr = start;
	while( curr != NULL && curr != end ) {

		if( in_header == 1 ) {
			if( is_empty_line(curr->s) ) {
				in_header = 0;
			}
		} else {
			base64_append_chunk(buf, curr->s, strlen(curr->s));
		}

		curr = (struct message_line*)curr->list.next;
	}

	base64_decode_chunk(buf, NULL, 0);
	fwrite(buf->s, 1, buf->len, fp);

	if( buf->s != NULL)
		free(buf->s);
	if( buf->encoded != NULL)
		free(buf->encoded);
	free(buf);
}

/**
 * 
 */
void undecoded_save(struct message_line *start, struct message_line *end, FILE *fp) {

	int in_header = 1;

	struct message_line *curr = start;
	while( curr != NULL && curr != end ) {

		if( in_header == 1 ) {
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

	FILE *fp = fopen(fd->filename, "w");
	if( fp == NULL ) {
		fprintf(stderr, "Failed to create file: %s\n", fd->filename);
		return;
	}

	if( show_result_filename_only != 1 ) {
		printf("    Saving part to: %s\n", fd->filename);
		printf("    Encoding: %s\n", fd->encoding);
	}

	if( strcasestr(fd->encoding, "quoted-printable") != NULL ) {
		qp_decode_save(start, end, fp);
	} else if( strcasestr(fd->encoding, "base64") != NULL ) {
		base64_decode_save(start, end, fp);
	} else {
		undecoded_save(start, end, fp);
	}

	fclose(fp);

	if( create_text_files ) {

		if( strcasestr(create_text_files_from, fd->filename_suffix) != NULL ) {

			if( show_result_filename_only != 1 )
				printf("    Saving text content to: %s\n", fd->text_filename);

			char cat_program[4096];
			sprintf(cat_program, cat_program_tpl, working_dir, fd->filename_suffix);

			char *command = strreplace(cat_program, "{{input_file}}", fd->filename);
			command = strreplace_free(command, "{{output_file}}", fd->text_filename);

			struct stat file_stat;
			if( system(command) !=0 || stat(fd->text_filename, &file_stat) != 0 ) {
				fprintf(stderr, "Failed to create text file: %s\n", fd->filename);
			}
		}
	}
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
		if( start_free != end ) {	
			struct message_line *end_free = (struct message_line*)end->list.prev;
			end_free->list.next = NULL;

			content_start->list.next = (void*)end;
			end->list.prev = (void*)content_start;

			message_line_free(start_free);
		}
	}
}

/**
 * 
 */
void save_message(struct message_line *start) {

	char filename[1024];
	sprintf(filename, "%s/%s", output_dir, message_output_filename);

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
	fd->filename[0] = '\0';
	fd->original_filename[0] = '\0';
	fd->filename_suffix[0] = '\0';
	fd->encoding[0] = '\0';

	char *encoding = get_header_value("Content-Transfer-Encoding", part);
	if( encoding != NULL ) {
		strcpy(fd->encoding, encoding);
	}

	char *content_type = get_header_value("Content-Type", part);
	if( content_type != NULL ) {

		strcpy(fd->content_type, content_type);

		if( strcasestr(content_type, "multipart/") != NULL ) {
			//Don't save multipart-container.
			free(fd);
			return NULL;
		}

		char *name = get_header_attribute("name", content_type);
		if( name != NULL ) {
			strcpy(fd->original_filename, name);
			char *p = strrchr(name, '.');
			if( p != NULL )
				strcpy(fd->filename_suffix, p+1);
			free(name);
		}

		if( strcasestr(content_type, "text/plain") != NULL ) {
			strcpy(fd->filename_suffix, "txt");
		} else if( strcasestr(content_type, "text/html") != NULL ) {
			strcpy(fd->filename_suffix, "html");
		} else if( strcasestr(content_type, "application/pdf") != NULL ) {
			strcpy(fd->filename_suffix, "pdf");
		} else if( strcasestr(content_type, "image/jpg") != NULL || strcasestr(content_type, "image/jpeg") != NULL ) {
			strcpy(fd->filename_suffix, "jpg");
		} else if( strcasestr(content_type, "image/gif") != NULL ) {
			strcpy(fd->filename_suffix, "gif");
		} else if( strcasestr(content_type, "image/png") != NULL ) {
			strcpy(fd->filename_suffix, "png");
		} else if ( !fd->filename_suffix ) {
			strcpy(fd->filename_suffix, "bin");
		}
	}

	if( !fd->original_filename || !fd->filename_suffix ) {
		char *content_disposition = get_header_value("Content-Disposition", part);
		if( content_disposition != NULL ) {
			char *name = get_header_attribute("filename", content_disposition);
			if( name != NULL ) {

				if( !fd->original_filename )
					strcpy(fd->original_filename, name);

				if( !fd->filename_suffix ) {
					char *p = strrchr(name, '.');
					if( p != NULL )
						strcpy(fd->filename_suffix, p+1);
				}

				free(name);
			}
		}
	}

	if( !fd->filename_suffix ) {
		strcpy(fd->filename_suffix, "bin");
	}

	//printf("****** CT(%s) OFN(%s) SF(%s)\n", fd->content_type, fd->original_filename, fd->filename_suffix);
	sprintf(fd->filename, "%s/%s.%i.%s", output_dir, part_output_filename_prefix, ++last_file_num, fd->filename_suffix);
	sprintf(fd->text_filename, "%s/%s.%i.%s", output_dir, part_text_filename_prefix, last_file_num, "txt");

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

	//Determine working directory, needed to find cat-* utils
	char *p = strrchr(argv[0], '/');
	if( p == NULL || !p ) {
		fprintf(stderr, "Failed to determine working direcotry\n");
		exit(EX_IOERR);
	}
	working_dir = malloc(strlen(argv[0]));
	strncpy(working_dir, argv[0], p-argv[0]);

	if (argc - optind +1 < 2) {
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

	struct message_line *message = read_message(message_file);
	if( message != NULL ) {
		find_parts(message, &export_part_content, show_result_filename_only == 1 ? 0 : 1);
		save_message(message);
	} else {
		fprintf(stderr, "Ignore empty file\n");
	}
}
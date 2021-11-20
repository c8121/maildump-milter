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

#include "./lib/char_util.c"
#include "../lib/sntools/src/lib/linked_items.c"
#include "./lib/message.c"
#include "./lib/multipart_parser.c"

char *add_doc_program = "curl -X POST -H 'Content-Type: application/json' 'http://localhost:8983/solr/{{collection}}/update/json/docs' --data-binary '{{json}}'";
char *add_doc_json_tpl = "./config/solr-add-mail.tpl.json";

/**
 * 
 */
void usage() {
	printf("Usage: mailindexer-solr <collection> <ID> <email file>  <text-file> [<text-files>...]\n");
}


/**
 * Caller must free result
 */
struct char_buffer *append_file(struct char_buffer *cb, char *filename) {

	FILE *fp = fopen(filename, "r");
	if( fp == NULL ) {
		fprintf(stderr, "Failed to open file: %s\n", filename);
		exit(EX_IOERR);
	}

	char line[4096];
	while(fgets(line, sizeof(line), fp)) {
		cb = strappend(cb, line, strlen(line));
	}

	fclose(fp);

	return cb;
}

/**
 * Create json-encoded value (not suitable for other purposes as chars will be deleted)
 * 
 * Caller must free result 
 */
char *json_string(char *s) {

	char *result = malloc(strlen(s)+1);

	char *p = s;
	char *o = result;
	char last = '\0';

	while( *p ) {
		switch( *p ) {
		case ' ':
		case '\t':
		case '\r':
		case '\n':
		case '\f':
			if( last != ' ' )
				*o++ = ' ';
			last = ' ';
			p++;
			break;

		case '\\':
		case '"':
		case '\'':
			//Ignore
			p++;
			break;

		default:
			if( (*p >= 21 && *p <= 47) || (*p >= 58 && *p <= 64) || (*p >= 91 && *p <= 96) || (*p >= 123 && *p <= 127) ) {
				//Don't repeat these
				if( last != *p ) {
					last = *p;
					*o++ = *p;
				}

			} else {
				last = *p;
				*o++ = *p;
			}
			p++;
			break;
		}
	}
	*o = '\0';

	return result;
}


/**
 * 
 */
int main(int argc, char *argv[]) {

	if (argc < 4) {
		fprintf(stderr, "Missing arguments\n");
		usage();
		exit(EX_USAGE);
	}

	struct stat file_stat;

	char *collection = argv[1];
	if( !collection || strchr(collection, '/') != NULL ) {
		fprintf(stderr, "Please provide a valid collection name\n");
		usage();
		exit(EX_USAGE);
	}

	char *id = argv[2];
	if( !id ) {
		fprintf(stderr, "Please provide a ID\n");
		usage();
		exit(EX_USAGE);
	} else if ( stat(id, &file_stat) == 0 ) {
		fprintf(stderr, "Provided ID is a file\n");
		usage();
		exit(EX_USAGE);
	}


	if( stat(add_doc_json_tpl, &file_stat) != 0 ) {
		fprintf(stderr, "JSON Template not found: %s\n", add_doc_json_tpl);
		exit(EX_IOERR);
	}

	struct char_buffer *json_tpl = append_file(NULL, add_doc_json_tpl);



	char *email_file = argv[3];
	if( stat(email_file, &file_stat) != 0 ) {
		fprintf(stderr, "File not found: %s\n", email_file);
		exit(EX_IOERR);
	}
	//printf("Reading metadata/fields from: %s\n", email_file);

	struct message_line *message = read_message(email_file);
	if( message == NULL ) {
		exit(EX_IOERR);
	}

	char *from = decode_header_value(get_header_value("From", message), 1, 1);
	char *to = decode_header_value(get_header_value("To", message), 1, 1);
	char *subject = decode_header_value(get_header_value("Subject", message), 1, 1);

	message_line_free(message);


	struct char_buffer *buf = NULL;
	char *text_file;
	for( int i=4 ; i < argc ; i++ ) {

		text_file = argv[i];
		if( stat(text_file, &file_stat) != 0 ) {
			fprintf(stderr, "File not found: %s\n", text_file);
			exit(EX_IOERR);
		}
		//printf("Reading text from: %s\n", text_file);

		buf = append_file(buf, text_file);
		buf = strappend(buf, " ", 1);
	}

	char *body = json_string(buf->s);

	char *json = strreplace(json_tpl->s, "{{id}}", id);
	json = strreplace(json, "{{from}}", from);
	json = strreplace(json, "{{to}}", to);
	json = strreplace(json, "{{subject}}", subject);
	json = strreplace(json, "{{body}}", body);
	printf("%s\n", json);

	char *command = strreplace(add_doc_program, "{{collection}}", collection);
	command = strreplace_free(command, "{{json}}", json);

	system(command);

	free(body);
	free(command);
	free(json);
	cb_free(json_tpl);
	cb_free(buf);
}
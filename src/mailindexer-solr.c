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

/*
 * Adds contents of a mime message file to a SOLR index. 
 * Expects the following data:
 * 
 *  - index name
 *  - message ID (hash created by archive-program)
 *  - file containing mime headers (email file)
 *  - text files containing plain text to be indexed
 */


#define _GNU_SOURCE //to enable strcasestr(...)

#define JSON_VALUE_MAX_LENGTH 32000

#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sysexits.h>
#include <sys/stat.h>

#include <bsd/stdlib.h> //required by file_util

#include "./lib/char_util.c"
#include "./lib/file_util.c"
#include "./lib/message.c"
#include "./lib/multipart_parser.c"
#include "./lib/config_file_util.c"

char *add_doc_program = "curl -s -X POST -H 'Content-Type: application/json' 'http://localhost:8983/solr/{{collection}}/update/json/docs' --data-binary '{{json}}'";
char *add_doc_json_tpl = "{{CONFDIR}}/solr-add-mail.tpl.json";

char *config_file = NULL;

int verbosity = 0;

/**
 * 
 */
void usage() {
	printf("Usage:\n");
	printf("	mailindexer-solr [-c <config file>] <collection> <ID> <email file> <text-file> [<text-files>...]\n");
	printf("\n");
	printf("Options:\n");
	printf("    -c <path> Config file.\n");
	printf("\n");
	printf("    -v        verbosity (can be repeated to increase verbosity)\n");
	printf("\n");
}

/**
 * Read command line arguments and configure application
 */
void configure(int argc, char *argv[]) {

	const char *options = "c:v";
	int c;

	while ((c = getopt(argc, argv, options)) != -1) {
		switch(c) {
			
		case 'c':
			if( *optarg ) {
				config_file = optarg;
			
				conf_dir = strcopy(config_file) ;//See file_util.c
				char *last_slash = strrchr(conf_dir, '/');
				if( last_slash != NULL ) {
					*last_slash = '\0';
				} else {
					free(conf_dir);
					conf_dir = ".";
				}
			}
			break;

		case 'v':
			verbosity++;
		}
	}
	
	// Set default conf_dir if no config file was used (see parse_path(...) on file_util.c)
	if( conf_dir == NULL )
		conf_dir = parse_path("{{BINDIR}}/../config");
	
	// Read config from file (if option 'c' is present):
	if( config_file != NULL ) {
		if( read_config(config_file) == 0 ) {
			
			set_config(&add_doc_program, "add_doc_program", 1, 1, 1, verbosity);
			set_config(&add_doc_json_tpl, "add_doc_json_tpl", 1, 1, 1, verbosity);
			
		} else {
			exit(EX_IOERR);
		}
	}
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
 * Create json-encoded value (not suitable for other purposes as chars will be deleted).
 * Modifies *s directly.
 * 
 * Retuns *s
 *  
 */
char *json_string(char *s) {

	if( s == NULL )
		return NULL;
	
	char *p = s;
	char *o = s;
	char last = '\0';

	while( *p ) {
		
		if( (o - s) > JSON_VALUE_MAX_LENGTH ) {
			*o = '\0';
			break;
		}
		
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

	return s;
}


/**
 * 
 */
int main(int argc, char *argv[]) {
	
	configure(argc, argv);

	if (argc - optind +1 < 5) {
		fprintf(stderr, "Missing arguments\n");
		usage();
		exit(EX_USAGE);
	}

	struct stat file_stat;

	char *collection = argv[optind];
	if( !collection || strchr(collection, '/') != NULL ) {
		fprintf(stderr, "Please provide a valid collection name\n");
		usage();
		exit(EX_USAGE);
	}

	char *id = argv[optind + 1];
	if( !id ) {
		fprintf(stderr, "Please provide a ID\n");
		usage();
		exit(EX_USAGE);
	} else if ( stat(id, &file_stat) == 0 ) {
		fprintf(stderr, "Provided ID is a file\n");
		usage();
		exit(EX_USAGE);
	}


	add_doc_json_tpl = parse_path(add_doc_json_tpl);
	if( stat(add_doc_json_tpl, &file_stat) != 0 ) {
		fprintf(stderr, "JSON Template not found: %s\n", add_doc_json_tpl);
		exit(EX_IOERR);
	}

	struct char_buffer *json_tpl = append_file(NULL, add_doc_json_tpl);



	char *email_file = argv[optind + 2];
	if( stat(email_file, &file_stat) != 0 ) {
		fprintf(stderr, "E-Mail file not found: %s\n", email_file);
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
	for( int i=optind + 3 ; i < argc ; i++ ) {

		text_file = argv[i];
		if( stat(text_file, &file_stat) != 0 ) {
			fprintf(stderr, "Text file not found: %s\n", text_file);
			exit(EX_IOERR);
		}
		//printf("Reading text from: %s\n", text_file);

		buf = append_file(buf, text_file);
		buf = strappend(buf, " ", 1);
	}

	char *json = strreplace(json_tpl->s, "{{id}}", id);
	json = strreplace(json, "{{from}}", json_string(from));
	json = strreplace(json, "{{to}}", json_string(to));
	json = strreplace(json, "{{subject}}", json_string(subject));
	json = strreplace(json, "{{body}}", json_string(buf->s));
	//printf("%s\n", json);

	char *command = strreplace(add_doc_program, "{{collection}}", collection);
	command = strreplace_free(command, "{{json}}", json);

	system(command);

	free(command);
	free(json);
	cb_free(json_tpl);
	cb_free(buf);
}
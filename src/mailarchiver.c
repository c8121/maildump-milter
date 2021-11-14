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

#define MAX_LINE_LENGTH 1024

char *parser_program = "./bin/mailparser -f";
char *add_to_archive_program = "./bin/archive add";
char *copy_from_archive_program = "./bin/archive copy";

struct message_line {
	struct linked_item list;
	char s[MAX_LINE_LENGTH];
};

char *output_dir = "/tmp";

void usage() {
	printf("Usage:\n");
	printf("    mailarchiver add <file>\n");
	printf("    mailarchiver get <hash> <file>\n");
	printf("\n");
	printf("Commands:\n");
	printf("    add: Add a e-mail message file to archive\n");
	printf("         Returns the ID (=hash) of the file\n");
	printf("\n");
	printf("    get: Get a e-mail by its ID (hash) from archive\n");
	printf("\n");
}

/**
 * Caller must free result
 */
char* get_file_from_archive(char *hash, char *dest_filename) {

	char command[2048];
	sprintf(command, "%s %s \"%s\"", copy_from_archive_program, hash, dest_filename);

	FILE *cmd = popen(command, "r");
	if( cmd == NULL ) {
		fprintf(stderr, "Failed to execute: %s\n", command);
		return NULL;
	}

	char *result = malloc(2048);

	char line[2048];
	while( fgets(line, sizeof(line), cmd) ) {
		//printf("HASH> %s\n", line);
		strcpy(result, line);
	}

	if( feof(cmd) ) {
		pclose(cmd);
	} else {
		fprintf(stderr, "Broken pipe: %s\n", command);
	}

	char *e = strchr(result, '\n');
	if( e != NULL ) {
		e[0] = '\0';
	}

	return result;

}

/**
 * Take archived message and retrieves reference archive files
 * Replaces hashes by file-name-references
 */
void get_parts_from_archive(struct message_line *message) {

	int file_num = 0;
	
	struct message_line *curr = message;
	while( curr != NULL ) {

		if( strncmp("{{ARCHIVE((", curr->s, 11) == 0 ) {
			char *e = strstr(curr->s, "))}}");
			char *s = curr->s + 11;
			char hash[e-s+1];
			strncpy(hash, s, e-s);
			hash[e-s] = '\0';
			
			char dest_filename[1024];
			sprintf(dest_filename, "/tmp/message-part.%i", ++file_num);

			char *filename = get_file_from_archive(hash, dest_filename);
			sprintf(curr->s, "{{REF((%s))}}\r\n", filename);
			free(filename);
		}

		curr = (struct message_line*)curr->list.next;
	}
}

/**
 * Caller must free result
 */
char* add_file_to_archive(char *filename) {

	char command[2048];
	sprintf(command, "%s \"%s\"", add_to_archive_program, filename);

	FILE *cmd = popen(command, "r");
	if( cmd == NULL ) {
		fprintf(stderr, "Failed to execute: %s\n", command);
		return NULL;
	}

	char *result = malloc(2048);

	char line[2048];
	while( fgets(line, sizeof(line), cmd) ) {
		//printf("HASH> %s\n", line);
		strcpy(result, line);
	}

	if( feof(cmd) ) {
		pclose(cmd);
	} else {
		fprintf(stderr, "Broken pipe: %s\n", command);
	}

	char *e = strchr(result, '\n');
	if( e != NULL ) {
		e[0] = '\0';
	}

	return result;

}

/**
 * Take parsed message (result of mailparser) and adds referenced files to archive
 * Replaces file-name-references by hashes
 */
void add_parts_to_archive(struct message_line *message) {

	struct message_line *curr = message;
	while( curr != NULL ) {

		if( strncmp("{{REF((", curr->s, 7) == 0 ) {
			char *e = strstr(curr->s, "))}}");
			char *s = curr->s + 7;
			char filename[e-s+1];
			strncpy(filename, s, e-s);
			filename[e-s] = '\0';

			char *hash = add_file_to_archive(filename);
			sprintf(curr->s, "{{ARCHIVE((%s))}}\r\n", hash);
			free(hash);
		}

		curr = (struct message_line*)curr->list.next;
	}
}

/**
 * Caller must free result
 */
char* save_message(struct message_line *start, char *filename) {

	FILE *fp = fopen(filename, "w");
	if( fp == NULL ) {
		fprintf(stderr, "Failed to create file: %s\n", filename);
		return NULL;
	}

	struct message_line *curr = start;
	while( curr != NULL ) {
		fwrite(curr->s, 1, strlen(curr->s), fp);
		curr = (struct message_line*)curr->list.next;
	}

	fclose(fp);

	return filename;
}

/**
 * Calls mailparser program, reads filename of created file from program output.
 * 
 * Caller must free result
 */
char* parse_message(char *filename) {

	char command[2048];
	sprintf(command, "%s \"%s\"", parser_program, filename);

	FILE *cmd = popen(command, "r");
	if( cmd == NULL ) {
		fprintf(stderr, "Failed to execute: %s\n", command);
		return NULL;
	}

	char *result = malloc(1024);

	char line[2048];
	while( fgets(line, sizeof(line), cmd) ) {
		strcpy(result, line);
	}

	if( feof(cmd) ) {
		pclose(cmd);
	} else {
		fprintf(stderr, "Broken pipe: %s\n", command);
	}

	char *e = strchr(result, '\n');
	if( e != NULL ) {
		e[0] = '\0';
	}

	return result;
}

/**
 * 
 */
void get_message(int argc, char *argv[]) {

	if( argc < 4 ) {
		fprintf(stderr, "Missing arguments\n");
		usage();
		exit(EX_USAGE);
	}

	char *hash = argv[2];
	if( strlen(hash) < 8 ) {
		fprintf(stderr, "Invliad hash\n");
		exit(EX_USAGE);
	}

	char *destination = argv[3];
	struct stat file_stat;
	if( stat(destination, &file_stat) == 0 ) {
		fprintf(stderr, "Destination file already exists: %s\n", destination);
		exit(EX_IOERR);
	}


	char *message_file = get_file_from_archive(hash, destination);

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
		get_parts_from_archive(message);

		char *tmp_filename = malloc(1024);
		sprintf(tmp_filename, "%s/archive-message", output_dir);

		char *filename = save_message(message, tmp_filename);
		if( filename != NULL ) {
			char *hash = add_file_to_archive(filename);
			printf("COPIED: %s\n", hash);
			free(hash);
		}
		free(filename);
	} else {
		fprintf(stderr, "Ignore empty file\n");
	}
}


/**
 * 
 */
void add_message(int argc, char *argv[]) {

	char *message_file = argv[2];
	struct stat file_stat;
	if( stat(message_file, &file_stat) != 0 ) {
		fprintf(stderr, "File not found: %s\n", message_file);
		exit(EX_IOERR);
	}

	char *parsed_file = parse_message(message_file);

	FILE *fp = fopen(parsed_file, "r");
	if( fp == NULL ) {
		fprintf(stderr, "Failed to open file: %s\n", parsed_file);
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
		add_parts_to_archive(message);

		char *tmp_filename = malloc(1024);
		sprintf(tmp_filename, "%s/archive-message", output_dir);

		char *filename = save_message(message, tmp_filename);
		if( filename != NULL ) {
			char *hash = add_file_to_archive(filename);
			printf("ADDED: %s\n", hash);
			free(hash);
		}
		free(filename);
	} else {
		fprintf(stderr, "Ignore empty file\n");
	}
}


/**
 * 
 */
int main(int argc, char *argv[]) {

	if( argc < 3 ) {
		fprintf(stderr, "Missing arguments\n");
		exit(EX_USAGE);
	}

	char *cmd = argv[1];

	if( strcasecmp(cmd, "add") == 0 ) {
		add_message(argc, argv);
	} else if( strcasecmp(cmd, "get") == 0 ) {
		get_message(argc, argv);
	} else {
		fprintf(stderr, "Unknown command: %s\n", cmd);
		exit(EX_USAGE);
	}

}
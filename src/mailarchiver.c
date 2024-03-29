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
 * Add & retrieve mime messages from a archive.
 * 
 * Add a message:
 * Takes a Mime message file and parses its contents.
 * The message itself and each part of the message will be added to a archive directory (using archive program).
 * Parts will be deduplicated in archive, as archiving is based on hashes created from part contents.
 * 
 * If a index name is configured, it launches a indexer program to add message and part-contents to a index for searching.
 * See mailindexer-* program(s)
 * 
 * Retrieve a message:
 * Takes a hash and looks for the archived message assigned to the hash.
 * Reassembles the message by adding all parts and saves the message to a file.
 */

#define _GNU_SOURCE //to enable strcasestr(...)

#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <dirent.h>
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

#define CONFIG_SECTION_NAME "mailarchiver"

#define MAX_LINE_LENGTH 1024


char *parser_program = "{{BINDIR}}/mailparser -q -t -x \"{{index_text_files_prefix}}\" -f \"{{output_file}}\" \"{{input_file}}\"";
char *assembler_program = "{{BINDIR}}/mailassembler -q -d \"{{input_file}}\" \"{{output_file}}\"";

char *password_file = NULL;
char *add_to_archive_program = "{{BINDIR}}/archive -c \"{{config_file}}\" -n -s \"{{suffix}}\" -p \"{{password_file}}\" add \"{{input_file}}\"";
char *copy_from_archive_program = "{{BINDIR}}/archive -c \"{{config_file}}\"  -s \"{{suffix}}\" -p \"{{password_file}}\" copy {{hash}} \"{{output_file}}\"";

char *archivemetadb_program = "{{BINDIR}}/archivemetadb -c \"{{config_file}}\" add {{hash}} \"{{subject}}\" \"{{from}}\" \"{{to}}\"";
char *empty_address_replacement = "undisclosed";

char *index_name = NULL;
char *indexer_program = "{{BINDIR}}/mailindexer-solr -c \"{{config_file}}\" {{index_name}} {{hash}} {{message_file}} {{text_files}}";

char *create_files_prefix;


char *config_file = NULL;

int verbosity = 0;

/**
 * 
 */
void usage() {
    printf("Usage:\n");
    printf("    mailarchiver [-p <password file>] add <file>\n");
    printf("    mailarchiver [-p <password file>] get <hash> <file>\n");
    printf("\n");
    printf("Commands:\n");
    printf("    add: Add a e-mail message file to archive\n");
    printf("         Returns the ID (=hash) of the file\n");
    printf("\n");
    printf("    get: Get a e-mail by its ID (hash) from archive\n");
    printf("\n");
    printf("Options:\n");
    printf("    -c <path>       Config file.\n");
    printf("\n");
    printf("    -p              Password file. Files will be encrypted when a password is provided.\n");
    printf("\n");
    printf("    -i <index name> Index name (Solr collection). Files will be fulltext-indexed when a index name is provided.\n");
    printf("\n");
    printf("    -v              verbosity (can be repeated to increase verbosity)\n");
    printf("\n");
}

/**
 * Read command line arguments and configure application
 */
void configure(int argc, char *argv[]) {

    const char *options = "c:p:i:v";
    int c;

    while ((c = getopt(argc, argv, options)) != -1) {
        switch (c) {

            case 'c':
                if (*optarg)
                    config_file = optarg;
                break;

            case 'p':
                password_file = optarg;
                struct stat file_stat;
                if (stat(password_file, &file_stat) != 0) {
                    fprintf(stderr, "Password file not found: %s\n", password_file);
                    exit(EX_IOERR);
                }
                break;

            case 'i':
                index_name = optarg;
                if (strchr(index_name, '/') != NULL) {
                    fprintf(stderr, "Invalid index name: %s\n", index_name);
                    exit(EX_IOERR);
                }
                break;

            case 'v':
                verbosity++;
        }
    }

    // Read config from file (if option 'c' is present):
    if (config_file != NULL) {
        if (read_config(config_file, CONFIG_SECTION_NAME) == 0) {

            set_config(&parser_program, "parser_program", 1, 1, 1, verbosity);
            set_config(&assembler_program, "assembler_program", 1, 1, 1, verbosity);

            set_config(&password_file, "password_file", 1, 1, 1, 0);
            set_config(&add_to_archive_program, "add_to_archive_program", 1, 1, 1, verbosity);
            set_config(&copy_from_archive_program, "copy_from_archive_program", 1, 1, 1, verbosity);

            set_config(&archivemetadb_program, "archivemetadb_program", 1, 1, 1, verbosity);
            set_config(&empty_address_replacement, "empty_address_replacement", 1, 1, 1, verbosity);

            set_config(&index_name, "index_name", 1, 1, 1, verbosity);
            set_config(&indexer_program, "indexer_program", 1, 1, 1, verbosity);

        } else {
            exit(EX_IOERR);
        }
    }
}

/**
 * Caller must free result
 */
void get_file_from_archive(char *hash, char *suffix, char *dest_filename) {

    char *command = strreplace(copy_from_archive_program, "{{hash}}", hash);
    command = strreplace_free(command, "{{suffix}}", suffix);
    command = strreplace_free(command, "{{output_file}}", dest_filename);
    if (password_file != NULL)
        command = strreplace_free(command, "{{password_file}}", password_file);
    else
        command = strreplace_free(command, "{{password_file}}", "NULL");
    command = strreplace_free(command, "{{config_file}}", config_file);
    command = parse_path_free(command);

    //printf("EXEC: %s\n", command);
    struct stat file_stat;
    if (system(command) != 0 || stat(dest_filename, &file_stat) != 0) {
        fprintf(stderr, "Failed to get file: %s\n", hash);
        exit(EX_IOERR);
    }
    free(command);
}

/**
 * Take archived message and retrieves reference archive files
 * Replaces hashes by file-name-references
 */
void get_parts_from_archive(struct message_line *message) {

    struct message_line *curr = message;
    while (curr != NULL) {

        if (strncmp("{{ARCHIVE((", curr->s, 11) == 0) {
            char *e = strstr(curr->s, "))}}");
            char *s = curr->s + 11;
            char hash[e - s + 1];
            strncpy(hash, s, e - s);
            hash[e - s] = '\0';

            char *dest_filename = temp_filename("message-part", "part");
            get_file_from_archive(hash, "", dest_filename);

            char *ref_format = "{{REF((%s))}}\r\n";
            char reference[strlen(ref_format) + strlen(dest_filename) + 1];
            sprintf(reference, ref_format, dest_filename);
            message_line_set_s(curr, reference);

            free(dest_filename);
        }

        curr = curr->next;
    }
}

/**
 * Caller must free result
 */
char *add_file_to_archive(char *filename, char *suffix) {

    char *command = strreplace(add_to_archive_program, "{{input_file}}", filename);
    command = strreplace_free(command, "{{suffix}}", suffix);
    if (password_file != NULL)
        command = strreplace_free(command, "{{password_file}}", password_file);
    else
        command = strreplace_free(command, "{{password_file}}", "NULL");
    command = strreplace_free(command, "{{config_file}}", config_file);
    command = parse_path_free(command);

    //printf("EXEC: %s\n", command);
    FILE *cmd = popen(command, "r");
    if (cmd == NULL) {
        fprintf(stderr, "Failed to execute: %s\n", command);
        return NULL;
    }

    char *result = malloc(2048);
    memset(result, '\0', 2048);

    char line[2048];
    while (fgets(line, sizeof(line), cmd)) {
        //printf("HASH> %s\n", line);
        strcpy(result, line);
    }

    if (feof(cmd)) {
        pclose(cmd);
    } else {
        fprintf(stderr, "Broken pipe: %s\n", command);
    }

    free(command);

    char *e = strchr(result, '\n');
    if (e != NULL) {
        e[0] = '\0';
    }

    return strlen(result) > 0 ? result : NULL;

}

/**
 * Take parsed message (result of mailparser) and adds referenced files to archive
 * Replaces file-name-references by hashes
 */
void add_parts_to_archive(struct message_line *message) {

    struct message_line *curr = message;
    while (curr != NULL) {

        if (strncmp("{{REF((", curr->s, 7) == 0) {
            char *e = strstr(curr->s, "))}}");
            char *s = curr->s + 7;
            char filename[e - s + 1];
            strncpy(filename, s, e - s);
            filename[e - s] = '\0';

            char *hash = add_file_to_archive(filename, "");
            if (hash == NULL) {
                fprintf(stderr, "Failed to add file to archive: %s\n", filename);
                exit(EX_IOERR);
            }

            char *ref_format = "{{ARCHIVE((%s))}}\r\n";
            char reference[strlen(ref_format) + strlen(hash) + 1];
            sprintf(reference, ref_format, hash);
            message_line_set_s(curr, reference);
            free(hash);

            unlink(filename);
        }

        curr = curr->next;
    }
}

/**
 * 
 */
void save_message(struct message_line *start, char *filename) {

    FILE *fp = fopen(filename, "w");
    if (fp == NULL) {
        fprintf(stderr, "Failed to create file: %s\n", filename);
        exit(EX_IOERR);
    }

    struct message_line *curr = start;
    while (curr != NULL) {
        fwrite(curr->s, 1, strlen(curr->s), fp);
        curr = curr->next;
    }

    fclose(fp);
}

/**
 * 
 */
void index_message(char *hash, char *message_file) {

    char *p = strrchr(message_file, '/');
    if (p == NULL) {
        fprintf(stderr, "Failed to determine directory from message file path: %s\n", message_file);
        return;
    }

    char *index_file_prefix = malloc(strlen(create_files_prefix) + 7);
    sprintf(index_file_prefix, "%s-index", create_files_prefix);

    char *dir = malloc(strlen(message_file));
    strncpy(dir, message_file, p - message_file);
    dir[p - message_file] = '\0';


    DIR *d = opendir(dir);
    if (d == NULL) {
        fprintf(stderr, "Failed to open directory: \"%s\", message file \"%s\", name \"%s\"\n", dir, message_file, p);
        return;
    }

    struct char_buffer *file_list = NULL;

    struct dirent *entry;
    while ((entry = readdir(d)) != NULL) {

        if (strstr(entry->d_name, index_file_prefix) == entry->d_name) {

            char fullname[4096];
            sprintf(fullname, "%s/%s", dir, entry->d_name);

            if (file_list != NULL)
                file_list = strappend(file_list, " ", 1);

            file_list = strappend(file_list, "\"", 1);
            file_list = strappend(file_list, fullname, strlen(fullname));
            file_list = strappend(file_list, "\"", 1);
        }

    }
    closedir(d);

    if (file_list != NULL && index_name) {

        char *command = strreplace(indexer_program, "{{index_name}}", index_name);
        command = strreplace_free(command, "{{hash}}", hash);
        command = strreplace_free(command, "{{message_file}}", message_file);
        command = strreplace_free(command, "{{text_files}}", file_list->s);
        command = strreplace_free(command, "{{config_file}}", config_file);
        command = parse_path_free(command);

        //printf("EXEC: %s\n", command);
        system(command);

        cb_free(file_list);
    }

    //Cleanup index files anyway
    d = opendir(dir);
    if (d != NULL) {
        while ((entry = readdir(d)) != NULL) {

            if (strstr(entry->d_name, index_file_prefix) == entry->d_name) {
                char fullname[4096];
                sprintf(fullname, "%s/%s", dir, entry->d_name);
                unlink(fullname);
            }

        }
        closedir(d);
    }

    free(index_file_prefix);
    free(dir);
}

/**
 * 
 */
void add_message_to_archivedb(char *hash, struct message_line *message) {

    char *from = decode_header_value(get_header_value("From", message), 1, 1);
    char *from_adr = extract_address(from);
    if (from_adr == NULL || !*from_adr) {
        fprintf(stderr, "WARN: From-Header is empty\n");
        free(from_adr);
        from_adr = strcopy(empty_address_replacement);
    }


    char *to = decode_header_value(get_header_value("To", message), 1, 1);
    char *to_adr = extract_address(to);
    if (to_adr == NULL || !*to_adr) {
        fprintf(stderr, "WARN: To-Header is empty\n");
        free(to_adr);
        to_adr = strcopy(empty_address_replacement);
    }

    char *subject = decode_header_value(get_header_value("Subject", message), 1, 1);

    char *command = strreplace(archivemetadb_program, "{{hash}}", hash);
    command = strreplace_free(command, "{{from}}", strreplace(from_adr, "\"", "\\\""));
    command = strreplace_free(command, "{{to}}", strreplace(to_adr, "\"", "\\\""));
    command = strreplace_free(command, "{{subject}}", strreplace(subject, "\"", "\\\""));
    command = strreplace_free(command, "{{config_file}}", config_file);
    command = parse_path_free(command);

    //printf("EXEC: %s\n", command);
    system(command);

    free(command);
    free(from);
    free(from_adr);
    free(to);
    free(to_adr);
    free(subject);
}


/**
 * Calls mailparser program, reads filename of created file from program output.
 * 
 */
int parse_message(char *filename, char *out_filename) {

    char *command = strreplace(parser_program, "{{input_file}}", filename);
    command = strreplace_free(command, "{{output_file}}", out_filename);
    command = parse_path_free(command);

    char index_files_prefix[512];
    sprintf(index_files_prefix, "%s-index", create_files_prefix);
    command = strreplace_free(command, "{{index_text_files_prefix}}", index_files_prefix);

    //printf("EXEC: %s\n", command);
    int r = system(command);
    free(command);

    return r;
}

/**
 * Calls mailassembler program
 * 
 */
int assemble_message(char *filename, char *out_filename) {

    char *command = strreplace(assembler_program, "{{input_file}}", filename);
    command = strreplace_free(command, "{{output_file}}", out_filename);
    command = parse_path_free(command);

    //printf("EXEC: %s\n", command);
    int r = system(command);
    free(command);

    return r;
}

/**
 * 
 */
void get_message(int argc, char *argv[]) {

    if (argc - optind + 1 < 4) {
        fprintf(stderr, "Missing arguments\n");
        usage();
        exit(EX_USAGE);
    }

    char *hash = argv[optind + 1];
    if (strlen(hash) < 8) {
        fprintf(stderr, "Invalid hash: %s\n", hash);
        exit(EX_USAGE);
    }

    char *destination = argv[optind + 2];
    struct stat file_stat;
    if (stat(destination, &file_stat) == 0) {
        fprintf(stderr, "Destination file already exists: %s\n", destination);
        exit(EX_IOERR);
    }

    char *tmp_filename = temp_filename("archive", ".msg");
    get_file_from_archive(hash, ".msg", tmp_filename);

    FILE *fp = fopen(tmp_filename, "r");
    if (fp == NULL) {
        fprintf(stderr, "Failed to open file: %s\n", tmp_filename);
        exit(EX_IOERR);
    }

    struct message_line *message = NULL;
    struct message_line *curr_line = NULL;
    char line[MAX_LINE_LENGTH];
    int line_number = 0;
    while (fgets(line, sizeof(line), fp)) {

        if (message == NULL) {
            message = message_line_create(NULL, line);
            curr_line = message;
        } else {
            curr_line = message_line_create(curr_line, line);
        }

        curr_line->line_number = ++line_number;
    }

    fclose(fp);

    if (message != NULL) {

        get_parts_from_archive(message);

        save_message(message, tmp_filename);

        assemble_message(tmp_filename, destination);

    } else {
        fprintf(stderr, "Ignore empty file\n");
    }
}


/**
 * 
 */
void add_message(int argc, char *argv[]) {

    char *message_file = argv[optind + 1];
    struct stat file_stat;
    if (stat(message_file, &file_stat) != 0) {
        fprintf(stderr, "File not found: %s\n", message_file);
        exit(EX_IOERR);
    }

    char *parsed_file = temp_filename(create_files_prefix, ".msg");
    if (parse_message(message_file, parsed_file) != 0) {
        fprintf(stderr, "Parser call failed: %s\n", message_file);
        exit(EX_IOERR);
    }

    struct message_line *message = read_message(parsed_file);
    if (message != NULL) {

        add_parts_to_archive(message);

        char *tmp_filename = temp_filename(create_files_prefix, ".msg");
        save_message(message, tmp_filename);

        char *hash = add_file_to_archive(tmp_filename, ".msg");
        if (hash != NULL) {

            add_message_to_archivedb(hash, message);
            index_message(hash, tmp_filename);

            printf("%s\n", hash);
            free(hash);

        }

        unlink(tmp_filename);
        unlink(parsed_file);

    } else {
        fprintf(stderr, "Ignore empty file\n");
    }
}


/**
 * 
 */
int main(int argc, char *argv[]) {

    configure(argc, argv);

    if (argc - optind + 1 < 3) {
        fprintf(stderr, "Missing arguments\n");
        usage();
        exit(EX_USAGE);
    }

    //Create seed for rand() used in file_util.c for example.
    srand(time(NULL));

    create_files_prefix = malloc(254);
    sprintf(create_files_prefix, "%i-%X", rand(), arc4random());

    char *cmd = argv[optind];

    if (strcasecmp(cmd, "add") == 0) {
        add_message(argc, argv);
    } else if (strcasecmp(cmd, "get") == 0) {
        get_message(argc, argv);
    } else {
        fprintf(stderr, "Unknown command: %s\n", cmd);
        usage();
        exit(EX_USAGE);
    }

}
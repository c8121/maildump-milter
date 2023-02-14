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
 * Add & retrieve files from a archive.
 * 
 * Saves files into a "storage" directory
 *
 * - Each file must extist only once, based on hash of file contents
 * - To enable incremental backups, storage is structured by direcotries on first level containig 
 *   files added in a certain period. These directories will not be touched later on. 
 * - Archive file name ist created from content-hash of the original file content,
 *   which means archive contents are deduplicated.
 *   (all first-level directories will be checked if one already contains a file before adding it) 
 */


#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <sysexits.h>
#include <sys/stat.h>

#include "./lib/char_util.c"
#include "./lib/file_util.c"
#include "./lib/config_file_util.c"

#define CONFIG_SECTION_NAME "archive"

#define STORAGE_SUBDIR_LENGTH 2
#define METADATA_FILE_EXTENSION ".meta"
#define MAX_ARCHIVE_PATH_LENGTH 4096

char *hash_program = "sha256sum -z";
//char *hash_program = "sha1sum -z";
//char *hash_program = "xxhasum";
//char *hash_program = "xxh64sum";

char *copy_program = "cp -f \"{{input_file}}\" \"{{output_file}}\"";
char *mkdir_program = "mkdir -p \"{{dirname}}\"";

char *compress_program = "gzip -c -n \"{{input_file}}\" > \"{{output_file}}\"";
char *uncompress_program = "gzip -c -d \"{{input_file}}\" > \"{{output_file}}\"";

char *password_file = NULL;
char *encode_program = "openssl enc -aes-256-cbc -e -in \"{{input_file}}\" -out \"{{output_file}}\" -pbkdf2 -pass \"file:{{password_file}}\"";
char *decode_program = "openssl enc -aes-256-cbc -d -in \"{{input_file}}\" -out \"{{output_file}}\" -pbkdf2 -pass \"file:{{password_file}}\"";

char *storage_base_dir = "/tmp/test-archive";
char *archive_file_suffix = NULL;
int save_metadata = 1;

char *config_file = NULL;

int verbosity = 0;

/**
 * 
 */
void usage() {
    printf("Usage:\n");
    printf("    archive [-c <config file>] [-v] [-b <storage base dir>] [-p <password file>] [-s <filename suffix>] [-n] add <file>\n");
    printf("    archive [-c <config file>] [-v] [-b <storage base dir>] get <hash>\n");
    printf("    archive [-c <config file>] [-v] [-b <storage base dir>] [-p <password file>] copy <hash> <file>\n");
    printf("\n");
    printf("Commands:\n");
    printf("    add: Add a file to archive\n");
    printf("         Returns the ID (=hash) of the file\n");
    printf("\n");
    printf("    get: Get a file by its ID (hash) from archive\n");
    printf("         Returns the path of the file\n");
    printf("\n");
    printf("    copy: Get a file by its ID (hash) from archive\n");
    printf("         Copy the file to another file\n");
    printf("\n");
    printf("Options:\n");
    printf("    -c <path>   Config file.\n");
    printf("\n");
    printf("    -b <dir>    Storage base directory.\n");
    printf("\n");
    printf("    -s <suffix> Archive file suffix. This suffix will be appended to the archive file.\n");
    printf("\n");
    printf("    -p <path>   Password file for encoding. Can be NULL to explicity omit encoding.\n");
    printf("\n");
    printf("    -n          No meta data: No not create/update/save meta data file along with content file.\n");
    printf("\n");
    printf("    -v          verbosity (can be repeated to increase verbosity)\n");
    printf("\n");
}

/**
 * Read command line arguments and configure application
 */
void configure(int argc, char *argv[]) {

    const char *options = "c:b:p:s:nv";
    int c;

    while ((c = getopt(argc, argv, options)) != -1) {
        switch (c) {

            case 'c':
                if (*optarg)
                    config_file = optarg;
                break;

            case 'b':
                storage_base_dir = optarg;
                break;

            case 's':
                archive_file_suffix = optarg;
                break;

            case 'p':
                password_file = optarg;
                if (strcmp(password_file, "NULL") !=
                    0) { //Passing NULL to actively omit passwort (convinient for program mailarchiver to omit passwort)
                    struct stat file_stat;
                    if (stat(password_file, &file_stat) != 0) {
                        fprintf(stderr, "Password file not found: %s\n", password_file);
                        exit(EX_IOERR);
                    }
                } else {
                    password_file = NULL;
                }
                break;

            case 'n':
                save_metadata = 0;
                break;

            case 'v':
                verbosity++;
        }
    }


    // Read config from file (if option 'c' is present):
    if (config_file != NULL) {
        if (read_config(config_file, CONFIG_SECTION_NAME) == 0) {

            set_config(&hash_program, "hash_program", 1, 1, 1, verbosity);

            set_config(&copy_program, "copy_program", 1, 1, 1, verbosity);
            set_config(&mkdir_program, "mkdir_program", 1, 1, 1, verbosity);

            set_config(&compress_program, "compress_program", 1, 1, 1, verbosity);
            set_config(&uncompress_program, "uncompress_program", 1, 1, 1, verbosity);

            set_config(&password_file, "password_file", 1, 1, 1, verbosity);
            set_config(&encode_program, "encode_program", 1, 1, 1, verbosity);
            set_config(&decode_program, "decode_program", 1, 1, 1, verbosity);

            set_config(&storage_base_dir, "storage_base_dir", 1, 1, 1, verbosity);
            set_config(&archive_file_suffix, "archive_file_suffix", 1, 1, 1, verbosity);

        } else {
            exit(EX_IOERR);
        }
    }
}

/**
 * Get a hash calculated from file contents.
 * 
 * Caller must free result 
 */
char *create_hash(char *filename) {

    char command[2048];
    sprintf(command, "%s \"%s\"", hash_program, filename);

    FILE *cmd = popen(command, "r");
    if (cmd == NULL) {
        fprintf(stderr, "Failed to execute: %s\n", command);
        return NULL;
    }

    char *result = malloc(2048);

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

    char *e = strchr(result, ' ');
    if (e != NULL) {
        e[0] = '\0';
    }

    return result;
}

/**
 * Copy a file.
 * Create destination dir if create_dir == 1
 * 
 * Return 0 on success
 */
int cp(char *source, char *dest, int create_dir) {

    char *command;
    struct stat file_stat;

    if (create_dir == 1) {
        char dirname[strlen(dest) + 1];
        strcpy(dirname, dest);
        char *p = strrchr(dirname, '/');
        if (p == NULL) {
            fprintf(stderr, "Invalid name (no dir name): %s\n", dest);
            return -1;
        }
        p[0] = '\0';


        if (stat(dirname, &file_stat) != 0) {
            command = strreplace(mkdir_program, "{{dirname}}", dirname);
            //printf("EXEC %s\n", command);
            if (system(command) == 0) {
                if (stat(dirname, &file_stat) != 0) {
                    fprintf(stderr, "Failed to create directory: %s\n", dirname);
                    return -1;
                }
            } else {
                return -1;
            }
            free(command);
        }
    }

    command = strreplace(copy_program, "{{input_file}}", source);
    command = strreplace_free(command, "{{output_file}}", dest);
    //printf("EXEC %s\n", command);
    if (system(command) == 0) {
        if (stat(dest, &file_stat) != 0)
            return -1;
        else
            return 0;
    } else {
        return -1;
    }
    free(command);
}

/**
 * Caller must free result
 */
char *encode_file(char *filename) {

    char *out_filename = filename; //Init if no encoding will be done below due to config
    char *in_filename = filename;
    char *command;
    struct stat file_stat;

    if (compress_program) {

        char *tmp_filename = temp_filename("compressed-", "");
        command = strreplace(compress_program, "{{input_file}}", in_filename);
        command = strreplace_free(command, "{{output_file}}", tmp_filename);
        //printf("EXEC %s\n", command);

        if (system(command) != 0 || stat(tmp_filename, &file_stat) != 0) {
            fprintf(stderr, "Failed to encode file: %s\n", in_filename);
            exit(EX_IOERR);
        }
        free(command);

        out_filename = tmp_filename;
        in_filename = out_filename; //Input for next encoder step
    }

    if (password_file && encode_program) {

        char *tmp_filename = temp_filename("encoded-", "");

        char *command = strreplace(encode_program, "{{input_file}}", in_filename);
        command = strreplace_free(command, "{{output_file}}", tmp_filename);
        command = strreplace_free(command, "{{password_file}}", password_file);
        //printf("EXEC %s\n", command);

        if (system(command) != 0 || stat(tmp_filename, &file_stat) != 0) {
            fprintf(stderr, "Failed to encode file: %s\n", in_filename);
            exit(EX_IOERR);
        }
        free(command);

        if (strcmp(in_filename, filename) != 0)
            unlink(in_filename); //Delete result from previous step
        out_filename = tmp_filename;
    }

    return out_filename;
}

/**
 * Caller must free result
 */
char *decode_file(char *filename) {

    char *out_filename = filename; //Init if no decoding will be done below due to config
    char *in_filename = filename;
    char *command;
    struct stat file_stat;

    if (password_file && decode_program) {

        char *tmp_filename = temp_filename("decoded-", "");

        char *command = strreplace(decode_program, "{{input_file}}", in_filename);
        command = strreplace_free(command, "{{output_file}}", tmp_filename);
        command = strreplace_free(command, "{{password_file}}", password_file);
        //printf("EXEC %s\n", command);

        if (system(command) != 0 || stat(tmp_filename, &file_stat) != 0) {
            fprintf(stderr, "Failed to encode file: %s\n", in_filename);
            exit(EX_IOERR);
        }
        free(command);

        out_filename = tmp_filename;
        in_filename = out_filename; //Input for next encoder step
    }

    if (compress_program) {

        char *tmp_filename = temp_filename("uncompressed-", "");
        command = strreplace(uncompress_program, "{{input_file}}", in_filename);
        command = strreplace_free(command, "{{output_file}}", tmp_filename);
        //printf("EXEC %s\n", command);

        if (system(command) != 0 || stat(tmp_filename, &file_stat) != 0) {
            fprintf(stderr, "Failed to encode file: %s\n", in_filename);
            exit(EX_IOERR);
        }
        free(command);

        if (strcmp(in_filename, filename) != 0)
            unlink(in_filename); //Delete result from previous step
        out_filename = tmp_filename;
    }

    return out_filename;
}


/**
 * Check storage directory
 * Exit program on failure
 */
void init_storage() {
    struct stat file_stat;
    if (stat(storage_base_dir, &file_stat) != 0) {
        fprintf(stderr, "Storage directory does not exist: %s\n", storage_base_dir);
        exit(EX_IOERR);
    }
}


/**
 * Find file path in storage of file related to given hash.
 * 
 * Caller must free result
 */
char *find_archived_file(char *hash) {

    DIR *d = opendir(storage_base_dir);
    if (d == NULL) {
        fprintf(stderr, "Failed to open directory: \"%s\"\n", storage_base_dir);
        return NULL;
    }

    char subdir[STORAGE_SUBDIR_LENGTH + 1];
    memset(subdir, '\0', STORAGE_SUBDIR_LENGTH + 1);
    for (int i = 0; i < STORAGE_SUBDIR_LENGTH; i++) {
        subdir[i] = hash[i];
    }

    char *result = NULL;
    struct stat file_stat;

    struct dirent *entry;
    while ((entry = readdir(d)) != NULL) {

        if (entry->d_name[0] == '\0' || strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char curr[strlen(storage_base_dir) + strlen(entry->d_name) + 2];
        sprintf(curr, "%s/%s", storage_base_dir, entry->d_name);
        //printf("Looking for %s in %s\n", subdir, curr);

        result = malloc(strlen(curr) + strlen(subdir) + strlen(hash) + 3 +
                        (archive_file_suffix ? strlen(archive_file_suffix) : 0));
        sprintf(result, "%s/%s/%s", curr, subdir, hash + STORAGE_SUBDIR_LENGTH);

        if (archive_file_suffix)
            strcat(result, archive_file_suffix);

        if (stat(result, &file_stat) != 0) {
            free(result);
            result = NULL;
        } else {
            break;
        }
    }
    closedir(d);

    return result;
}

/**
 * Build a full file path to store file with given hash.
 * First directory underneath storage directory reflects current time period
 * 
 * Caller must free result 
 */
char *get_archive_filename(char *hash) {

    char *result = malloc(MAX_ARCHIVE_PATH_LENGTH);
    memset(result, '\0', MAX_ARCHIVE_PATH_LENGTH);

    time_t now = time(NULL);
    char subdir[20];
    sprintf(subdir, "%lx", now / 60 / 60 / 24 / 3);

    sprintf(result,
            "%s/%s/",
            storage_base_dir,
            subdir
    );


    int n = strlen(result);
    for (int i = 0; i < STORAGE_SUBDIR_LENGTH; i++) {
        result[n + i] = hash[i];
    }

    strcat(result, "/");
    strcat(result, hash + STORAGE_SUBDIR_LENGTH);

    if (archive_file_suffix)
        strcat(result, archive_file_suffix);

    //fprintf(stderr, "ARCHIVE FILE: %s\n", result);

    return result;
}

/**
 * 
 */
void copy_from_archive(int argc, char *argv[]) {

    if (argc - optind + 1 < 4) {
        fprintf(stderr, "Missing arguments\n");
        usage();
        exit(EX_USAGE);
    }

    char *hash = argv[optind + 1];
    if (strlen(hash) <= STORAGE_SUBDIR_LENGTH) {
        fprintf(stderr, "Invliad hash\n");
        exit(EX_USAGE);
    }

    char *destination = argv[optind + 2];
    struct stat file_stat;
    if (stat(destination, &file_stat) == 0) {
        fprintf(stderr, "Destination file already exists: %s\n", destination);
        exit(EX_IOERR);
    }

    char *archive_file = find_archived_file(hash);
    if (archive_file != NULL) {

        if (decode_program || uncompress_program) {
            char *tmp_file = decode_file(archive_file);
            cp(tmp_file, destination, 0);
            unlink(tmp_file);
            free(tmp_file);
        } else {
            cp(archive_file, destination, 0);
        }

        printf("%s\n", destination);
        free(archive_file);

    } else {

        fprintf(stderr, "NOT FOUND: %s\n", hash);

    }

}

/**
 * 
 */
void get_from_archive(int argc, char *argv[]) {

    char *hash = argv[optind + 1];
    if (strlen(hash) <= STORAGE_SUBDIR_LENGTH) {
        fprintf(stderr, "Invalid hash\n");
        exit(EX_USAGE);
    }

    char *archive_file = find_archived_file(hash);
    if (archive_file != NULL) {

        printf("%s\n", archive_file);
        free(archive_file);

    } else {

        fprintf(stderr, "NOT FOUND: %s\n", hash);

    }
}

/**
 * 
 */
void add_to_archive(int argc, char *argv[]) {

    char *filename = argv[optind + 1];
    struct stat file_stat;
    if (stat(filename, &file_stat) != 0) {
        fprintf(stderr, "File not found: %s\n", filename);
        exit(EX_IOERR);
    }

    char *hash = create_hash(filename);
    if (hash == NULL || strlen(hash) < 8) {
        fprintf(stderr, "Invalid hash: '%s'\n", hash);
        exit(EX_IOERR);
    }

    char *archive_file = find_archived_file(hash);

    if (archive_file != NULL) {

        //File already exists in archive, print hash only

        printf("%s\n", hash);
        free(archive_file);

    } else {

        //File does not exist in archive

        char *source_file;
        int delete_source_file = 0;
        if (encode_program || compress_program) {
            source_file = encode_file(filename);
            delete_source_file = 1;
        } else {
            source_file = malloc(strlen(filename) + 1);
            strcpy(source_file, filename);
            delete_source_file = 0;
        }

        archive_file = get_archive_filename(hash);
        //printf("CREATE: \"%s\"\n", archive_file);

        if (cp(source_file, archive_file, 1) != 0) {

            fprintf(stderr, "Failed to copy file: \"%s\" to \"%s\"\n", filename, archive_file);

        } else {

            if (save_metadata != 0) {

                char metadata_file[strlen(archive_file) + strlen(METADATA_FILE_EXTENSION) + 1];
                sprintf(metadata_file, "%s%s", archive_file, METADATA_FILE_EXTENSION);
                //printf("CREATE: \"%s\"\n", metadata_file);

                FILE *fp = fopen(metadata_file, "w");
                if (fp == NULL) {
                    fprintf(stderr, "Failed to create meta file\n");
                    exit(EX_IOERR);
                } else {
                    fprintf(fp, "NAME: %s\n", filename);
                    fprintf(fp, "ADDED: %li\n", time(NULL));
                    fprintf(fp, "MTIME: %li\n", file_stat.st_mtime);
                    fprintf(fp, "CTIME: %li\n", file_stat.st_ctime);
                    fclose(fp);
                }
            }

            printf("%s\n", hash);
        }

        if (delete_source_file == 1)
            unlink(source_file);
        free(source_file);
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

    init_storage();

    char *cmd = argv[optind];

    if (strcasecmp(cmd, "add") == 0) {
        add_to_archive(argc, argv);
    } else if (strcasecmp(cmd, "get") == 0) {
        get_from_archive(argc, argv);
    } else if (strcasecmp(cmd, "copy") == 0) {
        copy_from_archive(argc, argv);
    } else {
        fprintf(stderr, "Unknown command: %s\n", cmd);
        exit(EX_USAGE);
    }
}
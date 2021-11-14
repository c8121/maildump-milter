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
 * Saves files into a "storage" directory
 *
 * - Each file must extist only once, based on hash of file contents
 * - To enable incremental backups, storage is structured by direcotries on first level containig 
 *   files added in a certain period. There directories will not be touched later on. 
 */


/// -------------- WIP: JUST TESTING AT THE MOMENT -------------------- //

#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <sysexits.h>
#include <sys/stat.h>

#define STORAGE_SUBDIR_LENGTH 2
#define METADATA_FILE_EXTENSION ".meta"

char *hash_program = "sha256sum -z";
//char *hash_program = "sha1sum -z";
//char *hash_program = "xxhasum";
//char *hash_program = "xxh64sum";

char *copy_program = "cp -f";
char *mkdir_program = "mkdir -p";

char *storage_base_dir = "/tmp/test-archive";
int save_metadata = 1;

void usage() {
	printf("Usage:\n");
	printf("    archive add <file>\n");
	printf("    archive get <hash>\n");
	printf("    archive copy <hash> <file>\n");
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

	char *e = strchr(result, ' ');
	if( e != NULL ) {
		e[0] = '\0';
	}

	return result;
}


/**
 * Execute a command, ignore output.
 * 
 * Return 0 on success
 */
int exec_command(char *command) {

	FILE *cmd = popen(command, "r");
	if( cmd == NULL ) {
		fprintf(stderr, "Failed to execute: %s\n", command);
		return -1;
	}
	//printf("EXEC> %s\n", command);

	char line[1024];
	while( fgets(line, sizeof(line), cmd) ) {
		//printf("%s", line);
	}

	if( feof(cmd) ) {
		pclose(cmd);
		return 0;
	} else {
		fprintf(stderr, "Broken pipe: %s\n", command);
		return -1;
	}
}

/**
 * Copy a file, create destination dir if create_dir == 1
 * 
 * Return 0 on success
 */
int cp(char *source, char *dest, int create_dir) {

	char command[4096];
	struct stat file_stat;

	if( create_dir == 1 ) {
		char dirname[strlen(dest)+1];
		strcpy(dirname, dest);
		char *p = strrchr(dirname, '/');
		if( p == NULL ) {
			fprintf(stderr, "Invalid name (no dir name): %s\n", dest);
			return -1;
		}

		p[0] = '\0';
		sprintf(command, "%s \"%s\"", mkdir_program, dirname);
		if( exec_command(command) == 0 ) {
			if( stat(dirname, &file_stat) != 0 ) {
				fprintf(stderr, "Failed to create directory: %s\n", dirname);
				return -1;
			}
		} else {
			return -1;
		}
	}


	sprintf(command, "%s \"%s\" \"%s\"", copy_program, source, dest);
	if( exec_command(command) == 0 ) {
		if( stat(dest, &file_stat) != 0 )
			return -1;
		else
			return 0;
	} else {
		return -1;
	}
}

/**
 * Check storage directory
 * Exit program on failure
 */
void init_storage() {
	struct stat file_stat;
	if( stat(storage_base_dir, &file_stat) != 0 ) {
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
	if( d == NULL ) {
		fprintf(stderr, "Failed to open directory: \"%s\"\n", storage_base_dir);
		return NULL;
	}

	char subdir[STORAGE_SUBDIR_LENGTH+1];
	memset(subdir, '\0', STORAGE_SUBDIR_LENGTH+1);
	for(int i=0 ; i < STORAGE_SUBDIR_LENGTH ; i++ ) {
		subdir[i] = hash[i];
	}

	char *result = NULL;
	struct stat file_stat;

	struct dirent *entry;
	while ((entry = readdir(d)) != NULL) {

		if( entry->d_name[0] == '\0' || strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0 )
			continue;

		char curr[strlen(storage_base_dir) + strlen(entry->d_name) + 2];
		sprintf(curr, "%s/%s", storage_base_dir, entry->d_name);
		//printf("Looking for %s in %s\n", subdir, curr);

		result = malloc(strlen(curr) + strlen(subdir) + strlen(hash) + 3);
		sprintf(result, "%s/%s/%s", curr, subdir, hash + STORAGE_SUBDIR_LENGTH);

		if( stat(result, &file_stat) != 0 ) {
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

	char *result = malloc(256);
	memset(result, '\0', 256);

	strcat(result, storage_base_dir);
	strcat(result, "/");

	time_t now = time(NULL);
	char subdir[20];
	sprintf(subdir, "%lx", now / 60 / 60 / 24 / 3);


	strcat(result, subdir);
	strcat(result, "/");

	int n = strlen(result);
	for(int i=0 ; i < STORAGE_SUBDIR_LENGTH ; i++ ) {
		result[n+i] = hash[i];
	}

	strcat(result, "/");
	strcat(result, hash+STORAGE_SUBDIR_LENGTH);

	return result;
}

/**
 * 
 */
void copy_from_archive(int argc, char *argv[]) {

	if( argc < 4 ) {
		fprintf(stderr, "Missing arguments\n");
		usage();
		exit(EX_USAGE);
	}

	char *hash = argv[2];
	if( strlen(hash) <= STORAGE_SUBDIR_LENGTH ) {
		fprintf(stderr, "Invliad hash\n");
		exit(EX_USAGE);
	}

	char *destination = argv[3];
	struct stat file_stat;
	if( stat(destination, &file_stat) == 0 ) {
		fprintf(stderr, "Destination file already exists: %s\n", destination);
		exit(EX_IOERR);
	}

	char *archive_file = find_archived_file(hash);
	if( archive_file != NULL ) {

		cp(archive_file, destination, 0);
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

	char *hash = argv[2];
	if( strlen(hash) <= STORAGE_SUBDIR_LENGTH ) {
		fprintf(stderr, "Invliad hash\n");
		exit(EX_USAGE);
	}

	char *archive_file = find_archived_file(hash);
	if( archive_file != NULL ) {

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

	char *filename = argv[2];
	struct stat file_stat;
	if( stat(filename, &file_stat) != 0 ) {
		fprintf(stderr, "File not found: %s\n", filename);
		exit(EX_IOERR);
	}
	//printf("FILE: \"%s\"\n", filename);

	char *hash = create_hash(filename);
	if( hash == NULL || strlen(hash) < 8 ) {
		fprintf(stderr, "Invalid hash: '%s'\n", hash);
		exit(EX_IOERR);
	}
	//printf("HASH: \"%s\"\n", hash);

	char *archive_file = find_archived_file(hash);
	if( archive_file != NULL ) {

		printf("%s\n", hash);
		free(archive_file);

	} else {

		archive_file = get_archive_filename(hash);
		//printf("CREATE: \"%s\"\n", archive_file);
		if( cp(filename, archive_file, 1) != 0 ) {
			fprintf(stderr, "Failed to copy file: \"%s\" to \"%s\"\n", filename, archive_file);
		} else {

			if( save_metadata != 0 ) {

				char metadata_file[strlen(archive_file) + strlen(METADATA_FILE_EXTENSION)+1];
				sprintf(metadata_file, "%s%s", archive_file, METADATA_FILE_EXTENSION);
				//printf("CREATE: \"%s\"\n", metadata_file);

				FILE *fp = fopen(metadata_file, "w");
				if( fp == NULL ) {
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
	}
}

/**
 * 
 */
int main(int argc, char *argv[]) {

	if( argc < 3 ) {
		fprintf(stderr, "Missing arguments\n");
		usage();
		exit(EX_USAGE);
	}

	init_storage();

	char *cmd = argv[1];

	if( strcasecmp(cmd, "add") == 0 ) {
		add_to_archive(argc, argv);
	} else if( strcasecmp(cmd, "get") == 0 ) {
		get_from_archive(argc, argv);
	} else if( strcasecmp(cmd, "copy") == 0 ) {
		copy_from_archive(argc, argv);
	} else {
		fprintf(stderr, "Unknown command: %s\n", cmd);
		exit(EX_USAGE);
	}
}
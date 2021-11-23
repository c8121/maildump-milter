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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>
#include <time.h>

#include "./lib/db_archive.c"

#define EX_HASH_EXIST 1
#define EX_HASH_FAILED 2
#define EX_ORIGIN_FAILED 3

char *db_host = "localhost";
unsigned int db_port = 0;
char *db_user = "archive";
char *db_pwd = NULL;
char *db_name = "archive";

#include "./lib/mysql_util.c"

char c_time[20] = "";
char m_time[20] = "";

/**
 * 
 */
void usage() {
	printf("Usage:\n");
	printf("    archivemeta add <hash> <origin> <owner>\n");

}

/**
 * Read command line arguments and configure application
 */
void configure(int argc, char *argv[]) {

	const char *options = "c:m:";
	int c;

	while ((c = getopt(argc, argv, options)) != -1) {
		switch(c) {

		case 'c':
			if(strlen(optarg) > (sizeof(c_time) - 1)) {
				fprintf(stderr, "Invalid ctime\n");
				exit(EX_USAGE);
			}
			strcpy(c_time, optarg);
			break;

		case 'm':
			if(strlen(optarg) > (sizeof(m_time) - 1)) {
				fprintf(stderr, "Invalid mtime\n");
				exit(EX_USAGE);
			}
			strcpy(m_time, optarg);
			break;
		}
	}
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

	char *cmd = argv[optind];

	char *hash = argv[optind + 1];
	if( !*hash ) {
		fprintf(stderr, "Hash cannot be empty\n");
		usage();
		exit(EX_USAGE);
	}

	char *origin = argv[optind + 2];
	if( !*origin ) {
		fprintf(stderr, "Origin cannot be empty\n");
		usage();
		exit(EX_USAGE);
	}

	char *owner = argv[optind + 3];
	if( !*owner ) {
		fprintf(stderr, "Owner cannot be empty\n");
		usage();
		exit(EX_USAGE);
	}

	if( strcasecmp(cmd, "add") == 0 ) {

		int exit_c = 0;
		
		db_open();

		struct a_entry *e = malloc(sizeof(struct a_entry));
		if( get_entry(hash, e) == 0 ) {

			fprintf(stderr, "Hash already exists: ID=%li, NAME=%s\n", e->id, e->name);
			exit_c = EX_HASH_EXIST;

		} else {

			unsigned long id = add_entry(hash, origin);
			if( id == 0 ) {

				fprintf(stderr, "Failed to add hash (%li)\n", id);
				exit_c = EX_HASH_FAILED;

			} else {

				printf("Added Entry: ID=%li\n", id);

				if( !c_time[0] || !m_time[0] ) {

					time_t secs = time(NULL);
					struct tm *local = localtime(&secs);

					if( !c_time[0] )
						sprintf(c_time, "%04d-%02d-%02d %02d:%02d:%02d", local->tm_year+1900, local->tm_mon+1, local->tm_mday, local->tm_hour, local->tm_min, local->tm_sec);

					if( !m_time[0] )
						sprintf(m_time, "%04d-%02d-%02d %02d:%02d:%02d", local->tm_year+1900, local->tm_mon+1, local->tm_mday, local->tm_hour, local->tm_min, local->tm_sec);
				}

				unsigned long origin_id = add_entry_origin(id, origin, owner, c_time, m_time);
				if( origin_id == 0 ) {

					fprintf(stderr, "Failed to add origin (%li)\n", origin_id);
					exit_c = EX_ORIGIN_FAILED;

				} else {

					printf("Added Origin: ID=%li\n", origin_id);

				}
			}
		}

		db_close_all();

		exit(exit_c);

	} else {
		fprintf(stderr, "Unknown command: %s\n", cmd);
		exit(EX_USAGE);
	}

}

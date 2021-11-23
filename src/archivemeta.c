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

#include "./lib/db_archive.c"

char *db_host = "localhost";
unsigned int db_port = 0;
char *db_user = "archive";
char *db_pwd = NULL;
char *db_name = "archive";

#include "./lib/mysql_util.c"


/**
 * 
 */
void usage() {
	printf("Usage:\n");
	printf("    archivemeta add <hash> <origin> <owner>\n");

}

/**
 * 
 */
int main(int argc, char *argv[]) {

	if( argc < 5 ) {
		fprintf(stderr, "Missing arguments\n");
		usage();
		exit(EX_USAGE);
	}

	char *cmd = argv[1];

	char *hash = argv[2];
	if( !*hash ) {
		fprintf(stderr, "Hash cannot be empty\n");
		usage();
		exit(EX_USAGE);
	}

	char *origin = argv[3];
	if( !*origin ) {
		fprintf(stderr, "Origin cannot be empty\n");
		usage();
		exit(EX_USAGE);
	}

	char *owner = argv[4];
	if( !*owner ) {
		fprintf(stderr, "Owner cannot be empty\n");
		usage();
		exit(EX_USAGE);
	}

	if( strcasecmp(cmd, "add") == 0 ) {

		db_open();

		struct a_entry *e = malloc(sizeof(struct a_entry));
		if( get_entry(hash, e) == 0 ) {
			
			fprintf(stderr, "Hash already exists: ID=%li, NAME=%s\n", e->id, e->name);
			
		} else {
			
			unsigned long id = add_entry(hash, origin);
			if( id <= 0 ) {
				
				fprintf(stderr, "Failed to add hash (%li)\n", id);
				
			} else {
				
				printf("Added Entry: ID=%li\n", id);
				
				unsigned long origin_id = add_entry_origin(id, origin, owner);
				if( origin_id <= 0 ) {
					
					fprintf(stderr, "Failed to add origin (%li)\n", origin_id);
					
				} else {
					
					printf("Added Origin: ID=%li\n", origin_id);
					
				}
			}
		}

		db_close_all();


	} else {
		fprintf(stderr, "Unknown command: %s\n", cmd);
		exit(EX_USAGE);
	}

}

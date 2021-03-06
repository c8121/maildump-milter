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
 * Store meta data into a database (MariaDB)
 * 
 * Primary key is a hash (sha256 for example)
 * 
 * Following information will be stored as meta data to a hash:
 * 
 *  - name: A arbitrary name (an original file name or the subject of a mail message)
 *  - origin: Describes the origin (can be the original file path or the sender of a mail message)
 *  - owner: Name (or id) of the original owner (or the recipient of a mail message) 
 *  - create time (optional, via option 'c', format "yyyy-mm-dd hh:mm:ss")
 *  - modified time (optional, via option 'm', format "yyyy-mm-dd hh:mm:ss")
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>
#include <time.h>

#include "./lib/db_archive.c"
#include "./lib/config_file_util.c"

#define CONFIG_SECTION_NAME "mailindexer"

#define EX_HASH_EXIST 1
#define EX_HASH_FAILED 2
#define EX_OWNER_FAILED 3
#define EX_ORIGIN_FAILED 4
#define EX_ENTRY_ORIGIN_FAILED 5

#define MAX_LEN_NAME 254
#define MAX_LEN_OWNER 254

char *db_host = "localhost";
unsigned int db_port = 0;
char *db_user = "archive";
char *db_pwd = NULL;
char *db_name = "archive";

#include "./lib/char_util.c"
#include "./lib/mysql_util.c"
#include "./lib/mysql_query_util.c"

char c_time[20] = "";
char m_time[20] = "";

char *config_file = NULL;

int verbosity = 0;

/**
 * 
 */
void usage() {
	printf("Usage:\n");
	printf("	archivemetadb add [-c <config file>] [-v] [-t <created datetime>] [-m <modified datetime>] <hash> <name> <origin> <owner>\n");
	printf("\n");
	printf("	archivemetadb find [-c <config file>] [-v] <term>\n");
	printf("		Term syntax:\n");
	printf("			\"FIELD VALUE\" (valid fields: HASH, NAME, ORIGIN, OWNER)\n");
	printf("			Asterisk can be use at end and beginning of value:\n");
	printf("			\"FIELD VALUE*\": Field begins with value\n");
	printf("			\"FIELD *VALUE\": Field ends with value\n");
	printf("			\"FIELD *VALUE*\": Field contains value\n");
	printf("			Example:\n");
	printf("			\"ORIGIN chris*\"\n");
	printf("\n");
	printf("Options:\n");
	printf("    -c <path>                 Config file.\n");
	printf("\n");
	printf("    -t <yyyy-mm-ss hh:mm:ss>  Create time to be stored\n");
	printf("\n");
	printf("    -m <yyyy-mm-ss hh:mm:ss>  Modified time to be stored\n");
	printf("\n");
	printf("    -v                        verbosity (can be repeated to increase verbosity)\n");
	printf("\n");

}

/**
 * Read command line arguments and configure application
 */
void configure(int argc, char *argv[]) {

	const char *options = "c:t:m:v";
	int c;

	while ((c = getopt(argc, argv, options)) != -1) {
		switch(c) {
			
		case 'c':
			if( *optarg ) 
				config_file = optarg;
			break;

		case 't':
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
			
		case 'v':
			verbosity++;
		}
	}
	
	// Read config from file (if option 'c' is present):
	if( config_file != NULL ) {
		if( read_config(config_file, CONFIG_SECTION_NAME) == 0 ) {
			
			set_config(&db_host, "db_host", 1, 1, 1, verbosity);
			set_config_uint(&db_port, "db_port", verbosity);
			set_config(&db_user, "db_user", 1, 1, 1, verbosity);
			set_config(&db_pwd, "db_pwd", 1, 1, 1, 0);
			set_config(&db_name, "db_name", 1, 1, 1, verbosity);
			
		} else {
			exit(EX_IOERR);
		}
	}
}

/**
 * Add or update an entry
 */
int add(char *hash, char *name, char *origin, char *owner) {
	
	struct a_entry *e = malloc(sizeof(struct a_entry));
	memset(e, 0, sizeof(struct a_entry));
	strcpy(e->hash, hash);
	strcpy(e->name, name);
	

	if( get_entry(e) == 0 ) {

		fprintf(stderr, "Entry: ID=%li, HASH=%s, NAME=%s\n", e->id, e->hash, e->name);
		return EX_HASH_EXIST;
		
		//TODO: Add ORIGIN and/or OWNER if different

	} else {

		if( add_entry(e) != 0 ) {

			fprintf(stderr, "Failed to add entry\n");
			return EX_HASH_FAILED;

		} else {

			printf("Entry: ID=%li, HASH=%s, NAME=%s\n", e->id, e->hash, e->name);

			struct a_owner *o = malloc(sizeof(struct a_owner));
			memset(o, 0, sizeof(struct a_owner));
			strcpy(o->name, owner);

			if( get_create_owner(o) != 0 ) {

				fprintf(stderr, "Cannot get or create owner\n");
				return EX_OWNER_FAILED;

			} else {

				//printf("Owner: ID=%li, NAME=%s\n", o->id, o->name);

				struct a_origin *or = malloc(sizeof(struct a_origin));
				memset(or, 0, sizeof(struct a_origin));
				strcpy(or->name, origin);

				if( get_create_origin(or) != 0 ) {

					fprintf(stderr, "Cannot get or create origin\n");
					return EX_ORIGIN_FAILED;

				} else {

					struct a_entry_origin *eo = malloc(sizeof(struct a_entry_origin));
					memset(eo, 0, sizeof(struct a_entry_origin));
					eo->entry_id = e->id;
					eo->owner_id = o->id;
					eo->origin_id = or->id;

					if( !c_time[0] || !m_time[0] ) {

						time_t secs = time(NULL);
						struct tm *local = localtime(&secs);

						if( !c_time[0] )
							sprintf(eo->c_time, "%04d-%02d-%02d %02d:%02d:%02d", local->tm_year+1900, local->tm_mon+1, local->tm_mday, local->tm_hour, local->tm_min, local->tm_sec);
						else
							strcpy(eo->c_time, c_time);

						if( !m_time[0] )
							sprintf(eo->m_time, "%04d-%02d-%02d %02d:%02d:%02d", local->tm_year+1900, local->tm_mon+1, local->tm_mday, local->tm_hour, local->tm_min, local->tm_sec);
						else
							strcpy(eo->m_time, m_time);

					} else {

						strcpy(eo->c_time, c_time);
						strcpy(eo->m_time, m_time);

					}

					if( add_entry_origin(eo) != 0 ) {

						fprintf(stderr, "Failed to add origin\n");
						return EX_ENTRY_ORIGIN_FAILED;

					} else {

						//printf("Origin: ID=%li, ORIGIN=%s, CTIME=%s, MTIME=%s\n", eo->id, eo->origin, eo->c_time, eo->m_time);
						return 0;
					}
				}
			}
		}
	}
}


/**
 * 
 */
int find(char *term) {
	
	struct query_filter *filter = create_query_filter_from_term(term);
	
	// Pre check valid names
	struct query_filter *f = filter;
	while( f != NULL ) {
		
		if( strcasecmp(f->name, "OWNER") == 0 || strcasecmp(f->name, "ORIGIN") == 0 ) {
			
			char *tmp = malloc(strlen(f->name)+6);
			sprintf(tmp, "%s.NAME", f->name);
			free(f->name);
			f->name = tmp;
			
		} else if( strcasecmp(f->name, "NAME") == 0 || strcasecmp(f->name, "HASH") == 0 ) {
			
			char *tmp = malloc(strlen(f->name)+7);
			sprintf(tmp, "ENTRY.%s", f->name);
			free(f->name);
			f->name = tmp;
			
		} else {
			
			fprintf(stderr, "Invalid column name: \"%s\"\n", f->name);
			return EX_USAGE;
		}
		
		f = f->next;
	}
	
	
	mysql_print_query(
	
			"?\n"
			"   NAME: ?\n"
			" ORIGIN: ?\n"
			"  OWNER: ?\n",
			
			"SELECT "
				"ENTRY.HASH, ENTRY.NAME, "
				"ORIGIN.NAME AS ORIGIN, "
				"OWNER.NAME AS OWNER "
						"FROM ((ENTRY_ORIGIN "
							"INNER JOIN ENTRY ON ENTRY_ORIGIN.ENTRY=ENTRY.ID) "
							"INNER JOIN ORIGIN ON ENTRY_ORIGIN.ORIGIN=ORIGIN.ID) "
							"INNER JOIN OWNER ON ENTRY_ORIGIN.OWNER=OWNER.ID "
					"WHERE ?;", 
			filter
	);
	
	return 0;
}


/**
 * 
 */
int main(int argc, char *argv[]) {

	configure(argc, argv);

	if (optind >= argc) {
		fprintf(stderr, "Missing command\n");
		usage();
		exit(EX_USAGE);
	}

	char *cmd = argv[optind];

	if( strcasecmp(cmd, "find") == 0 ) {
		
		if (argc - optind +1 < 3) {
			fprintf(stderr, "Missing arguments\n");
			usage();
			exit(EX_USAGE);
		}
		
		char *term = argv[optind + 1];
		if( !*term ) {
			fprintf(stderr, "Term cannot be empty\n");
			usage();
			exit(EX_USAGE);
		}
		
		db_open();
		
		int exit_c = find(term);
		
		db_close_all();
		
		exit(exit_c);
		
	
	} else if( strcasecmp(cmd, "add") == 0 ) {
		
		if (argc - optind +1 < 6) {
			fprintf(stderr, "Missing arguments\n");
			usage();
			exit(EX_USAGE);
		}
		
		char *hash = argv[optind + 1];
		if( !*hash ) {
			fprintf(stderr, "Hash cannot be empty\n");
			usage();
			exit(EX_USAGE);
		}

		char *name = argv[optind + 2];
		if( strlen(name) > MAX_LEN_NAME ) {
			fprintf(stderr, "WARN: Name to long, truncate\n");
			*(name + MAX_LEN_NAME) = '\0';
		}

		char *origin = argv[optind + 3];
		if( !*origin ) {
			fprintf(stderr, "Origin cannot be empty\n");
			usage();
			exit(EX_USAGE);
		}

		char *owner = argv[optind + 4];
		if( !*owner ) {
			fprintf(stderr, "Owner cannot be empty\n");
			usage();
			exit(EX_USAGE);
		} else if( strlen(owner) > MAX_LEN_OWNER ) {
			fprintf(stderr, "WARN: Owner to long, truncate\n");
			*(owner + MAX_LEN_OWNER) = '\0';
		}

		db_open();

		int exit_c = add(hash, name, origin, owner);

		db_close_all();

		exit(exit_c);

	} else {
		fprintf(stderr, "Unknown command: %s\n", cmd);
		exit(EX_USAGE);
	}

}

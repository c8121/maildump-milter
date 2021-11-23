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

char *db_host = "localhost";
unsigned int db_port = 0;
char *db_user = "archive";
char *db_pwd = NULL;
char *db_name = "archive";

#include "./lib/mysql_util.c"



/**
 * 
 */
int get_entry(char *hash) {

	if( db_get_entry_stmt == NULL ) {
		db_get_entry_stmt = mysql_stmt_init(db);
		int r = mysql_stmt_prepare(
				db_get_entry_stmt,
				db_get_entry_sql,
				strlen(db_get_entry_sql)
		);
		if( r != 0 ) {
			fprintf(stderr, "%s\n", db_error(db));
			exit(EX_IOERR);
		}
	}

	MYSQL_BIND p[1];
	memset(p, 0, sizeof(p));

	unsigned long hash_length = strlen(hash);
	p[0].buffer_type = MYSQL_TYPE_STRING;
	p[0].buffer = hash;
	p[0].length = &hash_length;
	p[0].is_null = 0;

	if( mysql_stmt_bind_param(db_get_entry_stmt, p) != 0 ) {
		fprintf(stderr, "%s\n", db_error(db));
		return -1;
	}

	if( mysql_stmt_execute(db_get_entry_stmt) != 0 ) {
		fprintf(stderr, "%s\n", db_error(db));
		return -1;
	}

	MYSQL_BIND r[1];
	memset(r, 0, sizeof(r));

	char name[4096];
	r[0].buffer_type = MYSQL_TYPE_STRING;
	r[0].buffer = &name;
	r[0].buffer_length = 4096;

	if( mysql_stmt_bind_result(db_get_entry_stmt, r) != 0 ) {
		fprintf(stderr, "%s\n", db_error(db));
		return -1;
	}

	if( mysql_stmt_store_result(db_get_entry_stmt) != 0 ) {
		fprintf(stderr, "%s\n", db_error(db));
		return -1;
	}

	int s;
	while( 1 ) {
		s = mysql_stmt_fetch(db_get_entry_stmt);
		if( s == MYSQL_NO_DATA || s != 0 )
			break;

		printf(" - %s NAME=\"%s\"\n", hash, name);
	}

	mysql_stmt_free_result(db_get_entry_stmt);

	return 0;
}

/**
 * 
 */
int add_entry(char *hash, char *name) {

	if( db_add_entry_stmt == NULL ) {
		db_add_entry_stmt = mysql_stmt_init(db);
		int r = mysql_stmt_prepare(
				db_add_entry_stmt,
				db_add_entry_sql,
				strlen(db_add_entry_sql)
		);
		if( r != 0 ) {
			fprintf(stderr, "%s\n", db_error(db));
			exit(EX_IOERR);
		}
	}

	MYSQL_BIND p[2];
	memset(p, 0, sizeof(p));

	unsigned long hash_length = strlen(hash);
	p[0].buffer_type = MYSQL_TYPE_STRING;
	p[0].buffer = hash;
	p[0].length = &hash_length;
	p[0].is_null = 0;

	unsigned long name_length = strlen(name);
	p[1].buffer_type = MYSQL_TYPE_STRING;
	p[1].buffer = name;
	p[1].length = &name_length;
	p[1].is_null = 0;

	if( mysql_stmt_bind_param(db_add_entry_stmt, p) != 0 ) {
		fprintf(stderr, "%s\n", db_error(db));
		return -1;
	}

	if( mysql_stmt_execute(db_add_entry_stmt) != 0 ) {
		fprintf(stderr, "%s\n", db_error(db));
		return -1;
	}

	mysql_stmt_free_result(db_add_entry_stmt);

	return 0;
}


/**
 * 
 */
int main(int argc, char *argv[]) {

	db_open();

	add_entry("hash", "test");
	get_entry("hash");

	db_close_all();


}

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

#include <mysql/mysql.h>

void *db;

char *db_get_entry_sql = "SELECT ID, NAME FROM ENTRY WHERE HASH=?;";
MYSQL_STMT *db_get_entry_stmt = NULL;

char *db_add_entry_sql = "INSERT INTO ENTRY(HASH, NAME) VALUES(?,?);";
MYSQL_STMT *db_add_entry_stmt = NULL;


/**
 * 
 */
const char *db_error(void *c) {
	return mysql_error(c);
}

/**
 *
 */
MYSQL *db_connect(char *host, char *user, char *pwd, char *db, unsigned int port) {

	MYSQL *c = mysql_init(NULL);
	if( c == NULL ) {
		fprintf(stderr, "%s\n", mysql_error(c));
		return NULL;
	}

	if( mysql_real_connect(c, host, user, pwd, db, port, NULL, 0) == 0 )  {
		fprintf(stderr, "%s\n", mysql_error(c));
		mysql_close(c);
		return NULL;
	}

}



/**
 * 
 */
void db_open() {
	db = db_connect(db_host, db_user, db_pwd, db_name, db_port);
	if( db == NULL )
		exit(EX_IOERR);
}

/**
 * 
 */
void db_close(void *c) {
	mysql_close(c);
}

/**
 * 
 */
void db_close_all() {

	if( db_add_entry_stmt != NULL )
		mysql_stmt_close(db_add_entry_stmt);

	if( db_get_entry_stmt != NULL )
		mysql_stmt_close(db_get_entry_stmt);

	db_close(db);
}



/**
 * 
 */
int get_entry(char *hash, struct a_entry *entry) {

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

	memset(entry, 0, sizeof(entry));
	
	MYSQL_BIND r[2];
	memset(r, 0, sizeof(r));

	r[0].buffer_type = MYSQL_TYPE_LONG;
	r[0].buffer = &entry->id;
	r[0].buffer_length = sizeof(unsigned long);

	r[1].buffer_type = MYSQL_TYPE_STRING;
	r[1].buffer = &entry->name;
	r[1].buffer_length = A_MAX_LENGTH_NAME;

	if( mysql_stmt_bind_result(db_get_entry_stmt, r) != 0 ) {
		fprintf(stderr, "%s\n", db_error(db));
		return -1;
	}

	if( mysql_stmt_store_result(db_get_entry_stmt) != 0 ) {
		fprintf(stderr, "%s\n", db_error(db));
		return -1;
	}

	int s = mysql_stmt_fetch(db_get_entry_stmt);
	if( s == 0 ) {
		printf(" - %li %s NAME=\"%s\"\n", entry->id, hash, entry->name);
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



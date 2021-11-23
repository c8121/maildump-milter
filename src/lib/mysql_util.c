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

char *db_add_entry_sql = "INSERT INTO ENTRY(HASH, NAME, ARRIVED) VALUES(?,?, NOW());";
MYSQL_STMT *db_add_entry_stmt = NULL;

char *db_add_entry_origin_sql = "INSERT INTO ENTRY_ORIGIN(ENTRY, ORIGIN, OWNER, CTIME, MTIME) VALUES(?,?,?,?,?);";
MYSQL_STMT *db_add_entry_origin_stmt = NULL;

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
 * Returns 0 on success, -1 in failure, 1 if no data found 
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
			return -1;
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

	int result = 0;

	int s = mysql_stmt_fetch(db_get_entry_stmt);
	if( s == MYSQL_NO_DATA ) {
		result = 1;
	} else if( s != 0 ) {
		fprintf(stderr, "mysql_stmt_fetch = %i, %s\n", s, db_error(db));
	}


	mysql_stmt_free_result(db_get_entry_stmt);

	return result;
}

/**
 * Returns ID on success, 0 on failure 
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
			return 0;
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
		return 0;
	}

	if( mysql_stmt_execute(db_add_entry_stmt) != 0 ) {
		fprintf(stderr, "%s\n", db_error(db));
		return 0;
	}

	unsigned long id = mysql_stmt_insert_id(db_add_entry_stmt);

	mysql_stmt_free_result(db_add_entry_stmt);

	return id;
}

/**
 * @param ctime DateTime: YYYY-MM-DD hh:mm:ss
 * @param mtime DateTime: YYYY-MM-DD hh:mm:ss
 * 
 * Returns ID on success, 0 on failure 
 */
int add_entry_origin(unsigned long id, char *origin, char *owner, char *c_time, char *m_time) {

	if( db_add_entry_origin_stmt == NULL ) {
		db_add_entry_origin_stmt = mysql_stmt_init(db);
		int r = mysql_stmt_prepare(
				db_add_entry_origin_stmt,
				db_add_entry_origin_sql,
				strlen(db_add_entry_origin_sql)
		);
		if( r != 0 ) {
			fprintf(stderr, "%s\n", db_error(db));
			return 0;
		}
	}

	MYSQL_BIND p[5];
	memset(p, 0, sizeof(p));

	p[0].buffer_type = MYSQL_TYPE_LONG;
	p[0].buffer = &id;

	unsigned long origin_length = strlen(origin);
	p[1].buffer_type = MYSQL_TYPE_STRING;
	p[1].buffer = origin;
	p[1].length = &origin_length;

	unsigned long owner_length = strlen(owner);
	p[2].buffer_type = MYSQL_TYPE_STRING;
	p[2].buffer = owner;
	p[2].length = &owner_length;

	unsigned long ctime_length = strlen(c_time);
	p[3].buffer_type = MYSQL_TYPE_STRING;
	p[3].buffer = c_time;
	p[3].length = &ctime_length;

	unsigned long mtime_length = strlen(m_time);
	p[4].buffer_type = MYSQL_TYPE_STRING;
	p[4].buffer = m_time;
	p[4].length = &mtime_length;

	if( mysql_stmt_bind_param(db_add_entry_origin_stmt, p) != 0 ) {
		fprintf(stderr, "%s\n", db_error(db));
		return 0;
	}

	if( mysql_stmt_execute(db_add_entry_origin_stmt) != 0 ) {
		fprintf(stderr, "%s\n", db_error(db));
		return 0;
	}

	unsigned long origin_id = mysql_stmt_insert_id(db_add_entry_origin_stmt);

	mysql_stmt_free_result(db_add_entry_origin_stmt);

	return origin_id;
}



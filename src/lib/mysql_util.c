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
 * Requires these variables:
 * db_host, db_port, db_user, db_pwd, db_name
 * 
 */


#include <mysql/mysql.h>

#include "charset_util.c"

void *db;

char *db_get_owner_sql = "SELECT ID, NAME FROM OWNER WHERE NAME=?;";
MYSQL_STMT *db_get_owner_stmt = NULL;

char *db_add_owner_sql = "INSERT INTO OWNER(NAME) VALUES(?);";
MYSQL_STMT *db_add_owner_stmt = NULL;

char *db_get_origin_sql = "SELECT ID, NAME FROM ORIGIN WHERE NAME=?;";
MYSQL_STMT *db_get_origin_stmt = NULL;

char *db_add_origin_sql = "INSERT INTO ORIGIN(NAME) VALUES(?);";
MYSQL_STMT *db_add_origin_stmt = NULL;

char *db_get_entry_sql = "SELECT ID, NAME FROM ENTRY WHERE HASH=?;";
MYSQL_STMT *db_get_entry_stmt = NULL;

char *db_add_entry_sql = "INSERT INTO ENTRY(HASH, NAME, ARRIVED) VALUES(?,?, NOW());";
MYSQL_STMT *db_add_entry_stmt = NULL;

char *db_add_entry_origin_sql = "INSERT INTO ENTRY_ORIGIN(ENTRY, OWNER, ORIGIN, CTIME, MTIME) VALUES(?,?,?,?,?);";
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
	
	return c;
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
 * Returns 0 on success, -1 in failure
 */
int __prepare_stmt(MYSQL_STMT **stmt, char *sql) {

	if( *stmt == NULL ) {
		*stmt = mysql_stmt_init(db);
		int r = mysql_stmt_prepare(
				*stmt,
				sql,
				strlen(sql)
		);
		if( r != 0 ) {
			fprintf(stderr, "%s\n", db_error(db));
			return -1;
		}
	}

	return 0;
}

/**
 * Returns 0 on success, -1 in failure, 1 if no data found 
 */
int get_owner(struct a_owner *owner) {

	if( __prepare_stmt(&db_get_owner_stmt, db_get_owner_sql) != 0 )
		return -1;
	
	char *valid_name = charset_validate_string(owner->name);

	MYSQL_BIND p[1];
	memset(p, 0, sizeof(p));

	unsigned long name_length = strlen(valid_name);
	p[0].buffer_type = MYSQL_TYPE_STRING;
	p[0].buffer = valid_name;
	p[0].length = &name_length;
	p[0].is_null = 0;

	if( mysql_stmt_bind_param(db_get_owner_stmt, p) != 0 ) {
		fprintf(stderr, "%s\n", db_error(db));
		return -1;
	}

	if( mysql_stmt_execute(db_get_owner_stmt) != 0 ) {
		fprintf(stderr, "%s\n", db_error(db));
		return -1;
	}

	memset(owner, 0, sizeof(struct a_owner));

	MYSQL_BIND r[2];
	memset(r, 0, sizeof(r));

	r[0].buffer_type = MYSQL_TYPE_LONG;
	r[0].buffer = &owner->id;

	r[1].buffer_type = MYSQL_TYPE_STRING;
	r[1].buffer = &owner->name;
	r[1].buffer_length = strlen(owner->name);

	if( mysql_stmt_bind_result(db_get_owner_stmt, r) != 0 ) {
		fprintf(stderr, "%s\n", db_error(db));
		return -1;
	}

	if( mysql_stmt_store_result(db_get_owner_stmt) != 0 ) {
		fprintf(stderr, "%s\n", db_error(db));
		return -1;
	}

	int result = 0;

	int s = mysql_stmt_fetch(db_get_owner_stmt);
	if( s == MYSQL_NO_DATA ) {
		result = 1;
	} else if( s != 0 ) {
		fprintf(stderr, "mysql_stmt_fetch = %i, %s\n", s, db_error(db));
	}


	mysql_stmt_free_result(db_get_owner_stmt);
	free(valid_name);

	return result;
}

/**
 * Returns 0 on success, -1 in failure 
 */
int add_owner(struct a_owner *owner) {

	if( __prepare_stmt(&db_add_owner_stmt, db_add_owner_sql) != 0 )
		return -1;

	char *valid_name = charset_validate_string(owner->name);

	MYSQL_BIND p[1];
	memset(p, 0, sizeof(p));

	p[0].buffer_type = MYSQL_TYPE_STRING;
	p[0].buffer = valid_name;
	p[0].buffer_length = strlen(valid_name);
	p[0].is_null = 0;

	if( mysql_stmt_bind_param(db_add_owner_stmt, p) != 0 ) {
		fprintf(stderr, "%s\n", db_error(db));
		return 0;
	}

	if( mysql_stmt_execute(db_add_owner_stmt) != 0 ) {
		fprintf(stderr, "%s\n", db_error(db));
		return 0;
	}

	owner->id = mysql_stmt_insert_id(db_add_owner_stmt);

	mysql_stmt_free_result(db_add_owner_stmt);
	free(valid_name);

	return 0;
}

/**
 * 
 */
int get_create_owner(struct a_owner *owner) {
	
	int r = get_owner(owner);
	if( r == 1 )
		return add_owner(owner);
	else
		return r;
}


/**
 * Returns 0 on success, -1 in failure, 1 if no data found 
 */
int get_origin(struct a_origin *origin) {

	if( __prepare_stmt(&db_get_origin_stmt, db_get_origin_sql) != 0 )
		return -1;
	
	char *valid_name = charset_validate_string(origin->name);

	MYSQL_BIND p[1];
	memset(p, 0, sizeof(p));

	unsigned long name_length = strlen(valid_name);
	p[0].buffer_type = MYSQL_TYPE_STRING;
	p[0].buffer = valid_name;
	p[0].length = &name_length;
	p[0].is_null = 0;

	if( mysql_stmt_bind_param(db_get_origin_stmt, p) != 0 ) {
		fprintf(stderr, "%s\n", db_error(db));
		return -1;
	}

	if( mysql_stmt_execute(db_get_origin_stmt) != 0 ) {
		fprintf(stderr, "%s\n", db_error(db));
		return -1;
	}

	memset(origin, 0, sizeof(struct a_origin));

	MYSQL_BIND r[2];
	memset(r, 0, sizeof(r));

	r[0].buffer_type = MYSQL_TYPE_LONG;
	r[0].buffer = &origin->id;

	r[1].buffer_type = MYSQL_TYPE_STRING;
	r[1].buffer = &origin->name;
	r[1].buffer_length = strlen(origin->name);

	if( mysql_stmt_bind_result(db_get_origin_stmt, r) != 0 ) {
		fprintf(stderr, "%s\n", db_error(db));
		return -1;
	}

	if( mysql_stmt_store_result(db_get_origin_stmt) != 0 ) {
		fprintf(stderr, "%s\n", db_error(db));
		return -1;
	}

	int result = 0;

	int s = mysql_stmt_fetch(db_get_origin_stmt);
	if( s == MYSQL_NO_DATA ) {
		result = 1;
	} else if( s != 0 ) {
		fprintf(stderr, "mysql_stmt_fetch = %i, %s\n", s, db_error(db));
	}


	mysql_stmt_free_result(db_get_origin_stmt);
	free(valid_name);

	return result;
}

/**
 * Returns 0 on success, -1 in failure 
 */
int add_origin(struct a_origin *origin) {

	if( __prepare_stmt(&db_add_origin_stmt, db_add_origin_sql) != 0 )
		return -1;

	char *valid_name = charset_validate_string(origin->name);

	MYSQL_BIND p[1];
	memset(p, 0, sizeof(p));

	p[0].buffer_type = MYSQL_TYPE_STRING;
	p[0].buffer = valid_name;
	p[0].buffer_length = strlen(valid_name);
	p[0].is_null = 0;

	if( mysql_stmt_bind_param(db_add_origin_stmt, p) != 0 ) {
		fprintf(stderr, "%s\n", db_error(db));
		return 0;
	}

	if( mysql_stmt_execute(db_add_origin_stmt) != 0 ) {
		fprintf(stderr, "%s\n", db_error(db));
		return 0;
	}

	origin->id = mysql_stmt_insert_id(db_add_origin_stmt);

	mysql_stmt_free_result(db_add_origin_stmt);
	free(valid_name);

	return 0;
}

/**
 * 
 */
int get_create_origin(struct a_origin *origin) {
	
	int r = get_origin(origin);
	if( r == 1 )
		return add_origin(origin);
	else
		return r;
}



/**
 * Returns 0 on success, -1 in failure, 1 if no data found 
 */
int get_entry(struct a_entry *entry) {

	if( __prepare_stmt(&db_get_entry_stmt, db_get_entry_sql) != 0 )
		return -1;

	MYSQL_BIND p[1];
	memset(p, 0, sizeof(p));
		
	p[0].buffer_type = MYSQL_TYPE_STRING;
	p[0].buffer = entry->hash;
	p[0].buffer_length = strlen(entry->hash);

	if( mysql_stmt_bind_param(db_get_entry_stmt, p) != 0 ) {
		fprintf(stderr, "%s\n", db_error(db));
		return -1;
	}

	if( mysql_stmt_execute(db_get_entry_stmt) != 0 ) {
		fprintf(stderr, "%s\n", db_error(db));
		return -1;
	}

	memset(entry, 0, sizeof(struct a_entry));

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
 * Returns 0 on success, -1 on failure 
 */
int add_entry(struct a_entry *entry) {

	char *valid_name = charset_validate_string(entry->name);

	if( __prepare_stmt(&db_add_entry_stmt, db_add_entry_sql) != 0 )
		return -1;

	MYSQL_BIND p[2];
	memset(p, 0, sizeof(p));

	p[0].buffer_type = MYSQL_TYPE_STRING;
	p[0].buffer = entry->hash;
	p[0].buffer_length = strlen(entry->hash);

	p[1].buffer_type = MYSQL_TYPE_STRING;
	p[1].buffer = valid_name;
	p[1].buffer_length = strlen(valid_name);

	if( mysql_stmt_bind_param(db_add_entry_stmt, p) != 0 ) {
		fprintf(stderr, "%s\n", db_error(db));
		return -1;
	}

	if( mysql_stmt_execute(db_add_entry_stmt) != 0 ) {
		fprintf(stderr, "%s\n", db_error(db));
		return -1;
	}

	entry->id = mysql_stmt_insert_id(db_add_entry_stmt);

	mysql_stmt_free_result(db_add_entry_stmt);
	free(valid_name);

	return 0;
}

/**
 * @param ctime DateTime: YYYY-MM-DD hh:mm:ss
 * @param mtime DateTime: YYYY-MM-DD hh:mm:ss
 * 
 * Returns 0 on success, -1 on failure 
 */
int add_entry_origin(struct a_entry_origin *origin) {

	if( __prepare_stmt(&db_add_entry_origin_stmt, db_add_entry_origin_sql) != 0 )
		return -1;

	MYSQL_BIND p[5];
	memset(p, 0, sizeof(p));

	p[0].buffer_type = MYSQL_TYPE_LONG;
	p[0].buffer = &origin->entry_id;
	
	p[1].buffer_type = MYSQL_TYPE_LONG;
	p[1].buffer = &origin->owner_id;

	p[2].buffer_type = MYSQL_TYPE_LONG;
	p[2].buffer = &origin->origin_id;

	p[3].buffer_type = MYSQL_TYPE_STRING;
	p[3].buffer = &origin->c_time;
	p[3].buffer_length = strlen(origin->c_time);

	p[4].buffer_type = MYSQL_TYPE_STRING;
	p[4].buffer = &origin->m_time;
	p[4].buffer_length = strlen(origin->m_time);

	if( mysql_stmt_bind_param(db_add_entry_origin_stmt, p) != 0 ) {
		fprintf(stderr, "%s\n", db_error(db));
		return -1;
	}

	if( mysql_stmt_execute(db_add_entry_origin_stmt) != 0 ) {
		fprintf(stderr, "%s\n", db_error(db));
		return -1;
	}

	origin->id = mysql_stmt_insert_id(db_add_entry_origin_stmt);

	mysql_stmt_free_result(db_add_entry_origin_stmt);

	return 0;
}



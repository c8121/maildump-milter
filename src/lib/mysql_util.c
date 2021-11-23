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

char *db_get_entry_sql = "SELECT NAME FROM ENTRY WHERE HASH=?;";
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



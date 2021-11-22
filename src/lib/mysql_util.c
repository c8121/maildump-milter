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
void db_close(void *c) {
	mysql_close(c);
}

/**
 * 
 */
const char *db_error(void *c) {
	return mysql_error(c);
}

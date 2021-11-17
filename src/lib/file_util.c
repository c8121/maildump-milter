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

#define TEMP_PATH_MAX 2048 

char *temp_directory_name = "/tmp";

/**
 * Caller must free result
 */
char* temp_filename(char *prefix, char *suffix) {

	struct stat file_stat;
	char *name = NULL;
	
	for( int i=0 ; i < 1000 ; i++ ) {

		if( name != NULL )
			free(name);
		
		name = malloc(TEMP_PATH_MAX);
		sprintf(name, "%s/%s%jd-%i-%X%s", temp_directory_name, prefix, time(0), rand(), arc4random(), suffix);
		
		if( stat(name, &file_stat) != 0 ) {
			//fprintf(stderr, "TEMP FILE: %s\n", name);
			return name;
		}
		
		usleep(100);
	}

	fprintf(stderr, "Cannot create temp filename");
	return NULL;
}

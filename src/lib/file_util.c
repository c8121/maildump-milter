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

#define PROC_SELF_EXEC "/proc/self/exe" //Linux
		//FreeBSD: "/proc/curproc/file"
		//Solaris: "/proc/self/path/a.out
		
#define TEMP_PATH_MAX 2048 

char *temp_directory_name = "/tmp";

char *bin_dir = NULL;
char *conf_dir = NULL;

/**
 * Creates a path name in temp_directory_name (usually /tmp).
 * Note: Does not creata a file or directory, returns a new and not existing name only.
 *
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



/**
 * Replaces placeholders:
 * 
 *   - {{BINDIR}}:  Path of current executable
 *   - {{CONFDIR}}: Path to location of config-file (conf_dir must be set before)
 * 
 * Caller must free result
 */
char* parse_path(char *path) {
	
	char *result = malloc(strlen(path) + 1);
	strcpy(result, path);
	
	if( strstr(path, "{{BINDIR}}") != NULL ) {
		
		if( bin_dir == NULL ) {
			bin_dir = malloc(TEMP_PATH_MAX);
			readlink(PROC_SELF_EXEC, bin_dir, TEMP_PATH_MAX);
			char *last_slash = strrchr(bin_dir, '/');
			if( last_slash != NULL )
				*last_slash = '\0';
		}
		
		result = strreplace_free(result, "{{BINDIR}}", bin_dir);
	}
	
	if( strstr(path, "{{CONFDIR}}") != NULL ) {
		
		if( conf_dir == NULL ) {
			fprintf(stderr, "Cannot parse path, conf_dir was not set: %s\n", path);
			exit(EX_IOERR);
		}
		
		result = strreplace_free(result, "{{CONFDIR}}", conf_dir);
	}
	
	//printf("%s = %s\n", path, result);
	return result;
}

/**
 * Same as parse_path(...), but does free given path-parameter.
 */
char* parse_path_free(char* path) {
	char *result = parse_path(path);
	free(path);
	return result;
}
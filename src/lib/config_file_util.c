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

#ifndef MM_CONFIG_FILE_UTIL
#define MM_CONFIG_FILE_UTIL

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define CONFIG_FILE_MAX_LINE_LENGTH 1024

struct config_item {
	char *name;
	char *value;
	struct config_item *next;
};

struct config_item *config = NULL;


/**
 * malloc & copy value  
 */
char *get_config(char *name, int ltrim, int rtrim) {

	struct config_item *curr = config;
	while( curr != NULL ) {

		if( strcmp(name, curr->name) == 0 ) {

			char *v = curr->value;
			for( ; ltrim != 0 && (*v == ' ' || *v == '\r' || *v == '\n') ; v++ );
			
			char *result = malloc(strlen(v)+1);
			strcpy(result, v);
			
			for( char *p = result + strlen(result)-1; rtrim != 0 && (*p == ' ' || *p == '\r' || *p == '\n') ; p-- )
				*p = '\0';
			
			return result;

		}

		curr = curr->next;
	}

	return NULL;
}

/**
 * Assign to dst if value was found
 */
void set_config(char **dst, char *name, int ltrim, int rtrim, int empty_is_null, int verbosity) {
	
	char *value = get_config(name, ltrim, rtrim);
	if( value != NULL ) {
		
		if( empty_is_null != 0 && value[0] == '\0' ) {
			free(value);
			value = NULL;
		}
		
		if( verbosity > 0 ) {
			fprintf(stderr, "Set config: %s = %s\n", name, value);
			if( verbosity > 1 )
				fprintf(stderr, "  (previous value: %s)\n", *dst);
		}
		
		*dst = value;
	}
	
}

/**
 * Assign to dst if value was found
 */
void set_config_uint(unsigned int *dst, char *name, int verbosity) {
	
	char *value = get_config(name, 1, 1);
	if( value != NULL ) {
		
		if( verbosity > 0 ) {
			fprintf(stderr, "Set config: %s = %i\n", name, atoi(value));
			if( verbosity > 1 )
				fprintf(stderr, "  (previous value: %i)\n", *dst);
		}
		
		*dst = atoi(value);
	}
	
}


/**
 * Return NULL if it is not a section name,
 * return the name otherwise.
 * 
 * Caller must free result
 */
char* __is_section_name(char *line) {
	
	char *s = line;
	//Find start, ignore space && tab
	while( *s && (*s == ' ' || *s == '\t' || *s == '[')) {
		if( *s == '[' ) {
			s++;
			char *e = s;
			while( *e ) {
				if( *e == ']') {
					char *name = malloc(e - s);
					strcpy(name, s);
					name[e - s] = '\0';
					return name;
				}
				e++;
			}
		}
		s++;
	}
	
	return NULL;
	
}

/**
 * Return 0 on success
 */
int read_config(char *filename, char *section) {

	FILE *fp = fopen(filename, "r");
	if( fp == NULL ) {
		fprintf(stderr, "Failed to open config-file: %s\n", filename);
		return -1;
	}
	
	//printf("Read config from \"%s\" (section=%s)\n", filename, section);


	char *curr_section = NULL;

	struct config_item *curr = NULL;
	struct config_item *prev = NULL;
	char line[CONFIG_FILE_MAX_LINE_LENGTH];
	while(fgets(line, sizeof(line), fp)) {
		
		if( line[0] == '#' )
			continue;
		
		char *check_section = __is_section_name(line);	
		if( check_section != NULL ) {
			if( curr_section != NULL )
				free(curr_section);
			curr_section = check_section;
			//printf("****** SECTION %s *****\n", curr_section);
			continue;
		}
		
		if( section != NULL && curr_section != NULL && strcmp(section, curr_section) != 0 )
			continue;

		char *div = strchr(line, ':');
		if( div != NULL ) {

			curr = malloc(sizeof(struct config_item));
			curr->next = NULL;

			curr->name = malloc(div - line + 1);
			strncpy(curr->name, line, div - line);
			
			char *p = curr->name;
			char *o = curr->name;
			while( *p ) {
				if( *p != ' ' && *p != '\t' )
					*o++ = *p;
				p++;
			}
			*o = '\0';
			//printf("   NAME: %s\n", curr->name);

			curr->value = malloc(strlen(div)+1);
			strcpy(curr->value, div+1);
			//printf("   VALUE: %s\n", curr->value);

			if( prev != NULL )
				prev->next = curr;

			if( config == NULL )
				config = curr;

			prev = curr;

		} else if ( curr != NULL ) {

			char *tmp = malloc(strlen(curr->value) + strlen(line) + 1);
			strcpy(tmp, curr->value);
			strcat(tmp, line);

			free(curr->value);
			curr->value = tmp;
			//printf("   VALUE: %s\n", curr->value);
		}
	}

	fclose(fp);
	
	if( curr_section != NULL )
		free(curr_section);

	return 0;

}

#endif //MM_CONFIG_FILE_UTIL
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

#define CONFIG_FILE_MAX_LINE_LENGTH 1024

struct config_item {
	char *name;
	char *value;
	struct config_item *next;
};

struct config_item *config = NULL;


/**
 * malloc & copie value  
 */
char *get_config(char *name, int ltrim, int rtrim) {

	struct config_item *curr = config;
	while( curr != NULL ) {

		//printf("(%s): (%s)\n", curr->name, curr->value);
		if( strcmp(name, curr->name) == 0 ) {

			char *result = malloc(strlen(curr->value));
			strcpy(result, curr->value);
			
			for( ; ltrim != 0 && (*result == ' ' || *result == '\r' || *result == '\n') ; result++ );
			for( char *p = result + strlen(result)-1; rtrim != 0 && (*p == ' ' || *p == '\r' || *p == '\n') ; p-- )
				*p = '\0';
			
			return result;

		}

		curr = curr->next;
	}

	return NULL;
}

/**
 * Assign if value was found
 */
void set_config(char **dst, char *name, int ltrim, int rtrim) {
	
	char *value = get_config(name, ltrim, rtrim);
	if( value != NULL )
		*dst = value;
	
}

/**
 * Return 0 on success
 */
int read_config(char *filename) {

	FILE *fp = fopen(filename, "r");
	if( fp == NULL ) {
		fprintf(stderr, "Failed to open file: %s\n", filename);
		return -1;
	}

	struct config_item *curr = NULL;
	struct config_item *prev = NULL;
	char line[CONFIG_FILE_MAX_LINE_LENGTH];
	while(fgets(line, sizeof(line), fp)) {

		if( line[0] == '#' )
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
				*p++;
			}
			*o = '\0';

			curr->value = malloc(strlen(div)+1);
			strcpy(curr->value, div+1);

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
		}
	}

	fclose(fp);

	return 0;

}

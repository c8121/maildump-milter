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


#include <ctype.h>

struct query_filter {
	char *name;
	char *value;
	char *op;
	char *prev_join;
	struct query_filter *next;
};


/**
 * 
 */
struct query_filter* create_query_filter(struct query_filter *prev) {
	
	struct query_filter *result = malloc(sizeof(struct query_filter));
	result->name = NULL;
	result->value = NULL;
	result->op = NULL;
	result->prev_join = NULL;
	
	result->next = NULL;
	if( prev != NULL )
		prev->next = result;
		
	return result;
}

/**
 * 
 */
 void free_query_filter(struct query_filter *filter) {
	 
	if( filter->next != NULL )
		 free_query_filter(filter->next);
		 
	if( filter->name != NULL )
		free(filter->name);
		
	if( filter->value != NULL )
		free(filter->value);
		
	if( filter->op != NULL )
		free(filter->op);
		
	if( filter->prev_join != NULL )
		free(filter->prev_join);
		 
	free(filter);
 }


/**
 * print_s is a template containing '?' as placeholders for column values
 * sql must contain '?', it will be replaced by where-statement
 */
int mysql_print_query(char *print_s, char *sql, struct query_filter *filter) {

	struct char_buffer *query = NULL;
	struct char_buffer *where = NULL;
	
	int filter_cnt = 0;
	struct query_filter *f = filter;
	while( f != NULL) {
		
		if( f->prev_join != NULL ) {
			where = strappend(where, " ", 1);
			where = strappend(where, f->prev_join, strlen(f->prev_join));
			where = strappend(where, " ", 1);
		}
		
		where = strappend(where, "(", 1);
		where = strappend(where, f->name, strlen(f->name));
		where = strappend(where, " ", 1);
		where = strappend(where, f->op, strlen(f->op));
		where = strappend(where, " ?)", 3);
		
		f = f->next;
		filter_cnt++;
	}
	
	char *pos = strrchr(sql, '?');
	if( pos != NULL ) {
		query = strappend(query, sql, pos - sql);
		query = strappend(query, where->s, strlen(where->s));
		query = strappend(query, pos+1, strlen(pos)-1);
	}
	
	//printf("SQL: %s\n", query->s);
	
	MYSQL_STMT *stmt = NULL;
	if( __prepare_stmt(&stmt, query->s) != 0 )
		return -1;

	MYSQL_BIND p[filter_cnt];
	memset(p, 0, sizeof(p));
	
	f = filter;
	int i = 0;
	while( f != NULL) {
		
		p[i].buffer_type = MYSQL_TYPE_STRING;
		p[i].buffer = f->value;
		p[i].buffer_length = strlen(f->value);
		p[i].is_null = 0;
		
		f = f->next;
		i++;
	}

	if( mysql_stmt_bind_param(stmt, p) != 0 ) {
		fprintf(stderr, "BIND PARAMS FAILED: %s\n", db_error(db));
		return -1;
	}
	
	if( mysql_stmt_execute(stmt) != 0 ) {
		fprintf(stderr, "%s\n", db_error(db));
		return -1;
	}
	
	int field_cnt = mysql_stmt_field_count(stmt);
	char column_value[field_cnt][A_MAX_LENGTH_NAME];
	
	MYSQL_BIND r[field_cnt];
	memset(r, 0, sizeof(r));
	
	//Treat all columns as string
	for( i=0 ; i < field_cnt ; i++ ) {
		r[i].buffer_type = MYSQL_TYPE_STRING;
		r[i].buffer = &column_value[i];
		r[i].buffer_length = A_MAX_LENGTH_NAME;
	}
	
	if( mysql_stmt_bind_result(stmt, r) != 0 ) {
		fprintf(stderr, "BIND RESULT FAILED: %s\n", db_error(db));
		return -1;
	}

	int s;
	unsigned long cnt = 0;
	while( 1 ) {
		
		s = mysql_stmt_fetch(stmt);
		if( s == 1 || s == MYSQL_NO_DATA )
			break;
		
		i = 0;
		char *p = print_s;
		while( *p ) {
			if( *p == '?' ) {
				printf("%s", column_value[i]);
				i++;
			} else {
				printf("%c", *p);
			}
			p++;
		}
		
		cnt++;
	}

	printf("TOTAL ITEMS: %ld\n", cnt);

	mysql_stmt_free_result(stmt);
	__close_stmt(&stmt);
	
	return 0;
}


/**
 * Term syntax:
 * 
 * NAME VALUE
 * NAME *VALUE
 * NAME *VALUE*
 * NAME VALUE*
 */
struct query_filter* create_query_filter_from_term(char *term) {
	
	struct query_filter *filter = NULL;
	struct query_filter *result = NULL;
	
	
	char *p = term;
	char *s = term;
	char *l = NULL;
	do {
		if( *p == '&' || *p == '|' || *p == '\0' ) {
			
			for( ; *s && isspace(*s) ; s++ );
			char *e = p-1;
			for( ; e > s && isspace(*e) ; e-- );
			*(e+1) = '\0';
			
			char *sep = strchr(s, ' ');
			if( sep == NULL ) {
				fprintf(stderr, "Invalid term. Expected \"NAME VALUE\". Term was: \"%s\"\n", s);
				exit(EX_DATAERR);
			}
			
			*sep = '\0';
			
			char *v = sep +1;
			for( ; *v && isspace(*v) ; v++ );
			
			filter = create_query_filter(filter);
			filter->name = strcopy(s);
			filter->value = strcopy(v);
			
			if( *v == '*' || *e == '*' ) {
				if( *v == '*' )
					filter->value[0] = '%';
				if( *e == '*' )
					filter->value[strlen(v)-1] = '%';
					
				filter->op = strcopy("like");
			} else {
				filter->op = strcopy("=");
			}
			
			if( l != NULL ) {
				if( *l == '&' )
					filter->prev_join = strcopy("AND");
				else if( *l == '|' )
					filter->prev_join = strcopy("OR");
			}
			
			
			if( result == NULL )
				result = filter;
			
			l = p;
			s = p + 1;
		}
	} while(*p++);
	
	return result;
}

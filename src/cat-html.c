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

#define _GNU_SOURCE //to enable strcasestr(...)

#include <stdlib.h>
#include <string.h>

#include "./lib/cat_util.c"
#include "./lib/char_util.c"

char *htmltotext_program = "html2text {{charset_param}} -nometa \"{{input_file}}\"";

/**
 * 
 */
int main(int argc, char *argv[]) {

	configure(argc, argv);
		
	char *command = strreplace(htmltotext_program, "{{input_file}}", cat_util_inputfile);

	if( strcasestr(cat_util_charset, "utf") != NULL )
		command = strreplace_free(command, "{{charset_param}}", "-utf8");
	else
		command = strreplace_free(command, "{{charset_param}}", "");
	
	//printf("EXEC: %s\n", command);
	int r = system(command);
	free(command);

	return r;
}
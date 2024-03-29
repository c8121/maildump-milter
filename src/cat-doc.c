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

#include <stdlib.h>

#include "./lib/cat_util.c"
#include "./lib/char_util.c"

char *catdoc_program = "catdoc -d utf-8 \"{{input_file}}\"";

/**
 * 
 */
int main(int argc, char *argv[]) {

	configure(argc, argv);
		
	char *command = strreplace(catdoc_program, "{{input_file}}", cat_util_inputfile);

	//printf("EXEC: %s\n", command);
	int r = system(command);
	free(command);

	return r;
}
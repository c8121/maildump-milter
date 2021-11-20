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

char *cat_util_charset;
char *cat_util_inputfile;

/**
 * 
 */
void usage() {
	printf("Usage of cat-utils (prints out plain text of a document):\n");
	printf("cat-* <charset> <input filename>\n");
}

/**
 * Load arguments into 
 *   cat_util_charset
 *   cat_util_inputfile
 */
void configure(int argc, char *argv[]) {

	if( argc < 3 ) {
		fprintf(stderr, "Missing argument\n");
		usage();
		exit(EX_USAGE);
	}
	
	cat_util_charset = argv[1];
	
	cat_util_inputfile = argv[2];
	struct stat file_stat;
	if( stat(cat_util_inputfile, &file_stat) != 0 ) {
		fprintf(stderr, "File not found: %s\n", cat_util_inputfile);
		exit(EX_USAGE);
	}
}
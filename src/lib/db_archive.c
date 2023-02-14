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

#ifndef MM_DB_ARCHIVE
#define MM_DB_ARCHIVE

#define A_MAX_LENGTH_HASH 65
#define A_MAX_LENGTH_NAME 254
#define A_MAX_LENGTH_ORIGIN 4069

struct a_owner{
	unsigned long id;
	char name[A_MAX_LENGTH_NAME];
};

struct a_origin{
	unsigned long id;
	char name[A_MAX_LENGTH_NAME];
};

struct a_entry{
	unsigned long id;
	char hash[A_MAX_LENGTH_HASH];
	char name[A_MAX_LENGTH_NAME];
};

struct a_entry_origin{
	unsigned long id;
	unsigned long entry_id;
	unsigned long owner_id;
	unsigned long origin_id;
	char c_time[20];
	char m_time[20];
};

#endif //MM_DB_ARCHIVE
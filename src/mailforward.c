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

/**
 * Author: christian c8121 de
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <sysexits.h>

#include "../lib/sntools/src/lib/net_util.c"
#include "../lib/sntools/src/lib/smtp_socket_util.c"
#include "../lib/sntools/src/lib/linked_items.c"

char *smtpHost;
int smtpPort;
char *envFrom;
char *envTo;
char *heloName;

char *addExtensionBeforeForward = ".tmp";
char *addExtensionOnSuccess = ".archived";

#define MAX_LINE_SIZE 255

struct buffer_line {
	struct linked_item list;
	char s[MAX_LINE_SIZE];
};


/**
 * 
 */
void usage_message() {
	printf("Usage: mailforward <host> <port> <from> <to> <filename> [filename...]\n");
}


/**
 * 
 */
int send_file(const char *filePath) {

	FILE *fp = fopen(filePath, "r");
	if( fp == NULL ) {
		fprintf(stderr, "Failed to open file: %s\n", filePath);
		return -1;
	}

	// As we are sending a file as it is right after the SMTP DATA command, 
	// check if the given file has headers and body.
	struct buffer_line *header = NULL;
	struct buffer_line *body = NULL;
	char line[255];
	struct buffer_line *read_into = NULL;
	while(fgets(line, sizeof(line), fp)) {

		if( header == NULL ) {
			header = linked_item_create(NULL, sizeof(struct buffer_line));
			read_into = header;
		} else if( body == NULL && (line[0] == '\r' || line[0] == '\n') ) {
			body = linked_item_create(NULL, sizeof(struct buffer_line));
			read_into = body;
		} else {
			read_into = linked_item_create(read_into, sizeof(struct buffer_line));
		}

		strcpy(read_into->s, line);

		//read up to 3 body lines here
		//rest will be read in smtp dialog below
		if( body != NULL && linked_item_count(body) > 3 ) {
			break;
		}
	}

	int header_lines = linked_item_count(header);
	int body_lines = linked_item_count(body);
	if( header_lines == 0 || body_lines == 0 ) {
		fprintf(stderr, "Message does not seem to have headers or body (%i header lines, %i body lines)\n", header_lines, body_lines);
		fclose(fp);
		exit(EX_DATAERR);
	}

	int socket = open_socket(smtpHost, smtpPort);
	char buf[255];
	if( socket < 1 ) {
		fprintf(stderr, "Cannot connect to Server %s:%i\n", smtpHost, smtpPort);
		fclose(fp);
		return -1;
	} else {

		//Read SMTP Greeting
		if( smtp_read(socket) != 0 ) {
			return -1;
		}

		snprintf(buf, sizeof(buf), "helo %s\r\n", heloName);
		write(socket, buf, strlen(buf));
		if( smtp_read(socket) != 0 ) {
			return -1;
		}

		snprintf(buf, sizeof(buf), "mail from: <%s>\r\n", envFrom);
		write(socket, buf, strlen(buf));
		if( smtp_read(socket) != 0 ) {
			return -1;
		}

		snprintf(buf, sizeof(buf), "rcpt to: <%s>\r\n", envTo);
		write(socket, buf, strlen(buf));
		if( smtp_read(socket) != 0 ) {
			return -1;
		}

		snprintf(buf, sizeof(buf), "data\r\n");
		write(socket, buf, strlen(buf));
		if( smtp_read(socket) != 0 ) {
			return -1;
		}

		// Send header which has been read above
		struct buffer_line *curr = header;
		while( curr != NULL ) {
			write(socket, curr->s, strlen(curr->s));
			curr = (struct buffer_line *) curr->list.next;
		}
		linked_item_free(header, NULL);

		// Send body which has been read above
		curr = body;
		while( curr != NULL ) {
			smtp_write(socket, curr->s);
			curr = (struct buffer_line *) curr->list.next;
		}
		linked_item_free(body, NULL);

		// Send rest of body directly from file
		while(fgets(line, sizeof(line), fp)) {
			smtp_write(socket, line);
		}

		snprintf(buf, sizeof(buf), ".\r\n");
		write(socket, buf, strlen(buf));
		if( smtp_read(socket) != 0 ) {
			return -1;
		}

		snprintf(buf, sizeof(buf), "quit\r\n");
		write(socket, buf, strlen(buf));
		if( smtp_read(socket) != 0 ) {
			return -1;
		}

		close(socket);
	}

	fclose(fp);
	return 0;
}


/**
 * 
 */
int main(int argc, char *argv[]) {

	if (argc < 6) {
		fprintf(stderr, "Missing arguments\n");
		usage_message();
		exit(EX_USAGE);
	}

	// SMTP Server
	smtpHost = argv[1];
	smtpPort = atoi(argv[2]);

	// Check sender and recipient
	envFrom = argv[3];
	envTo = argv[4];
	if( envFrom[0] == '\0' || envTo[0] == '\0' ) {
		fprintf(stderr, "Parameter 'from' and 'to' cannot be empty\n");
		usage_message();
		exit(EX_USAGE);
	}

	// Read hostname
	char buf[128];
	gethostname(buf, sizeof(buf));
	heloName = buf;


	for( int i=5 ; i < argc ; i++ ) {

		// Check if file exists
		struct stat fileStat;
		if( stat(argv[i], &fileStat) != 0 ) {
			fprintf(stderr, "File not found: %s\n", argv[i]);
			exit(EX_IOERR);
		}

		char tmpName[strlen(argv[i]) + strlen(addExtensionBeforeForward) +1];
		sprintf(tmpName, "%s%s", argv[i], addExtensionBeforeForward);
		//printf("Rename to: '%s'\n", newName);
		if( rename(argv[i], tmpName) != 0 ) {
			fprintf(stderr, "Failed to rename %s to %s\n", argv[i], tmpName);
			exit(EX_IOERR);
		}

		if( send_file(tmpName) != 0 ) {
			fprintf(stderr, "Failed to send message %s\n", argv[i]);
			fprintf(stderr, "Exit.\n");
			exit(EX_IOERR);
		} else {
			printf("Successfully sent %s\n", argv[i]);
			if( addExtensionOnSuccess != NULL && strlen(addExtensionOnSuccess) > 0 ) {
				char newName[strlen(argv[i]) + strlen(addExtensionOnSuccess) +1];
				sprintf(newName, "%s%s", argv[i], addExtensionOnSuccess);
				//printf("Rename to: '%s'\n", newName);
				if( rename(tmpName, newName) != 0 ) {
					fprintf(stderr, "Failed to rename %s to %s\n", tmpName, newName);
					exit(EX_IOERR);
				}
			}
		}
	}

	return 0;
}




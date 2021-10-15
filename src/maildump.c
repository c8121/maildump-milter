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

#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <sysexits.h>
#include <sys/stat.h>

#include <libmilter/mfapi.h>


char milterName[] = "Maildump Milter";

char *socketPath = "/var/spool/postfix/maildump/maildump.socket";
char *outputDir = "/var/spool/postfix/maildump";

char *envFromHeader = "X-ACHRIVED-FROM";
char *envRcptHeader = "X-ACHRIVED-RCPT";


/**
 * Data structure assigned to each MILTER-Session.
 */
struct SessionData {
	char *fileName; //basename
	char *filePath; //full path
	FILE *file;
};

/**
 * Get or create SessionData assigned to given context.
 * Opens file to write message to (with temporary name).
 */
struct SessionData* get_session_data(SMFICTX *ctx) {

	struct SessionData *sessionData = smfi_getpriv(ctx);

	if( sessionData == NULL ) {
		printf("Create new session: %p\n", ctx);
		sessionData = malloc(sizeof(sessionData));

		sessionData->fileName = (char*) malloc(64);
		sprintf(sessionData->fileName, "%jd-%i.msg", time(0), rand());

		sessionData->filePath = (char*) malloc(1024);
		sprintf(sessionData->filePath, "%s/.%s.filepart", outputDir, sessionData->fileName);


		sessionData->file = fopen(sessionData->filePath, "w");
		if( sessionData->file == NULL ) {
			fprintf(stderr,"Failed to open file (w): %s\n", sessionData->filePath);
		}

		smfi_setpriv(ctx, sessionData);
	}

	printf("Session: %p, File=%s\n", ctx, sessionData->filePath);

	return sessionData;
}

/**
 * Free SessionData resources.
 * Closes message file, rename file from tempoary name to final name.
 */
void cleanup_session_data(SMFICTX *ctx) {

	struct SessionData *sessionData = smfi_getpriv(ctx);

	if( sessionData != NULL ) {

		printf("Cleanup session: %p, File=%s\n", ctx, sessionData->filePath);

		if( sessionData->file != NULL ) {
			if( fclose(sessionData->file) != 0 ) {
				fprintf(stderr,"Failed to close file: %s\n", sessionData->filePath);
			}

			char *finalFileName = (char*) malloc(1024);
			sprintf(finalFileName, "%s/%s", outputDir, sessionData->fileName);
			link(sessionData->filePath, finalFileName);
			unlink(sessionData->filePath);

			printf("Created file: %s\n", finalFileName);
			free(finalFileName);
		}

		free(sessionData->filePath);
		free(sessionData->fileName);
		free(sessionData);
		smfi_setpriv(ctx, NULL);
	}

}


/**
 * Sendmail MILTER Interface
 */

sfsistat mlfi_envfrom(SMFICTX *ctx, char **envfrom) {

	struct SessionData *sessionData = get_session_data(ctx);
	if( sessionData->file != NULL && envFromHeader[0] != '\0') {
		fprintf(sessionData->file, "%s: %s\r\n", envFromHeader, *envfrom);
	}

	return SMFIS_CONTINUE;
}

sfsistat mlfi_envrcpt(SMFICTX *ctx, char **envrcpt) {

	struct SessionData *sessionData = get_session_data(ctx);
	if( sessionData->file != NULL && !envRcptHeader[0] != '\0') {
		fprintf(sessionData->file, "%s: %s\r\n", envRcptHeader, *envrcpt);
	}

	return SMFIS_CONTINUE;

}

sfsistat mlfi_header(SMFICTX *ctx, char *headerName, char *headerValue) {

	struct SessionData *sessionData = get_session_data(ctx);
	if( sessionData->file != NULL ) {
		fprintf(sessionData->file, "%s: %s\r\n", headerName, headerValue);
	}

	return SMFIS_CONTINUE;

}

sfsistat mlfi_eoh(SMFICTX *ctx) {

	struct SessionData *sessionData = get_session_data(ctx);
	if( sessionData->file != NULL ) {
		fprintf(sessionData->file, "\r\n");
	}

	return SMFIS_CONTINUE;

}


sfsistat mlfi_body(SMFICTX *ctx, u_char *bodyp, size_t bodylen) {

	struct SessionData *sessionData = get_session_data(ctx);
	if( sessionData->file != NULL ) {
		if (fwrite(bodyp, bodylen, 1, sessionData->file) <= 0) {
			fprintf(stderr, "Failed to write body\n");
			cleanup_session_data(ctx);
			return SMFIS_TEMPFAIL;
		}
	}

	return SMFIS_CONTINUE;
}

sfsistat mlfi_eom(SMFICTX *ctx) {

	cleanup_session_data(ctx);

	return SMFIS_CONTINUE;

}

sfsistat mlfi_close(SMFICTX *ctx) {

	cleanup_session_data(ctx);

	return SMFIS_ACCEPT;
}

sfsistat mlfi_abort(SMFICTX *ctx) {

	printf("%p: mlfi_abort\n", ctx);

	struct SessionData *sessionData = get_session_data(ctx);
	if( sessionData != NULL && sessionData->file != NULL ) {
		printf("Close and remove: %p, File=%s\n", ctx, sessionData->filePath);
		if( fclose(sessionData->file) != 0 ) {
			fprintf(stderr, "Failed to close file: %s\n", sessionData->filePath);
		}
		if( remove(sessionData->filePath) != 0 ) {
			fprintf(stderr, "Failed to remove file: %s\n", sessionData->filePath);
		}
		sessionData->file = NULL;
	}

	cleanup_session_data(ctx);

	return SMFIS_CONTINUE;
}


struct smfiDesc smfilter =
{
		milterName,	/* filter name */
		SMFI_VERSION,	/* version code -- do not change */
		SMFIF_ADDHDRS,	/* flags */
		NULL,		/* connection info filter */
		NULL,		/* SMTP HELO command filter */
		mlfi_envfrom,	/* envelope sender filter */
		mlfi_envrcpt,	/* envelope recipient filter */
		mlfi_header,	/* header filter */
		mlfi_eoh,	/* end of header */
		mlfi_body,	/* body block filter */
		mlfi_eom,	/* end of message */
		mlfi_abort,	/* message aborted */
		mlfi_close	/* connection cleanup */
};


/**
 * 
 */
void print_usage_message() {
	printf( "%s\n", milterName);
	printf( "Usage:\n");
	printf( "  -s <socket path> (default: %s)\n", socketPath);
	printf( "  -o <output directory> (default: %s)\n", outputDir);
	printf( "  -f <envelope from header name> (default: %s, empty to omit this header)\n", envFromHeader);
	printf( "  -t <envelope rcpt-to header name> (default: %s, empty to omit this header)\n", envRcptHeader);
}


/**
 * 
 */
void configure(int argc, char *argv[]) {

	const char *options = "s:o:f:t:h";
	int c;
	struct stat fileStat;

	while ((c = getopt(argc, argv, options)) != -1) {
		switch(c) {

		case 's':
		socketPath = optarg;
		break;

		case 'o':
			if (stat(optarg, &fileStat) == -1) {
				fprintf(stderr, "Cannot set output dir (does not exist)\n");
			} else {
				outputDir = optarg;
				printf("Store messages in %s\n", outputDir);
			}
			break;

		case 'f':
			envFromHeader = optarg;
			if( envFromHeader[0] == '\0' ) {
				printf("Will not add 'envelope from'-header\n");
			}
			break;

		case 't':
			envRcptHeader = optarg;
			if( envRcptHeader[0] == '\0' ) {
				printf("Will not add 'enveloper rcpt-to'-header\n");
			}
			break;

		case 'h':
			print_usage_message();
			break;
		}
	}

}


/**
 * 
 */
int main(int argc, char *argv[]) {

	configure(argc, argv);

	//Create seed for rand() used in get_session_data(...) above.
	srand(time(NULL));

	//Check if socket file exists and try to remove before bind
	struct stat fileStat;
	if( stat(socketPath, &fileStat) == 0 ) {
		printf("Socket exists, remove: %s\n", socketPath);
		unlink(socketPath);
	}


	printf("Bind to %s\n", socketPath);
	smfi_setconn(socketPath);

	if (smfi_register(smfilter) == MI_FAILURE) {
		fprintf(stderr, "smfi_register failed\n");
		exit(EX_UNAVAILABLE);
	}

	if (smfi_main() == MI_FAILURE) {
		fprintf(stderr, "smfi_main failed\n");
		exit(EX_UNAVAILABLE);
	}

	//Cleanup    
	unlink(socketPath);

	printf("Exit\n");

}




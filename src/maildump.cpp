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
#include <unistd.h>
#include <sysexits.h>
#include <string>
#include <iostream>
#include <ctime>
#include <sys/stat.h>

#include <libmilter/mfapi.h>

using namespace std;


string milterName = "Maildump Milter";
string socketPath = "/var/spool/postfix/maildump/maildump.socket";
string outputDir = "/var/spool/postfix/maildump";

string envFromHeader = "X-ACHRIVED-FROM";
string envRcptHeader = "X-ACHRIVED-RCPT";


/**
 * Data structure assigned to each MILTER-Session.
 */
struct SessionData {
    char *fileName = NULL; //basename
    char *filePath = NULL; //full path
    FILE *file = NULL;
};

/**
 * Get or create SessionData assigned to given context.
 * Opens file to write message to (with temporary name).
 */
SessionData* get_session_data(SMFICTX *ctx) {
    
    SessionData *sessionData = (SessionData*) smfi_getpriv(ctx);
    
    if( sessionData == NULL ) {
        cout << "Create new session: " << ctx << endl;
        sessionData = (SessionData*) malloc(sizeof(sessionData));
        
        sessionData->fileName = (char*) malloc(64);
        sprintf(sessionData->fileName, "%jd-%i", (intmax_t)time(0), std::rand());
        
        sessionData->filePath = (char*) malloc(1024);
        sprintf(sessionData->filePath, "%s/.%s.filepart", const_cast<char*>(outputDir.c_str()), sessionData->fileName);
        
        
        sessionData->file = fopen(sessionData->filePath, "w");
        if( sessionData->file == NULL ) {
            cerr << "Failed to open file (w): " << sessionData->filePath;
        }
        
        smfi_setpriv(ctx, sessionData);
    }
    
    //cout << "Session: " << ctx << ", File=" << sessionData->filePath << endl;
    
    return sessionData;
}

/**
 * Free SessionData resources.
 * Closes message file, rename file from tempoary name to final name.
 */
void cleanup_session_data(SMFICTX *ctx) {

    SessionData *sessionData = (SessionData*) smfi_getpriv(ctx);
    
    if( sessionData != NULL ) {
        
        cout << "Cleanup session: " << ctx << ", File=" << sessionData->filePath << endl;
        
        if( sessionData->file != NULL ) {
            if( fclose(sessionData->file) != 0 ) {
                cerr << "Failed to close file: " << sessionData->filePath << endl;
            }
            
            char *finalFileName = (char*) malloc(1024);
            sprintf(finalFileName, "%s/%s", const_cast<char*>(outputDir.c_str()), sessionData->fileName);
            link(sessionData->filePath, finalFileName);
	        unlink(sessionData->filePath);
            
            cout << "Created file: " << finalFileName << endl;
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
    
    //cout << ctx << ": mlfi_envfrom" << endl;
    
    SessionData *sessionData = get_session_data(ctx);
    if( sessionData->file != NULL && !envFromHeader.empty()) {
        fprintf(sessionData->file, "%s: %s\r\n", const_cast<char*>(envFromHeader.c_str()), *envfrom);
    }
    
    return SMFIS_CONTINUE;
}

sfsistat mlfi_envrcpt(SMFICTX *ctx, char **envrcpt) {
    
    //cout << ctx << ": mlfi_envrcpt" << endl;
    
    SessionData *sessionData = get_session_data(ctx);
    if( sessionData->file != NULL && !envRcptHeader.empty()) {
        fprintf(sessionData->file, "%s: %s\r\n", const_cast<char*>(envRcptHeader.c_str()), *envrcpt);
    }
    
    return SMFIS_CONTINUE;
    
}

sfsistat mlfi_header(SMFICTX *ctx, char *headerName, char *headerValue) {
    
    //cout << ctx << ": mlfi_header" << endl;
    
    SessionData *sessionData = get_session_data(ctx);
    if( sessionData->file != NULL ) {
        fprintf(sessionData->file, "%s: %s\r\n", headerName, headerValue);
    }
    
    return SMFIS_CONTINUE;
    
}

sfsistat mlfi_eoh(SMFICTX *ctx) {
   
    //cout << ctx << ": mlfi_eoh" << endl;
    
    SessionData *sessionData = get_session_data(ctx);
    if( sessionData->file != NULL ) {
        fprintf(sessionData->file, "\r\n");
    }
    
    return SMFIS_CONTINUE;
    
}


sfsistat mlfi_body(SMFICTX *ctx, u_char *bodyp, size_t bodylen) {
    
    //cout << ctx << ": mlfi_body" << endl;
    
    SessionData *sessionData = get_session_data(ctx);
    if( sessionData->file != NULL ) {
        if (fwrite(bodyp, bodylen, 1, sessionData->file) <= 0) {
            cerr << "Failed to write body" << endl;
            cleanup_session_data(ctx);
            return SMFIS_TEMPFAIL;
        }
    }
    
    return SMFIS_CONTINUE;
}

sfsistat mlfi_eom(SMFICTX *ctx) {
    
    //cout << ctx << ": mlfi_eom" << endl;
    
    cleanup_session_data(ctx);
    
    return SMFIS_CONTINUE;
    
}

sfsistat mlfi_close(SMFICTX *ctx) {
    
    //cout << ctx << ": mlfi_close" << endl;
    
    cleanup_session_data(ctx);
    
    return SMFIS_ACCEPT;
}

sfsistat mlfi_abort(SMFICTX *ctx) {
    
    cout << ctx << ": mlfi_abort" << endl;
    
    SessionData *sessionData = get_session_data(ctx);
    if( sessionData != NULL && sessionData->file != NULL ) {
        cout << ctx << ": Close and remove " << sessionData->filePath << endl;
        if( fclose(sessionData->file) != 0 ) {
            cerr << "Failed to close file: " << sessionData->filePath << endl;
        }
        if( remove(sessionData->filePath) != 0 ) {
            cerr << "Failed to remove file: " << sessionData->filePath << endl;
        }
        sessionData->file = NULL;
    }
    
    cleanup_session_data(ctx);
    
    return SMFIS_CONTINUE;
}


struct smfiDesc smfilter =
{
    const_cast<char*>(milterName.c_str()),	/* filter name */
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

void usage_message() {
    cout 
        << milterName << endl
        << "Usage:" << endl
        << "  -s <socket path> (default: " << socketPath << ")" << endl
        << "  -o <output directory> (default: " << outputDir << ")" << endl
        << "  -f <envelope from header name> (default: " << envFromHeader << ", empty to omit this header)" << endl
        << "  -t <envelope rcpt-to header name> (default: " << envRcptHeader << ", empty to omit this header)" << endl;
}


/**
 * 
 */
void configure(int argc, char *argv[]) {

    const char *options = "s:o:f:t:h";
    int c;
    while ((c = getopt(argc, argv, options)) != -1) {
        switch(c) {
            
            case 's':
                socketPath = optarg;
                break;
                
            case 'o':
                struct stat statbuf;
                if (stat(optarg, &statbuf) == -1) {
                    cerr << "Cannot set output dir (does not exist)" << endl;
                } else {
                    outputDir = optarg;
                    cout << "Store messages in " << outputDir << endl;
                }
                break;
                
            case 'f':
                envFromHeader = optarg;
                if( envFromHeader.empty() ) {
                    cout << "Will not add 'envelope from'-header " << endl;
                }
                break;
                
            case 't':
                envRcptHeader = optarg;
                if( envRcptHeader.empty() ) {
                    cout << "Will not add 'enveloper rcpt-to'-header " << endl;
                }
                break;
                
            case 'h':
                usage_message();
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
    std::srand(std::time(nullptr));

    cout << "Bind to " << socketPath << endl;
    smfi_setconn(const_cast<char*>(socketPath.c_str()));

    if (smfi_register(smfilter) == MI_FAILURE) {
        cout << "smfi_register failed\n";
        exit(EX_UNAVAILABLE);
    }

    if (smfi_main() == MI_FAILURE) {
        cout << "smfi_main failed\n";
        exit(EX_UNAVAILABLE);
    }
    
}




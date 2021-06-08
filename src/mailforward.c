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

char *smtpHost;
int smtpPort;
char *envFrom;
char *envTo;
char *heloName;


typedef struct message_line {
    char *line;
    struct message_line *next;
} message_line_t;

/**
 * 
 */
int message_line_count(message_line_t *start) {
    int c = 0;
    message_line_t *curr = start;
    while( curr != NULL ) {
        c++;
        curr = curr->next;
    }
    return c;
}

/**
 * Free memory of whole chain
 */
void message_line_free(message_line_t *start) {
    if( start == NULL ) {
        return;
    }
    message_line_free(start->next);
    
    free(start->line);
    free(start->next);
}

/**
 * 
 */
int resolve(const char *serverName, char *resolvedName, int size) {
    
    struct addrinfo hints;
    memset (&hints, 0, sizeof (hints));
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags |= AI_CANONNAME;
    
    
    struct addrinfo *info;
    int r = getaddrinfo(serverName, NULL, &hints, &info);
    if( r != 0 ) {
        fprintf(stderr, "Cannot resolve host: %s\n", serverName);
        return r;
    }
    
    struct addrinfo *result = info;
    while( result ) {
    
        void *ptr;
        if( result->ai_family == AF_INET ) {
            ptr = &((struct sockaddr_in *) result->ai_addr)->sin_addr;
        } else if( result->ai_family == AF_INET6 ) {
            ptr = &((struct sockaddr_in6 *) result->ai_addr)->sin6_addr;
        }
        
        inet_ntop( result->ai_family, ptr, resolvedName, size );
        
        result = result->ai_next;
    }
    
    return 0;
}

/**
 * 
 */
int open_socket(const char *serverName, int port) {
    
    char ip[255];
    resolve(serverName, ip, sizeof(ip));
    if( ip[0] == '\0' ) {
        return -1;
    }
    
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if( sockfd < 0 ) {
        fprintf(stderr, "Failed to create socket\n");
        return -1;
    }
    
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    
    if(inet_pton(AF_INET, ip, &serv_addr.sin_addr)<=0) {
        fprintf(stderr, "Invalid address/ Address not supported: %s\n", serverName);
        close(sockfd);
        return -1;
    }
    
    if (connect(sockfd,(struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        fprintf(stderr, "Connection failed: %s:%i\n", serverName, port);
        close(sockfd);
        sockfd = -1;
    }
    
    return sockfd;
}

/**
 * Read and check server response.
 * Return -1 on error response, 0 otherwise
 */
int read_line(int socket) {

    char response[255];
    char c;
    int i=0;
    while( 1 ) {
        
        int n = read(socket, &c, 1);
        if( n < 1 ) {
            return -1;
        }
        
        if( c == '\n' ) {
            
            if( i > 0 && (response[0] == '2' || response[0] == '3') ) {
                return 0;
            } else {
                fprintf(stderr, "Got error response: %s\n", response);
                return -1;
            }
        }
        
        if( i < 254 ) {
            response[i] = c;
            response[i+1] ='\0';
            i++;
        }
    }
    
}


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
    
    // Check if file exists
    struct stat fileStat;
    if( stat(filePath, &fileStat) != 0 ) {
        fprintf(stderr, "File not found: %s\n", filePath);
        return -1;
    }
    
    FILE *fp = fopen(filePath, "r");
    if( fp == NULL ) {
        fprintf(stderr, "Failed to open file: %s\n", filePath);
        return -1;
    }
    
    //printf("Reading %s\n", filePath);
    
    // As we are sending a file as it is right after the SMTP DATA command, 
    // check if the given file has headers and body.
    message_line_t *header = NULL;
    message_line_t *body = NULL;
    char line[255];
    message_line_t *readInto = NULL;
    while(fgets(line, sizeof(line), fp)) {
        
        if( header == NULL ) {
            header = malloc(sizeof(message_line_t));
            readInto = header;
        } else if( body == NULL && (line[0] == '\r' || line[0] == '\n') ) {
            body = malloc(sizeof(message_line_t));
            readInto = body;
        } else {
            readInto->next = malloc(sizeof(message_line_t));
            readInto = readInto->next;
        }
        
        readInto->next = NULL;
        
        int len = strlen(line);
        readInto->line = malloc(len+1);
        strcpy(readInto->line, line);
        
        //read up to 3 body lines here
        //rest will be read in smtp dialog below
        if( body != NULL && message_line_count(body) > 3 ) {
            break;
        }
    }
    
    int headerLines = message_line_count(header);
    int bodyLines = message_line_count(body);
    //printf("%s: %i header lines, %i body lines\n", filePath, headerLines, bodyLines);
    if( headerLines == 0 || bodyLines == 0 ) {
        fprintf(stderr, "Message does not seem to have headers or body (%i header lines, %i body lines)\n", headerLines, bodyLines);
        fclose(fp);
        exit(EX_DATAERR);
    }
    
    int socket = open_socket(smtpHost, smtpPort);
    char buf[255];
    if( socket > 0 ) {
        
        //Read SMTP Greeting
        if( read_line(socket) != 0 ) {
            return -1;
        }
        
        snprintf(buf, sizeof(buf), "helo %s\r\n", heloName);
        write(socket, buf, strlen(buf));
        if( read_line(socket) != 0 ) {
            return -1;
        }
        
        snprintf(buf, sizeof(buf), "mail from: <%s>\r\n", envFrom);
        write(socket, buf, strlen(buf));
        if( read_line(socket) != 0 ) {
            return -1;
        }
        
        snprintf(buf, sizeof(buf), "rcpt to: <%s>\r\n", envTo);
        write(socket, buf, strlen(buf));
        if( read_line(socket) != 0 ) {
            return -1;
        }
        
        snprintf(buf, sizeof(buf), "data\r\n");
        write(socket, buf, strlen(buf));
        if( read_line(socket) != 0 ) {
            return -1;
        }
        
        // Send header which has been read above
        message_line_t *curr = header;
        while( curr != NULL ) {
            snprintf(buf, sizeof(buf), "%s", curr->line);
            write(socket, buf, strlen(buf));
            curr = curr->next;
        }
        message_line_free(header);
        
        // Send body which has been read above
        curr = body;
        while( curr != NULL ) {
            snprintf(buf, sizeof(buf), "%s", curr->line);
            write(socket, buf, strlen(buf));
            curr = curr->next;
        }
        message_line_free(body);
        
        // Send rest of body directly from file
        while(fgets(line, sizeof(line), fp)) {
            write(socket, line, strlen(line));
        }
        
        snprintf(buf, sizeof(buf), ".\r\n");
        write(socket, buf, strlen(buf));
        if( read_line(socket) != 0 ) {
            return -1;
        }
        
        snprintf(buf, sizeof(buf), "quit\r\n");
        write(socket, buf, strlen(buf));
        if( read_line(socket) != 0 ) {
            return -1;
        }
        
        close(socket);
    }
    
    fclose(fp);
    
    printf("Successfully sent %s\n", filePath);
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
        if( send_file(argv[i]) != 0 ) {
            fprintf(stderr, "Failed to send message %s\n", argv[i]);
            fprintf(stderr, "Exit.\n");
            exit(EX_IOERR);
        }
    }
    
    return 0;
}




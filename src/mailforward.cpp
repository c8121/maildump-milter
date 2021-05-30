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
#include <unistd.h>
#include <string.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <sysexits.h>

using namespace std;

string smtpHost;
int smtpPort;
string envFrom;
string envTo;
string heloName;

/**
 * 
 */
string resolve(const char *serverName) {

    string resolvedName {""};
    
    struct addrinfo hints;
    memset (&hints, 0, sizeof (hints));
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags |= AI_CANONNAME;
    
    
    struct addrinfo *info;
    int r = getaddrinfo(serverName, NULL, &hints, &info);
    if( r != 0 ) {
        cout << "Cannot resolve host: " << serverName << endl;
        return resolvedName;
    }
    
    struct addrinfo *result = info;
    char addrstr[255];
    while( result ) {
    
        void *ptr;
        if( result->ai_family == AF_INET ) {
            ptr = &((struct sockaddr_in *) result->ai_addr)->sin_addr;
        } else if( result->ai_family == AF_INET6 ) {
            ptr = &((struct sockaddr_in6 *) result->ai_addr)->sin6_addr;
        }
        
        inet_ntop( result->ai_family, ptr, addrstr, 255 );
        //cout << "Resolve " << serverName << ": " << addrstr << endl;
        resolvedName = string(addrstr);
        
        result = result->ai_next;
    }
    
    return resolvedName;
}

/**
 * 
 */
int open_socket(const char *serverName, int port) {
    
    string ip = resolve(serverName);
    if( ip.empty() ) {
        return -1;
    }
    
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if( sockfd < 0 ) {
        cerr << "Failed to create socket" << endl;
        return -1;
    }
    
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    
    if(inet_pton(AF_INET, const_cast<char*>(ip.c_str()), &serv_addr.sin_addr)<=0) {
        cerr << "Invalid address/ Address not supported: " << serverName << endl;
        close(sockfd);
        return -1;
    }
    
    if (connect(sockfd,(struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        cerr << "Connection failed: " << serverName << ":" << port << endl;
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

    string response = "";
    char c;
    while( true ) {
        
        int n = read(socket, &c, 1);
        if( n < 1 ) {
            cout << endl;
            return -1;
        }
        
        if( c == '\n' ) {
            
            //cout << "< " << response << endl;
            
            if( response.size() > 0 && (response.at(0) == '2' || response.at(0) == '3') ) {
                return 0;
            } else {
                cerr << "Got error response: " << response << endl;
                return -1;
            }
        }
        
        response.push_back(c);
    }
    
}

/**
 * 
 */
void send_line(int socket, string command) {
    
    //cout << "> " << command << endl;
    
    if( !command.empty() ) {
        write(socket, const_cast<char*>(command.c_str()), command.length());
    }
    write(socket, "\r\n", 2);
}

/**
 * 
 */
void usage_message() {
    cout << "Usage: mailforward <host> <port> <from> <to> <filename> [filename...]" << endl;
}


/**
 * 
 */
int send_file(const char *filePath) {
    
    // Check if file exists
    struct stat fileStat;
    if( stat(filePath, &fileStat) != 0 ) {
        cerr << "File not found: " << filePath << endl;
        return -1;
    }
    
    ifstream fileInput(filePath);
    if( !fileInput.is_open() ) {
        cerr << "Failed to open file: " << filePath << endl;
        return -1;
    }
    
    // As we are sending a file as it is right after the SMTP DATA command, 
    // check if the given file has headers and body.
    vector<string> headers(0);
    vector<string> body(0);
    string line;
    vector<string> *readInto = &headers;
    while(getline(fileInput, line)) {
        
        if( line.empty() ) {
            readInto = &body;
        } else {
        
            //read up to 3 body lines here
            //rest will be read in smtp dialog below
            if( body.size() > 3 ) {
                break;
            }
            
            readInto->push_back(line);
        }
    }
    
    if( headers.size() == 0 || body.size() == 0 ) {
        cerr << "Message does not seem to have headers or body (" << headers.size() << " header lines, " << body.size() << " body lines)" << endl;
        fileInput.close();
        exit(EX_DATAERR);
    }
    
    //cout << "Message with " << headers.size() << " headers" << endl;
    
    int socket = open_socket(const_cast<char*>(smtpHost.c_str()), smtpPort);
    if( socket > 0 ) {
        
        send_line(socket, "helo " + heloName);
        if( read_line(socket) != 0 ) {
            return -1;
        }
        
        send_line(socket, "mail from: <" + envFrom + ">");
        if( read_line(socket) != 0 ) {
            return -1;
        }
        
        send_line(socket, "rcpt to: <" + envTo + ">");
        if( read_line(socket) != 0 ) {
            return -1;
        }
        
        send_line(socket, "data");
        if( read_line(socket) != 0 ) {
            return -1;
        }
        
        for( int i=0 ; i < headers.size() ; i++ ) {
            send_line(socket, headers[i]);
        }
        
        send_line(socket, "");
        
        for( int i=0 ; i < body.size() ; i++ ) {
            send_line(socket, body[i]);
        }
        
        while(getline(fileInput, line)) {
            send_line(socket, line);
        }
        
        send_line(socket, ".");
        if( read_line(socket) != 0 ) {
            return -1;
        }
        
        send_line(socket, "quit");
        if( read_line(socket) != 0 ) {
            return -1;
        }
        
        close(socket);
    }
    
    fileInput.close();
    
    return 0;
}


/**
 * 
 */
int main(int argc, char *argv[]) {
    
    if (argc < 6) {
        cerr << "Missing arguments" << endl;
        usage_message();
        exit(EX_USAGE);
    }
    
    // SMTP Server
    smtpHost = argv[1];
    smtpPort = atoi(argv[2]);
    
    // Check sender and recipient
    envFrom = string(argv[3]);
    envTo = string(argv[4]);
    if( envFrom.empty() || envTo.empty() ) {
        cerr << "Parameter 'from' and 'to' cannot be empty" << endl;
        usage_message();
        exit(EX_USAGE);
    }
    
    // Read hostname
    char buf[128] {0};
    gethostname(buf, sizeof(buf));
    heloName = string(buf);
    
    
    for( int i=5 ; i < argc ; i++ ) {
        if( send_file(argv[i]) != 0 ) {
            cerr << "Failed to send message " << argv[i] << endl;
            cerr << "Exit." << endl;
            exit(EX_IOERR);
        }
    }
    
    return 0;
}




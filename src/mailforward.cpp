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
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <sysexits.h>

using namespace std;

/**
 * 
 */
int open_socket(const char *serverName, int port) {
    
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if( sockfd < 0 ) {
        cerr << "Failed to create socket" << endl;
        return -1;
    }
    
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    
    if(inet_pton(AF_INET, serverName, &serv_addr.sin_addr)<=0) {
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
 * 
 */
void read_line(int socket) {

    char c;
    while( true ) {
        int n = read(socket, &c, 1);
        if( n < 1 ) {
            cout << endl;
            return;
        }
        cout << c;
        if( c == '\n' ) {
            cout << endl;
            return;
        }
    }
    
}

/**
 * 
 */
void send_command(int socket, string command) {
    write(socket, const_cast<char*>(command.c_str()), command.length());
    write(socket, "\r\n", 2);
}


/**
 * 
 */
int main(int argc, char *argv[]) {
    
    if (argc < 4) {
       cerr << "Usage: " << argv[0] << " <ip> <port> <filename>" << endl;
       exit(EX_USAGE);
    }
    
    int socket = open_socket(argv[1], atoi(argv[2]));
    if( socket > 0 ) {
        
        send_command(socket, "helo me");
        read_line(socket);
        
        send_command(socket, "mail from: <me>");
        read_line(socket);
        
        send_command(socket, "rcpt to: <root>");
        read_line(socket);
        
        // TODO (WIP): Send File
        
        //TEST 
        /*send_command(socket, "data");
        read_line(socket);
        
        
        send_command(socket, "Subject: Test");
        send_command(socket, "");
        send_command(socket, "Test");
        send_command(socket, ".");
        
        
        send_command(socket, "quit");
        read_line(socket);*/
        
        send_command(socket, "rset");
        read_line(socket);
        
        close(socket);
    }
    
}




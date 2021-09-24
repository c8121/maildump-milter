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
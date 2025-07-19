#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>  // for linux functions
#include <arpa/inet.h> // for internet operations
#include <sys/socket.h>   // for socket based functions


#define PORT 8080
#define BUFFER_SIZE 1024  // max size of request

int main(){
    int server_fd ; 
    int new_socket;
    struct sockaddr_in address;
    int addrlen= sizeof(address); 
    char buffer[BUFFER_SIZE]={0}; // stoore incoming client req in buffer


    const char *hello_message ="HTTP/1.1 200 OK\r\n"
                                "Content-Type: text/plain\r\n"
                                "Content-Length: 13\r\n" // Length of content 
                                "\r\n" // CRLF to separate headers from body
                                "Hello, World!";

    //AF_INET says IPV4 
    // SOCK_STREAM says TCP 
    if((server_fd=socket(AF_INET , SOCK_STREAM , 0))==0){
        perror("Socket Failed");
        exit(EXIT_FAILURE);
    }

    // allows port reusability imidiately after closing
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR , &opt, sizeof(opt))) {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr=INADDR_ANY;
    address.sin_port=htons(PORT);


    // bind the socket
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    // listen to requests activly
     if (listen(server_fd, 1) < 0) { 
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

     printf("Server listening on port %d...\n", PORT);
    printf("Waiting for one connection, then will exit.\n");


    //Accept incomming connections
    // creates new socket for the client connection
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
        perror("accept failed");
        exit(EXIT_FAILURE); // Exit if accept fails
    }

    ssize_t bytes_read = read(new_socket, buffer, BUFFER_SIZE - 1);
    if (bytes_read < 0) {
        perror("read failed");
        close(new_socket); // Close client socket
        close(server_fd);  // Close server socket
        exit(EXIT_FAILURE);
    }  // if data present in client req read it 


    buffer[bytes_read] = '\0'; // Null-terminate the received data for string handling

    printf("Received request:\n%s\n", buffer);


    
    if (write(new_socket, hello_message, strlen(hello_message)) < 0) {
        perror("write failed");
    }//send fixed http respose every times

    printf("Sent 'Hello, World!' response.\n");

    // --- 8. Close Sockets and Exit ---
    // Close the client socket first, then the listening server socket.
    close(new_socket);
    close(server_fd);

    printf("Server finished and exited.\n");
    return 0; // Program exits successfully



}
 


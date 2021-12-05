#include <stdio.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>


int main(){

    int network_socket = 0;
    int size = 0;

    struct sockaddr_in server_address;

    if((network_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        perror("Socket could not start - ERROR");
        return EXIT_FAILURE;
    }

    

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(9002);

    server_address.sin_addr.s_addr = INADDR_ANY;


    if(connect(network_socket,(struct  sockaddr *)&server_address, sizeof(server_address)) == -1){
        perror("Connection ERROR - no server on Address");
        return EXIT_FAILURE;
    }

    char server_response[256];
    size = recv(network_socket, &server_response, sizeof(server_response), 0);

    if(size == -1){
        perror("Recver Error");
    }else if(size == 0){
        printf("Server closed");
    }else{
        printf("%s \n", server_response);
    }

    close(network_socket);
    return 0;
}
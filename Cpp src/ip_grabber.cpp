#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sstream>

int main(int argc, char* argv[]) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    int port = atoi(argv[1]);

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port); 

    bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    listen(sockfd, 5);

    std::cout << "Server is listening on port " << port << "\n";

    while(true) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int client_fd = accept(sockfd, (struct sockaddr*)&client_addr, &client_addr_len);

        uint32_t clientip = htonl(client_addr.sin_addr.s_addr);

        std::ostringstream stream;
        for (int i = 3; i >= 0; i--) {
            stream << ((clientip >> (i * 8)) & 0xFF);
            if (i != 0) stream << ".";
        }

        std::cout << "Client IP: " << stream.str() << "\n";
        close(client_fd);
    }

    
}

#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <fcntl.h>


bool scan_port_tcp(const std::string& ip, int port) {
    struct sockaddr_in server_addr;

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) {
        exit(1);
    }

    // set up sockaddr
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip.data());

    fcntl(sockfd, F_SETFL, O_NONBLOCK);

    int result = connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));

    if (result == 0) {
        close(sockfd);
        return true;
    }

    // Use select() to set a timeout
    fd_set write_fds;
    FD_ZERO(&write_fds);
    FD_SET(sockfd, &write_fds);

    struct timeval timeout = { 1, 0 };

    result = select(sockfd + 1, NULL, &write_fds, NULL, &timeout);

    if (result > 0) {
        int so_error;
        socklen_t len = sizeof(so_error);
        if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &so_error, &len) == 0 && so_error == 0) {
            close(sockfd);
            return true;
        }
    }

    close(sockfd);
    return false;
}

void scan_ip(const std::string& ip, bool scan_type (const std::string&, int)) {
    for(size_t i = 0; i < 1500; i++) {
        if (scan_type(ip, i)) {
            std::cout << "Port " << i << " is open on " << ip << "\n";
        }
    }
}

int main(int argc, char* argv[]) {

    std::string ip = argv[1];

    scan_ip(ip, &scan_port_tcp);
}
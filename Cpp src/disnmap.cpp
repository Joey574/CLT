#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <fcntl.h>
#include <vector>

#define DEBUG 1

inline void success(const std::string&, int);
inline std::string as_ip(int, int, int, int);

void scan_port_tcp(const std::string& ip, int port, std::vector<int>& fd, std::vector<int>& p);
void scan_ip(const std::string& ip, void (*scan_type) (const std::string&, int, std::vector<int>&, std::vector<int>&));


inline void success(const std::string& ip, int port) {
    std::cout << "Port " << port << " is open on " << ip << "\n";
}
inline std::string as_ip(int a, int b, int c, int d) {
    return std::to_string(a).append(".").append(std::to_string(b)).append(".").append(std::to_string(c)).append(".").append(std::to_string(d));
}


void scan_port_tcp(const std::string& ip, int port, std::vector<int>& fd, std::vector<int>& p) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) {
        std::cerr << "Error creating socket\n";
        return;
    }

    // set up sockaddr
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip.data());

    fcntl(sockfd, F_SETFL, O_NONBLOCK);

    int result = connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));

    if (result == 0) {
        // success
        success(ip, port);
        close(sockfd);
    } else if (result == -1 && errno != EINPROGRESS) {
        // failure
        close(sockfd);
    } else if (result == -1 && errno == EINPROGRESS) {
        // in progress
        fd.push_back(sockfd);
        p.push_back(port);
    }
}

void scan_ip(const std::string& ip, void (*scan_type) (const std::string&, int, std::vector<int>&, std::vector<int>&)) {
    std::vector<int> fd;
    std::vector<int> port;

    for(size_t i = 0; i < 1500; i++) {
        scan_type(ip, i, fd, port);
    }

    fd_set write_fds;
    FD_ZERO(&write_fds);
    int max_fd = 0;

    for (size_t i = 0; i < fd.size(); i++) {
        FD_SET(fd[i], &write_fds);
        max_fd = fd[i] > max_fd ? fd[i] : max_fd;
    }

    // Set timeout: { seconds, microseconds }
    struct timeval timeout = {1, 0};
    select(max_fd + 1, NULL, &write_fds, NULL, &timeout);

    for (size_t i = 0; i < fd.size(); i++) {
        if (FD_ISSET(fd[i], &write_fds)) {
            int so_error;
            socklen_t len = sizeof(so_error);

            if (getsockopt(fd[i], SOL_SOCKET, SO_ERROR, &so_error, &len) == 0 && so_error == 0) {
                success(ip, port[i]);
            }
            close(fd[i]);
        } else {
            close(fd[i]);
        }
    }
    std::cout << "out\n";
}


int main(int argc, char* argv[]) {

    int a = 192;
    int b = 168;
    int c = 4;

    //#pragma omp parallel for
    for (int d = 0; d < 256; d++) {
        std::string ip = as_ip(a, b, c, d);

        #if DEBUG
        std::string o = "Scanning " + ip + "\n";
        std::cout << o;
        #endif

        scan_ip(ip, &scan_port_tcp);
    }
}
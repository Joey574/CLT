#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <fcntl.h>
#include <vector>
#include <sys/poll.h>
#include <chrono>

#define DEBUG 0

inline void success(const std::string&, int);
inline std::string as_ip(int, int, int, int);

void scan_port_tcp(const std::string& ip, int port, std::vector<struct pollfd>& fd, std::vector<int>& p);
void scan_ip(const std::string& ip, void (*scan_type) (const std::string&, int, std::vector<struct pollfd>&, std::vector<int>&));


inline void success(const std::string& ip, int port) {
    std::cout << "Port " << port << " is open on " << ip << "\n";
}
inline std::string as_ip(int a, int b, int c, int d) {
    return std::to_string(a).append(".").append(std::to_string(b)).append(".").append(std::to_string(c)).append(".").append(std::to_string(d));
}


void scan_port_tcp(const std::string& ip, int port, std::vector<struct pollfd>& poll_fds, std::vector<int>& p) {
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

        struct pollfd pfd;
        pfd.fd = sockfd;
        pfd.events = POLLOUT;
        pfd.revents = 0;
        poll_fds.push_back(pfd);

        p.push_back(port);
    }
}

void scan_ip(const std::string& ip, void (*scan_type) (const std::string&, int, std::vector<struct pollfd>&, std::vector<int>&)) {
    std::vector<struct pollfd> poll_fds;
    std::vector<int> port;

    // reserve space for up to n ports we scan
    poll_fds.reserve(5000);
    port.reserve(5000);

    // scan n ports
    for (size_t i = 0; i < 5000; i++) {
        scan_type(ip, i, poll_fds, port);
    }

    // timeout in ms
    int ret = poll(poll_fds.data(), poll_fds.size(), 500);

    if (ret == -1) {
        perror("poll");
        return;
    } else {
        for (size_t i = 0; i < poll_fds.size(); i++) {
            if (poll_fds[i].revents & POLLOUT) {
                int error = 0;
                socklen_t len = sizeof(error);

                if (getsockopt(poll_fds[i].fd, SOL_SOCKET, SO_ERROR, &error, &len) == 0 && error == 0) {
                    success(ip, port[i]);
                }
            }
        }
    }

    // close 'er up
    for(size_t i = 0; i < poll_fds.size(); i++) {
        close(poll_fds[i].fd);
    }
}


int main(int argc, char* argv[]) {

    auto start = std::chrono::high_resolution_clock::now();

    int a = 10;
    int b = 12;

    #pragma omp parallel for collapse(2)
    for (int c = 0; c < 256; c++) {
        for (int d = 0; d < 256; d++) {
            std::string ip = as_ip(a, b, c, d);

            #if DEBUG
            std::string o = "Scanning " + ip + "\n";
            std::cout << o;
            #endif

            scan_ip(ip, &scan_port_tcp);
        }
    }
    
    auto dur = std::chrono::high_resolution_clock::now() - start;
    std::cout << dur.count() / 1000000.00 << "ms\n";
}
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <fcntl.h>
#include <vector>
#include <sys/poll.h>
#include <chrono>
#include <fstream>

#define LOG 0

#if LOG
std::ofstream log_file;
#endif


struct host {
    std::string ip;
    std::vector<int> ports;
};

struct output_data {
    std::vector<host> active;

    std::string toString() {
        std::string o = "Hosts up: " + std::to_string(active.size()) + "\n\n";

        for (size_t i = 0; i < active.size(); i++) {
            o.append("Host ").append(std::to_string(i)).append(": ").append(active[i].ip).append("\n");
            for(size_t k = 0; k < active[i].ports.size(); k++) {
                o.append("\tPort ").append(std::to_string(active[i].ports[k])).append(": OPEN\n");
            }
        }

        return o;
    }
};


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

    if (result == -1 && errno != EINPROGRESS) {
        // failure
        close(sockfd);
    } else {
        // in progress
        struct pollfd pfd;
        pfd.fd = sockfd;
        pfd.events = POLLOUT;
        pfd.revents = 0;
        poll_fds.push_back(pfd);

        p.push_back(port);
    }
}

void scan_ip(const std::string& ip, void (*scan_type) (const std::string&, int, std::vector<struct pollfd>&, std::vector<int>&), output_data& data) {
    std::vector<struct pollfd> poll_fds;
    std::vector<int> port;
    host h; h.ip = ip;

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

                    #if LOG
                    std::string o = "Port " + std::to_string(port[i]) + " is open on " + ip + "\n";
                    log_file << o;
                    #endif

                    h.ports.push_back(port[i]);
                }
            }
        }
    }

    // close fds
    for(size_t i = 0; i < poll_fds.size(); i++) {
        close(poll_fds[i].fd);
    }

    if (h.ports.size() > 0) {
        data.active.push_back(h);
    }
}


int main(int argc, char* argv[]) {

    auto start = std::chrono::high_resolution_clock::now();

    #if LOG
    log_file = std::ofstream("log", std::ios::trunc);
    #endif

    output_data data;

    int a = 192;
    int b = 168;

    #pragma omp parallel for collapse(2)
    for (int c = 240; c < 256; c++) {
        for (int d = 0; d < 256; d++) {
            std::string ip = as_ip(a, b, c, d);

            #if LOG
            std::string o = "Scanning " + ip + "\n";
            log_file << o;
            #endif

            scan_ip(ip, &scan_port_tcp, data);
        }
    }
    
    #if LOG
    log_file.close();
    #endif

    std::cout << data.toString();

    auto dur = std::chrono::high_resolution_clock::now() - start;
    std::cout << dur.count() / 1000000000.00 << " seconds\n";
}
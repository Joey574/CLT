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
#include <sstream>
#include <set>

#define LOG 1

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
        std::string o = "Hosts up: " + std::to_string(active.size()) + "\n";

        for (size_t i = 0; i < active.size(); i++) {
            o.append("\nHost ").append(std::to_string(i)).append(": ").append(active[i].ip).append("\n");
            for(size_t k = 0; k < active[i].ports.size(); k++) {
                o.append("\tPort ").append(std::to_string(active[i].ports[k])).append(": OPEN\n");
            }
        }
        o.append("\n");

        return o;
    }
};


inline std::string as_ip(int a, int b, int c, int d) {
    return std::to_string(a).append(".").append(std::to_string(b)).append(".").append(std::to_string(c)).append(".").append(std::to_string(d));
}
void parse_ip(const std::string& ipAddress, int& a, int& b, int& c, int& d) {
    std::stringstream ss(ipAddress);
    char dot;

    ss >> a >> dot >> b >> dot >> c >> dot >> d;
}

void scan_port_tcp(const std::string& ip, int port, int idx, std::vector<struct pollfd>& poll_fds, std::vector<int>& p) {
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
        poll_fds[idx] = pfd;

        p[idx] = port;
    }
}

void scan_ip(const std::string& ip, void (*scan_type) (const std::string&, int, int, std::vector<struct pollfd>&, std::vector<int>&), output_data& data) {
    host h; h.ip = ip;

    // outer iteration attempt look
    for (size_t it = 0; it < 5 && h.ports.size() == 0; it++) {
        std::vector<struct pollfd> poll_fds = std::vector<struct pollfd>(1000);
        std::vector<int> ports = std::vector<int>(1000, -1);

        // scan n ports
        #pragma omp parallel for
        for (size_t i = 0; i < 1000; i++) {
            scan_type(ip, i, i, poll_fds, ports);
        }

        /*
    
        Ok so, a lot of work needs to be done here, for example, if a given ip responds to at least one request, we should retry other requests
        as it's possible, if not incredibly likely some got dropped

        On top of that, if we find an active host, we should expand our search range, looking through more ports
    
        */

        // remove any unused spots
        size_t aidx = 0;
        for (size_t i = 0; i < poll_fds.size(); i++) {
            if (ports[i] != -1) {
                // move forward to earliest available spot
                poll_fds[aidx] = poll_fds[i];
                ports[aidx] = ports[i];
                aidx++;
            }
        }
        poll_fds.resize(aidx);
        ports.resize(aidx);

        // timeout in ms
        int ret = poll(poll_fds.data(), poll_fds.size(), 5000);

        if (ret == -1) {
            // poll error, really bad
            perror("poll");
            return;
        } else {
            // check for fds we can write to
            for (size_t i = 0; i < poll_fds.size(); i++) {
                if (poll_fds[i].revents & POLLOUT) {
                    int error = 0;
                    socklen_t len = sizeof(error);

                    if (getsockopt(poll_fds[i].fd, SOL_SOCKET, SO_ERROR, &error, &len) == 0 && error == 0) {

                        #if LOG
                        std::string o = "Port " + std::to_string(ports[i]) + " is open on " + ip + "\n";
                        log_file << o;
                        #endif

                        h.ports.push_back(ports[i]);
                    }
                }
            }
        }

        // close fds
        for(size_t i = 0; i < poll_fds.size(); i++) {
            close(poll_fds[i].fd);
        }
    }

    if (h.ports.size() > 0) {
        data.active.push_back(h);
    }
}


int main(int argc, char* argv[]) {

    auto start = std::chrono::high_resolution_clock::now();

    #if LOG
    log_file = std::ofstream("/home/joey574/repos/CLT/log", std::ios::trunc);

    if (!log_file.is_open()) {
        std::cerr << "Problem opening log file\n";
    }
    #endif

    output_data data;

    std::string ip = argv[1];

    size_t pos = ip.find('/');
    if (pos != std::string::npos) {
        int subnet = std::stoi(&ip[pos + 1]);
        ip = ip.substr(0, pos);

        int a, b, c, d;
        parse_ip(ip, a, b, c, d);

        switch(subnet) {
            case 32:
                for (a = 0; a < 256; a++) {
                    for (b = 0; b < 256; b++) {
                        for (c = 0; c < 256; c++) {
                            for (d = 0; d < 256; d++) {
                                std::string t = as_ip(a, b, c, d);

                                #if LOG
                                std::string o = "Scanning " + t + "\n";
                                log_file << o;
                                #endif

                                scan_ip(t, &scan_port_tcp, data);
                            }
                        }
                    }
                }
                break;
            case 24:
                for (b = 0; b < 256; b++) {
                    for (c = 0; c < 256; c++) {
                        for (d = 0; d < 256; d++) {
                            std::string t = as_ip(a, b, c, d);

                            #if LOG
                            std::string o = "Scanning " + t + "\n";
                            log_file << o;
                            #endif

                            scan_ip(t, &scan_port_tcp, data);
                        }
                    }
                }
                break;
            case 16:
                for (c = 0; c < 256; c++) {
                    for (d = 0; d < 256; d++) {
                        std::string t = as_ip(a, b, c, d);

                        #if LOG
                        std::string o = "Scanning " + t + "\n";
                        log_file << o;
                        #endif

                        scan_ip(t, &scan_port_tcp, data);
                    }
                }
                break;
            case 8:
                for (d = 0; d < 256; d++) {
                    std::string t = as_ip(a, b, c, d);

                    #if LOG
                    std::string o = "Scanning " + t + "\n";
                    log_file << o;
                    #endif

                    scan_ip(t, &scan_port_tcp, data);
                }
                break;
        }

    } else {
        #if LOG
        std::string o = "Scanning " + ip + "\n";
        log_file << o;
        #endif

        scan_ip(ip, &scan_port_tcp, data);
    }

    #if LOG
    log_file << data.toString();
    log_file.close();
    #endif

    std::cout << data.toString();

    auto dur = std::chrono::high_resolution_clock::now() - start;
    std::cout << dur.count() / 1000000000.00 << " seconds\n";
}
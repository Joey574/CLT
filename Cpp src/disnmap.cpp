#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <fcntl.h>

struct pseudo_header {
    u_int32_t source_address;
    u_int32_t dest_address;
    u_int8_t placeholder;
    u_int8_t protocol;
    u_int16_t tcp_length;
};
struct tcp_header {
    u_int16_t source_port;
    u_int16_t dest_port;
    u_int32_t sequence_number;
    u_int32_t acknowledgement_number;
    u_int8_t data_offset;
    u_int8_t flags;
    u_int16_t window_size;
    u_int16_t checksum;
    u_int16_t urgent_pointer;
};
struct ip_header {
    u_int8_t ip_header_length:4;
    u_int8_t ip_version:4;
    u_int8_t type_of_service;
    u_int16_t total_length;
    u_int16_t identification;
    u_int16_t flags_offset;
    u_int8_t ttl;
    u_int8_t protocol;
    u_int16_t checksum;
    u_int32_t source_address;
    u_int32_t dest_address;
};

unsigned short checksum(void *b, int len) {
    unsigned short *buf = (unsigned short *)b;
    unsigned int sum = 0;
    unsigned short result;

    for (sum = 0; len > 1; len -= 2)
        sum += *buf++;
    if (len == 1)
        sum += *(unsigned char *)buf;
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;
    return result;
}



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
bool scan_port_syn(const std::string& ip, int port) {
    int sockfd;

    sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
    if (sockfd < 0) {
        return false;
    }

    struct sockaddr_in dest_addr;
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(port);
    dest_addr.sin_addr.s_addr = inet_addr(ip.data());

    struct ip_header iphdr;
    memset(&iphdr, 0, sizeof(iphdr));
    iphdr.ip_version = 4;
    iphdr.ip_header_length = 5;
    iphdr.total_length = sizeof(struct ip_header) + sizeof(struct tcp_header);
    iphdr.identification = htons(54321);
    iphdr.flags_offset = 0;
    iphdr.ttl = 255;
    iphdr.protocol = IPPROTO_TCP;
    iphdr.source_address = inet_addr("192.168.0.1"); // Use your own source IP address
    iphdr.dest_address = dest_addr.sin_addr.s_addr;

    struct tcp_header tcphdr;
    memset(&tcphdr, 0, sizeof(tcphdr));
    tcphdr.source_port = htons(12345);  // Arbitrary source port
    tcphdr.dest_port = htons(port);
    tcphdr.sequence_number = 0;
    tcphdr.acknowledgement_number = 0;
    tcphdr.data_offset = 5 << 4;  // TCP data offset (5 words = 20 bytes)
    tcphdr.flags = 0x02;  // SYN flag
    tcphdr.window_size = htons(5840);
    tcphdr.checksum = 0;  // Leave checksum 0 now, filled later by pseudo header
    tcphdr.urgent_pointer = 0;

    struct pseudo_header psh;
    psh.source_address = inet_addr("192.168.0.1");  // Use your own source IP address
    psh.dest_address = dest_addr.sin_addr.s_addr;
    psh.placeholder = 0;
    psh.protocol = IPPROTO_TCP;
    psh.tcp_length = htons(sizeof(struct tcp_header));

    int psize = sizeof(struct pseudo_header) + sizeof(struct tcp_header);
    char *packet = (char *)malloc(psize);

    memcpy(packet, (char *)&psh, sizeof(struct pseudo_header));
    memcpy(packet + sizeof(struct pseudo_header), (char *)&tcphdr, sizeof(struct tcp_header));

    tcphdr.checksum = checksum((unsigned short *)packet, psize);

    iphdr.checksum = checksum((unsigned short *)&iphdr, sizeof(struct ip_header));

    char *final_packet = (char *)malloc(sizeof(struct ip_header) + sizeof(struct tcp_header));
    memcpy(final_packet, &iphdr, sizeof(struct ip_header));
    memcpy(final_packet + sizeof(struct ip_header), &tcphdr, sizeof(struct tcp_header));

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
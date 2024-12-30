#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdint.h>

#define BUFFER_SIZE 1024
#define PORT 1234

#define LOG 1

#define Nb 4
#define Nk 8
#define Nr 14

typedef struct {
    uint64_t data[4];
} uint_256_t;
typedef struct {
    uint8_t data[16];
} uint_128_t;

static const uint8_t sbox[256] = {
    0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 
    0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76, 
    0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 
    0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0, 
    0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 
    0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15, 
    0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 
    0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75, 
    0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 
    0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84, 
    0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 
    0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf, 
    0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 
    0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8, 
    0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 
    0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2, 
    0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 
    0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73, 
    0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 
    0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb, 
    0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 
    0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79, 
    0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 
    0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08, 
    0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 
    0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a, 
    0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 
    0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e, 
    0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 
    0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf, 
    0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 
    0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16
};
static const uint8_t rsbox[256] = {
    0x52, 0x09, 0x6a, 0xd5, 0x30, 0x36, 0xa5, 0x38,
    0xbf, 0x40, 0xa3, 0x9e, 0x81, 0xf3, 0xd7, 0xfb,
    0x7c, 0xe3, 0x39, 0x82, 0x9b, 0x2f, 0xff, 0x87,
    0x34, 0x8e, 0x43, 0x44, 0xc4, 0xde, 0xe9, 0xcb,
    0x54, 0x7b, 0x94, 0x32, 0xa6, 0xc2, 0x23, 0x3d,
    0xee, 0x4c, 0x95, 0x0b, 0x42, 0xfa, 0xc3, 0x4e,
    0x08, 0x2e, 0xa1, 0x66, 0x28, 0xd9, 0x24, 0xb2,
    0x76, 0x5b, 0xa2, 0x49, 0x6d, 0x8b, 0xd1, 0x25,
    0x72, 0xf8, 0xf6, 0x64, 0x86, 0x68, 0x98, 0x16,
    0xd4, 0xa4, 0x5c, 0xcc, 0x5d, 0x65, 0xb6, 0x92,
    0x6c, 0x70, 0x48, 0x50, 0xfd, 0xed, 0xb9, 0xda,
    0x5e, 0x15, 0x46, 0x57, 0xa7, 0x8d, 0x9d, 0x84,
    0x90, 0xd8, 0xab, 0x00, 0x8c, 0xbc, 0xd3, 0x0a,
    0xf7, 0xe4, 0x58, 0x05, 0xb8, 0xb3, 0x45, 0x06,
    0xd0, 0x2c, 0x1e, 0x8f, 0xca, 0x3f, 0x0f, 0x02,
    0xc1, 0xaf, 0xbd, 0x03, 0x01, 0x13, 0x8a, 0x6b,
    0x3a, 0x91, 0x11, 0x41, 0x4f, 0x67, 0xdc, 0xea,
    0x97, 0xf2, 0xcf, 0xce, 0xf0, 0xb4, 0xe6, 0x73,
    0x96, 0xac, 0x74, 0x22, 0xe7, 0xad, 0x35, 0x85,
    0xe2, 0xf9, 0x37, 0xe8, 0x1c, 0x75, 0xdf, 0x6e,
    0x47, 0xf1, 0x1a, 0x71, 0x1d, 0x29, 0xc5, 0x89,
    0x6f, 0xb7, 0x62, 0x0e, 0xaa, 0x18, 0xbe, 0x1b,
    0xfc, 0x56, 0x3e, 0x4b, 0xc6, 0xd2, 0x79, 0x20,
    0x9a, 0xdb, 0xc0, 0xfe, 0x78, 0xcd, 0x5a, 0xf4,
    0x1f, 0xdd, 0xa8, 0x33, 0x88, 0x07, 0xc7, 0x31,
    0xb1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xec, 0x5f,
    0x60, 0x51, 0x7f, 0xa9, 0x19, 0xb5, 0x4a, 0x0d,
    0x2d, 0xe5, 0x7a, 0x9f, 0x93, 0xc9, 0x9c, 0xef,
    0xa0, 0xe0, 0x3b, 0x4d, 0xae, 0x2a, 0xf5, 0xb0,
    0xc8, 0xeb, 0xbb, 0x3c, 0x83, 0x53, 0x99, 0x61,
    0x17, 0x2b, 0x04, 0x7e, 0xba, 0x77, 0xd6, 0x26,
    0xe1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0c, 0x7d
};

static const uint8_t rcon[14] = {
    0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 
    0x80, 0x1b, 0x36, 0x6c, 0xd8, 0xab
};

void display_usage() {
    printf("usage\n");
    exit(1);
}

int init_server();
int connect_to_host();

uint64_t mod_exp(uint64_t base, uint64_t exp, uint64_t mod) {
    uint64_t result = 1;
    base = base % mod;

    while (exp > 0) {
        if (exp % 2 == 1) {
            result = (result * base) % mod;
        }

        exp = exp >> 1;
        base = (base * base) % mod;
    }

    return result;    
}
uint64_t rand_uint64_t() {
    uint64_t result = 0;

    result |= (uint64_t)rand() << 48;
    result |= (uint64_t)rand() << 32;
    result |= (uint64_t)rand() << 16;
    result |= (uint64_t)rand();

    return result;
}

uint8_t xtime(uint8_t x) {
    return ((x << 1) ^ (((x >> 7) & 1) * 0x1b));
}
uint8_t invxtime(uint8_t x, uint8_t y) {
    return (((y & 1) * x) ^ 
            ((y >> 1 & 1) * xtime(x)) ^
            ((y >> 2 & 1) * xtime(xtime(x))) ^ 
            ((y >> 3 & 1) * xtime(xtime(xtime(x)))) ^ 
            ((y >> 4 & 1) * xtime(xtime(xtime(xtime(x))))));
}

uint_256_t dhke_handshake(int connection);

void key_expansion(uint8_t* round_keys, const uint8_t* key);
void substitute_bytes(uint8_t state[16]);
void shiftrows(uint8_t state[16]);
void mixcolumns(uint8_t state[16]);
void add_roundkey(uint8_t state[16], const uint8_t* round_keys, uint64_t round);

void inv_substitute_bytes(uint8_t state[16]);
void inv_shiftrows(uint8_t state[16]);
void inv_mixcolumns(uint8_t state[16]);

void cipher(uint8_t* output, const uint8_t* input, const uint_256_t key);
void inv_cipher(uint8_t* output, const uint8_t* input, const uint_256_t key);

char* encrypt(const char* plaintext, const uint_256_t key);
char* decrypt(const char* ciphertext, const uint_256_t key);

int main(int argc, char* argv[]) {
    srand(time(0));

    if (argc < 2 || argc > 3) {
        display_usage();
    }

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0) {
            display_usage();
        }
    }

    int connection;
    int hosting = 0;
    if (strcmp(argv[1], "-h") == 0) {
        // hosting
        hosting = 1;

        #if LOG
        printf("Hosting server\n");
        #endif

        int server_socket = init_server();
        if (connection == -1) {
            printf("Error hosting server\n");
            return 1;
        }

        connection = accept(server_socket, NULL, NULL);
        if (connection == -1) {
            printf("Error connecting to client\n");
            return 1;
        }

        #if LOG
        printf("Client connected\n");
        #endif
    } else {
        // connecting
        char* ip = argv[1];

        #if LOG
        printf("Attempting to connect to %s\n", ip);
        #endif

        connection = connect_to_host(ip);

        if (connection == -1) {
            printf("Error connecting to host\n");
            return 1;
        }

        #if LOG
        printf("Connected to host\n");
        #endif
    }

    // create a shared key via DHKE before further communications
    uint_256_t shared_key = dhke_handshake(connection);

    #if LOG
    printf("DHKE Complete\n");
    #endif

    char* input;
    if (hosting) {
        input = "Hello client... how are you this wonderful morning? It's been a long day of debugging this stupid aes stuff for me :/\x00";
    } else {
        input = "Hello server... im doing great :) tysm, yeah I get you, it's definitely a little finicky, especially that mix columns stuff\x00";
    }

    printf("Local plaintext: %s\n", input);

    for(int i = 0; i < 50; i++) {

        char* ciphertext = encrypt(input, shared_key);
        send(connection, ciphertext, strlen(ciphertext), 0);

        //printf("Local encrypted data: %s\n", ciphertext);

        char* plaintext = decrypt(ciphertext, shared_key);
        printf("Local decrypted: %s\n", plaintext);
    }
    


    // main loop for communicating, at this point client and server are connected and can send data
    while(1) {

    }
}

uint_256_t dhke_handshake(int connection) {
    uint64_t prime = 18446744073709551557UL;
    uint64_t base = 5;

    uint64_t private_key = rand_uint64_t();
    uint64_t public_key = rand_uint64_t();

    uint64_t A = mod_exp(base, private_key, prime);
    send(connection, &A, sizeof(A), 0);

    uint64_t B;
    recv(connection, &B, sizeof(B), 0);

    uint64_t s = mod_exp(B, private_key, prime);

    #if LOG
    printf("Shared Secret: %lu\n", s);
    #endif

    // now that we have a shared secret, we can derive a shared 256 bit key for aes
    srand(s);

    uint_256_t key = { rand_uint64_t(), rand_uint64_t(), rand_uint64_t(), rand_uint64_t()};

    #if LOG
    printf("Shared Key: %lu%lu%lu%lu\n", key.data[0], key.data[1], key.data[2], key.data[3]);
    #endif

    return key;
}


int init_server() {
    int server_socket;
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        return -1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr))) {
        return -1;
    }

    if (listen(server_socket, 1)) {
        return -1;
    }


    return server_socket;
}
int connect_to_host(const char* ip) {
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        return -1;
    }
    
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, ip, &server_addr.sin_addr);

    if (connect(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr))) {
        return -1;
    }

    return server_socket;
}


void key_expansion(uint8_t* round_keys, const uint8_t* key) {
    uint8_t temp[4];
    uint8_t i = 0;

    while (i < Nk * 4) {
        round_keys[i] = key[i];
        i++;
    }

    i = Nk * 4;
    while (i < (Nb * (Nr + 1))) {
        for (int j = 0; j < 4; j++) {
            temp[j] = round_keys[i - 4 + j];
        }

        if (i % (Nk * 4) == 0) {
            uint8_t temp_byte = temp[0];
            temp[0] = sbox[temp[1]];
            temp[1] = sbox[temp[2]];
            temp[2] = sbox[temp[3]];
            temp[3] = sbox[temp_byte];
            temp[0] ^= rcon[i / (Nk * 4)];
        }

        for (int j = 0; j < 4; j++) {
            round_keys[i] = round_keys[i - (Nk * 4)] ^ temp[j];
            i++;
        }
    }
}
void add_roundkey(uint8_t state[16], const uint8_t* round_keys, uint64_t round) {
    for(uint8_t i = 0; i < Nb * 4; i++) {
        state[i] ^= round_keys[i];
    }
}

void substitute_bytes(uint8_t state[16]) {
    for(uint8_t i = 0; i < Nb * 4; i++) {
        state[i] = sbox[state[i]];
    }
}
void shiftrows(uint8_t state[16]) {
    uint8_t tmp;

    // first row rotated 1 column to the left
    tmp = state[4];
    state[4] = state[5];
    state[5] = state[6];
    state[6] = state[7];
    state[7] = tmp;

    // second row rotated 2 columns to the left
    tmp = state[8];
    state[8] = state[10];
    state[10] = tmp;

    tmp = state[9];
    state[9] = state[11];
    state[11] = tmp;

    // third row rotated 3 columns to the left
    tmp = state[12];
    state[12] = state[15];
    state[15] = state[14];
    state[14] = state[13];
    state[13] = tmp;
}
void mixcolumns(uint8_t state[16]) {
    uint8_t tmp, tm, t;

    for(uint8_t i = 0; i < 4; i++) {
        t = state[i];
        tmp = state[i] ^ state[i + 4] ^ state[i + 8] ^ state[i + 12];

        tm = state[i] ^ state[i + 4];
        tm = xtime(tm);
        state[i] ^= tm ^ tmp;

        tm = state[i + 4] ^ state[i + 8];
        tm = xtime(tm);
        state[i + 4] ^= tm ^ tmp;

        tm = state[i + 8] ^ state[i + 12];
        tm = xtime(tm);
        state[i + 8] ^= tm ^ tmp;

        tm = state[i + 12] ^ t;
        tm = xtime(tm);
        state[i + 12] ^= tm ^ tmp;
    }
}

void inv_substitute_bytes(uint8_t state[16]) {
    for(uint8_t i = 0; i < 16; i++) {
        state[i] = rsbox[state[i]];
    }
}
void inv_shiftrows(uint8_t state[16]) {
    uint8_t tmp;

    // first row rotated 1 column to the right
    tmp = state[7];
    state[7] = state[6];
    state[6] = state[5];
    state[5] = state[4];
    state[4] = tmp;

    // second row rotated 2 columns to the right
    tmp = state[8];
    state[8] = state[10];
    state[10] = tmp;

    tmp = state[9];
    state[9] = state[11];
    state[11] = tmp;

    // third row rotated 3 columns to the right
    tmp = state[12];
    state[12] = state[13];
    state[13] = state[14];
    state[14] = state[15];
    state[15] = tmp;
}
void inv_mixcolumns(uint8_t state[16]) {
    uint8_t a, b, c, d;

    for(uint8_t i = 0; i < 4; i++) {
        a = state[i];
        b = state[i + 4];
        c = state[i + 8];
        d = state[i + 12];

        state[i] = invxtime(a, 0x0e) ^ invxtime(b, 0x0b) ^ invxtime(c, 0x0d) ^ invxtime(d, 0x09);
        state[i + 4] = invxtime(a, 0x09) ^ invxtime(b, 0x0e) ^ invxtime(c, 0x0b) ^ invxtime(d, 0x0d);               
        state[i + 8] = invxtime(a, 0x0d) ^ invxtime(b, 0x09) ^ invxtime(c, 0x0e) ^ invxtime(d, 0x0b);
        state[i + 12] = invxtime(a, 0x0b) ^ invxtime(b, 0x0d) ^ invxtime(c, 0x09) ^ invxtime(d, 0x0e);
    }
}

void cipher(uint8_t* output, const uint8_t* input, const uint_256_t key) {
    uint8_t state[16];
    uint8_t round_keys[240];
    memcpy(state, input, 16);

    key_expansion(round_keys, (uint8_t*)&key);

    add_roundkey(state, round_keys, 0);

    for(uint64_t round = 1; round < Nr; round++) {
        substitute_bytes(state);
        shiftrows(state);
        //mixcolumns(state);
        add_roundkey(state, round_keys, round);
    }

    substitute_bytes(state);
    shiftrows(state);
    add_roundkey(state, round_keys, Nr);

    memcpy(output, state, 16);
}
void inv_cipher(uint8_t* output, const uint8_t* input, const uint_256_t key) {
    uint8_t state[16];
    uint8_t round_keys[240];
    memcpy(state, input, 16);

    key_expansion(round_keys, (uint8_t*)&key);

    add_roundkey(state, round_keys, Nr);

    for (uint64_t round = Nr - 1; round > 0; round--) {
        inv_substitute_bytes(state);
        inv_shiftrows(state);
        //inv_mixcolumns(state);
        add_roundkey(state, round_keys, round);
    }

    inv_substitute_bytes(state);
    inv_shiftrows(state);
    add_roundkey(state, round_keys, 0);

    memcpy(output, state, 16);
}

char* encrypt(const char* plaintext, const uint_256_t key) {
    uint8_t input[16];
    uint8_t output[16];

    size_t len = strlen(plaintext) % 16 == 0 ? strlen(plaintext) : (((strlen(plaintext) / 16) + 1) * 16);
    char* ciphertext = (char*)calloc(len + 1, 1);

    // store plaintext in ciphertext to prevent over reading
    memcpy(ciphertext, plaintext, strlen(plaintext));

    // assign last n bytes of padding to n
    uint8_t pad = (len - strlen(plaintext));
    printf("Applied padding: %d\n", pad);
    for(size_t i = strlen(plaintext); i < len; i++) {
        ciphertext[i] = (uint8_t)(len - strlen(plaintext));
    }
    printf("last byte: %d\n", ciphertext[len]);

    for(size_t i = 0; i  < len / 16; i++) {
        memset(output, 0, 16);

        memcpy(input, &ciphertext[i * 16], 16);
        cipher(output, input, key);
        memcpy(&ciphertext[i * 16], output, 16);
    }

    return ciphertext;
}
char* decrypt(const char* ciphertext, const uint_256_t key) {
    uint8_t input[16];
    uint8_t output[16];

    size_t len = strlen(ciphertext);
    char* plaintext = (char*)malloc(len);

    for(size_t i = 0; i < len / 16; i++) {
        memset(output, 0, 16);
        memset(input, 0, 16);

        memcpy(input, &ciphertext[i * 16], 16);
        inv_cipher(output, input, key);
        memcpy(&plaintext[i * 16], output, 16);
    }

    // remove n bytes containing n at end
    printf("last byte: %d\n", plaintext[len - 1]);
    if ((uint8_t)plaintext[len - 1] < 16) {
        uint8_t padlen = plaintext[len - 1];

        printf("padding len: %d\n", padlen);
        memset(&plaintext[len - padlen], 0, padlen);
    }

    

    return plaintext;
}

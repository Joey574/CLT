#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#define LOG 0
#define BUFFER_SIZE 4096

typedef struct {
    size_t len;
    char* str;
} string;

typedef struct {
    size_t* lines;

    size_t failed;
    size_t api_requests;
} LOCs;
size_t locs_size;

typedef struct {
    size_t elements;
    string* str;
} stringarr;

const char* supported_extensions[] = {
    "c",
    "cpp",
    "h",
    "rs",
    "java",
    "cs",
    "js",
    "ts",
    "py",
    "asm",
    "go",
    "compute",
    "cu",
    "cuh",
    "sh",
    "gdb",
    "html",
    "md",
    "lua",
    "php"
};
const char* supported_languages[] = {
    "C",
    "C++",
    "C/C++ Headers",
    "Rust",
    "Java",
    "C#",
    "Javascript",
    "Typescript",
    "Python",
    "Assembly",
    "Go",
    "Hlsl",
    "Cuda",
    "Cuda Headers",
    "Shell Script",
    "Gdb Script",
    "Html",
    "Markdown",
    "Lua",
    "PHP"
};

static const unsigned char base64_table[128] = {
    [0 ... 127] = 255,

    ['A'] = 0,  ['B'] = 1,  ['C'] = 2,  ['D'] = 3,  ['E'] = 4,  ['F'] = 5,
    ['G'] = 6,  ['H'] = 7,  ['I'] = 8,  ['J'] = 9,  ['K'] = 10, ['L'] = 11,
    ['M'] = 12, ['N'] = 13, ['O'] = 14, ['P'] = 15, ['Q'] = 16, ['R'] = 17,
    ['S'] = 18, ['T'] = 19, ['U'] = 20, ['V'] = 21, ['W'] = 22, ['X'] = 23,
    ['Y'] = 24, ['Z'] = 25, ['a'] = 26, ['b'] = 27, ['c'] = 28, ['d'] = 29,
    ['e'] = 30, ['f'] = 31, ['g'] = 32, ['h'] = 33, ['i'] = 34, ['j'] = 35,
    ['k'] = 36, ['l'] = 37, ['m'] = 38, ['n'] = 39, ['o'] = 40, ['p'] = 41,
    ['q'] = 42, ['r'] = 43, ['s'] = 44, ['t'] = 45, ['u'] = 46, ['v'] = 47,
    ['w'] = 48, ['x'] = 49, ['y'] = 50, ['z'] = 51, ['0'] = 52, ['1'] = 53,
    ['2'] = 54, ['3'] = 55, ['4'] = 56, ['5'] = 57, ['6'] = 58, ['7'] = 59,
    ['8'] = 60, ['9'] = 61, ['+'] = 62, ['/'] = 63
};

LOCs g_locs;

void append(string* str, const char* data) {
    str->str = realloc(str->str, str->len + strlen(data) + 1);
    memcpy(&str->str[str->len], data, strlen(data));
    str->len += strlen(data);
    str->str[str->len] = '\x00';
}
void appendn(string* str, const char* data, size_t len) {
    str->str = realloc(str->str, str->len + len + 1);
    memcpy(&str->str[str->len], data, len);
    str->len += len;
    str->str[str->len] = '\x00';
}
void appendch(string* str, char data) {
    str->len++;
    str->str = realloc(str->str, str->len + 1);
    str->str[str->len - 1] = data;
    str->str[str->len] = '\x00';
}
void clearstr(string* str) {
    str->len = 0;
    str->str = (char*)realloc(str->str, 1);
    str->str[0] = '\x00';
}
void printstrarr(stringarr* arr) {
    for(size_t i = 0; i < arr->elements; i++) {
        printf("%s\n", arr->str[i].str);
    }
}

void feedback(const char* pat) {
    const char* CMD = "curl -s -L -H \"Authorization: token ";

    string fcmd = { 0, (char*)malloc(0) };
    append(&fcmd, CMD);
    append(&fcmd, pat);
    append(&fcmd, "\" \"https://api.github.com/user\"");

    char recvbuf[BUFFER_SIZE];
    FILE* pipe;

    string html = { 0, (char*)malloc(0) };

    while(1) {
        printf("Trying request...\n");
        pipe = popen(fcmd.str, "r");

        while(fgets(recvbuf, sizeof(recvbuf), pipe) != NULL) {
            append(&html, recvbuf);
            memset(recvbuf, 0, sizeof(recvbuf));
        }

        pclose(pipe);

        if (strstr(html.str, "API rate limit exceeded") == NULL) {
            printf("Request succeeded: continuing\n");
            break;
        }

        clearstr(&html);
        sleep(120);
    }    
}

stringarr parse_repos(char* user, char* pat, stringarr exclude, LOCs* locs) {
    const char* CURLCMD = "curl -s -H \"Authorization: token ";
    const char* SUBSTRING = "\"url\": \"";

    const char* URL_A = "https://api.github.com/users/";
    const char* URL_B = "https://api.github.com/user/repos?per_page=100";

    const char* USERCHECK = "https://api.github.com/user";

    char recvbuf[BUFFER_SIZE];
    FILE* pipe;

    int u = -1;

    string usercheckcmd = { 0, (char*)malloc(0) };
    append(&usercheckcmd, CURLCMD);
    append(&usercheckcmd, pat);
    append(&usercheckcmd, "\" ");
    append(&usercheckcmd, USERCHECK);

    pipe = popen(usercheckcmd.str, "r");

    while(fgets(recvbuf, sizeof(recvbuf), pipe) != NULL) {
        char* t = strstr(recvbuf, "\"login\": \"");

        if (t != NULL) {
            size_t idx = t - recvbuf + strlen("\"login\": \"");
            size_t eidx = strstr(&recvbuf[idx], "\"") - recvbuf;

            string name = { 0, (char*)malloc(0) };
            appendn(&name, &recvbuf[idx], eidx - idx);

            if (strcmp(name.str, user) == 0) {
                u = 1;
            } else {
                u = 2;
            }

            free(name.str);
        }
    }

    free(usercheckcmd.str);
    pclose(pipe);
    locs->api_requests++;

    string finalcmd = { 0, (char*)malloc(0) };

    append(&finalcmd, CURLCMD);
    append(&finalcmd, pat);
    append(&finalcmd, "\" ");

    if (u == 1) {
        // own user, use URL_B
        append(&finalcmd, URL_B);
    } else if (u == 2) {
        // other user, use URL_A
        append(&finalcmd, URL_A);
        append(&finalcmd, user);
        append(&finalcmd, "/repos?per_page=100");
    } else {
        // ERROR
        printf("ERROR GETTING USER\n%s", recvbuf);
        exit(1);
    }

    string html = { 0, (char*)malloc(0) };

    pipe = popen(finalcmd.str, "r");

    while(fgets(recvbuf, sizeof(recvbuf), pipe) != NULL) {
        append(&html, recvbuf);
        memset(recvbuf, 0, 512);
    }

    pclose(pipe);
    free(finalcmd.str);
    locs->api_requests++;

    string home_repo = { 0, (char*)malloc(0) };
    append(&home_repo, URL_A);
    append(&home_repo, user);

    stringarr repos = { 0, (string*)malloc(1 * sizeof(string)) };

    char* searchstring = html.str;
    while ((searchstring = strstr(searchstring, SUBSTRING)) != NULL) {
        searchstring += strlen(SUBSTRING);

        size_t idx = searchstring - html.str;
        size_t eidx = strstr(searchstring, "\"") - html.str;

        string tmp = { 0, (char*)malloc(0) };
        appendn(&tmp, searchstring, eidx - idx);

        if (strstr(tmp.str, "licenses") == NULL && tmp.len > strlen(home_repo.str) && strstr(tmp.str, user) != NULL) {

            __uint8_t valid = 1;
            for (size_t i = 0; i < exclude.elements && valid; i++) {
                if (strstr(tmp.str, exclude.str[i].str) != NULL) {
                    valid = 0;
                }
            }

            if (valid) {
                repos.elements++;

                repos.str = realloc(repos.str, repos.elements * sizeof(string));
                repos.str[repos.elements - 1].len = 0;
                repos.str[repos.elements - 1].str = (char*)malloc(0);
                append(&repos.str[repos.elements - 1], tmp.str);
                append(&repos.str[repos.elements - 1], "/contents");
            }
        }

        free(tmp.str);
    }

    return repos;
}

void parse_file(LOCs* locs, string url, const char* pat, size_t idx) {
    const char* CURLCMD = "curl -s -L -H \"Authorization: token ";
    const char* SUBSTRING = "\"html\": \"";

    int retried = 0;

    char recvbuf[BUFFER_SIZE];
    FILE* pipe;

    start_f:

    string finalcmd = { 0, (char*)malloc(0) };
    append(&finalcmd, CURLCMD);
    append(&finalcmd, pat);
    append(&finalcmd, "\" \"");
    append(&finalcmd, url.str);
    append(&finalcmd, "\"");

    string html = { 0, (char*)malloc(0) };

    pipe = popen(finalcmd.str, "r");
    while(fgets(recvbuf, sizeof(recvbuf), pipe) != NULL) {
        append(&html, recvbuf);
        memset(recvbuf, 0, sizeof(recvbuf));
    }

    free(finalcmd.str);
    pclose(pipe);

    if (strstr(html.str, "\"content\": \"") != NULL) {
        locs->api_requests++;

        size_t tmp = 1;

        size_t sidx = strstr(html.str, "\"content\": \"") - html.str + strlen("\"content\": \"");
        size_t eidx = strstr(&html.str[sidx], "\"") - html.str;
        size_t len = eidx - sidx;

        char* searchstring = &html.str[sidx];
        for(size_t i = 0, j = 0; i < len;) {
            if(searchstring[i] == '\\' && searchstring[i + 1] == 'n') {
                i += 2;
            } else {
                int val = base64_table[searchstring[i++]] << 18;
                val |= base64_table[searchstring[i++]] << 12;
                val |= base64_table[searchstring[i++]] << 6;
                val |= base64_table[searchstring[i++]];

                if ((char)(val >> 16) == '\n') { tmp++; }
                if ((char)(val >> 8) == '\n') { tmp++; }
                if ((char)(val & 0xff) == '\n') { tmp++; }
            }
        }

        locs->lines[idx] += tmp;
        free(html.str);
    } else {
        if (strstr(html.str, "API rate limit exceeded") != NULL) {
            free(html.str);
            feedback(pat);
            goto start_f;
        } else if (!retried) {
            retried++;
            free(html.str);
            printf("Unknown error, sleeping then retrying\n");
            sleep(5);
            goto start_f;
        }

        printf("SUBSTRING NOT FOUND\n");
        printf("%s\n%s\n", html.str, url.str);
        free(html.str);
        locs->failed++;
    }
}
void parse_dir(LOCs* locs, string url, const char* pat) {
    const char* CURLCMD = "curl -s -L -H \"Authorization: token ";
    const char* NAMESUBSTRING = "\"name\": \"";
    const char* URLSUBSTRING = "\"url\": \"";
    const char* TYPESUBSTRING = "\"type\": \"";

    int retried = 0;

    char recvbuf[BUFFER_SIZE];
    FILE* pipe;

    start_d:

    string finalcmd = { 0, (char*)malloc(0) };
    append(&finalcmd, CURLCMD);
    append(&finalcmd, pat);
    append(&finalcmd, "\" \"");
    append(&finalcmd, url.str);
    append(&finalcmd, "\"");

    string html = { 0, (char*)malloc(0) };

    pipe = popen(finalcmd.str, "r");

    while(fgets(recvbuf, sizeof(recvbuf), pipe) != NULL) {
        append(&html, recvbuf);
        memset(recvbuf, 0, sizeof(recvbuf));
    }

    free(finalcmd.str);
    pclose(pipe);

    if (strstr(html.str, NAMESUBSTRING) != NULL) {
        locs->api_requests++;
        string name = { 0, (char*)malloc(0) };

        char* searchstring = html.str;
        while ((searchstring = strstr(searchstring, NAMESUBSTRING)) != NULL) {
            searchstring += strlen(NAMESUBSTRING);

            size_t idx = searchstring - html.str;
            size_t eidx = strstr(searchstring, "\"") - html.str;

            appendn(&name, searchstring, eidx - idx);

            size_t urlidx = strstr(searchstring, URLSUBSTRING) - searchstring + strlen(URLSUBSTRING);
            size_t eurlidx = strstr(&searchstring[urlidx], "\"") - searchstring;

            string tmp_url = { 0, (char*)malloc(0) };
            appendn(&tmp_url, &searchstring[urlidx], eurlidx - urlidx);

            size_t typeidx = strstr(searchstring, TYPESUBSTRING) - searchstring + strlen(TYPESUBSTRING);

            if (searchstring[typeidx] == 'f') {
            
                char* fileext = name.str;
                while((fileext = strstr(fileext, ".")) != NULL) {
                    fileext += strlen(".");
                    if (strstr(fileext, ".") == NULL) {
                        break;
                    }
                }

                if (fileext != NULL) {
                    for(size_t i = 0; i < locs_size; i++) {
                        if (strcmp(fileext, supported_extensions[i]) == 0) {
                            #if LOG
                            printf("File: %s\n", name.str);
                            #endif

                            parse_file(locs, tmp_url, pat, i);
                            break;
                        }
                    }  

                    /*
                        additional check for hpp files, 2 -> index of h files, done here to keep consistent indexing
                        between supported languages and supported extentions for easy output
                    */
                    if (strcmp(fileext, "hpp") == 0) {
                        parse_file(locs, tmp_url, pat, 2);
                    }
                }
            } else if (searchstring[typeidx] == 'd') {
                #if LOG
                printf("Searching: %s\n", tmp_url.str);
                #endif

                parse_dir(locs, tmp_url, pat);
            } else {
                printf("ERROR READING TYPE\n");
                exit(1);
            }

            free(tmp_url.str);
            clearstr(&name);        
        }
        
        free(name.str);
        free(html.str);
    } else {
        if (strstr(html.str, "API rate limit exceeded") != NULL) {
            free(html.str);
            feedback(pat);
            goto start_d;
        } else if (strstr(html.str, "This repository is empty") != NULL) {
            free(html.str);
            return;
        } else if (!retried) {
            retried++;
            free(html.str);
            printf("Unknown error, sleeping then retrying\n");
            sleep(5);
            goto start_d;
        }

        printf("SUBSTRING NOT FOUND\n");
        printf("%s\n%s\n", html.str, url.str);
        free(html.str);
        locs->failed++;
    }
}

void output_locs(LOCs* locs) {
    printf("\nLines of Code:\n");

    size_t total = 0;
    for(size_t i = 0; i < locs_size; i++) {
        if (locs->lines[i]) {
            total += locs->lines[i];
            printf("%s: %lu\n", supported_languages[i], locs->lines[i]);
        }
    }

    if (total) { printf("Total Lines: %lu\n", total); }
    if (locs->api_requests) { printf("Made %lu Github API requests\n", locs->api_requests); }
    if (locs->failed) { printf("Failed to read %lu files and/or directories\n", locs->failed); }
}
void sigint_handle(int sig) {
    printf("\nProgram Interrupt:\n");
    output_locs(&g_locs);
    exit(1);
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        return 1;
    }

    char* user;
    char* pat;
    
    stringarr excluded = { 0, (string*)malloc(0) };

    for(int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-u") == 0) {
            if (argc > i + 1) {
                user = argv[i + 1];
            }
        }

        if (strcmp(argv[i], "-t") == 0) {
            if (argc > i + 1) {
                pat = argv[i + 1];
            }
        }
        
        if (strcmp(argv[i], "-e") == 0) {
            if (argc > i + 1) {
                excluded.elements++;
                excluded.str = (string*)realloc(excluded.str, excluded.elements * sizeof(string));

                excluded.str[excluded.elements - 1].len = strlen(argv[i + 1]);
                excluded.str[excluded.elements - 1].str = argv[i + 1];
            }
            
        }
    }

    locs_size = sizeof(supported_languages) / sizeof(supported_languages[0]);

    memset(&g_locs, 0, sizeof(g_locs));
    g_locs.lines = (size_t*)calloc(locs_size, sizeof(size_t));

    // setup function to handle interrupts
    if (signal(SIGINT, sigint_handle) == SIG_ERR) {
        perror("Error setting up signal handler");
        return 1;
    }

    stringarr repos = parse_repos(user, pat, excluded, &g_locs);

    #if LOG
    printf("Repos:\n");
    printstrarr(&repos);
    printf("\n");
    #endif

    string name = { 0, (char*)malloc(0) };

    // start search on each repo in repos
    for(size_t i = 0; i < repos.elements; i++) {
        size_t sidx = strstr(repos.str[i].str, user) - repos.str[i].str + strlen(user) + 1;
        size_t eidx = strstr(&repos.str[i].str[sidx], "/contents") - repos.str[i].str;

        appendn(&name, &repos.str[i].str[sidx], eidx - sidx);

        printf("Searching: %s\n", name.str);
        clearstr(&name);

        parse_dir(&g_locs, repos.str[i], pat);
    }

    free(name.str);
    output_locs(&g_locs);
}

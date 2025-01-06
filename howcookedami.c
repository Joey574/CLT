#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOG 0

typedef struct {
    size_t len;
    char* str;
} string;

typedef struct {
    size_t c;
    size_t h;
    size_t cpp;
    size_t hpp;
    size_t csharp;
    size_t js;
    size_t py;
    size_t as;
    size_t java;
    size_t go;
    size_t hlsl;
    size_t cuda;
    size_t cudah;
    size_t shell;
    size_t gdb;
    size_t html;
    size_t md;
    size_t failed;
} LOCs;

typedef struct {
    size_t elements;
    string* str;
} stringarr;

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

stringarr parse_repos(char* user, char* pat, stringarr exclude) {
    const char* CURLCMD = "curl -s -H \"Authorization: token ";
    const char* SUBSTRING = "\"url\": \"";
    const char* URL = "https://api.github.com/users/";
    const char* HOMEURL = "https://api.github.com/users/";

    char recvbuf[512];
    FILE* pipe;

    string finalcmd = { 0, (char*)malloc(0) };
    append(&finalcmd, CURLCMD);
    append(&finalcmd, pat);
    append(&finalcmd, "\" ");
    append(&finalcmd, URL);
    append(&finalcmd, user);
    append(&finalcmd, "/repos");

    string html = { 0, (char*)malloc(0) };

    pipe = popen(finalcmd.str, "r");

    while(fgets(recvbuf, sizeof(recvbuf), pipe) != NULL) {
        append(&html, recvbuf);
        memset(recvbuf, 0, 512);
    }

    pclose(pipe);
    free(finalcmd.str);

    string home_repo = { 0, (char*)malloc(0) };
    append(&home_repo, HOMEURL);
    append(&home_repo, user);

    stringarr repos = { 0, (string*)malloc(1 * sizeof(string)) };

    char* searchstring = html.str;
    while ((searchstring = strstr(searchstring, SUBSTRING)) != NULL) {
        searchstring += strlen(SUBSTRING);

        size_t idx = searchstring - html.str;
        size_t eidx = strstr(searchstring, "\"") - html.str;

        string tmp = { 0, (char*)malloc(0) };
        appendn(&tmp, searchstring, eidx - idx);

        if (strstr(tmp.str, "licenses") == NULL && tmp.len > strlen(home_repo.str)) {
            
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

    #if LOG
    printstrarr(&repos);
    #endif

    return repos;
}

void parse_file(LOCs* locs, string url, const char* pat, const char* fileext) {
    const char* CURLCMD = "curl -s -L -H \"Authorization: token ";
    const char* SUBSTRING = "\"html\": \"";

    char recvbuf[512];
    FILE* pipe;

    string finalcmd = { 0, (char*)malloc(1) };
    append(&finalcmd, CURLCMD);
    append(&finalcmd, pat);
    append(&finalcmd, "\" \"");
    append(&finalcmd, url.str);
    append(&finalcmd, "\"");

    string html = { 0, (char*)malloc(0) };

    pipe = popen(finalcmd.str, "r");
    while(fgets(recvbuf, sizeof(recvbuf), pipe) != NULL) {
        append(&html, recvbuf);
        memset(recvbuf, 0, 512);
    }

    free(finalcmd.str);
    pclose(pipe);

    size_t tmp = 0;

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

    if (strcmp(fileext, "c") == 0) { locs->c += tmp; }
    else if (strcmp(fileext, "h") == 0) { locs->h += tmp; }
    else if (strcmp(fileext, "cpp") == 0) { locs->cpp += tmp; }
    else if (strcmp(fileext, "hpp") == 0) { locs->hpp += tmp; }
    else if (strcmp(fileext, "cs") == 0) { locs->csharp += tmp; }
    else if (strcmp(fileext, "js") == 0) { locs->js += tmp; }
    else if (strcmp(fileext, "py") == 0) { locs->py += tmp; }
    else if (strcmp(fileext, "asm") == 0) { locs->as += tmp; }
    else if (strcmp(fileext, "java") == 0) { locs->java += tmp; }
    else if (strcmp(fileext, "go") == 0) { locs->go += tmp; }
    else if (strcmp(fileext, "compute") == 0) { locs->hlsl += tmp; }
    else if (strcmp(fileext, "cu") == 0) { locs->cuda += tmp; }
    else if (strcmp(fileext, "cuh") == 0) { locs->cudah += tmp; }
    else if (strcmp(fileext, "sh") == 0) { locs->shell += tmp; }
    else if (strcmp(fileext, "gdb") == 0) { locs->gdb += tmp; }
    else if (strcmp(fileext, "html") == 0) { locs->html += tmp; }
    else if (strcmp(fileext, "md") == 0) { locs->md += tmp; }
    else { printf("ERROR PARSING EXT\n"); }

    if (strstr(html.str, "\"content\": \"") == NULL)  {
        locs->failed++;
    }

    free(html.str);
}
void parse_dir(LOCs* locs, string url, const char* pat) {
    const char* CURLCMD = "curl -s -L -H \"Authorization: token ";
    const char* NAMESUBSTRING = "\"name\": \"";
    const char* URLSUBSTRING = "\"url\": \"";
    const char* TYPESUBSTRING = "\"type\": \"";

    char recvbuf[512];
    FILE* pipe;

    string finalcmd = { 0, (char*)malloc(1) };
    append(&finalcmd, CURLCMD);
    append(&finalcmd, pat);
    append(&finalcmd, "\" \"");
    append(&finalcmd, url.str);
    append(&finalcmd, "\"");

    string html = { 0, (char*)malloc(0) };

    pipe = popen(finalcmd.str, "r");

    while(fgets(recvbuf, sizeof(recvbuf), pipe) != NULL) {
        append(&html, recvbuf);
        memset(recvbuf, 0, 512);
    }

    free(finalcmd.str);
    pclose(pipe);


    string name = { 0, (char*)malloc(0) };

    char* searchstring = html.str;
    while ((searchstring = strstr(searchstring, NAMESUBSTRING)) != NULL) {
        searchstring += strlen(NAMESUBSTRING);

        size_t idx = searchstring - html.str;
        size_t eidx = strstr(searchstring, "\"") - html.str;

        appendn(&name, searchstring, eidx - idx);

        size_t urlidx = strstr(searchstring, URLSUBSTRING) - searchstring + strlen(URLSUBSTRING);
        size_t eurlidx = strstr(&searchstring[urlidx], "\"") - searchstring;

        string tmp_url = { 0, (char*)malloc(1) };
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

            if (
                fileext != NULL && (
                strcmp(fileext, "c") == 0 ||
                strcmp(fileext, "h") == 0 ||
                strcmp(fileext, "cpp") == 0 ||
                strcmp(fileext, "hpp") == 0 ||
                strcmp(fileext, "cs") == 0 ||
                strcmp(fileext, "js") == 0 ||
                strcmp(fileext, "py") == 0 ||
                strcmp(fileext, "asm") == 0 ||
                strcmp(fileext, "java") == 0 ||
                strcmp(fileext, "go") == 0 ||
                strcmp(fileext, "compute") == 0 ||
                strcmp(fileext, "cu") == 0 ||
                strcmp(fileext, "cuh") == 0 ||
                strcmp(fileext, "sh") == 0 ||
                strcmp(fileext, "gdb") == 0 ||
                strcmp(fileext, "html") == 0 ||
                strcmp(fileext, "md") == 0
            )) {
                #if LOG
                printf("File: %s\n", name.str);
                #endif

                parse_file(locs, tmp_url, pat, fileext);
            }
        } else if (searchstring[typeidx] == 'd') {

            #if LOG
            printf("Searching: %s\n", tmp_url.str);
            #endif

            parse_dir(locs, tmp_url, pat);

            #if LOG
            printf("Searching: %s\n", url.str);
            #endif
            
        } else {
            printf("ERROR READING TYPE\n");
            exit(1);
        }

        free(tmp_url.str);
        clearstr(&name);        
    }

    free(html.str);
    free(name.str);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
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

    if (pat == NULL) {
        pat = "";
    }

    stringarr repos = parse_repos(user, pat, excluded);

    #if LOG
    printf("Repos:\n");
    printstrarr(&repos);
    printf("\n");
    #endif

    LOCs locs;
    memset(&locs, 0, sizeof(locs));

    for(size_t i = 0; i < repos.elements; i++) {
        printf("Searching: %s\n", repos.str[i].str);
        parse_dir(&locs, repos.str[i], pat);
    }

    printf("\nLines of Code:"
    "\nC: %lu"
    "\nCpp: %lu"
    "\nC/Cpp Headers: %lu"
    "\nC#: %lu"
    "\nJavascript: %lu"
    "\nPython: %lu"
    "\nAssembly: %lu"
    "\nJava: %lu"
    "\nGo: %lu"
    "\nHlsl: %lu"
    "\nCuda: %lu"
    "\nCuda Headers: %lu"
    "\nShell: %lu"
    "\nGdb Script: %lu"
    "\nHtml: %lu"
    "\nMarkdown: %lu"
    "\nFailed to read: %lu files\n",
    locs.c, locs.cpp, locs.h + locs.hpp, locs.csharp, locs.js,
    locs.py, locs.as, locs.java, locs.go, locs.hlsl, locs.cuda,
    locs.cudah, locs.shell, locs.gdb, locs.html, locs.md, locs.failed
    );
}

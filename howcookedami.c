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
    size_t failed;
} LOCs;


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

string parse_repos(char* user, char* pat) {
    const char* CURLCMD = "curl -s -H \"Authorization: token ";
    const char* SUBSTRING = "\"url\": \"";
    const char* URL = "https://api.github.com/user/repos";
    const char* HOMEURL = "https://api.github.com/users/";

    char recvbuf[512];
    FILE* pipe;

    string finalcmd = { 0, (char*)malloc(1) };
    append(&finalcmd, CURLCMD);
    append(&finalcmd, pat);
    append(&finalcmd, "\" ");
    append(&finalcmd, URL);

    string html = { 0, (char*)malloc(1) };

    pipe = popen(finalcmd.str, "r");

    while(fgets(recvbuf, sizeof(recvbuf), pipe) != NULL) {
        append(&html, recvbuf);
        memset(recvbuf, 0, 512);
    }

    pclose(pipe);
    free(finalcmd.str);

    string home_repo = { 0, (char*)malloc(1) };
    append(&home_repo, HOMEURL);
    append(&home_repo, user);

    string repos = { 0, (char*)malloc(1) };

    char* searchstring = html.str;
    while ((searchstring = strstr(searchstring, SUBSTRING)) != NULL) {
        searchstring += strlen(SUBSTRING);

        size_t idx = searchstring - html.str;
        size_t eidx = strstr(searchstring, "\"") - html.str;

        if ((strstr(searchstring, "licenses") - html.str > eidx || 
            strstr(searchstring, "licenses") == NULL) && 
            eidx - idx > strlen(home_repo.str)) {

                appendn(&repos, searchstring, eidx - idx);
                appendch(&repos, '\n');
            }
    }

    #if LOG
    printf("Repos:\n%s\n", repos.str);
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

    __uint8_t mod = 0;

    pipe = popen(finalcmd.str, "r");
    while(fgets(recvbuf, sizeof(recvbuf), pipe) != NULL) {

        char* t = strstr(recvbuf, " loc)");

        if (t != NULL) {
            size_t sidx = t - recvbuf;

            while(recvbuf[sidx] != '(') { sidx--; }
            size_t eidx = strstr(&recvbuf[sidx], " ") - recvbuf;

            string lines = { 0, (char*)malloc(1) };
            appendn(&lines, &recvbuf[sidx + 1], eidx - sidx);

            size_t tmp = atoi(lines.str);

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
            else { printf("ERROR PARSING EXT\n"); }
            mod = 1;

            break;
        }
    }

    if (!mod)  {
        locs->failed++;
    }

    free(finalcmd.str);
    pclose(pipe);
}
void parse_dir(LOCs* locs, string url, const char* pat) {
    const char* CURLCMD = "curl -s -L -H \"Authorization: token ";
    const char* SUBSTRING = "\"name\": \"";

    char recvbuf[512];
    FILE* pipe;

    string finalcmd = { 0, (char*)malloc(1) };
    append(&finalcmd, CURLCMD);
    append(&finalcmd, pat);
    append(&finalcmd, "\" \"");
    append(&finalcmd, url.str);
    append(&finalcmd, "\"");

    string html = { 0, (char*)malloc(1) };

    pipe = popen(finalcmd.str, "r");

    while(fgets(recvbuf, sizeof(recvbuf), pipe) != NULL) {
        append(&html, recvbuf);
        memset(recvbuf, 0, 512);
    }

    free(finalcmd.str);
    pclose(pipe);


    string name = { 0, (char*)malloc(1) };

    char* searchstring = html.str;
    while ((searchstring = strstr(searchstring, SUBSTRING)) != NULL) {
        searchstring += strlen(SUBSTRING);

        size_t idx = searchstring - html.str;
        size_t eidx = strstr(searchstring, "\"") - html.str;

        appendn(&name, searchstring, eidx - idx);

        size_t urlidx = strstr(searchstring, "\"url\": \"") - searchstring + strlen("\"url\": \"");
        size_t eurlidx = strstr(&searchstring[urlidx], "\"") - searchstring;

        string tmp_url = { 0, (char*)malloc(1) };
        appendn(&tmp_url, &searchstring[urlidx], eurlidx - urlidx);

        size_t typeidx = strstr(searchstring, "\"type\": \"") - searchstring + strlen("\"type\": \"");

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
                strcmp(fileext, "html") == 0
            )) {
                #if LOG
                printf("File: %s\n", name.str);
                #endif

                string html_url = { 0, (char*)malloc(1) };

                size_t shtmlidx = strstr(&searchstring[typeidx], "\"html\": \"") - searchstring + strlen("\"html\": \"");
                size_t ehtmlidx = strstr(&searchstring[shtmlidx], "\"") - searchstring;

                appendn(&html_url, &searchstring[shtmlidx], ehtmlidx - shtmlidx);

                parse_file(locs, html_url, pat, fileext);

                free(html_url.str);
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

    char* user = argv[1];
    char* pat = argv[2];

    string repos = parse_repos(user, pat);

    LOCs locs;
    memset(&locs, 0, sizeof(locs));

    string tmp = { 0, (char*)malloc(1) };
    for(size_t i = 0; i < repos.len; i++) {
        if (repos.str[i] != '\n') {
            appendch(&tmp, repos.str[i]);
        } else {
            append(&tmp, "/contents");
            printf("Searching: %s\n", tmp.str);
            parse_dir(&locs, tmp, pat);

            clearstr(&tmp);
        }
    }

    free(repos.str);

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
    "\nFailed to read: %lu files\n",
    locs.c, locs.cpp, locs.h + locs.hpp, locs.csharp, locs.js,
    locs.py, locs.as, locs.java, locs.go, locs.hlsl, locs.cuda,
    locs.cudah, locs.shell, locs.gdb, locs.html, locs.failed
    );
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char* URL = "https://github.com/";

typedef struct {
    size_t len;
    char* str;
} string;

typedef struct {
    size_t c;
    size_t cpp;
    size_t csharp;
    size_t js;
    size_t py;
    size_t as;
    size_t java;
    size_t go;
    size_t hlsl;
    size_t cuda;
    size_t shell;
    size_t gdb;
    size_t html;
} LOCs;


void append(string* str, const char* data) {
    str->str = realloc(str->str, str->len + strlen(data) + 1);
    memcpy(&str->str[str->len], data, strlen(data));
    str->len += strlen(data);
    str->str[str->len] = '\x00';
}
void appendch(string* str, char data) {
    str->str = realloc(str->str, str->len + 1);
    str->str[str->len] = data;
    str->len++;
    str->str[str->len] = '\x00';
}

string final_url(char* user) {
    const char* URLSUFFIX = "?tab=repositories";

    size_t len = strlen(user);

    string final = { 0, (char*)malloc(1) };
    append(&final, URL);
    append(&final, user);
    append(&final, URLSUFFIX);

    return final;
}

string parse_repos(string url, char* user) {
    const char* CURLCMD = "curl -s ";

    char recvbuf[512];
    FILE* pipe;

    string finalcmd = { 0, (char*)malloc(1) };
    append(&finalcmd, CURLCMD);
    append(&finalcmd, url.str);

    char* html = (char*)malloc(1);

    pipe = popen(finalcmd.str, "r");

    while(fgets(recvbuf, sizeof(recvbuf), pipe) != NULL) {
        size_t t = strlen(html);

        html = (char*)realloc(html, strlen(html) + strlen(recvbuf));
        memcpy(&html[t], recvbuf, strlen(recvbuf));

        memset(recvbuf, 0, 512);
    }


    const char* ref = "<a href=\"/";

    string substring = { 0, (char*)malloc(1) };
    append(&substring, ref);
    append(&substring, user);

    string url_template = { 0, (char*)malloc(1) };
    append(&url_template, URL);
    append(&url_template, user);

    char* sstring = html;
    string repos = { 0, (char*)malloc(1) };

    while ((sstring = strstr(sstring, substring.str)) != NULL) {
        size_t idx = sstring - html + strlen(substring.str);

        append(&repos, url_template.str);

        while(html[idx] != '"') {
            appendch(&repos, html[idx]);
            idx++;
        }
        appendch(&repos, '\n');

        sstring += strlen(substring.str);
    }

    pclose(pipe);
    free(finalcmd.str);
    free(substring.str);

    return repos;
}

void parse_dir(LOCs* locs, string url) {
    
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        return 1;
    }

    char* user = argv[1];

    string final = final_url(user);

    string repos = parse_repos(final, user);
    printf("Repos:\n%s\n", repos.str);

    LOCs locs;
    memset(&locs, 0, sizeof(locs));
}

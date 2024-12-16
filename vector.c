#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

typedef struct {
    float x;
    float y;
} vector;

float dot_product(vector a, vector b);
float min_angle(vector a, vector b);
vector normalized (vector a);
float magnitude(vector a);

void display_usage();

vector parse_vector(char* input);

int main(int argc, char* argv[]) {

    if (argc < 2) {
        display_usage();
        return 0;
    }

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0) {
            display_usage();
            return 0;
        }
    }

    for (int i = 1; i < argc; i++) {
        // magnitude
        if (strcmp(argv[i], "-m") == 0) {
            if (argc > i + 1) {
                vector a = parse_vector(argv[i + 1]);

                if (a.x == NAN || a.y == NAN) {
                    display_usage();
                    return 1;
                }

                float mag = magnitude(a);

                printf("The magnitude of <%f, %f> is %f\n", a.x, a.y, mag);
            } else {
                // error
                display_usage();
                return 1;
            }
        }

        // normalized
        if (strcmp(argv[i], "-n") == 0) {
            if (argc > i + 1) {
                vector a = parse_vector(argv[i + 1]);

                if (a.x == NAN || a.y == NAN) {
                    display_usage();
                    return 1;
                }

                vector norm = normalized(a);

                printf("The normal vector for <%f, %f> is <%f, %f>\n", a.x, a.y, norm.x, norm.y);
            } else {
                // error
                display_usage();
                return 1;
            }
        }

        // angle between
        if (strcmp(argv[i], "-a") == 0) {
            if (argc > i + 2) {
                vector a = parse_vector(argv[i + 1]);
                vector b = parse_vector(argv[i + 2]);

                if (a.x == NAN || a.y == NAN || b.x == NAN || b.y == NAN) {
                    display_usage();
                    return 1;
                }

                float angle = min_angle(a, b);

                printf("The min angle betweem <%f, %f> and <%f, %f> is %f\n",
                a.x, a.y, b.x, b.y, angle);
            } else {
                // error
                display_usage();
                return 1;
            }
        }

        // dot prod
        if (strcmp(argv[i], "-d") == 0) {
            if (argc > i + 2) {
                vector a = parse_vector(argv[i + 1]);
                vector b = parse_vector(argv[i + 2]);

                if (a.x == NAN || a.y == NAN || b.x == NAN || b.y == NAN) {
                    display_usage();
                    return 1;
                }

                float c = dot_product(a, b);

                printf("The dot prod between <%f, %f> and <%f, %f> is %f\n",
                a.x, a.y, b.x, b.y, c);
            }
        }
    }

    return 0;
}

void display_usage() {
    printf("Vector (1.0)\n"
    "Usage: vector [-m v1] [-n v1]\n"
    "              [-a v1 v2] [-d v1 v2]\n"
    "\t\tCommand Summary:\n"
    "\t\t\t-m\t\treturns the magnitude of v1\n"
    "\t\t\t-n\t\treturns the normal vector of v1\n"
    "\t\t\t-a\t\treturns the angle between v1 and v2\n"
    "\t\t\t-d\t\treturns the dot prod between v1 and v2\n"
    );
}

vector parse_vector(char* input) {
    char* token = strtok(input, ",");
    
    if (token == NULL) {
        // error
        vector err = { NAN, NAN };
        return err;
    }

    vector a;
    a.x = strtof(token, NULL);
    token = strtok(NULL, ",");
    a.y = strtof(token, NULL);

    return a;
}


float min_angle(vector a, vector b) {
    return acos(dot_product(normalized(a), normalized(b)));
}
float magnitude(vector a) {
    return sqrt(a.x * a.x + a.y * a.y);
}
float dot_product(vector a, vector b) {
    return a.x * b.x + a.y * b.y;
}
vector normalized(vector a) {
    float div = magnitude(a);

    vector norm = { a.x / div, a.y / div };
    return norm;
}
#include "utils.h"

int min_int(int a, int b) {
    if (a > b) {
        return b;
    } else {
        return a;
    }
}

int max_int(int a, int b) {
    if (a < b) {
        return b;
    } else {
        return a;
    }
}
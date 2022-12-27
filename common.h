#ifndef __COMMON_H
#define __COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

int is_integer(char *arg){
    for (int i = 0; i < strlen(arg); i++)
    {
        if (isdigit(arg[i]) == 0)
            return 0;
    }
    return 1;
}

int _checked(int ret, char* calling_function) {
    if (ret < 0) {
        perror(calling_function);
        exit(EXIT_FAILURE);
    }
    return ret;
}

// The macro allows us to retrieve the name of the calling function
#define checked(call) _checked(call, #call)
#endif  // __COMMON_H
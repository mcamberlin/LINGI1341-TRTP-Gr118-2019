#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <stdint.h>
#include <stdint.h>
#include <zlib.h>
#include <errno.h>

#include "utils.h"
#include "packet_interface.h"

void error(int err, char *msg) {
    fprintf(stderr,"%s\nReturned: %d\nErrno message: %s\n", msg, err, strerror(errno));
    exit(EXIT_FAILURE);
}

int create_file(char* format, int* iout, FILE** fp){
    char filename[strlen(format)];
    int rslt = snprintf(filename, strlen(format)+1, format, *iout);
    if(rslt <= 0){
        error(rslt, "Cannot decode output file name format");
        return -1;
    }

    *fp = fopen(filename, "w+");
    if (fp == NULL) {
        error(-1,"Cannot open new file");
        return -1;
    }
    *iout = *iout + 1;
    return 1;
}

void endianess(){
    union {
        struct {
            uint8_t window:5;   //0b 000x xxxx
            uint8_t type:2;     //0b 0xx0 0000 
            uint8_t tr:1;       //0b x000 0000 
            char c;
        } p1;
        uint8_t i;
    }p;

    p.i = 0b11011111;
    printf("p1.window: %d, expected: 31 (0b11111)\n", p.p1.window);
    printf("p1.type: %d, expected: 2 (0b10)\n", p.p1.type);
    printf("p1.tr: %d, expected: 1 (0b1)\n", p.p1.tr);    
}

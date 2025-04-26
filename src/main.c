#include <stdio.h>
#include <stdlib.h>
#include "chead.h"



int main(int argc, char* argv[]){
    if(argc < 2){
        printf("The program expects a path to a gb rom.\n");
        return 1;
    }

    char* rom_path = argv[1];
    FILE* rom_ptr = fopen(rom_path, "rb");
    if(rom_ptr == NULL){
        printf("The specified rom does not exist.\n");
        return 1;
    }

    struct Header header;
    readCart(rom_ptr, &header);
    testCart(rom_ptr, &header);




    fclose(rom_ptr);
    free(header.data);
    return 0;
}




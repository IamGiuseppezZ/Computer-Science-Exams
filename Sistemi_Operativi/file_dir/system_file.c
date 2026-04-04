#include "mac_sem.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <pthread.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>

int main(){
    DIR* dir;
    dir = opendir("dir2");
    struct dirent* entry;
    while((entry = readdir(dir)) != NULL){
        if (entry->d_type == DT_REG){
            aggiungi_file();
        }
        else {
            aggiungi_directory();
        }
    }

}
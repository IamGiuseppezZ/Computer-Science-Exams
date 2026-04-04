#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main(){
    size_t page_size = (size_t) sysconf(_SC_PAGE_SIZE);
    printf("%zu\n", page_size);
}
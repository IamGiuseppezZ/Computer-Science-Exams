#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#define filename "2151051398.jpg"
#define new_filename "2151051398_copy.jpg"
#define BLOCK_SIZE 1024;

int main(){

    struct stat buf;
    int fd = open(filename, O_RDONLY);
    if (fd == -1){
        perror("error opening file.");
        exit(EXIT_FAILURE);
    }
    fstat(fd, &buf);
    size_t file_size = buf.st_size;
    char* mapped = mmap(NULL, file_size, PROT_READ, MAP_SHARED, fd, 0);
    if (mapped == MAP_FAILED){
        perror("Error map file.\n");
        exit(EXIT_FAILURE);
    }

    int newfd = open(new_filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (newfd == -1){
        perror("Error creating file.\n");
        exit(1);
    }
    char* copy = mapped;
    char buffer[1024];
    size_t offset = 0;
    while(offset < file_size){
        size_t to_copy = 1024;
        if (offset + to_copy > file_size){
            to_copy = file_size - offset;
        }
        memcpy(buffer, mapped + offset, to_copy);
        size_t written = write(newfd, buffer, to_copy);
        if (written != to_copy){
            perror("error copy\n");
            munmap(mapped, file_size);
            close(newfd);
            close(fd);
            exit(EXIT_FAILURE);
        }
        offset += to_copy;
    }
    if (ftruncate(newfd, file_size) != 0){
        perror("error truncate\n");
        munmap(mapped, file_size);
        close(newfd);
        close(fd);
        exit(EXIT_FAILURE);
        exit(EXIT_FAILURE);
    }



    munmap(mapped, file_size);
    close(fd);
    close(newfd);
    puts("File duplicated.");

}
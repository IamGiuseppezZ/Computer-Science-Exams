#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(){
    srand (time (NULL)); // define a seed for the random number generator
    const char ALLOWED[] = "abcdefghijklmnopqrstuvwxyz1234567890";
    char random[10+1];
    int i = 0;
    int c = 0;
    int nbAllowed = sizeof(ALLOWED)-1;
    for(i=0;i<10;i++) {
        c = rand() % nbAllowed ;
        random[i] = ALLOWED[c];
    }
    random[10] = '\0';
    printf("%s\n", random);
}
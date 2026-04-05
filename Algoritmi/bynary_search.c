#include <stdio.h>

int binarySearch(int* array, int left, int right, int toFind) {
    if (left > right) {
        return -1; 
    }
    
    // per evitare overflow
    int mid = left + (right - left) / 2;
    
    if (array[mid] == toFind) {
        return mid;
    } 
    else if (array[mid] > toFind) {
        return binarySearch(array, left, mid - 1, toFind);
    }
    else {
        return binarySearch(array, mid + 1, right, toFind);
    }
}

int main(void) {
    int array[] = {1, 2, 3, 4, 5, 6, 7, 8}; 
    int len = sizeof(array) / sizeof(array[0]);
    
    int target = 7;
    int index = binarySearch(array, 0, len - 1, target);
    
    if (index != -1) {
        printf("Elemento %d trovato all'indice: [%d]\n", target, index);
    } else {
        printf("Elemento %d NON trovato.\n", target);
    }
    
    return 0;
}
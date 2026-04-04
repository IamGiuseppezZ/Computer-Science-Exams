#include <stdio.h>

// Restituisce l'indice (0-based) se trovato, altrimenti -1
int binarySearch(int* array, int left, int right, int toFind) {
    // Caso base: se left supera right, l'elemento non è nell'array
    if (left > right) {
        return -1; 
    }
    
    int mid = left + (right - left) / 2; // Evita overflow
    
    if (array[mid] == toFind) {
        return mid; // Trovato!
    } 
    else if (array[mid] > toFind) {
        // Il target è più piccolo: cerco nella metà di sinistra
        return binarySearch(array, left, mid - 1, toFind);
    }
    else {
        // Il target è più grande: cerco nella metà di destra
        return binarySearch(array, mid + 1, right, toFind);
    }
}

int main(void) {
    int array[] = {1, 2, 3, 4, 5, 6, 7, 8}; // Deve essere già ordinato!
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
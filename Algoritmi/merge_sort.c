#include <stdio.h>
#include <stdlib.h>

void merge(int* array, int p, int q, int r) {
    int nL = q - p + 1;
    int nR = r - q;
    
    int* leftArray = (int*)malloc(nL * sizeof(int));
    int* rightArray = (int*)malloc(nR * sizeof(int));
    
    // Controllo sicurezza memoria
    if (!leftArray || !rightArray) {
        fprintf(stderr, "Errore allocazione memoria.\n");
        exit(EXIT_FAILURE);
    }
    
    for (int i = 0; i < nL; ++i) {
        leftArray[i] = array[p + i];
    }
    for (int j = 0; j < nR; ++j) {
        rightArray[j] = array[q + 1 + j];
    }
    
    int i = 0, j = 0, k = p;
    
    // Ricostruisco l'array ordinato prendendo il minore tra i due sotto-array
    while (i < nL && j < nR) {
        if (leftArray[i] <= rightArray[j]) {
            array[k] = leftArray[i];
            i++;
        } else {
            array[k] = rightArray[j];
            j++;
        }
        k++;
    }
    
    // Copio eventuali elementi rimanenti
    while (i < nL) {
        array[k] = leftArray[i];
        i++;
        k++;
    }
    
    while (j < nR) {
        array[k] = rightArray[j];
        j++;
        k++;
    }
    
    free(leftArray);
    free(rightArray);
}

void mergeSort(int* array, int p, int r) {
    if (p < r) {
        // Calcolo il punto medio evitando potenziali overflow rispetto a (p+r)/2
        int q = p + (r - p) / 2;
        
        mergeSort(array, p, q);
        mergeSort(array, q + 1, r);
        merge(array, p, q, r);
    }
}

int main(void) {
    int array[] = {1, 9, 2, 8, 3, 7, 4, 6, 5};
    int len = sizeof(array) / sizeof(array[0]);
    
    mergeSort(array, 0, len - 1);
    
    printf("Array ordinato: ");
    for (int i = 0; i < len; i++) {
        printf("%d ", array[i]);
    }
    printf("\n");
    
    return 0;
}

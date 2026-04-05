#include <stdio.h>

// Helper function to swap two elements
void swap(int* a, int* b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

// Maintains the max-heap property (CLRS 0-based indexing)
void maxHeapify(int arr[], int n, int i) {
    int largest = i;
    int left = 2 * i + 1;
    int right = 2 * i + 2;

    if (left < n && arr[left] > arr[largest])
        largest = left;

    if (right < n && arr[right] > arr[largest])
        largest = right;

    if (largest != i) {
        swap(&arr[i], &arr[largest]);
        maxHeapify(arr, n, largest); // Recursively heapify the affected sub-tree
    }
}

// Main function to do heap sort
void heapSort(int arr[], int n) {
    // Build max heap from bottom-up
    for (int i = n / 2 - 1; i >= 0; i--) {
        maxHeapify(arr, n, i);
    }

    // Extract elements one by one from heap
    for (int i = n - 1; i > 0; i--) {
        swap(&arr[0], &arr[i]); // Move current root to end
        maxHeapify(arr, i, 0);  // Call max heapify on the reduced heap
    }
}

int main() {
    int arr[] = {12, 11, 13, 5, 6, 7};
    int n = sizeof(arr) / sizeof(arr[0]);

    heapSort(arr, n);

    printf("Sorted array (HeapSort): ");
    for (int i = 0; i < n; ++i)
        printf("%d ", arr[i]);
    printf("\n");
    return 0;
}
#include <stdio.h>
#include <stdlib.h>

#define TABLE_SIZE 7

// Node for chaining collisions
typedef struct Node {
    int key;
    struct Node* next;
} Node;

Node* hashTable[TABLE_SIZE];

// Simple modulo hash function
int hashFunction(int key) {
    return key % TABLE_SIZE;
}

void insert(int key) {
    int hashIndex = hashFunction(key);
    
    Node* newNode = (Node*)malloc(sizeof(Node));
    newNode->key = key;
    
    // Insert at the beginning of the chain (O(1) insertion)
    newNode->next = hashTable[hashIndex];
    hashTable[hashIndex] = newNode;
}

int search(int key) {
    int hashIndex = hashFunction(key);
    Node* temp = hashTable[hashIndex];
    
    while (temp != NULL) {
        if (temp->key == key) return 1; // Found
        temp = temp->next;
    }
    return 0; // Not found
}

int main() {
    // Initialize table with NULL
    for(int i = 0; i < TABLE_SIZE; i++) hashTable[i] = NULL;

    insert(15);
    insert(11);
    insert(27);
    insert(8);  // 15 and 8 will collide (both % 7 = 1), chaining handles it!

    printf("Search 11: %s\n", search(11) ? "Found" : "Not Found");
    printf("Search 100: %s\n", search(100) ? "Found" : "Not Found");

    return 0;
}
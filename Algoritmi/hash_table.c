#include <stdio.h>
#include <stdlib.h>

#define TABLE_SIZE 7

// liste per le collisioni
typedef struct Node {
    int key;
    struct Node* next;
} Node;

Node* hashTable[TABLE_SIZE];

int hashFunction(int key) {
    return key % TABLE_SIZE;
}

void insert(int key) {
    int hashIndex = hashFunction(key);
    
    Node* newNode = (Node*)malloc(sizeof(Node));
    newNode->key = key;
    
    // push in testa
    newNode->next = hashTable[hashIndex];
    hashTable[hashIndex] = newNode;
}

int search(int key) {
    int hashIndex = hashFunction(key);
    Node* temp = hashTable[hashIndex];
    
    while (temp != NULL) {
        if (temp->key == key) return 1;
        temp = temp->next;
    }
    return 0;
}

int main() {
    for(int i = 0; i < TABLE_SIZE; i++) hashTable[i] = NULL;

    insert(15);
    insert(11);
    insert(27);
    insert(8); 

    printf("Search 11: %s\n", search(11) ? "Found" : "Not Found");
    printf("Search 100: %s\n", search(100) ? "Found" : "Not Found");

    return 0;
}
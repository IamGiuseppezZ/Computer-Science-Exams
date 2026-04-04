#include <stdio.h>
#include <stdlib.h>

#define MAX_VERTICES 100

// Using an adjacency matrix for simplicity in C
int adjMatrix[MAX_VERTICES][MAX_VERTICES];
int visited[MAX_VERTICES];

// Simple Queue array implementation
int queue[MAX_VERTICES];
int front = -1, rear = -1;

void enqueue(int vertex) {
    if (rear == MAX_VERTICES - 1) return;
    if (front == -1) front = 0;
    queue[++rear] = vertex;
}

int dequeue() {
    if (front == -1 || front > rear) return -1;
    return queue[front++];
}

int isQueueEmpty() {
    return (front == -1 || front > rear);
}

void BFS(int startVertex, int numVertices) {
    printf("BFS Traversal: ");
    
    visited[startVertex] = 1;
    enqueue(startVertex);

    while (!isQueueEmpty()) {
        int currentVertex = dequeue();
        printf("%d ", currentVertex);

        // Explore all adjacent vertices
        for (int i = 0; i < numVertices; i++) {
            if (adjMatrix[currentVertex][i] == 1 && !visited[i]) {
                visited[i] = 1; // Mark as visited as soon as we enqueue it
                enqueue(i);
            }
        }
    }
    printf("\n");
}

int main() {
    int vertices = 5;
    // Edge setup (Undirected graph)
    adjMatrix[0][1] = 1; adjMatrix[1][0] = 1;
    adjMatrix[0][2] = 1; adjMatrix[2][0] = 1;
    adjMatrix[1][3] = 1; adjMatrix[3][1] = 1;
    adjMatrix[1][4] = 1; adjMatrix[4][1] = 1;

    for (int i = 0; i < vertices; i++) visited[i] = 0;

    BFS(0, vertices);
    return 0;
}
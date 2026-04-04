#include <stdio.h>
#include <stdlib.h>

#define MAX_VERTICES 100

int adjMatrix[MAX_VERTICES][MAX_VERTICES];
int visited[MAX_VERTICES];

// Recursive DFS approach (implicit stack)
void DFS(int vertex, int numVertices) {
    printf("%d ", vertex);
    visited[vertex] = 1;

    for (int i = 0; i < numVertices; i++) {
        // If there's an edge and the neighbor is unvisited, go deep
        if (adjMatrix[vertex][i] == 1 && !visited[i]) {
            DFS(i, numVertices);
        }
    }
}

int main() {
    int vertices = 5;
    
    // Edge setup
    adjMatrix[0][1] = 1; adjMatrix[1][0] = 1;
    adjMatrix[0][2] = 1; adjMatrix[2][0] = 1;
    adjMatrix[1][3] = 1; adjMatrix[3][1] = 1;
    adjMatrix[1][4] = 1; adjMatrix[4][1] = 1;

    for (int i = 0; i < vertices; i++) visited[i] = 0;

    printf("DFS Traversal: ");
    DFS(0, vertices); // Start from vertex 0
    printf("\n");

    return 0;
}
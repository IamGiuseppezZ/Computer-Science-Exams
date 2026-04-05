#include <stdio.h>
#include <limits.h>

#define V 5

// trova il nodo a distanza minima
int minDistance(int dist[], int sptSet[]) {
    int min = INT_MAX, min_index;

    for (int v = 0; v < V; v++) {
        if (sptSet[v] == 0 && dist[v] <= min) {
            min = dist[v];
            min_index = v;
        }
    }
    return min_index;
}

void dijkstra(int graph[V][V], int src) {
    int dist[V];     
    int sptSet[V];   

    // init
    for (int i = 0; i < V; i++) {
        dist[i] = INT_MAX;
        sptSet[i] = 0;
    }

    dist[src] = 0; 

    for (int count = 0; count < V - 1; count++) {
        int u = minDistance(dist, sptSet); 
        sptSet[u] = 1; 

        // relax degli archi
        for (int v = 0; v < V; v++) {
            if (!sptSet[v] && graph[u][v] && dist[u] != INT_MAX 
                && dist[u] + graph[u][v] < dist[v]) {
                dist[v] = dist[u] + graph[u][v]; 
            }
        }
    }

    printf("Vertex \t Distance from Source\n");
    for (int i = 0; i < V; i++) {
        printf("%d \t %d\n", i, dist[i]);
    }
}

int main() {
    int graph[V][V] = { { 0, 10, 0, 5, 0 },
                        { 0, 0, 1, 2, 0 },
                        { 0, 0, 0, 0, 4 },
                        { 0, 3, 9, 0, 2 },
                        { 7, 0, 6, 0, 0 } };

    dijkstra(graph, 0);
    return 0;
}
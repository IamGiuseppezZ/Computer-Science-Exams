#include <stdio.h>
#include <stdlib.h>

typedef struct Node {
    int data;
    struct Node* sx;
    struct Node* dx;
} Node;

// alloca nodo
Node* createNode(int data) {
    Node* temp = (Node*)malloc(sizeof(Node));
    
    if (!temp) {
        printf("Errore: memoria esaurita!\n");
        exit(EXIT_FAILURE);
    }
    
    temp->sx = NULL;
    temp->dx = NULL;
    temp->data = data;
    
    return temp;
}

// inserimento
Node* insertNode(Node* root, int data) {
    if (root == NULL) {
        return createNode(data);
    }
    
    if (data <= root->data) {
        root->sx = insertNode(root->sx, data);
    } else {
        root->dx = insertNode(root->dx, data);
    }
    
    return root;
}

void visitInOrder(Node* root) {
    if (root != NULL) {
        visitInOrder(root->sx);
        printf("%d\t", root->data);
        visitInOrder(root->dx);
    }
}

int max(int a, int b) {
    return (a > b) ? a : b;
}

int calcMaxHeight(Node* root) {
    if (root == NULL) return 0;
    
    int sx = calcMaxHeight(root->sx);
    int dx = calcMaxHeight(root->dx);
    
    return 1 + max(sx, dx);
}

int searchOnBst(Node* root, int toFind) {
    if (root == NULL) return 0;
    if (root->data == toFind) return 1;
    
    if (toFind < root->data) {
        return searchOnBst(root->sx, toFind);
    } else {
        return searchOnBst(root->dx, toFind);
    }
}

// trova il minimo scendendo a sx
Node* findMin(Node* root) {
    if (root == NULL) return NULL;
    if (root->sx == NULL) return root;
    
    return findMin(root->sx);
}

// rimozione nodo
Node* deleteNode(Node* root, int toDelete) {
    if (root == NULL) return root;
    
    if (toDelete < root->data) {
        root->sx = deleteNode(root->sx, toDelete);
    } else if (toDelete > root->data) {
        root->dx = deleteNode(root->dx, toDelete);
    } 
    else {
        // caso 1 o 2: un figlio o foglia
        if (root->sx == NULL) {
            Node* temp = root->dx;
            free(root);
            return temp;
        } 
        else if (root->dx == NULL) {
            Node* temp = root->sx;
            free(root);
            return temp;
        }
        
        // caso 3: due figli
        Node* temp = findMin(root->dx);
        root->data = temp->data;
        root->dx = deleteNode(root->dx, temp->data);
    }
    return root;
}

// clean up in post order
void freeTree(Node* root) {
    if (root != NULL) {
        freeTree(root->sx);
        freeTree(root->dx);
        free(root);
    }
}

int main() {
    Node* root = NULL;
    
    printf("--- CREAZIONE E INSERIMENTO ---\n");
    root = insertNode(root, 10);
    root = insertNode(root, 9);
    root = insertNode(root, 5);
    root = insertNode(root, 20);
    root = insertNode(root, 30);
    root = insertNode(root, 24);
    root = insertNode(root, 40);
    root = insertNode(root, 60);
    root = insertNode(root, 34);
    root = insertNode(root, 91);
    
    printf("Visita In-Order:\n");
    visitInOrder(root);
    printf("\n\n");

    printf("--- STATISTICHE DELL'ALBERO ---\n");
    printf("Altezza Massima: [%d]\n", calcMaxHeight(root));
    
    int target = 40;
    int found = searchOnBst(root, target);
    printf("Risultato ricerca per %d: [%s]\n", target, found ? "Trovato" : "Non Trovato");

    Node* minNode = findMin(root);
    if (minNode != NULL) {
        printf("Valore Minimo nell'albero: [%d]\n", minNode->data);
    }
    printf("\n");
    
    printf("--- CANCELLAZIONE NODI ---\n");
    printf("Elimino il 5...\n");
    root = deleteNode(root, 5);
    
    printf("Elimino il 60...\n");
    root = deleteNode(root, 60);
    
    printf("Elimino il 30...\n");
    root = deleteNode(root, 30);
    
    printf("\nVisita In-Order post-cancellazione:\n");
    visitInOrder(root);
    printf("\n");

    freeTree(root);
    
    return 0;
}
#include <stdio.h>
#include <stdlib.h>

// Classica struttura del nodo per un albero binario. 
// sx = puntatore al figlio sinistro, dx = puntatore al figlio destro.
typedef struct Node {
    int data;
    struct Node* sx;
    struct Node* dx;
} Node;

// Funzione di utilità per creare "fisicamente" il nodo in memoria
Node* createNode(int data) {
    Node* temp = (Node*)malloc(sizeof(Node));
    
    // Controllo sempre se la malloc fallisce, non si sa mai
    if (temp == NULL) {
        printf("Errore: memoria esaurita!\n");
        exit(EXIT_FAILURE);
    }
    
    // Inizializzo i puntatori a NULL (essenziale per non avere segmentation fault) e copio il dato
    temp->sx = NULL;
    temp->dx = NULL;
    temp->data = data;
    
    return temp;
}

// Inserimento ricorsivo in un BST. 
// Nota: l'ho chiamata insertNode al posto di createBST perché concettualmente stiamo inserendo un dato.
Node* insertNode(Node* root, int data) {
    // Caso base: sono arrivato in fondo all'albero (o l'albero è vuoto), qui attacco il nuovo nodo!
    if (root == NULL) {
        return createNode(data);
    }
    
    // Regola d'oro del BST: minori o uguali a sinistra, maggiori a destra
    if (data <= root->data) {
        root->sx = insertNode(root->sx, data);
    } else {
        root->dx = insertNode(root->dx, data);
    }
    
    // Ritorno la radice (che non è cambiata, a meno che non fosse il primo inserimento in assoluto)
    return root;
}

// Visita Simmetrica (In-Order: Sinistro, Radice, Destro)
// La cosa bella dell'In-Order nel BST è che stampa i numeri in ordine crescente!
void visitInOrder(Node* root) {
    if (root != NULL) {
        visitInOrder(root->sx);
        printf("%d\t", root->data);
        visitInOrder(root->dx);
    }
}

// Funzioncina di appoggio per calcolare il massimo tra due interi
int max(int a, int b) {
    return (a > b) ? a : b;
}

// Calcolo dell'altezza massima dell'albero (profondità)
int calcMaxHeight(Node* root) {
    // Un albero vuoto ha altezza 0
    if (root == NULL) {
        return 0;
    }
    
    // Calcolo ricorsivamente l'altezza del ramo sinistro e di quello destro
    int sx = calcMaxHeight(root->sx);
    int dx = calcMaxHeight(root->dx);
    
    // L'altezza totale è 1 (il nodo corrente) + il massimo tra i due rami
    return 1 + max(sx, dx);
}

// Ricerca binaria classica: sfrutto la proprietà del BST per scartare metà albero a ogni passo
int searchOnBst(Node* root, int toFind) {
    if (root == NULL) return 0;       // Non trovato
    if (root->data == toFind) return 1; // Trovato!
    
    // Se il numero che cerco è più piccolo, vado a sinistra, altrimenti a destra
    if (toFind < root->data) {
        return searchOnBst(root->sx, toFind);
    } else {
        return searchOnBst(root->dx, toFind);
    }
}

// Trova il nodo col valore minimo. 
// Nel BST il minimo si trova andando sempre a sinistra finché non trovo NULL.
Node* findMin(Node* root) {
    if (root == NULL) return NULL;
    if (root->sx == NULL) return root; // Non c'è niente di più piccolo, sono arrivato!
    
    return findMin(root->sx);
}

// Cancellazione di un nodo. Questa è la funzione più complessa perché ha 3 casi.
Node* deleteNode(Node* root, int toDelete) {
    // Se l'albero è vuoto o non trovo il nodo, ritorno NULL
    if (root == NULL) return root;
    
    // Fase 1: Cerco il nodo da eliminare scorrendo l'albero
    if (toDelete < root->data) {
        root->sx = deleteNode(root->sx, toDelete);
    } else if (toDelete > root->data) {
        root->dx = deleteNode(root->dx, toDelete);
    } 
    // Fase 2: Trovato! Ora devo gestire i 3 casi di eliminazione
    else {
        // Caso 1 e 2: Il nodo ha un solo figlio (o nessuno)
        // Se non ha il figlio sinistro, "salvo" il destro, libero la memoria del nodo attuale e ritorno il destro.
        if (root->sx == NULL) {
            Node* temp = root->dx;
            free(root);
            return temp;
        } 
        // Viceversa, se non ha il destro, salvo il sinistro.
        else if (root->dx == NULL) {
            Node* temp = root->sx;
            free(root);
            return temp;
        }
        
        // Caso 3: Il nodo ha ENTRAMBI i figli. 
        // Qui faccio la "furbata": cerco il successore in-order (ovvero il minimo del sottoalbero destro).
        Node* temp = findMin(root->dx);
        
        // Copio il valore del successore nel nodo che volevo eliminare (così mantengo intatta la struttura!)
        root->data = temp->data;
        
        // Ora vado a eliminare fisicamente il successore dal ramo destro (che rientrerà nel Caso 1 o 2)
        root->dx = deleteNode(root->dx, temp->data);
    }
    return root;
}

// Libero la memoria di tutto l'albero usando una visita Post-Order (Sinistro, Destro, Radice).
// Devo per forza usare la Post-Order, altrimenti se libero prima la radice perdo i puntatori ai figli!
void freeTree(Node* root) {
    if (root != NULL) {
        freeTree(root->sx);
        freeTree(root->dx);
        free(root); // Libero il nodo corrente solo dopo aver svuotato i suoi rami
    }
}

int main() {
    Node* root = NULL;
    
    printf("--- CREAZIONE E INSERIMENTO ---\n");
    // Popolo l'albero. Nota: riassegno sempre a root perché al primo giro root è NULL
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
    
    printf("Visita In-Order (dovrebbe essere ordinata crescente):\n");
    visitInOrder(root);
    printf("\n\n");

    printf("--- STATISTICHE DELL'ALBERO ---\n");
    printf("Altezza Massima: [%d]\n", calcMaxHeight(root));
    
    // Test della ricerca
    int target = 40;
    int found = searchOnBst(root, target);
    printf("Risultato ricerca per %d: [%s]\n", target, found ? "Trovato" : "Non Trovato");

    // Test ricerca del minimo
    Node* minNode = findMin(root);
    if (minNode != NULL) {
        printf("Valore Minimo nell'albero: [%d]\n", minNode->data);
    }
    printf("\n");
    
    printf("--- CANCELLAZIONE NODI ---\n");
    printf("Elimino il 5 (nodo foglia)...\n");
    root = deleteNode(root, 5);
    
    printf("Elimino il 60 (nodo con un figlio)...\n");
    root = deleteNode(root, 60);
    
    printf("Elimino il 30 (nodo con due figli, fa scattare la ricerca del successore)...\n");
    root = deleteNode(root, 30);
    
    printf("\nVisita In-Order post-cancellazione:\n");
    visitInOrder(root);
    printf("\n");

    // Faccio i compiti a casa e pulisco la memoria prima di uscire
    freeTree(root);
    
    return 0;
}
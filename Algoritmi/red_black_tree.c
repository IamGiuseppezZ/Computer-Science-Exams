#include <stdio.h>
#include <stdlib.h>

// Definizione dei colori per le proprietà del Red-Black Tree
typedef enum { RED, BLACK } Color;

// Struttura del nodo
typedef struct Node {
    int data;
    Color color;
    struct Node* left;
    struct Node* right;
    struct Node* parent;
} Node;

// Struttura dell'albero. 
// Implementa un nodo sentinella (nil) per ottimizzare la gestione delle foglie e della radice,
// evitando molteplici controlli sui puntatori a NULL.
typedef struct Tree {
    Node* root;
    Node* nil;
} Tree;

// Inizializza un albero vuoto e configura la sentinella (sempre nera)
Tree* createTree() {
    Tree* T = (Tree*)malloc(sizeof(Tree));
    T->nil = (Node*)malloc(sizeof(Node));
    T->nil->color = BLACK; 
    T->root = T->nil;      
    return T;
}

// Alloca e inizializza un nuovo nodo. Di default, i nuovi nodi sono rossi.
Node* createNode(Tree* T, int data) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    newNode->data = data;
    newNode->color = RED;  
    newNode->left = T->nil;
    newNode->right = T->nil;
    newNode->parent = T->nil;
    return newNode;
}

// Esegue una rotazione a sinistra attorno al nodo x
void leftRotate(Tree* T, Node* x) {
    Node* y = x->right;         
    x->right = y->left; 
    
    if (y->left != T->nil) {
        y->left->parent = x;    
    }
    
    y->parent = x->parent;
    
    if (x->parent == T->nil) {
        T->root = y;            
    } else if (x == x->parent->left) {
        x->parent->left = y;    
    } else {
        x->parent->right = y;   
    }
    
    y->left = x;                
    x->parent = y;              
}

// Esegue una rotazione a destra attorno al nodo x
void rightRotate(Tree* T, Node* x) {
    Node* y = x->left;
    x->left = y->right;
    
    if (y->right != T->nil) {
        y->right->parent = x;
    }
    
    y->parent = x->parent;
    
    if (x->parent == T->nil) {
        T->root = y;
    } else if (x == x->parent->right) {
        x->parent->right = y;
    } else {
        x->parent->left = y;
    }
    
    y->right = x;
    x->parent = y;
}

// Ripristina le proprietà del Red-Black Tree a seguito dell'inserimento di un nodo rosso.
// Gestisce le violazioni di tipo Red-Red scorrendo l'albero verso l'alto.
void insertFixup(Tree* T, Node* z) {
    while (z->parent->color == RED) {
        // Il padre di z è un figlio sinistro
        if (z->parent == z->parent->parent->left) {
            Node* y = z->parent->parent->right; // Zio di z
            
            // Caso 1: lo zio è rosso. Ricolorazione e shift del focus sul nonno.
            if (y->color == RED) {
                z->parent->color = BLACK;
                y->color = BLACK;
                z->parent->parent->color = RED;
                z = z->parent->parent; 
            } 
            else {
                // Caso 2: lo zio è nero e z è figlio destro. Rotazione a sinistra sul padre.
                if (z == z->parent->right) {
                    z = z->parent;
                    leftRotate(T, z);
                }
                
                // Caso 3: lo zio è nero e z è figlio sinistro. Ricolorazione e rotazione a destra sul nonno.
                z->parent->color = BLACK;
                z->parent->parent->color = RED;
                rightRotate(T, z->parent->parent);
            }
        } 
        // Il padre di z è un figlio destro (casi simmetrici)
        else {
            Node* y = z->parent->parent->left; // Zio di z
            
            // Caso 1 simmetrico
            if (y->color == RED) { 
                z->parent->color = BLACK;
                y->color = BLACK;
                z->parent->parent->color = RED;
                z = z->parent->parent;
            } 
            else {
                // Caso 2 simmetrico
                if (z == z->parent->left) { 
                    z = z->parent;
                    rightRotate(T, z);
                }
                
                // Caso 3 simmetrico
                z->parent->color = BLACK;
                z->parent->parent->color = RED;
                leftRotate(T, z->parent->parent);
            }
        }
    }
    // Garantisce che la radice rimanga sempre nera
    T->root->color = BLACK;
}

// Inserisce un nuovo nodo nell'albero mantenendo l'ordinamento BST, 
// successivamente richiama la procedura di fixup per bilanciare la struttura.
void insert(Tree* T, int data) {
    Node* z = createNode(T, data);
    
    Node* y = T->nil;       
    Node* x = T->root;      
    
    // Ricerca della posizione corretta di inserimento (O(log n))
    while (x != T->nil) {
        y = x; 
        if (z->data < x->data) {
            x = x->left;
        } else {
            x = x->right;
        }
    }
    
    // Collegamento del nuovo nodo
    z->parent = y;
    if (y == T->nil) {
        T->root = z;       
    } else if (z->data < y->data) {
        y->left = z;
    } else {
        y->right = z;
    }
    
    // Ribilanciamento post-inserimento
    insertFixup(T, z);
}

// Visita simmetrica dell'albero (In-Order Traversal)
void inorder(Tree* T, Node* x) {
    if (x != T->nil) {
        inorder(T, x->left);
        printf("%d(%c) ", x->data, x->color == RED ? 'R' : 'B');
        inorder(T, x->right);
    }
}

int main() {
    Tree* rbTree = createTree();
    
    int dataSet[] = {10, 20, 30, 15, 25, 5};
    int dataSize = sizeof(dataSet) / sizeof(dataSet[0]);
    
    for (int i = 0; i < dataSize; i++) {
        insert(rbTree, dataSet[i]);
    }
    
    printf("Visita In-Order (Valore(Colore)):\n");
    inorder(rbTree, rbTree->root);
    printf("\n");
    
    return 0;
}
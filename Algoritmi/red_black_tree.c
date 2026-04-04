#include <stdio.h>
#include <stdlib.h>

// Uso un enum per i colori, molto più pulito che usare 0 e 1 e rischiare di confondersi
typedef enum { RED, BLACK } Color;

// Struttura del nodo dell'albero Rosso-Nero
typedef struct Node {
    int data;
    Color color;
    struct Node* left;
    struct Node* right;
    struct Node* parent;
} Node;

// Struttura dell'albero.
// Uso il nodo sentinella (nil) al posto dei classici puntatori a NULL.
// Mi salva la vita per evitare un sacco di controlli "if (nodo != NULL)"!
typedef struct Tree {
    Node* root;
    Node* nil;
} Tree;

// =========================================================================
// FUNZIONI DI SUPPORTO E INIZIALIZZAZIONE
// =========================================================================

// Inizializzo l'albero e configuro il nodo sentinella (che per definizione deve essere NERO)
Tree* createTree() {
    Tree* T = (Tree*)malloc(sizeof(Tree));
    T->nil = (Node*)malloc(sizeof(Node));
    T->nil->color = BLACK; // Il nil è sempre nero
    T->root = T->nil;      // All'inizio l'albero è vuoto, quindi la radice punta a nil
    return T;
}

// Funzione comoda per creare un nuovo nodo "slegato"
Node* createNode(Tree* T, int data) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    newNode->data = data;
    newNode->color = RED;  // I nuovi nodi inseriti sono SEMPRE rossi all'inizio
    newNode->left = T->nil;
    newNode->right = T->nil;
    newNode->parent = T->nil;
    return newNode;
}

// =========================================================================
// ROTAZIONI
// =========================================================================

// Rotazione a sinistra: "tiro giù" il nodo x verso sinistra e "tiro su" il suo figlio destro y
void leftRotate(Tree* T, Node* x) {
    Node* y = x->right;         // y è il figlio destro di x
    x->right = y->left;         // il sottoalbero sinistro di y diventa il sottoalbero destro di x
    
    if (y->left != T->nil) {
        y->left->parent = x;    // aggiorno il padre del sottoalbero spostato
    }
    
    y->parent = x->parent;      // y prende il posto di x agganciandosi al padre di x
    
    // Devo capire a chi agganciare y: radice, figlio sinistro o figlio destro?
    if (x->parent == T->nil) {
        T->root = y;            // x era la radice, ora lo è y
    } else if (x == x->parent->left) {
        x->parent->left = y;    // x era un figlio sinistro
    } else {
        x->parent->right = y;   // x era un figlio destro
    }
    
    y->left = x;                // x diventa il figlio sinistro di y
    x->parent = y;              // e y diventa il padre di x
}

// Rotazione a destra: è praticamente la leftRotate scritta allo specchio!
// "Tiro giù" x verso destra e "tiro su" il suo figlio sinistro y
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

// =========================================================================
// INSERIMENTO E FIXUP (Il cuore dell'albero Rosso-Nero!)
// =========================================================================

// Questa funzione ripristina le proprietà RB dopo aver inserito un nodo rosso.
// L'unico problema che può succedere è avere un nodo rosso con un padre rosso (Red-Red violation).
void insertFixup(Tree* T, Node* z) {
    // Finché il padre di z è rosso, c'è una violazione
    while (z->parent->color == RED) {
        
        // CASO A: Il padre di z è un figlio SINISTRO del nonno
        if (z->parent == z->parent->parent->left) {
            Node* y = z->parent->parent->right; // y è lo ZIO di z
            
            // Caso 1: Lo zio y è ROSSO
            // Soluzione: Ricoloro padre e zio di nero, il nonno di rosso e sposto il problema sul nonno
            if (y->color == RED) {
                z->parent->color = BLACK;
                y->color = BLACK;
                z->parent->parent->color = RED;
                z = z->parent->parent; // Risalgo l'albero
            } 
            else {
                // Caso 2: Lo zio y è NERO e z è un figlio DESTRO (forma a "triangolo")
                // Soluzione: Faccio una rotazione a sinistra sul padre per trasformarlo in una linea
                if (z == z->parent->right) {
                    z = z->parent;
                    leftRotate(T, z);
                }
                
                // Caso 3: Lo zio y è NERO e z è un figlio SINISTRO (forma a "linea")
                // Soluzione: Ricoloro padre e nonno, poi rotazione a destra sul nonno
                z->parent->color = BLACK;
                z->parent->parent->color = RED;
                rightRotate(T, z->parent->parent);
            }
        } 
        // CASO B: Il padre di z è un figlio DESTRO del nonno (tutto simmetrico al Caso A)
        else {
            Node* y = z->parent->parent->left; // Lo ZIO di z
            
            // Caso 1 simmetrico
            if (y->color == RED) {
                z->parent->color = BLACK;
                y->color = BLACK;
                z->parent->parent->color = RED;
                z = z->parent->parent;
            } 
            else {
                // Caso 2 simmetrico (triangolo specchiato)
                if (z == z->parent->left) {
                    z = z->parent;
                    rightRotate(T, z);
                }
                
                // Caso 3 simmetrico (linea specchiata)
                z->parent->color = BLACK;
                z->parent->parent->color = RED;
                leftRotate(T, z->parent->parent);
            }
        }
    }
    // La radice deve sempre rimanere NERA alla fine di tutto!
    T->root->color = BLACK;
}

// Funzione principale di inserimento
void insert(Tree* T, int data) {
    Node* z = createNode(T, data);
    
    Node* y = T->nil;       // Puntatore per tenere traccia del padre durante la discesa
    Node* x = T->root;      // Puntatore per scendere nell'albero
    
    // Scendo giù nell'albero come in un normale BST (Binary Search Tree) per trovare il posto giusto
    while (x != T->nil) {
        y = x;
        if (z->data < x->data) {
            x = x->left;
        } else {
            x = x->right;
        }
    }
    
    // Aggancio il nuovo nodo z al padre trovato y
    z->parent = y;
    if (y == T->nil) {
        T->root = z;        // L'albero era vuoto, z diventa la radice
    } else if (z->data < y->data) {
        y->left = z;
    } else {
        y->right = z;
    }
    
    // z è stato inserito come nodo ROSSO. Chiamo il fixup per sistemare eventuali conflitti colore!
    insertFixup(T, z);
}

// =========================================================================
// STAMPA (In-Order Traversal)
// =========================================================================

// Una semplice visita simmetrica per verificare che l'inserimento funzioni
// Stampa i valori in ordine crescente e indica il colore (R o B)
void inorder(Tree* T, Node* x) {
    if (x != T->nil) {
        inorder(T, x->left);
        printf("%d(%c) ", x->data, x->color == RED ? 'R' : 'B');
        inorder(T, x->right);
    }
}

// =========================================================================
// MAIN
// =========================================================================

int main() {
    printf("Inizializzo l'albero Rosso-Nero...\n");
    Tree* mioAlbero = createTree();
    
    // Facciamo qualche inserimento di prova per far scattare un po' di rotazioni e fixup
    int valori[] = {10, 20, 30, 15, 25, 5};
    int num_valori = sizeof(valori) / sizeof(valori[0]);
    
    for (int i = 0; i < num_valori; i++) {
        printf("Inserisco il nodo: %d\n", valori[i]);
        insert(mioAlbero, valori[i]);
    }
    
    printf("\nVisita In-Order (Valore e Colore):\n");
    inorder(mioAlbero, mioAlbero->root);
    printf("\n");
    
    // Nota per me: ricordarsi di fare una funzione per liberare la memoria (free) se il prof lo chiede all'esame!
    
    return 0;
}
#include <stdio.h>
#include <string.h>

/*
    --MODI DI CREARE STRINGHE--
1. Stringa Letterale
Una stringa letterale è una sequenza di caratteri definita tra virgolette doppie ("). 
Questo tipo di stringa è immutabile (non può essere modificata) e viene memorizzata 
in una sezione di memoria di sola lettura. In C, le stringhe letterali sono trattate come array di caratteri 
con un carattere speciale di terminazione '\0'.
--char* str = "Ciao";--
Vantaggi: Facile da scrivere e usare, automaticamente terminata con '\0'.
Svantaggi: Non può essere modificata; la memoria è gestita dal sistema, quindi non puoi cambiare la lunghezza.

2. 
2. Array di Caratteri (Stringa Modificabile)
Un array di caratteri è una sequenza di caratteri che puoi modificare. 
Questo ti consente di cambiare il contenuto della stringa, ma devi prestare attenzione alla gestione della memoria,
in particolare quando la dimensione dell'array è fissa.

char str[] = "Ciao";  // La lunghezza viene calcolata automaticamente
Oppure puoi dichiararlo come un array di caratteri con una dimensione esplicitamente specificata:

char str[100] = "Ciao";  // Dichiara un array abbastanza grande per 100 caratteri
Vantaggi: Puoi modificare la stringa. La dimensione dell'array può essere grande quanto desideri.
Svantaggi: Devi gestire tu stesso la dimensione dell'array, assicurandoti che ci sia spazio sufficiente per 
contenere la stringa e il carattere di terminazione '\0'.
*/

int main(){
    //STRCAT APPENDE STR A STR1
    char str[] = "Ciao Ragazzi";
    char str1[50] = "Sono giuseppe";
    strcat(str1, str);
    printf("%s\n", str1);

    //STRNCAT SIMILE A STRCAT MA DECIDO QUANTI CARATTERI APPENDERE
    char str2[] = "Ciao Ragazzi";
    char str3[50] = "Sono giuseppe";
    strncat(str3, str2, 10);
    printf("%s\n", str3);

    //STRLEN CALCOLA LUNGHEZZA STRINGA, NON CONTANDO \0
    char *str4 = "Scrivo qualcosa";
    int n = strlen(str4);
    printf("%d\n", n); 

    //STRCMP COMPARA DUE STRINGHE LESSICOGRAFICAMENTE
    /*
    Ritorna < 0: se str1 è minore di str2
    Ritorna 0: se str1 è uguale a str2
    Ritorna > 0: se str1 è maggiore di str2
    */
   char* str5 = "Piao";
   char* str6 = "Mamma";
   int res = strcmp(str5, str6);
   printf("Stampo res stringhe: %d\n", res);

   //STRNCMP COMPARA N CARATTERI
   int strncmp (const char* str1 , const char* str2 , size_t numero );

    //STRCPY COPIA SCR IN DEST
    //char* strcpy (char* dest , const char* src );

    //STRNCPY STESSA COSA MA CON NUMERO DI CARATTERI
    //char* strncpy ( char* dest , const char* src , size_t n );

    //STRCHR TROVA PRIMA OCCORENZA DI UN CARATTERE IN UNA STRINGA
    //RESTITUISCE IL PUNTATORE AL PRIMO CARATTERE DELLA STRINGA
    char* str7 = "Stringa da controllare";
    char controllare = 'r';
    char* res_str = strchr(str7, controllare);
    printf("Il carattere è stato trovato in posizione: %ld\n", res_str - str7);

    //STRRCHR SERVE A TROVARE L'ULTIMA OCCORRENZA DEL CARATTERE 
    //RESTITUISCE PUNTATORE A ULTIMA OCCORRENZA
    //char* strchr(str7, controllare);

    //STRSTR RESTIUISCE LA PRIMA OCCORRENZA DI UNA SOTTOSTRINGA IN UNA STRINGA
    /*
    s1 : Questa è la stringa principale da esaminare.
    s2 : questa è la sottostringa da ricercare nella stringa s1.
    Valore di ritorno:
    -Se s2 si trova in s1, questa funzione restituisce un puntatore al primo carattere di s2 in s1, 
    altrimenti restituisce un puntatore null.
    -Restituisce s1 se s2 punta a una stringa vuota.
    */
    char *strstr (const char *str_1, const char *str_2);

    //STRTOK DIVIDE LA STRINGA IN UNA SOTTOSTRINGA IN BASE A SET DI CARATTERI DELIMITATORI
    char * strtok (char* str , const char * delims );
    /*
    Funzionamento
    -La funzione strtok cerca il primo token nella stringa str, cioè una porzione di stringa che non contiene 
    caratteri di delimitazione.
    -Restituisce un puntatore al primo token trovato. Alla fine del token, 
    la funzione sostituisce il carattere di delimitazione con un carattere \0 (terminatore di stringa) per 
    separare i vari token.
    -Quando viene chiamata di nuovo con str impostato su NULL, strtok continua a cercare token nella stessa stringa.
     Restituisce i successivi token uno alla volta, fino a che non ci sono più token da restituire.
    -Quando non ci sono più token nella stringa, la funzione restituisce NULL.
    */
    char str10[] = "Questo è un esempio di utilizzo della funzione strtok!";
    const char *delim = " ";  // I token saranno separati dallo spazio
    char *token;

    // Prima chiamata a strtok per ottenere il primo token
    token = strtok(str10, delim);
    
    // Ciclo che estrae tutti i token successivi
    while (token != NULL) {
        printf("Token: %s\n", token);
        token = strtok(NULL, delim);  // Chiamata successiva per continuare la tokenizzazione
    }
    //Modifica della stringa originale: La funzione strtok modifica la stringa originale sostituendo i delimitatori con il carattere \0. 
    //Pertanto, se è necessario preservare la stringa originale, è consigliabile lavorare su una copia della stessa.
    /*
    char str[] = "Ciao, come stai?";
    DIVENTA
    str = "Ciao\0 come\0 stai?\0";
    Al primo ciclo devo chiamare da (Start_str, Delim), dal secondo in poi da (NULL, delim);
    */






}
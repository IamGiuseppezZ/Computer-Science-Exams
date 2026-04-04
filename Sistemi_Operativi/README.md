# ⚙️ Sistemi Operativi (Operating Systems)

Benvenuti nella cartella dedicata alle prove pratiche e ai progetti del corso di Sistemi Operativi.

## 📄 Cosa troverai qui
Questa sezione raccoglie programmi scritti in C puro focalizzati sulla concorrenza, la gestione della memoria e l'interazione diretta con il kernel del sistema operativo tramite chiamate di sistema (System Calls). I progetti includono pipeline di elaborazione file, architetture Produttore-Consumatore e modelli Client-Server locali.

## 🎯 Obiettivo e Competenze Dimostrate
L'obiettivo di questi esercizi è padroneggiare il multithreading e la sincronizzazione di processi concorrenti, prevenendo problemi classici come *Race Conditions* e *Deadlocks*. Analizzando il codice, potrete valutare le mie competenze in:
* **Multithreading (POSIX Threads):** Creazione, gestione e join di thread concorrenti (`<pthread.h>`).
* **Primitive di Sincronizzazione:** Utilizzo avanzato di Mutex, Semafori (es. libreria custom `mac_sem` o standard) e Variabili di Condizione (`pthread_cond_t`) per implementare pattern complessi (es. Monitor e Barriere asimmetriche).
* **Gestione della Memoria Avanzata:** Utilizzo del memory mapping (`mmap`, `munmap`, `msync`) per l'accesso e la copia efficiente di file di grandi dimensioni scavalcando il buffering standard.
* **Terminazione Sicura:** Implementazione di meccanismi di uscita pulita (come la *Poison Pill*) per garantire che i thread terminino correttamente e deallochino tutte le risorse senza Memory Leaks.

## ⏱️ Nota sul Contesto
I file in questa repository sono nati come prove pratiche da laboratorio e sessioni d'esame. Sono stati sviluppati sotto stretta pressione temporale, in un lasso di tempo di **circa 2,5 - 3 ore**. Rappresentano quindi soluzioni focalizzate al core del problema: far comunicare e sincronizzare correttamente i thread senza mandare il sistema in stallo, privilegiando l'affidabilità logica in scenari critici.
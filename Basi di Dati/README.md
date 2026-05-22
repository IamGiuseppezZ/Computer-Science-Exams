🌐 ISP Management System - Database Design & Implementation

Questo repository contiene la progettazione completa (concettuale, logica e fisica) e l'implementazione in MySQL di un database relazionale per un Internet Service Provider (ISP). Il sistema è progettato per gestire in modo integrato l'erogazione dei contratti, l'infrastruttura di rete, il tracciamento dei log di sessione, l'assistenza tecnica e il ciclo di fatturazione.

🎯 Obiettivo del Progetto e Competenze Dimostrate

Il progetto simula un ambiente di produzione reale con volumi di dati massivi, dimostrando la capacità di gestire l'intero ciclo di vita del dato.
Attraverso questo progetto ho consolidato e dimostrato le seguenti competenze tecniche:

-Database Design: Modellazione E/R avanzata (gestione di gerarchie ISA totali ed esclusive) e derivazione dello schema logico.

-Normalizzazione: Progettazione rigorosa dello schema relazionale nel pieno rispetto della Terza Forma Normale (3NF) e della Forma Normale di Boyce-Codd (BCNF), garantendo l'assenza di anomalie e dipendenze transitive.

-Physical Tuning & Optimization: Analisi dei costi di ricerca I/O (calcolo del fattore di blocco dati) e implementazione strategica di Indici B+Tree (es. Indici multi-livello compositi) per ottimizzare le Range e Point Query, valutando accuratamente l'overhead di scrittura negli scenari ad alta frequenza di inserimento.

-Logica Attiva (Trigger): Sviluppo di automazioni lato server per imporre vincoli di integrità aziendale complessi, riducendo il carico applicativo.

-Advanced SQL (DML & DDL): Scrittura di query analitiche e batch (JOIN complesse, aggregazioni, interrogazioni temporali) per le operazioni di business.

🏗️ Architettura del Sistema

Il dominio applicativo è suddiviso in tre macro-aree fortemente coese:
Area Commerciale: Gestione anagrafiche clienti (Privati/Aziende) e sottoscrizione dei Piani Tariffari.
-Area Infrastruttura (Core): Mappatura dei Nodi di Rete (PoP), gestione degli Apparati fisici (MAC Address) e degli IP Pool per l'assegnazione degli indirizzi, con tracciamento massivo dei log di navigazione.
-Area Servizi: Sistema di ticketing per guasti, assegnazione interventi tecnici e riconciliazione contabile delle fatture e pagamenti.

⚙️ Logica di Business e Automazioni Incluse (Trigger)

All'interno degli script SQL forniti, è implementata una logica attiva avanzata per garantire l'integrità dei dati e i processi automatici:

-Prevenzione Conflitti di Rete: Trigger che impediscono l'assegnazione dello stesso IP a sessioni contemporanee (T6) e l'uso di MAC Address clonati su linee attive (T5).

-Automazione Contabile: Ricalcolo dinamico del saldo fattura ad ogni pagamento parziale con cambio di stato automatico in "Saldato" (T3) e blocco preventivo dei nuovi contratti per i clienti morosi (T4).

-Integrità Infrastrutturale: Blocco all'eliminazione di nodi fisici ancora collegati a risorse attive (T1) e chiusura automatica dei ticket a seguito di interventi risolutivi (T2).

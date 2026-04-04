# 🌐 Architetture di Reti (Computer Networks)

Benvenuti nella cartella dedicata ai miei progetti e alle prove d'esame del corso di Reti di Calcolatori.

## 📄 Cosa troverai qui
All'interno di questa directory sono presenti diverse implementazioni in C di architetture di rete distribuite. Troverai simulazioni di sistemi reali (come reti di droni, chat P2P tramite server di discovery, sensori IoT) che comunicano tra loro utilizzando i protocolli a livello di trasporto.

## 🎯 Obiettivo e Competenze Dimostrate
Questi progetti nascono con l'obiettivo di applicare a basso livello i concetti teorici delle reti. Il codice qui presente dimostra la mia capacità di:
* **Socket Programming:** Creazione e gestione di socket `TCP` (per flussi affidabili e connessioni persistenti) e `UDP` (per telemetria e comunicazioni a bassa latenza).
* **Design di Protocolli Custom:** Definizione di pacchetti di rete strutturati (`struct` in C) e allineamento della memoria tramite `#pragma pack`.
* **Gestione dell'Endianness:** Conversione rigorosa dei dati tra *Host Byte Order* e *Network Byte Order* tramite `htonl`/`ntohl` per garantire l'interoperabilità tra architetture hardware diverse.
* **Concorrenza di Rete:** Gestione di server in grado di accettare e servire client multipli simultaneamente tramite il multithreading.

## ⏱️ Nota sul Contesto
Tutti i progetti presenti in questa cartella sono il risultato di simulazioni o prove d'esame reali. Questo significa che il codice è stato interamente pensato, scritto e testato in un arco temporale molto ristretto, tipicamente **tra le 2,5 e le 3 ore**. Di conseguenza, l'obiettivo primario del codice è la solidità logica e il corretto funzionamento delle architetture di rete richieste, piuttosto che la creazione di applicazioni "enterprise" su larga scala.
-- =====================================================================
-- SCRIPT COMPLETO: SISTEMA DI GESTIONE INTEGRATA ISP
-- Studente: Giuseppe Angelo Mazzara
-- Contenuto: Creazione DB, Tabelle, Popolamento Dati e Trigger
-- =====================================================================

-- 1. RESET E CREAZIONE DATABASE
DROP DATABASE IF EXISTS isp_db;
CREATE DATABASE isp_db;
USE isp_db;

-- =====================================================================
-- 2. CREAZIONE DELLE TABELLE
-- =====================================================================

CREATE TABLE CLIENTE (
    ID_Cliente INT AUTO_INCREMENT PRIMARY KEY,
    Email VARCHAR(100) NOT NULL,
    Indirizzo_Fatturazione VARCHAR(255) NOT NULL,
    Tipo_Cliente CHAR(1) NOT NULL,
    Nome VARCHAR(50),
    Cognome VARCHAR(50),
    CF CHAR(16),
    Ragione_Sociale VARCHAR(100),
    Partita_IVA VARCHAR(20)
);

CREATE TABLE PIANO_TARIFFARIO (
    ID_Piano INT AUTO_INCREMENT PRIMARY KEY,
    Nome VARCHAR(50) NOT NULL,
    Canone_Mensile DECIMAL(6,2) NOT NULL,
    Velocita VARCHAR(20) NOT NULL
);

CREATE TABLE NODO_RETE (
    ID_Nodo INT AUTO_INCREMENT PRIMARY KEY,
    Posizione_GPS VARCHAR(100) NOT NULL,
    Capacita_Porte INT NOT NULL
);

CREATE TABLE APPARATO (
    MAC_Address VARCHAR(17) PRIMARY KEY,
    Modello VARCHAR(50) NOT NULL,
    Versione_Firmware VARCHAR(30) NOT NULL
);

CREATE TABLE TECNICO (
    ID_Tecnico INT AUTO_INCREMENT PRIMARY KEY,
    Nome VARCHAR(100) NOT NULL,
    Specializzazione VARCHAR(50) NOT NULL,
    Costo_Orario DECIMAL(5,2) NOT NULL
);

CREATE TABLE IP_POOL (
    ID_Pool INT AUTO_INCREMENT PRIMARY KEY,
    Indirizzo_IP VARCHAR(15) NOT NULL UNIQUE,
    Subnet VARCHAR(15) NOT NULL,
    Is_Static BOOLEAN NOT NULL DEFAULT FALSE,
    ID_Nodo INT NOT NULL,
    FOREIGN KEY (ID_Nodo) REFERENCES NODO_RETE(ID_Nodo) ON DELETE RESTRICT
);

CREATE TABLE CONTRATTO (
    ID_Contratto INT AUTO_INCREMENT PRIMARY KEY,
    Data_Stipula DATE NOT NULL,
    Stato VARCHAR(20) NOT NULL DEFAULT 'Attivo',
    Tecnologia VARCHAR(20) NOT NULL,
    ID_Cliente INT NOT NULL,
    ID_Piano INT NOT NULL,
    ID_Nodo INT NOT NULL,
    MAC_Address VARCHAR(17) NOT NULL UNIQUE,
    FOREIGN KEY (ID_Cliente) REFERENCES CLIENTE(ID_Cliente) ON DELETE CASCADE,
    FOREIGN KEY (ID_Piano) REFERENCES PIANO_TARIFFARIO(ID_Piano) ON DELETE RESTRICT,
    FOREIGN KEY (ID_Nodo) REFERENCES NODO_RETE(ID_Nodo) ON DELETE RESTRICT,
    FOREIGN KEY (MAC_Address) REFERENCES APPARATO(MAC_Address) ON DELETE RESTRICT
);

CREATE TABLE SESSIONE_LOG (
    ID_Sessione INT AUTO_INCREMENT PRIMARY KEY,
    Inizio DATETIME NOT NULL,
    Fine DATETIME,
    Byte_TX BIGINT DEFAULT 0,
    Byte_RX BIGINT DEFAULT 0,
    ID_Contratto INT NOT NULL,
    ID_Pool INT NOT NULL,
    FOREIGN KEY (ID_Contratto) REFERENCES CONTRATTO(ID_Contratto) ON DELETE CASCADE,
    FOREIGN KEY (ID_Pool) REFERENCES IP_POOL(ID_Pool) ON DELETE RESTRICT
);

CREATE TABLE TICKET (
    ID_Ticket INT AUTO_INCREMENT PRIMARY KEY,
    Data_Apertura DATETIME NOT NULL,
    Descrizione TEXT NOT NULL,
    Priorita VARCHAR(20) NOT NULL,
    ID_Contratto INT NOT NULL,
    FOREIGN KEY (ID_Contratto) REFERENCES CONTRATTO(ID_Contratto) ON DELETE CASCADE
);

CREATE TABLE FATTURA (
    ID_Fattura INT AUTO_INCREMENT PRIMARY KEY,
    Data_Emissione DATE NOT NULL,
    Totale DECIMAL(8,2) NOT NULL,
    Stato VARCHAR(20) NOT NULL DEFAULT 'Da Pagare',
    ID_Contratto INT NOT NULL,
    FOREIGN KEY (ID_Contratto) REFERENCES CONTRATTO(ID_Contratto) ON DELETE CASCADE
);

CREATE TABLE INTERVENTO (
    ID_Intervento INT AUTO_INCREMENT PRIMARY KEY,
    Data_Uscita DATE NOT NULL,
    Ore_Lavorate INT DEFAULT 1,
    Esito VARCHAR(50),
    ID_Ticket INT NOT NULL,
    ID_Tecnico INT NOT NULL,
    FOREIGN KEY (ID_Ticket) REFERENCES TICKET(ID_Ticket) ON DELETE CASCADE,
    FOREIGN KEY (ID_Tecnico) REFERENCES TECNICO(ID_Tecnico) ON DELETE RESTRICT
);

CREATE TABLE PAGAMENTO (
    ID_Pagamento INT AUTO_INCREMENT PRIMARY KEY,
    Data DATETIME NOT NULL,
    Importo DECIMAL(8,2) NOT NULL,
    Metodo VARCHAR(50) NOT NULL,
    ID_Fattura INT NOT NULL,
    FOREIGN KEY (ID_Fattura) REFERENCES FATTURA(ID_Fattura) ON DELETE CASCADE
);

-- =====================================================================
-- 3. POPOLAMENTO DATI DI ESEMPIO
-- =====================================================================

INSERT INTO CLIENTE (Email, Indirizzo_Fatturazione, Tipo_Cliente, Nome, Cognome, CF, Ragione_Sociale, Partita_IVA) VALUES 
('mario.rossi@email.it', 'Via Roma 10, Milano', 'P', 'Mario', 'Rossi', 'RSSMRA80A01F205X', NULL, NULL),
('amministrazione@techsrl.com', 'Viale Europa 45, Roma', 'A', NULL, NULL, NULL, 'Tech Solutions S.r.l.', 'IT12345678901');

INSERT INTO PIANO_TARIFFARIO (Nome, Canone_Mensile, Velocita) VALUES 
('UltraFibra 1000', 29.90, '1000/300 Mbps'),
('Business Pro FWA', 49.90, '200/50 Mbps');

INSERT INTO NODO_RETE (Posizione_GPS, Capacita_Porte) VALUES 
('45.4642, 9.1900 - POP Milano Centro', 1024),
('41.9028, 12.4964 - POP Roma EUR', 2048);

INSERT INTO APPARATO (MAC_Address, Modello, Versione_Firmware) VALUES 
('00:1A:2B:3C:4D:5E', 'Fritz!Box 7590 AX', 'v7.50'),
('AA:BB:CC:DD:EE:FF', 'Huawei B818 Router', 'v11.0.1');

INSERT INTO TECNICO (Nome, Specializzazione, Costo_Orario) VALUES 
('Luca Bianchi', 'Giuntista Fibra Ottica', 35.00),
('Andrea Verdi', 'Tecnico Radio FWA', 30.00);

INSERT INTO IP_POOL (Indirizzo_IP, Subnet, Is_Static, ID_Nodo) VALUES 
('2.34.56.78', '255.255.255.255', FALSE, 1),
('81.20.30.40', '255.255.255.248', TRUE, 2);

INSERT INTO CONTRATTO (Data_Stipula, Stato, Tecnologia, ID_Cliente, ID_Piano, ID_Nodo, MAC_Address) VALUES 
('2025-10-15', 'Attivo', 'FTTH', 1, 1, 1, '00:1A:2B:3C:4D:5E'),
('2025-11-20', 'Attivo', 'FWA', 2, 2, 2, 'AA:BB:CC:DD:EE:FF');

INSERT INTO SESSIONE_LOG (Inizio, Fine, Byte_TX, Byte_RX, ID_Contratto, ID_Pool) VALUES 
('2026-05-01 08:00:00', '2026-05-01 20:00:00', 500000000, 2500000000, 1, 1),
('2026-05-01 09:00:00', NULL, 1200000000, 8000000000, 2, 2); 

INSERT INTO TICKET (Data_Apertura, Descrizione, Priorita, ID_Contratto) VALUES 
('2026-05-02 10:30:00', 'Cadute di linea frequenti', 'Alta', 1);

INSERT INTO FATTURA (Data_Emissione, Totale, Stato, ID_Contratto) VALUES 
('2026-05-01', 29.90, 'Da Pagare', 1),
('2026-05-01', 49.90, 'Da Pagare', 2);
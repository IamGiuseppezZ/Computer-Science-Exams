-- ==========================================
-- TRIGGER 2: Chiusura automatica ticket
-- ==========================================
-- Quando il tecnico finisce il lavoro e inserisce l'esito 'Positivo',
-- il trigger va a cercare il ticket collegato e lo imposta su 'Risolto' in automatico.
CREATE TRIGGER T2_Chiusura_Automatica_Ticket
AFTER INSERT ON INTERVENTO
FOR EACH ROW
BEGIN
    IF NEW.Esito = 'Positivo' THEN
        UPDATE TICKET SET Priorita = 'Risolto' WHERE ID_Ticket = NEW.ID_Ticket;
    END IF;
END;
-- TRIGGER 1: Protezione nodi di rete
-- ==========================================
-- Evito che qualcuno cancelli per sbaglio un nodo di rete che sta ancora funzionando.
-- Se ha dei contratti attivi o dei pool IP assegnati, blocco la query di delete.
CREATE TRIGGER T1_Protezione_Infrastruttura
BEFORE DELETE ON NODO_RETE
FOR EACH ROW
BEGIN
    DECLARE v_contratti_attivi INT;
    DECLARE v_pool_associati INT;
    
    SELECT COUNT(*) INTO v_contratti_attivi FROM CONTRATTO WHERE ID_Nodo = OLD.ID_Nodo AND Stato = 'Attivo';
    SELECT COUNT(*) INTO v_pool_associati FROM IP_POOL WHERE ID_Nodo = OLD.ID_Nodo;
    
    IF v_contratti_attivi > 0 OR v_pool_associati > 0 THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'Impossibile eliminare: nodo in uso.';
    END IF;
END;
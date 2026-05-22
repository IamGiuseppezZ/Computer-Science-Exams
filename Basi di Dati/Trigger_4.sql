-- ==========================================
-- TRIGGER 4: Blocco per i clienti morosi
-- ==========================================
-- Prima di registrare un nuovo contratto, controllo se il cliente ha il "vizio" di non pagare.
-- Se trovo fatture non saldate vecchie di oltre 90 giorni, blocco la stipula.
CREATE TRIGGER T4_Blocco_Clienti_Morosi
BEFORE INSERT ON CONTRATTO
FOR EACH ROW
BEGIN
    DECLARE v_fatture_insolute INT;
    
    SELECT COUNT(*) INTO v_fatture_insolute FROM FATTURA f
    JOIN CONTRATTO c ON f.ID_Contratto = c.ID_Contratto
    WHERE c.ID_Cliente = NEW.ID_Cliente AND f.Stato != 'Saldato' AND DATEDIFF(CURRENT_DATE(), f.Data_Emissione) > 90;
    
    IF v_fatture_insolute > 0 THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'Cliente moroso da oltre 90 giorni.';
    END IF;
END;
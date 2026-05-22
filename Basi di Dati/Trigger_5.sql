-- ==========================================
-- TRIGGER 5: Unicità dei router (MAC Address)
-- ==========================================
-- Controllo base per evitare casini fisici: non posso assegnare lo stesso router 
-- a due contratti diversi che sono entrambi attivi contemporaneamente.
CREATE TRIGGER T5_Unicita_Apparato_Fisico
BEFORE INSERT ON CONTRATTO
FOR EACH ROW
BEGIN
    DECLARE v_mac_in_uso INT;
    
    SELECT COUNT(*) INTO v_mac_in_uso FROM CONTRATTO WHERE MAC_Address = NEW.MAC_Address AND Stato = 'Attivo';
    
    IF v_mac_in_uso > 0 THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'MAC Address gia assegnato.';
    END IF;
END;
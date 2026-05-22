-- ==========================================
-- TRIGGER 6: Check sugli indirizzi IP
-- ==========================================
-- Se un IP sta venendo già usato in una sessione ancora aperta (dove Fine è NULL),
-- non posso assegnarlo a un'altra sessione, sennò vanno in conflitto.
CREATE TRIGGER T6_Prevenzione_Conflitti_IP
BEFORE INSERT ON SESSIONE_LOG
FOR EACH ROW
BEGIN
    DECLARE v_sessioni_attive INT;
    
    SELECT COUNT(*) INTO v_sessioni_attive FROM SESSIONE_LOG WHERE ID_Pool = NEW.ID_Pool AND Fine IS NULL;
    
    IF v_sessioni_attive > 0 THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'IP attualmente in uso.';
    END IF;
END;
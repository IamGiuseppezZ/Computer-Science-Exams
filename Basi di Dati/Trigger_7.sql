-- ==========================================
-- TRIGGER 7: Validazione data stipula
-- ==========================================
-- Semplicemente evito che per errore di battitura venga inserito un contratto 
-- con una data nel futuro, che non avrebbe senso a livello logico.
CREATE TRIGGER T7_Validazione_Temporale
BEFORE INSERT ON CONTRATTO
FOR EACH ROW
BEGIN
    IF NEW.Data_Stipula > CURRENT_DATE() THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'Data stipula nel futuro non permessa.';
    END IF;
END;
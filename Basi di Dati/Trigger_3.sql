-- ==========================================
-- TRIGGER 3: Gestione saldo fatture
-- ==========================================
-- Ogni volta che entra un pagamento, faccio la somma per vedere a che punto siamo.
-- Se il cliente ha pagato tutto l'importo della fattura, la metto come 'Saldato'.
CREATE TRIGGER T3_Riconciliazione_Contabile
AFTER INSERT ON PAGAMENTO
FOR EACH ROW
BEGIN
    DECLARE v_totale_pagato DECIMAL(8,2);
    DECLARE v_totale_fattura DECIMAL(8,2);
    
    SELECT SUM(Importo) INTO v_totale_pagato FROM PAGAMENTO WHERE ID_Fattura = NEW.ID_Fattura;
    SELECT Totale INTO v_totale_fattura FROM FATTURA WHERE ID_Fattura = NEW.ID_Fattura;
    
    IF IFNULL(v_totale_pagato, 0) >= v_totale_fattura THEN
        UPDATE FATTURA SET Stato = 'Saldato' WHERE ID_Fattura = NEW.ID_Fattura;
    END IF;
END;
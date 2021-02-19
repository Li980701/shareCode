CREATE TRIGGER isRent AFTER INSERT ON Rental 
BEGIN
        UPDATE Rental
        SET odo_out = (SELECT odometer FROM Vehicle WHERE VIN = NEW.VIN)
        WHERE VIN = NEW.VIN;
END;

CREATE TRIGGER returnCar AFTER UPDATE ON Rental 
BEGIN
        UPDATE Vehicle
        SET odometer = NEW.odo_back
        WHERE VIN = NEW.VIN;
END;

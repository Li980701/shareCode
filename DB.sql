CREATE TABLE Car (
    carMake TEXT NOT NULL,
    carModel TEXT NOT NULL,
    carYear INTEGER,
    dailyCost INTEGER,
    kmCost REAL,
    PRIMARY KEY (carMake, carModel, carYear)
);

CREATE TABLE Vehicle (
carMake TEXT,
carModel TEXT,
carYear INTEGER,
VIN TEXT,
odometer INTEGER,
primary key (VIN),
FOREIGN KEY (carMake, carModel, carYear) REFERENCES Car(carMake, carModel, carYear),
CONSTRAINT "chk1" CHECK (LENGTH(VIN)=5 and instr(VIN,'I')=0 and instr(VIN,'O')=0 and instr(VIN,'Q')=0),
CONSTRAINT "chk2" CHECK (substr(VIN,1,1) between '0' and '9' or substr(VIN,1,1) between 'A' and 'Z'),
CONSTRAINT "chk3" CHECK (substr(VIN,2,1) between '0' and '9' or substr(VIN,2,1) between 'A' and 'Z'),
CONSTRAINT "chk4" CHECK (substr(VIN,3,1) between '0' and '9' or substr(VIN,3,1)='X'),
CONSTRAINT "chk5" CHECK (substr(VIN,4,1) between '0' and '9' or substr(VIN,4,1) between 'A' and 'Z')
);

CREATE TABLE Rental (
    customerId INTEGER NOT NULL,
    VIN TEXT NOT NULL,
    odo_out INTEGER,
    odo_back INTEGER,
    date_out TEXT NOT NULL,
    date_back TEXT,
    PRIMARY KEY (customerId, VIN),
    FOREIGN KEY (customerId) REFERENCES Customer(id) ON UPDATE CASCADE,
    FOREIGN KEY (VIN) REFERENCES Vehicle(VIN)
);

CREATE TABLE Customer (
    id INTEGER,
    name TEXT NOT NULL,
    email TEXT NOT NULL,
    PRIMARY KEY (id)
);
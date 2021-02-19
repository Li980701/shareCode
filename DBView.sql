create view CustomerSummary
AS
select
customerid as 'customerId',
date_out as 'rental_date_out ',
date_back as 'rental_date_back',
c.dailyCost*(julianday(date_back)-julianday(date_out))+(odo_back-odo_out)*c.kmCost as 'rental_cost'
FROM rental a, Vehicle b,Car c
WHERE a.VIN=B.VIN AND b.carMake=c.carMake and b.carModel=c.carModel and b.carYear=c.carYear
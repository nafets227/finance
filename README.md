# finance
Finance system based on AqBanking and MariaDB

This software helps to manage financial data of your accounts. It iomports all bookings via AqBanking 
that supports HBCI and stores them in a SQL Database. Using a standard SQL access tool like MS Access
you can categorize your bookings, thus enhancing the data imported via HBCI.
IN the end various reports are available in SQL that you can look at using the same tool as for editing
the data. 

## DB Setup
Setup a MariaDB (or MySQL) database, no matter if in container of not. 
Then provide the credentrials in Environment variables to the finance container (see below for details).

If the database is empty, finance will automatically detect it and create the needed tables and views.

### Accessing the database
Typically the database is accessed from a database tool like Microsoft Access. In order to prevent users
from accidently modifying any bookings their access rights are restricted. You need to list all userIDs in 
the Environment variable DB_USERS, then start the container and it will grant the restricted rights.

## Environment Variables
* MYSQL_HOST gives the DNS name or IP adress of the host that holds the database.
* MYSQL_DATABASE name of the database
* MYSQL_USER UserID to connect to the database
* MYSQL_PASSWORD Password to connect to the database
* DB_USERS List of users that should have "normal" access to the database
* DB_xxx_PASSWORD password to set for user xxx. xxx must be in the DB_USERS
  in order to take effect.
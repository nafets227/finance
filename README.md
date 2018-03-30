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

If MYSQL_ROOT_PASSWORD is set, the finance container will also setup Users and Access rights in the database:
* MYSQL_USER gets the full access on the database and can grant access to others.
  In Standard Mariadb Container the user is setup buth without being able to grant others.
  
If also DB_USERS is set, finance container will reset users:
* delete all users not in DB_USERS (except 'root' and MYSQL_USER)
* delete all roles
* create role fin_user that can be assigned to all users that access the database
  from remote, e.g. using Access
* create all users in DB_USERS and assign default role fin_users
* If users did not exist, assign password to users in DB_USERS if DB_xxx_PASSWORD is set. Already existing users 
  will stay untouched and no passwords will be changed.

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
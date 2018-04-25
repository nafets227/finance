# finance
Finance system based on AqBanking and MariaDB

This software helps to manage financial data of your accounts. It imports all bookings via AqBanking 
that supports HBCI / FinTS and stores them in a SQL Database. Using a standard SQL access tool like MS Access
you can categorize your bookings, thus enhancing the data imported via HBCI.
In the end various reports are available in SQL that you can look at using the same tool as for editing
the data.

## Limitations
* Only PIN access is currently supported, no HBCI Keys
* Only tested with german banks that have a "BLZ". 
* Only MariaDB is tested, MySQL should also work. Main divergence from Standard SQL is that tinance uses the "IF EXISTS" claus in CREATE TABLE and similar commands.

## AqBanking setup
AqBanking setup is expected to be mounted into the container via Docker-Volume at /finance. finance container
will set access rights as needed.
### PinFile
AqBanking PINFILE is expected at /finance/.hbci-pinfile
### Setup Parameters
For banks or accounts that need special parameters, you can add a line 
```
SETUP_<blz>_<user> = "aqhbci-tool4 parms"
```
into the Pinfile. The parameters wil be forwarded to aqhbci-tool4 adduser when it is called.
Please note that this is not available in AqBanking but just in this container.  
 

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

##Alerting
If MAIL_TO is set, every run will send the bookings of the last 7 days plus the current balance ot MAIL_ACCOUNTS to MAIL_TO.
The delivery will be based on SMTP to MAIL_URL. Authentication to the mail server is not yet supported.

## Environment Variables
* MYSQL_HOST gives the DNS name or IP adress of the host that holds the database.
* MYSQL_DATABASE name of the database
* MYSQL_USER UserID to connect to the database
* MYSQL_PASSWORD Password to connect to the database
* DB_USERS List of users that should have "normal" access to the database
* DB_xxx_PASSWORD password to set for user xxx. xxx must be in the DB_USERS
* MAIL_TO Recipient(s) of Alerting E-Mails. If set, it also activates the alerting feature. 
* MAIL_FROM sender of the Alerting E-Mails
* MAIL_ACCOUNTS account numbers, separated by blanks, to be alerted.
* MAIL_URL Server where to deliver the Alerting E-Mails like 'smtp[s]://user:password@some.host:port'
* MAIL_HOSTNAME optionally give the hostname that we report to SMTP server in HELO command. Usefule if the
  SMTP server requires a FQDN. Defaults to hostname. 

## Advanced features
fntxt2sql program supports more than mySql targets, have a look into its C sourceode. However, the scripts of this
container only support importing into a Mysql/Mariadb database.
  in order to take effect.
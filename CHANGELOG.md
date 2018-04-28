# finance CHANGELOG

## 0.3.2 (2018-04-28)
* Ignore accounts that AqBanking cannot handle (thanks to Michael Bladowski) 

## 0.3.1 (2018-04-26)
* Use noted balance if booked balance is not available. 
  Fixes [#1](https://github.com/nafets227/finance/issues/1)

## 0.3 (2018-04-25)
* Add View fn\_entry\_balance for monthly balances including corrections. Requires up-to-date MariaDB/Mysql as it uses 
  Database Window functions.

## 0.2 (2018-04-08)
* New Variabel MAIL_HOSTNAME (see [README.md](README.md) for more details)
* Bugfix testscript (drop fn\_entry\_manual before start)  

## 0.1.1 (2018-04-05)
* add view fn\_entry\_manual

## 0.1 (2018-04-05)
First release version (Beta)
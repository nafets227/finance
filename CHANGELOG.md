# finance CHANGELOG

## 0.5-ALPHA2
* Fixed Bug in Building the container

## 0.5-ALPHA1
* Switch to Aqbanking6 (or its beta), supporting PSD2
* limit download of transfers to last 90 days to avoid strong authentication
  as required by PSD2
* Build in separate Build Containers to keep the image smaller


## 0.4.1 (2019-10-02)
* Do not exit when DB Server does not respond to ping

## 0.4 (2019-05-15)
* move to archlinux/base as docker base image since pritunl/archlinux is discontinued

## 0.3.3 (2018-05.16)
* revert order of bookings in Alert EMail

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

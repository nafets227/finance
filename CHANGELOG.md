# finance CHANGELOG

## 0.7
* allow interactive input of tan received from 2-factor (see README)
* Bump to aqbanking 6.3.2
* Bump to gwnhywfar 5.7.3
* suppress warning printed by find on wrong order of parms (--maxdepth)

Known Bug:
* Initialising from scratch fails. It seems an aqbanking bug
  (reported to AqBanking, see https://www.aquamaniac.de/rdm/issues/244)

## 0.6.1
* Bump to aqbanking 6.3.0
* Bump to gwenhywfar 5.6.0
* use xmlsec from alpine linux
* stop using Docker Hub Builds, that are no longer free of charge
* Move default location for Docker Images is ghcr.io/nafets227/finance
* Keep updating deprecated location docker.io/nafets227/finance
  Only Releases are forwarded, forwarding may stop at any time

## 0.6
* Support Non-compliant ING Diba
* Allow interactive input of TAN

## 0.5.1
* ease testing with testlocal [ test | bash | initdata | initdb ]
* Bump to aqbanking 6.2.1
* bump to gwenhywfar 5.3.0
* bump to xmlsec1 1.2.30

## 0.5
## 0.5-BETA2
* Switch to Alpine linux, reducing Image size from >1GB to ~90MB
* Bump to AqBanking 6.1.0
* fn_entry_balance now includes bank code number (#13)

## 0.5-BETA1
* Fix typo in script (thanks to Alexander Gross)
* Implement Import of aqb6 download data into Database
* Dont use aqbanking and gwenhywfar packages but build them.
  This allow to specify the version to use instead of using the latest
  package version.

## 0.5-ALPHA2
* Fixed Bug in Building the container

## 0.5-ALPHA1
* Switch to Aqbanking6 (or its beta), supporting PSD2
* limit download of transfers to last 90 days to avoid strong authentication
  as required by PSD2
* Build in separate Build Containers to keep the image smaller
* Import of downloaded data into database not yet implemented

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

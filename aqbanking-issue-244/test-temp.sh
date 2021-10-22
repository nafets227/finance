#!/bin/bash

set -x

rm -rf ~/.aqbanking

aqbanking-cli versions

export AQHBCI_LOGLEVEL=info
export AQBANKING_LOGLEVEL=info
export GWEN_LOGLEVEL=info

aqhbci-tool4 adduser -b 70020270 -N 1935220770 -u 1935220770 -t pintan --context=1 -s hbci-01.hypovereinsbank.de/bank/hbci --hbciversion=300
aqhbci-tool4 getbankinfo -u 1
aqhbci-tool4 getsysid -u 1
aqhbci-tool4 setitanmode -u 1 -m 6904
aqhbci-tool4 getaccounts -u 1
aqhbci-tool4 listaccounts -v

ls -lRa ~/.aqbanking

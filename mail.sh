#!/bin/sh
#
# Mail Kontostand und Buchungen
#
# (C) 2012-2018 Stefan Schallenberg
#

myaccounts="12345 98765"

for a in $myaccounts ; do
	
	Saldo=$(
	mysql -s dbFinance <<EOF
	SELECT datum, betrag
	  FROM fn_entry 
	 WHERE buchart = 'E' AND
	         orig_ktonr="$a" AND 
	         datum = ( SELECT MAX(DATUM) from fn_entry where orig_ktonr='$a');
	EOF
	)
	
	( mysql dbFinance <<EOF
	SELECT datum, betrag, part_name1, vzweck1, vzweck2
	  FROM fn_entry
	 WHERE buchart='B' AND
	       orig_ktonr='$a' AND
	       datum >= DATE_SUB(CURDATE(), INTERVAL 7 DAY);
	EOF
	) | mail -s "Kontoinfos $a: $Saldo" -r "finance <no-reply@mydomain.tld>" recipient@mydomain.tld
donw
#!/bin/bash
#
# Test finance locally, assuming a working Mysql/MariaDB somewhere
#
# Environment Vairables to be set:
# 
# MYSQL_HOST, MYSQL_DATABASE, MYSQL_USER, MYSQL_PASSWORD

if [ -z "$MYSQL_HOST" ] && [ ! -z "$BASEURL" ] ; then
	printf "Using BASEURL %s for MYSQL_HOST\n" "$BASEURL"
	MYSQL_HOST="$BASEURL"
fi

if [ -z "$MYSQL_HOST" ] ||
   [ -z "$MYSQL_DATABASE" ] ||
   [ -z "$MYSQL_USER" ] ||
   [ -z "$MYSQL_PASSWORD" ] ; then
	printf "Error: Not all required Environment Variables are set.\n"
	exit 1
fi
 
docker build . -t nafets227/finance:local

export MYSQL_HOST MYSQL_DATABASE MYSQL_USER MYSQL_PASSWORD
docker run \
	-e MYSQL_HOST \
	-e MYSQL_DATABASE \
	-e MYSQL_USER \
	-e MYSQL_PASSWORD \
	"nafets227/finance:local"


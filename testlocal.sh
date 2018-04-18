#!/bin/bash
#
# Test finance locally, assuming a working Mysql/MariaDB somewhere
#
# Environment Vairables to be set:
# 
# MYSQL_HOST, MYSQL_DATABASE, MYSQL_USER, MYSQL_PASSWORD

function test_dbconnect {
	local user="$1"
	local pw="$2"
	local rcexp="${3:-0}"

	local CMD_PW	
	if [ -z "$pw" ] ; then
		CMD_PW=""
		PRT_PW=""
	else
		CMD_PW="--password=$pw"
		PRT_PW=", password \"$pw\""
	fi
	mysql \
		--host=$MYSQL_HOST \
		--user=$user \
		$CMD_PW \
		$MYSQL_DATABASE \
		>/dev/null \
		2>/dev/null \
		<<-EOF
			SELECT 1;
		EOF
	rc=$?
	if [ $rc != "$rcexp" ] ; then
		printf "ERR: Connecting to DB with user %s%s: RC=%s(Exp=%s)\n" \
			"$user" "$PRT_PW" "$rc" "$rcexp"  
		return 1
	fi
	
	return 0
}	

##### Setup Database for Tests ###############################################
function setup_testdb () {
	printf "Setting up Test Database start.\n"
	# Setup Helper Vars
	MYSQL_ROOT_CMD="mysql"
	MYSQL_ROOT_CMD="$MYSQL_ROOT_CMD --host=$MYSQL_HOST"
	MYSQL_ROOT_CMD="$MYSQL_ROOT_CMD --user=root"
	MYSQL_ROOT_CMD="$MYSQL_ROOT_CMD --password=$MYSQL_ROOT_PASSWORD"
	MYSQL_ROOT_CMD="$MYSQL_ROOT_CMD	$MYSQL_DATABASE"

	test_dbconnect "$MYSQL_USER" "$MYSQL_PASSWORD" 0 || return 1
	
	$MYSQL_ROOT_CMD <<-EOF
		CREATE OR REPLACE USER testusershouldbedeleted;
		GRANT select ON $MYSQL_DATABASE.* TO testusershouldbedeleted;
		DROP TABLE IF EXISTS fn_entry;
		DROP VIEW IF EXISTS fn_entry_cat;
		DROP VIEW IF EXISTS fn_entry_manual;
		DROP VIEW IF EXISTS fn_entry_balance;
		DROP USER IF EXISTS testuser1;
		DROP USER IF EXISTS testuser2;
		EOF
	if [ $? != "0" ] ; then return 1 ; fi
	
	test_dbconnect "testusershouldbedeleted" "" 0 || return 1
	test_dbconnect "testuser1" "" 1 || return 1
	test_dbconnect "testuser2" "" 1 || return 1

	printf "Setting up Test Database end.\n"
	return 0	
}

#### Test DB Setup ###########################################################
test_dbsetup () {
	printf "Testing DB Setup start.\n"
	test_dbconnect "testusershouldbedeleted" "" 1 || return 1
	test_dbconnect "testuser1" "" 1 || return 1
	test_dbconnect "testuser1" "dummypw" 0 || return 1
	test_dbconnect "testuser2" "" 0 || return 1
	
	printf "Testing DB Setup end.\n"
	return 0
}

##### Main ###################################################################
if [ -z "$MYSQL_HOST" ] && [ ! -z "$BASEURL" ] ; then
	printf "Using BASEURL %s for MYSQL_HOST\n" "$BASEURL"
	MYSQL_HOST="$BASEURL"
fi

if [ -z "$MAIL_URL" ] && [ ! -z "$BASEURL" ] ; then
	MAIL_URL="smtp://mail.${BASEURL#www.}"
	printf "Using BASEURL %s for MAIL_URL => %s\n" "$BASEURL" "$MAIL_URL"
fi

if [ -z "$MAIL_HOSTNAME" ] && [ ! -z "$BASEURL" ] ; then
	MAIL_HOSTNAME="finance-testlocal.${BASEURL#www.}"
	printf "Using BASEURL %s for MAIL_URL => %s\n" "$BASEURL" "$MAIL_HOSTNAME"
fi


if [ -z "$MYSQL_HOST" ] ||
   [ -z "$MYSQL_DATABASE" ] ||
   [ -z "$MYSQL_USER" ] ||
   [ -z "$MYSQL_PASSWORD" ] ||
   [ -z "$MYSQL_ROOT_PASSWORD" ] ; then
	printf "Error: Not all required Environment Variables are set.\n"
	exit 1
fi

# Build
docker build . -t nafets227/finance:local || exit 1

# prepare the database to have the right testcases
setup_testdb

# prepare the filesystem (make it empty)
test -d ./testdata  && rm -rf ./testdata
mkdir ./testdata
cp .hbci-pinfile ./testdata/.hbci-pinfile || exit 1

# Start our just built container
printf "Executing container 1st time - start.\n"
DB_USERS="testuser1 testuser2"
DB_testuser1_PASSWORD="dummypw"

export MYSQL_HOST MYSQL_DATABASE MYSQL_USER MYSQL_PASSWORD
export MYSQL_ROOT_PASSWORD DB_USERS DB_testuser1_PASSWORD
export MAIL_TO MAIL_FROM MAIL_URL MAIL_HOSTNAME MAIL_ACCOUNTS
docker run \
	-e MYSQL_HOST \
	-e MYSQL_DATABASE \
	-e MYSQL_USER \
	-e MYSQL_PASSWORD \
	-e MYSQL_ROOT_PASSWORD \
	-e DB_USERS \
	-e DB_testuser1_PASSWORD \
	-e MAIL_TO \
	-e MAIL_FROM \
	-e MAIL_URL \
	-e MAIL_HOSTNAME \
	-e MAIL_ACCOUNTS \
	-v $(pwd)/testdata:/finance \
	"nafets227/finance:local" \
	|| exit 1

printf "Executing container 1st time - end.\n"

# Now check is results are what we expected.
test_dbsetup

# Start our just built container another time
printf "Executing container 2nd time - start.\n"

export MYSQL_HOST MYSQL_DATABASE MYSQL_USER MYSQL_PASSWORD
export MYSQL_ROOT_PASSWORD DB_USERS DB_testuser1_PASSWORD
docker run \
	-e MYSQL_HOST \
	-e MYSQL_DATABASE \
	-e MYSQL_USER \
	-e MYSQL_PASSWORD \
	-e MYSQL_ROOT_PASSWORD \
	-e DB_USERS \
	-e DB_testuser1_PASSWORD \
	-e MAIL_TO \
	-e MAIL_FROM \
	-e MAIL_URL \
	-e MAIL_HOSTNAME \
	-e MAIL_ACCOUNTS \
	-v $(pwd)/testdata:/finance \
	"nafets227/finance:local" \
	|| exit 1

printf "Executing container 2nd time - end.\n"

# database should be still the same
test_dbsetup

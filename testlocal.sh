#!/bin/bash
#
# Test finance locally, assuming a working Mysql/MariaDB somewhere
#
# Environment Vairables to be set:
#
# MYSQL_LOCAL_HOST, MYSQL_DATABASE, MYSQL_USER, MYSQL_PASSWORD

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
	if [ $DEBUG != "1" ] ; then
		REDIR=">/dev/null 2>/dev/null"
	else
		REDIR=""
	fi
	eval mysql \
		--host=$MYSQL_LOCAL_HOST \
		--user=$user \
		$CMD_PW \
		'"--execute=SELECT 1;"' \
		$MYSQL_DATABASE \
		$REDIR
	rc=$?
	if [ $rc != "$rcexp" ] ; then
		printf "ERR: Connecting to DB at %s with user %s%s: RC=%s(Exp=%s)\n" \
			"$MYSQL_LOCAL_HOST" "$user" "$PRT_PW" "$rc" "$rcexp"
		return 1
	fi

	return 0
}

##### Setup Database for Tests ###############################################
function setup_testdb () {
	printf "Setting up Test Database start.\n"
	# Setup Helper Vars
	MYSQL_ROOT_CMD="mysql"
	MYSQL_ROOT_CMD="$MYSQL_ROOT_CMD --host=$MYSQL_LOCAL_HOST"
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

##### Setup Test data directory for Tests ####################################
function setup_testdata () {
	# prepare the filesystem (make it empty)
	test -d ./testdata  && rm -rf ./testdata
	mkdir ./testdata
	cp .hbci-pinfile ./testdata/.hbci-pinfile || return 1

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

#### Start container #########################################################
exec_container () {
	export MYSQL_HOST MYSQL_DATABASE MYSQL_USER MYSQL_PASSWORD
	export MYSQL_ROOT_PASSWORD DB_USERS DB_testuser1_PASSWORD
	export MAIL_TO MAIL_FROM MAIL_URL MAIL_HOSTNAME MAIL_ACCOUNTS
	if [ -z "${DNS-}" ] ; then
		DNS_PARM=""
	else
		DNS_PARM="--dns $DNS"
	fi

	CUSTOM_PARM="${1-}"
	shift

	if [ "${1-}" == "-ti" ] ; then
		ENTRY_PARM="-ti"
	elif [ ! -z "${1-}" ] ; then
		ENTRY_PARM="-ti --entrypoint $1"
	else
		ENTRY_PARM=""
	fi
	shift

	if [ -n "${FINNET-}" ] ; then
		NET_PARM="--net $FINNET"
	else
		NET_PARM=""
	fi

	docker run \
		$DNS_PARM \
		$ENTRY_PARM \
		$NET_PARM \
		$CUSTOM_PARM \
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
		"$FINIMG" "$@" \
	|| return 1
}
##### Main ###################################################################
set -euo pipefail

if [ -z "${MYSQL_HOST-}" ] && [ ! -z "${KUBE_BASEDOM-}" ] ; then
	MYSQL_HOST="www.$KUBE_BASEDOM"
	printf "Using KUBE_BASEDOM to set MYSQL_HOST to %s\n" "$MYSQL_HOST"
fi

if [ -z "${MAIL_URL-}" ] && [ ! -z "${KUBE_BASEDOM-}" ] ; then
	MAIL_URL="smtp://www.$KUBE_BASEDOM"
	printf "Using KUBE_BASEDOM to set MAIL_URL to %s\n" "$MAIL_URL"
fi

if [ -z "${MAIL_HOSTNAME-}" ] && [ ! -z "${KUBE_BASEDOM-}" ] ; then
	MAIL_HOSTNAME="finance-testlocal.$KUBE_BASEDOM"
	printf "Using KUBE_BASEDOM to set MAIL_HOSTNAME to %s\n" "$MAIL_HOSTNAME"
fi

if [ -z "${MYSQL_LOCAL_HOST-}" ] ; then
	MYSQL_LOCAL_HOST=${MYSQL_HOST-}
	printf "Using MYSQL_HOST to set MYSQL_LOCAL_HOST to %s\n" "${MYSQL_HOST-}"
fi

if [ -z "${FINIMG-}" ] ; then
	FINIMG="nafets227/finance:local"
fi
printf "Using docker image %s for testing\n" "$FINIMG"

if	[ -z "${MYSQL_HOST-}" ] ||
	[ -z "${MYSQL_DATABASE-}" ] ||
	[ -z "${MYSQL_USER-}" ] ||
	[ -z "${MYSQL_PASSWORD-}" ] ||
	[ -z "${MYSQL_ROOT_PASSWORD-}" ]
then
	printf "Error: Not all required Environment Variables are set.\n"
	printf "\tMYSQL_HOST=%s\n" "${MYSQL_HOST-}"
	printf "\tMYSQL_DATABASE=%s\n" "${MYSQL_DATABASE-}"
	printf "\tMYSQL_USER=%s\n" "${MYSQL_USER-}"
	printf "\tMYSQL_PASSWORD=%s\n" "${MYSQL_PASSWORD-}"
	printf "\tMYSQL_ROOT_PASSWORD=%s\n" "${MYSQL_ROOT_PASSWORD-}"
	printf "\tKUBE_BASEDOM=%s\n" "${KUBE_BASEDOM-}"
	printf "\tKUBE_BASEDOM would set default for MYSQL_HOST, MAIL_URL and MAIL_HOSTNAME\n"
	exit 1
fi

action=${1:-test}
shift || true # ignore error in shift if no parm was given

case $action in
	build)
		# Build
		docker build . -t nafets227/finance:local || exit 1
		;;
	test )
		if [ "${1-}" == "--debug" ] ; then
			container_env="-e DEBUG=1"
			DEBUG=1
			shift
		else
			container_env=""
			DEBUG=0
		fi

		if [ "${1-}" == "--taninteractive" ] ; then
			container_parms=( "-ti" "$@")
		else
			container_parms=( "" "$@")
		fi

		# prepare the database to have the right testcases
		setup_testdb || exit 1
		setup_testdata || exit 1

		# Start our just built container
		printf "Executing container 1st time - start.\n"
		DB_USERS="testuser1 testuser2"
		DB_testuser1_PASSWORD="dummypw"
		if exec_container "$container_env" "${container_parms[@]}"
		then printf "Executing container 1st time - ended OK.\n"
		else 
			printf "Executing container 1st time - ended in ERROR.\n"
			exit 1
		fi

		# Now check is results are what we expected.
		test_dbsetup || exit 1

		# Start our just built container another time
		printf "Executing container 2nd time - start.\n"
		if exec_container "$container_env" "${container_parms[@]}"
		then printf "Executing container 2nd time - ended OK.\n"
		else
			printf "Executing container 2nd time - ended in ERROR.\n"
			exit 1
		fi

		# database should be still the same
		test_dbsetup || exit 1

		# Start our just built container another time
		printf "Executing container 3rd time (no MAIL_TO) - start.\n"
		unset MAIL_TO
		export -n MAIL_TO
		if exec_container "$container_env" "${container_parms[@]}"
		then printf "Executing container 3rd time (no MAIL_TO) - ended OK.\n"
		else
			printf "Executing container 3rd time (no MAIL_TO) - ended in ERROR.\n"
			exit 1
		fi

		# Start our just built container another time
		printf "Executing container 4th time (no MYSQL_ROOT_PASSWORD) - start.\n"
		unset MYSQL_ROOT_PASSWORD
		export -n MYSQL_ROOT_PASSWORD
		if exec_container "$container_env" "${container_parms[@]}"
		then printf "Executing container 4th time (no MYSQL_ROOT_PASSWORD) - ended OK.\n"
		else
			printf "Executing container 4th time (no MYSQL_ROOT_PASSWORD) - ended in ERROR.\n"
			exit 1
		fi

		printf "***** All tests on finance have been successfully completed. *****\n"
		;;
	exec )
		exec_container "" "-ti" "$@"
		exit $?
		;;
	bash )
		exec_container "" "/bin/bash" "$@"
		exit $?
		;;
	initdata )
		setup_testdata
		exit $?
		;;
	initdb )
		setup_testdb
		exit $?
		;;

	*)
		printf "Error: Unknown action %s\n" "$action"
		printf "Valid actions: test bash initdata initdb\n"
		;;
esac

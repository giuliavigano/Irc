#!/bin/bash

# server IRC variables 
SERVER_IP="192.168.2.36"
SERVER_PORT="6667"
PASSWORD="ciao"

# comment colours
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

# comments print functions
print_pass() { echo -e "$GREEN[PASS]${NC} $1"; }
print_fail() { echo -e "$RED[FAIL]${NC} $1"; }

# function to check if server is still alive after stressing tests
check_server_alive() {
	echo "TRYING to connect with server IRC: $SERVER_IP $SERVER_PORT"

	if timeout 1 bash -c "echo 'PING' | nc $SERVER_IP $SERVER_PORT" >/dev/null 2>&1 | head -5; then
		return 0
	else
		ERROR_MSG=$(timeout 1 bash -c "echo '' | nc $SERVER_IP $SERVER_PORT" 2>&1)
		print_fail "ERROR : $ERROR_MSG"
		return 1
	fi
}

test_myrapid_connection() {
	echo " TEST 1: connecting and disconnecting 300 client rapidly"

	for i in {1..300}; do
		timeout 0.1 nc $SERVER_IP $SERVER_PORT >/dev/null 2>&1 & # & to start parallel connections

		if [ $((i % 10)) -eq 0 ]; then
				echo -n "."
		fi
	done

	sleep 1

	if check_server_alive; then
		print_pass "SUCCESS! : Server still alive."
	else
		print_fail "FAIL TEST : Server died somewhere!"
		exit 1
	fi
}

# testing connections and quit with registrations
test_myconnection() {
	echo "TEST 2: register and quit immidiately"

	for i in {1..300}; do
	{
		printf "PASS %s\r\nNICK user%d\r\nUSER user%d 0 * :User\r\nQUIT :byeee\r\n" "$PASSWORD" "$i" "$i" | nc -w 2 $SERVER_IP $SERVER_PORT >/dev/null  2>&1
	} &
	done

	sleep 1

	if check_server_alive; then
		print_pass "SUCCESS! : Server still alive."
	else
		print_fail "FAIL TEST : Server died somewhere!"
		exit 1
	fi
}

test_myrapid_connection
test_myconnection
echo "TEST COMPLETED."
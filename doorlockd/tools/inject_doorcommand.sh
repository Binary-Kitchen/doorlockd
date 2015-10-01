#!/bin/bash

echo -n "Cmd: "
read CMD

echo -n "User: "
read USER

echo -n "Password: "
read -s PW
echo 

echo -n "Token: "
read TOKEN

cat << EOF | nc localhost 5555
{
"command" : "$CMD",
"user" :  "$USER",
"password" : "$PW",
"token" : "$TOKEN"
}
EOF

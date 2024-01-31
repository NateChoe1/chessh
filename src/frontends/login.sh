#!/bin/sh

read -p 'username: ' chessh_username
read -s -p 'password: ' chessh_password
echo

echo "It worked! $$" >> /test.logs

/chessh/build/chessh-client -d /chessh-server -u "$chessh_username" -p "$chessh_password"

#!/bin/bash

function cleanExit() {
  (kill $stegasis_pid)
  exit
}

# Run stegasis
(stegasis format --alg=lsb --pass=test --cap=100 $1)
(stegasis mount --alg=lsb --pass=test $1 /tmp/stegtest) &
stegasis_pid=$!
# Wait for the video to mount
sleep 5

cp src/fileA /tmp/stegtest

expected="This is a test file."
output=$(cat /tmp/stegtest/fileA)
if [[ "$expected" != "$output" ]]; then
  echo "Content of fileA incorrect"
  cleanExit
fi

mv /tmp/stegtest/fileA /tmp/stegtest/fileB
output2=$(cat /tmp/stegtest/fileB)
if [[ "$expected2" != "$output" ]]; then
  echo "Content of fileB incorrect"
  cleanExit
fi

cp src/randData /tmp/stegtest
cp /tmp/stegtest /tmp
expectedmd5="d7fb762583ea90368cfac62a608890ad"
outputmd5=$(md5sum /tmp/randData | cut -f 1 -d ' ')
if [[ "$expectedmd5" != "$outputmd5" ]]; then
  echo "MD5 of randData did not match"
  cleanExit
fi

(kill $stegasis_pid)
sleep 3

echo -e "\nAll tests passed :)\n"
exit

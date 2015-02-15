#!/bin/bash

function cleanExit() {
  (kill $stegasis_pid)
  exit
}

# Run stegasis
(stegasis format --alg=lsb --pass=test $1)
(stegasis mount --alg=lsb --pass=test $1 /tmp/stegtest) &
stegasis_pid=$!
# Wait for the video to mount
sleep 5

cp *.tar /tmp/stegtest
for f in *.tar; do tar xf $f -C /tmp/stegtest; done
rm /tmp/stegtest/*.tar

expected_files="dirfile2.txt  file1.txt  file2.txt  testdir"
files=$(ls -C /tmp/stegtest)
if [ "$files" != "$expected_files" ]; then
  echo "ls returned incorrect file list"
  echo "$files"
  cleanExit
fi

expected_file_1="This is a test file."
expected_file_2="This is a different test file."
file1=$(cat /tmp/stegtest/file1.txt)
file2=$(cat /tmp/stegtest/file2.txt)
if [[ "$file1" != "$expected_file_1" || "$file2" != "$expected_file_2" ]]; then
  echo "Contents of file(s) incorrect"
  echo "$file1"
  echo "$file2"
  cleanExit
fi

expected_files_sub="dirfile1.txt"
files_sub=$(ls -C /tmp/stegtest/testdir)
if [ "$files_sub" != "$expected_files_sub" ]; then
  echo "ls returned incorrect file list for sub directory"
  echo "$files_sub"
  cleanExit
fi

expected_file_1_sub="I am in a directory."
file1_sub=$(cat /tmp/stegtest/testdir/dirfile1.txt)
if [[ "$file1_sub" != "$expected_file_1_sub" ]]; then
  echo "Contents of sub directory file(s) incorrect"
  echo "$file1_sub"
  cleanExit
fi


(kill $stegasis_pid)
sleep 3

echo -e "\nAll tests passed :)\n"
exit

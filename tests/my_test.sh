#!/bin/bash

# cleanup previous test
rm -f received_file input_file

# create new random file of fixed size
dd if=/dev/urandom of=input_file bs=1 count=512 &> /dev/null

# check valgrind enabled
valgrind_sender=""
valgrind_receiver=""
if [ ! -z "$VALGRIND" ] ; then
    echo "VALGRIND"
    valgrind_sender="valgrind --leak-check=full --log-file=valgrind_sender.log"
    valgrind_receiver="valgrind --leak-check=full --log-file=valgrind_receiver.log"
fi

# start receiver and redirect standard output
# valgrind./receiver -o received_file :: 12345  2> receiver.log &
./receiver -o received_file :: 12345 &
receiver_pid=$!

cleanup()
{
    kill -9 $receiver_pid
    kill -9 $link_pid
    exit 0
}
trap cleanup SIGINT  # Kill les process en arrière plan en cas de ^-C

# start sender
#if ! ./sender ::1 12345 < input_file 2> sender.log ; then
if ! ./sender ::1 12345 < input_file; then
  echo "Crash du sender!"
  cat sender.log
  err=1 # save error
fi

# wait 5 seconds for receiver to complete
sleep 5 

if kill -0 $receiver_pid &> /dev/null ; then
  echo "Le receiver ne s'est pas arreté à la fin du transfert!"
  kill -9 $receiver_pid
  err=1 # save error
else  # check value returned by the receiver
  if ! wait $receiver_pid ; then
    echo "Crash du receiver!"
    cat receiver.log
    err=1 #save error
  fi
fi

# check matching between received and sent files
if [[ "$(sha256sum input_file | awk '{print $1}')" != "$(sha256sum received_file | awk '{print $1}')" ]]; then
  echo "Le transfert a corrompu le fichier!"
  echo "Diff binaire des deux fichiers: (attendu vs produit)"
  diff -C 9 <(od -Ax -t x1z input_file) <(od -Ax -t x1z received_file)
  exit 1
else
  echo "Le transfert est réussi!"
  exit ${err:-0}  # return error code in case of failure
fi
# Note that this assumes to be called from the Makefile, you may want to adapt it.
echo "My test"
./tests/my_test.sh
# Run the same test, but this time with valgrind
#echo "A very simple test, with Valgrind"
#VALGRIND=1 ./tests/my_test.sh
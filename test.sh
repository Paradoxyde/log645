cd seq
chmod a+x run.sh
make
./run.sh $1 $2 $3
make clean
# Le repertoire ne devrait pas contenir d'executable
ls
cd ..

cd par
chmod a+x run.sh
make
./run.sh $1 $2 $3
make clean
# Le repertoire ne devrait pas contenir d'executable
ls
cd ..

#Les executions sequentielle et parallele devraient produire les memes resultats

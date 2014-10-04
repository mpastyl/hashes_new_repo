#!/bin/bash

cd ../
#init hash size
#for s in 512 1024 2048 4096 ;
##for s in 8192 16384 32768 65536;
##do 
##   for i in 1 2 3 6 12 24; 
##    do
##        for name in "global_lock" "striped" "refinable" "split_ordered"; 
##            do ./main.$name -t$i -s $s -l80 -i10 >> out_${name}_init_size${s}.out;
##        done;
##    done;
##done;


#preinsetion  size
##for z in 512 1024 2048 4096 ;
for z in 32768 131072 ;
do 
    for i in 1 2 3 6 12 24; 
    do
        for name in "global_lock" "striped" "refinable" "split_ordered"; 
            do ./main.$name -t$i -s 4096 -z $z -l80 -i10 >> out_${name}_init_size4096_preinsertion_$z.out;
        done;
    done;
done;

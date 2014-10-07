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
##for z in 32768 131072 ;
##do 
##    for i in 1 2 3 6 12 24; 
##    do
##        for name in "global_lock" "striped" "refinable" "split_ordered"; 
##            do ./main.$name -t$i -s 4096 -z $z -l80 -i10 >> out_${name}_init_size4096_preinsertion_$z.out;
##        done;
##    done;
##done;


#corss init_size
#for s in 2048 4096 8192 16384 32768 ;
#do 
#        for name in "global_lock" "striped" "refinable" "split_ordered"; 
#            do ./main.$name -t64 -s $s -l80 -i10 >> out_sandman_${name}_cross_init_size.out;
#        done;
#done;

#big table
#for i in 1 2 3 6 12 24;
#do
#        ##for name in "global_lock" "striped" "refinable" "split_ordered" "non_blocking"; 
#        ##    do ./main.$name -t$i -s 16777216 -z 1048576 -l80 -i10 >> out_bit_table_${name}.out;
#        ##done;
#        for name in "cuckoo"; 
#            do ./main.$name -t$i -s 16777216 -z 1048576 -l80 -i10 -k 2 >> out_bit_table_${name}_h.out;
#        done;
#done;


#corss init_size
for i in 1 2 3 4 5 6 7 8 ;
do 
                    ./main.tsx -t$i -s 4096  -l98 -i2 >> out_tsx_4096_98_2_0.out;
                    ./main.tsx -t$i -s 4096  -l97 -i3 >> out_tsx_4096_97_3_0.out;
                    ./main.tsx -t$i -s 4096  -l96 -i4 >> out_tsx_4096_96_4_0.out;
                    ./main.tsx -t$i -s 4096  -l95 -i5 >> out_tsx_4096_95_5_0.out;
done;

#!/bin/bash

cd ../
##init hash size
#for s in 2048 4096 8192 
#do 
#   for i in 1 2 3 6 12 24; 
#    do
#        for name in "global_lock" "striped" "refinable" "split_ordered"; 
#            do ./main.$name -t$i -s $s -l80 -i10 -z $((${s} * 8)) >> out_${name}_init_size${s}.out;
#        done;
#    done;
#done;
#
#
##preinsetion  size
#for _times in 4 8 16 32 ;
#do 
#    for i in 1 2 3 6 12 24; 
#    do
#        for name in "global_lock" "striped" "refinable" "split_ordered"; 
#            do ./main.$name -t$i -s 4096 -z $((4096 * ${_times})) -l80 -i10 >> out_${name}_init_size4096_preinsertion_$((4096 * ${_times})).out;
#        done;
#    done;
#done;
#
#
##corss init_size
#for s in 2048 4096 8192 16384 32768 ;
#do 
#        for name in "global_lock" "striped" "refinable" "split_ordered"; 
#            do ./main.$name -t24 -s $s -z $((${s} * 8)) -l80 -i10 >> out_${name}_cross_init_size.out;
#        done;
#done;
#
##big table
#for i in 1 2 3 6 12 24;
#do
#        for name in "global_lock" "striped" "refinable" "split_ordered" "non_blocking"; 
#            do ./main.$name -t$i -s 16777216 -z 1048576 -l80 -i10 >> out_bit_table_${name}.out;
#        done;
#        for name in "cuckoo"; 
#            do ./main.$name -t$i -s 16777216 -z 1048576 -l80 -i10 -k 4 >> out_bit_table_${name}_h.out;
#        done;
#done;
#
#different workloads
#for s in 4096  
#do 
#   for i in 1 2 3 6 12 24; 
#    do
#        for name in "global_lock" "striped" "refinable" "split_ordered"; 
#            do 
#            ./main.$name -t$i -s $s -l100 -i0 -z $((${s} * 8)) >> out_${name}_init_size${s}_100_0_0.out;
#            ./main.$name -t$i -s $s -l99 -i1 -z $((${s} * 8)) >> out_${name}_init_size${s}_99_1_0.out;
#            ./main.$name -t$i -s $s -l90 -i10 -z $((${s} * 8)) >> out_${name}_init_size${s}_90_10_0.out;
#            ./main.$name -t$i -s $s -l40 -i30 -z $((${s} * 8)) >> out_${name}_init_size${s}_40_30_30.out;
#        done;
#    done;
#done;
#
##corss init_size
##for i in 1 2 3 4 5 6 7 8 ;
##do 
##                    ./main.tsx -t$i -s 4096  -l98 -i2 >> out_tsx_4096_98_2_0.out;
##                    ./main.tsx -t$i -s 4096  -l97 -i3 >> out_tsx_4096_97_3_0.out;
##                    ./main.tsx -t$i -s 4096  -l96 -i4 >> out_tsx_4096_96_4_0.out;
##                    ./main.tsx -t$i -s 4096  -l95 -i5 >> out_tsx_4096_95_5_0.out;
##done;



######## striped#######
for locks in 1 8 16 64 128 256 1024 4096
do
    for i in 0 1 3 7 
    do 
            MT_CONF=$(seq -s, 0 $i) ./main.striped -l70 -i20 -s 4096 -p0 -x $locks>>var_striped_locks_no_resize.out
    done

     MT_CONF=0,1,2,3,4,5,6,7,32,33,34,35,36,37,38,39   ./main.striped -l70 -i20 -s 4096 -p0 -x$locks >>var_striped_locks_no_resize.out 
     MT_CONF=0,1,2,3,4,5,6,7,32,33,34,35,36,37,38,39,8,9,10,11,12,13,14,15,40,41,42,43,44,45,46,47   ./main.striped -l70 -i20 -s 4096 -p0 -x $locks>>var_striped_locks_no_resize.out 
     MT_CONF=0,1,2,3,4,5,6,7,32,33,34,35,36,37,38,39,8,9,10,11,12,13,14,15,40,41,42,43,44,45,46,47,16,17,18,19,20,21,22,23,48,49,50,51,52,53,54,55,24,25,26,27,28,29,30,31,56,57,58,59,60,61,62,63   ./main.striped -l70 -i20 -s 4096 -p0 -x$locks >>var_striped_locks_no_resize.out 
    MT_CONF=0,1,2,3,4,5,6,7,32,33,34,35,36,37,38,39,8,9,10,11,12,13,14,15,40,41,42,43,44,45,46,47,16,17,18,19,20,21,22,23,48,49,50,51,52,53,54,55,24,25,26,27,28,29,30,31,56,57,58,59,60,61,62,63,0,1,2,3,4,5,6,7,32,33,34,35,36,37,38,39,8,9,10,11,12,13,14,15,40,41,42,43,44,45,46,47,16,17,18,19,20,21,22,23,48,49,50,51,52,53,54,55,24,25,26,27,28,29,30,31,56,57,58,59,60,61,62,63  ./main.striped -l70 -i20 -s 4096 -p0 -x $locks>>var_striped_locks_no_resize.out 

done

##### refinable #######
for i in 0 1 3 7 
do 
        MT_CONF=$(seq -s, 0 $i) ./main.refinable -l70 -i20 -s 4096 -p0 >>var_refinable_no_resize.out
done
MT_CONF=0,1,2,3,4,5,6,7,32,33,34,35,36,37,38,39   ./main.refinable -l70 -i20 -s 4096 -p0 >>var_refinable_no_resize.out 
MT_CONF=0,1,2,3,4,5,6,7,32,33,34,35,36,37,38,39,8,9,10,11,12,13,14,15,40,41,42,43,44,45,46,47   ./main.refinable -l70 -i20 -s 4096 -p0 >>var_refinable_no_resize.out 
MT_CONF=0,1,2,3,4,5,6,7,32,33,34,35,36,37,38,39,8,9,10,11,12,13,14,15,40,41,42,43,44,45,46,47,16,17,18,19,20,21,22,23,48,49,50,51,52,53,54,55,24,25,26,27,28,29,30,31,56,57,58,59,60,61,62,63   ./main.refinable -l70 -i20 -s 4096 -p0 >>var_refinable_no_resize.out 
MT_CONF=0,1,2,3,4,5,6,7,32,33,34,35,36,37,38,39,8,9,10,11,12,13,14,15,40,41,42,43,44,45,46,47,16,17,18,19,20,21,22,23,48,49,50,51,52,53,54,55,24,25,26,27,28,29,30,31,56,57,58,59,60,61,62,63,0,1,2,3,4,5,6,7,32,33,34,35,36,37,38,39,8,9,10,11,12,13,14,15,40,41,42,43,44,45,46,47,16,17,18,19,20,21,22,23,48,49,50,51,52,53,54,55,24,25,26,27,28,29,30,31,56,57,58,59,60,61,62,63  ./main.refinable -l70 -i20 -s 4096 -p0 >>var_refinable_no_resize.out 

#!/bin/sh

make

#echo ./cmp $1-$2/$3-$4.in1 $1-$2/$3-$4.in2 $1 $2 $5 $6
#./cmp $1-$2/$3-$4.in1 $1-$2/$3-$4.in2 $1 $2 $5 $6
#echo return value is: $?
#./old_cmp 1 $1-$2/$3-$4.in1 $1-$2/$3-$4.in2


echo ../cmp $1/$2-$3.in1 $1/$2-$3.in2 $4 $5 $6 $7
../cmp $1/$2-$3.in1 $1/$2-$3.in2 $4 $5 $6 $7
echo return value is: $?
./old_cmp 1 $1/$2-$3.in1 $1/$2-$3.in2

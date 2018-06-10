#!/bin/bash

#TODO check for -o opt in $@ if found take next and use that as file out name
mkdir /tmp/stl2gif
./stlviewer $@
for f in /tmp/stl2gif/*.bmp; 
do 
    convert $f $f.jpg;
    rm $f;
    #echo $f.jpg;
done;
convert -delay 10 -loop 0 /tmp/stl2gif/*.jpg stl.gif
#cleanup
rm -R /tmp/stl2gif/


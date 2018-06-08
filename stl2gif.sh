#!/bin/bash

mkdir /tmp/stl2gif
./stlviewer $1
for f in /tmp/stl2gif/*.bmp; 
do 
    convert $f $f.jpg;
    rm $f;
    #echo $f.jpg;
done;
convert -delay 10 -loop 0 /tmp/stl2gif/*.jpg stl.gif
#cleanup
rm -R /tmp/stl2gif/


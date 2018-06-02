#ifndef BITMAP
#define BITMAP

#include <stdio.h>


void generateBitmapImage(unsigned char *image, int height, int width, char* imageFileName);

unsigned char* createBitmapFileHeader(int height, int width);

unsigned char* createBitmapInfoHeader(int height, int width);


#endif

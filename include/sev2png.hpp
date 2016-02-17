#ifndef _IMG2CPC_H_
#define _IMG2CPC_H_

#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <istream>
#include <FreeImage.h>
#include <cstdio>
#include "ezOptionParser.hpp"
#include "FileUtils.hpp"

using namespace std;
using namespace ez;

int initializeParser(ezOptionParser &parser);
void showUsage(ezOptionParser &options);

int initializeImageLoader();
int processImage(const string& filename);

void floodFill(FIBITMAP *bmp, int pX, int pY);

int main(int argc, const char** argv);

#endif
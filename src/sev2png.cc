
#include "sev2png.hpp"

int initializeParser(ezOptionParser &parser) {
	parser.overview = "sev2png - (c) 2016 Retroworks.";
	parser.syntax = "sev2png fileNames";
	parser.example = "sev2png *.sev\n";
	parser.footer = "If you liked this program, drop an email at: augusto.ruiz@gmail.com\n";

	parser.add("", 0, 0, 0, "Help. Show usage.", "--help");

	return 0;
}

void showUsage(ezOptionParser &options) {
	string usage;
	options.getUsage(usage);
	cout << usage << endl;
}

int initializeImageLoader() {
	FreeImage_Initialise();
	return 0;
}

int processImage(const string& filename) {
	int retCode = 0;
	ifstream file(filename, ios::binary | ios::ate);
	if(file.good()) {
		streampos size = file.tellg();
		cout << "Size of file: " << size << endl;
	    char* imgBuf = new char [size];
	    file.seekg (0, ios::beg);
	    file.read (imgBuf, size);
	    file.close();

		if(imgBuf[0]=='S' && imgBuf[1]=='e' && imgBuf[2] == 'v' && imgBuf[3]=='\0') {
			cout << "The file '" << filename << "' has a SevenuP header." << endl;
			cout << "Version: " << ((int)imgBuf[4]) << "." << ((int)imgBuf[5]) << endl;

			int properties = ((int)imgBuf[7]) * 256 + ((int)imgBuf[6]);
			int frameCount = ((int)imgBuf[9]) * 256 + ((int)imgBuf[8]) + 1;
			int sizeX = ((int)imgBuf[11]) * 256 + ((int)imgBuf[10]);
			int sizeY = ((int)imgBuf[13]) * 256 + ((int)imgBuf[12]);
			int bytesPerFrame = (sizeX / 8) * (sizeY / 8) * 9;

			cout << "Number of frames: "  << frameCount << ", properties: " << properties << ", X: " << sizeX << ", Y: " << sizeY << ", bytes per frame: " << bytesPerFrame << endl;

			FIBITMAP *bmp = FreeImage_Allocate(sizeX * frameCount, sizeY, 32);

			int posX=0;
			int posY=0;
			int bufPos=14;
			RGBQUAD opaquePixel = {255, 255, 255, 255};
			RGBQUAD transparentPixel = {0, 0, 0, 255};
			for(int iFrame=0;iFrame<frameCount;++iFrame) {
				for(int y=0; y<(sizeY/8); ++y) {
					posY = 8*y;
					for(int x=0; x<(sizeX/8); ++x) {
						posX = (iFrame*sizeX) + (8*x);
						for(int z=0; z<8; ++z) {
							unsigned char pixels = imgBuf[bufPos];
							FreeImage_SetPixelColor(bmp, posX + 0, posY + z, (pixels & 0x80) == 0x80? &opaquePixel : &transparentPixel);
							FreeImage_SetPixelColor(bmp, posX + 1, posY + z, (pixels & 0x40) == 0x40? &opaquePixel : &transparentPixel);
							FreeImage_SetPixelColor(bmp, posX + 2, posY + z, (pixels & 0x20) == 0x20? &opaquePixel : &transparentPixel);
							FreeImage_SetPixelColor(bmp, posX + 3, posY + z, (pixels & 0x10) == 0x10? &opaquePixel : &transparentPixel);
							FreeImage_SetPixelColor(bmp, posX + 4, posY + z, (pixels & 0x08) == 0x08? &opaquePixel : &transparentPixel);
							FreeImage_SetPixelColor(bmp, posX + 5, posY + z, (pixels & 0x04) == 0x04? &opaquePixel : &transparentPixel);
							FreeImage_SetPixelColor(bmp, posX + 6, posY + z, (pixels & 0x02) == 0x02? &opaquePixel : &transparentPixel);
							FreeImage_SetPixelColor(bmp, posX + 7, posY + z, (pixels & 0x01) == 0x01? &opaquePixel : &transparentPixel);
							bufPos++;
						}
						bufPos++; // Avanzamos el byte de atributo.
					}
				}
			}

			string outFilename = FileUtils::RemoveExtension(FileUtils::GetFileName(filename)) + ".png";
			FreeImage_FlipVertical(bmp);
			cout << "Saving png file: '" << outFilename << "'." << endl;
			FreeImage_Save(FIF_PNG, bmp, outFilename.c_str());

			cout << "Calculating mask for: 'colored_" << outFilename << "'." << endl;
			int imgSizeX = frameCount*sizeX; 
			int maxX = imgSizeX - 1;
			int maxY = sizeY - 1;
			for(int i=0;i<imgSizeX;++i) {
				floodFill(bmp, i, 0);
				floodFill(bmp, i, maxY);
			}
			for(int i=0;i<sizeY;++i) {
				floodFill(bmp, 0, i);
				floodFill(bmp, maxX, i);
			}
			FreeImage_Save(FIF_PNG, bmp, ("colored_" + outFilename).c_str());

			cout << "Scaling: 'cpc_" << outFilename << "'." << endl;
			FIBITMAP *cpcBmp = FreeImage_Rescale(bmp, (sizeX * frameCount) / 2, sizeY, FILTER_BOX);
			FreeImage_Save(FIF_PNG, cpcBmp, ("cpc_" + outFilename).c_str());
			FreeImage_Unload(cpcBmp);
			FreeImage_Unload(bmp);
			cout << "Done: '" << filename << "'." << endl;

			retCode = 0;
		}
		else {
			cout << "The file '" << filename << "' is not a SevenuP file." << endl;
			retCode = -1;			
		}
		delete imgBuf;
		return retCode;
	}
	cout << "Couldn't open file " << filename << "'." << endl;
	return -2;
}

void floodFill(FIBITMAP *bmp, int pX, int pY) {
	RGBQUAD pixColor;
	RGBQUAD magenta = {255,0,255,255};

	int width = FreeImage_GetWidth(bmp);
	int height = FreeImage_GetHeight(bmp);

	if(pX>=0 && pY>=0 && pX<width && pY<height) {
		FreeImage_GetPixelColor(bmp, pX, pY, &pixColor);
		if(pixColor.rgbRed == 0 && pixColor.rgbGreen == 0 && pixColor.rgbBlue == 0) {
			int fill=1;
			for(int dX=-1; dX<2; ++dX) {
				for(int dY=-1; dY<2; ++dY) {
					int posX=pX+dX;
					int posY=pY+dY;
					if( posX >=0 && posY >= 0 && posX < width && posY < height) {
						FreeImage_GetPixelColor(bmp, posX, posY, &pixColor);
						if(pixColor.rgbGreen == 255) {
							fill=0;
							break;
						}
					}
				}
			}
			if(fill) {
				FreeImage_SetPixelColor(bmp, pX, pY, &magenta);
				floodFill(bmp, pX+1, pY);
				floodFill(bmp, pX, pY+1);
				floodFill(bmp, pX-1, pY);
				floodFill(bmp, pX, pY-1);
			}			
		}
	}
}

int main(int argc, const char** argv)
{
	if (initializeImageLoader()) {
		return -1;
	}
	ezOptionParser options;
	if (initializeParser(options)) {
		return -1;
	}

	options.parse(argc, argv);

	if (options.isSet("--help")) {
		showUsage(options);
		return 0;
	}

	vector<string *> &lastArgs = options.lastArgs;

	for (long int i = 0, li = lastArgs.size(); i < li; ++i) {
		string filename = *lastArgs[i];
		int result = processImage(filename);
		if (result) {
			cout << "Error processing image: " << filename << endl;
			return result;
		}
	}

	return 0;
}
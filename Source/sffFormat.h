#pragma once

//Specifications for SFF v1 format: https://www.allegro.cc/forums/thread/342381/342381#target
//Ikemen code that parses SFF files: https://osdn.net/users/supersuehiro/pf/ikemen/scm/blobs/master/ssz/sff.ssz (written in a proprietary language)
//Specifications for SFF v2 format: https://mugenarchive.com/w/index.php?title=MUGEN:SFFv2&mobileaction=toggle_view_desktop
//Alternate source for SFF v2 format specifications: https://mugenguild.com/forum/topics/sffv2-format-information-106218.0.html
//Forum resource about Mugen technicalities: https://mugenguild.com/forum/topics/the-mugen-docs-master-thread-168949.0.html
//Small wiki with Mugen technical information: https://mugen-net.work/wiki/index.php/Main_Page
//Official documentation: http://www.elecbyte.com/mugendocs-11b1/mugen.html
//Older official documentation: http://www.elecbyte.com/mugendocs/mugen.html

#define PALETTESIZE 768
#define PCX_RLE 192 //Two most-significant bits in a bytes
#define PCX_TRANSPARENTCOLOURINDEX 0

#pragma pack(push, 1)
struct sffHeader_v1_s //Should be 512 bytes
{
	char signature[12]; //"ElecbyteSpr\0"
	char versionMinor3;
	char versionMinor2;
	char versionMinor1;
	char versionMajor; //1
	unsigned int groupNum;
	unsigned int imageNum;
	unsigned int startOffset; //256
	unsigned int subHeaderSize;
	char paletteType; //1=SPRPALTYPE_SHARED or 0=SPRPALTYPE_INDIV
	char blank1;
	char blank2;
	char blank3;
	char comment[476];
};

struct sffSubHeader_v1_s //Should be 32 bytes
{
	unsigned int offset; //File offset where next subfile in the "linked list" is located. Null if last subfile
	unsigned int size; //Size of subfile (not counting header) (length is 0 it uses the image data from another frame)
	short axisX;
	short axisY;
	unsigned short groupIndex;
	unsigned short imageIndex;
	unsigned short useImgDataOfIndex; //If not zero, this uses the image data of another frame
	unsigned char paletteIsSameAsLast; //If true, palette is same as previous image
	unsigned char comment[13];
	//PCX file data
	//If palette is available then 768 bytes of palette data is here (is it at the end PCX file?)
};

struct sffHeader_v2_s
{
	char signature[12]; //"ElecbyteSpr\0"
	char versionMinor3;
	char versionMinor2;
	char versionMinor1;
	char versionMajor; //2
};

struct pcxHeader_s
{
	char magic; //0x0A
	char type;
	char compression;
	char bitsPerPlane;
	short minXCoord;
	short minYCoord;
	short maxXCoord;
	short maxYCoord;
	short horResDPI;
	short verResDPI;
	char palette16bit[48];
	char reserved;
	char colorPlanes;
	short stride;
	short paletteType;
	short authorHorRes;
	short authorVerRes;
	char padding[54];
};

struct tgaHeader_s
{
	char IDLength;
	char ColormapType;
	char ImageType;
	short colourmaporigin;
	short colourmaplength;
	char  colourmapdepth;
	short XOrigin;
	short YOrigin;
	short ImageWidth;
	short ImageHeight;
	char PixelDepth;
	char ImageDescriptor;
};

#pragma pack(pop)
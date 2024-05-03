#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <stdlib.h>
#include <string.h>
#include "SFFExtract.h"
#include "sffFormat.h"
#include "misc.h"
#include "lodepng/lodepng.h"

//These are values related to saving image data using a specific canvas size (TODO: We might want to turn some of these into menu options)
static bool convertToCanvas = 0;
static bool offsetToFitIntoCanvas = 1;
static int canvasSizeX = 128;
static int canvasSizeY = 128;
static int canvasAnchorX = canvasSizeX / 2;
static int canvasAnchorY = canvasSizeY - 4;

void F_ExtractSFF(char *mugenFilePath, char *paletteFilePath) //Convert SFF into multiple images (gets converted into PCXs if it's version 1)
{
	unsigned char *palFileData = 0;

	if(mugenFilePath[0] == 0)
	{
		printf("Warning: There's no path for a Mugen character defined.");
		return;
	}

	//Get extension
	char *ext = GetExtension(mugenFilePath, MAX_PATH);

	//Check extension
	if(ext == 0 || _stricmp(ext, "sff") != 0)
	{
		printf("Warning: Expected SFF extension with %s", mugenFilePath);
		return;
	}

	//Load SFF
	unsigned char *fileData = 0;
	unsigned int fileDataSize = 0;
	if(!LoadFile(mugenFilePath, &fileData, &fileDataSize))
	{
		printf("Warning: Failed to open %s for reading.", mugenFilePath);
		return;
	}

	if(fileDataSize < sizeof(sffHeader_v1_s))
	{
		printf("Warning: File size is too small for %s.", mugenFilePath);
		goto finish;
	}

	//Header
	sffHeader_v1_s *header = (sffHeader_v1_s *) fileData;

	//Check magic
	if(strcmp(header->signature, "ElecbyteSpr") != 0)
	{
		printf("Warning: Magic is wrong in %s.", mugenFilePath);
		goto finish;
	}

	//Check version
	if(header->versionMajor != 1) //TODO: Add support for version 2
	{
		printf("Warning: Expected version to be 1 in %s.", mugenFilePath);
		goto finish;
	}

	//Get versions of the path that's without filename and one that's filename without extension
	char pathWithoutFilename[MAX_PATH] = {0};
	char filenameWithoutExt[MAX_PATH] = {0};
	{
		int lastSlash = -1;
		int lastDot = -1;
		int pos = 0;
		while(1)
		{
			if(mugenFilePath[pos] == 0 || pos >= MAX_PATH - 1)
				break;
			if(mugenFilePath[pos] == '.' && mugenFilePath[pos + 1] != 0)
				lastDot = pos;
			if((mugenFilePath[pos] == '\\' || mugenFilePath[pos] == '/') && mugenFilePath[pos + 1] != 0)
				lastSlash = pos;
			pos++;
		}

		if(lastSlash != -1)
		{
			memcpy(pathWithoutFilename, mugenFilePath, lastSlash);
			pathWithoutFilename[lastSlash] = 0;
		}
		if(lastDot != -1)
		{
			lastSlash++; //Skip the actual slash (and if this was -1, it's now 0, which makes it valid)
			memcpy(filenameWithoutExt, &mugenFilePath[lastSlash], lastDot - lastSlash);
			filenameWithoutExt[lastDot - lastSlash] = 0;
		}
		else
		{
			printf("Warning: Failed to acquire filename variants.");
			return;
		}
	}

	//Handle palette
	unsigned int palFileDataSize = 0;
	if(paletteFilePath && paletteFilePath[0] != 0)
	{
		char *palExt = GetExtension(paletteFilePath, MAX_PATH);
		if(palExt == 0 || _stricmp(palExt, "act") != 0) //TODO: We should add PAL support as well
		{
			printf("Warning: Expected ACT extension with %s", paletteFilePath);
			goto finish;
		}
		if(!LoadFile(paletteFilePath, &palFileData, &palFileDataSize))
		{
			printf("Warning: Failed to open %s for reading.", paletteFilePath);
			goto finish;
		}
		if(palFileDataSize < PALETTESIZE)
		{
			printf("Warning: File size is too small for %s.", paletteFilePath);
			goto finish;
		}

		//The order of pixels in the palette is reversed in the ACT compared to what the PCX data expects, so we fix it here
		unsigned char *newPal = new unsigned char[PALETTESIZE];
		for(int i = 0; i < 256; i++)
		{
			newPal[(i * 3) + 0] = palFileData[((255 - i) * 3) + 0];
			newPal[(i * 3) + 1] = palFileData[((255 - i) * 3) + 1];
			newPal[(i * 3) + 2] = palFileData[((255 - i) * 3) + 2];
		}
		delete[]palFileData;
		palFileData = newPal;
	}

	//Extract PCXs
	{
		unsigned long long currentOffset = sizeof(sffHeader_v1_s);
		unsigned long long previousPaletteOffset = 0;
		for(unsigned int i = 0; i < header->imageNum; i++)
		{
			sffSubHeader_v1_s *subHeader = (sffSubHeader_v1_s *) &fileData[currentOffset];
			currentOffset += sizeof(sffSubHeader_v1_s);
			if(subHeader->paletteIsSameAsLast == 0)
				previousPaletteOffset = (currentOffset + subHeader->size) - PALETTESIZE;
			FILE *file = 0;
			char pcxPath[MAX_PATH];
			sprintf_s(pcxPath, MAX_PATH, "%s\\out\\%s%04i.pcx", pathWithoutFilename, filenameWithoutExt, i); //Base filename on index within SFF
			//sprintf_s(pcxPath, MAX_PATH, "%s-g%05i-i%05i.pcx", pathWithoutFilename, subHeader->groupIndex, subHeader->imageIndex); //Based filename on group and image indices
			MakeDirectory_PathEndsWithFile(pcxPath);
			if(subHeader->size == 0) //If size is 0, then we use the image data from another frame
			{
				char pcxPathFrom[MAX_PATH];
				if(subHeader->useImgDataOfIndex >= i)
					printf("Warning: Trying to copy file that hasn't been created yet!");
				sprintf_s(pcxPathFrom, MAX_PATH, "%s\\out\\%s%04i.pcx", pathWithoutFilename, filenameWithoutExt, subHeader->useImgDataOfIndex);
				CopyFile(pcxPathFrom, pcxPath, 0);
			}
			else //Frame contains image data, so write it
			{
				fopen_s(&file, pcxPath, "wb");
				if(!file)
				{
					printf("Warning: Failed to open %s for writing.", pcxPath);
				}
				else
				{
					fwrite(&fileData[currentOffset], subHeader->size, 1, file);
					if(subHeader->paletteIsSameAsLast == 1)
					{
						if(previousPaletteOffset == 0 && palFileData == 0)
							printf("Warning: We have no palette to write for %s.", pcxPath);
						else
						{
							//PCX files expect a 0x0C byte just before the palette, but this should already exist for all the frames inside the SFF
							if(palFileData)
								fwrite(palFileData, PALETTESIZE, 1, file);
							else if(previousPaletteOffset > 0)
								fwrite(&fileData[previousPaletteOffset], PALETTESIZE, 1, file);
						}
					}
					fclose(file);
					printf("Wrote %s", pcxPath);
				}
			}
			currentOffset += subHeader->size;
		}
	}

	//Extract PCXs as TGAs or PNGs
#define CONVERTTOPNG
	{
		//TODO: Make it possible to put sprites into the middle of a canvas by reading in the offsets in subHeaders?

		unsigned int currentOffset = sizeof(sffHeader_v1_s);
		unsigned int previousPaletteOffset = 0;
		for(unsigned int i = 0; i < header->imageNum; i++)
		{
			sffSubHeader_v1_s *subHeader = (sffSubHeader_v1_s *) &fileData[currentOffset];
			currentOffset += sizeof(sffSubHeader_v1_s);
			if(subHeader->paletteIsSameAsLast == 0)
				previousPaletteOffset = (currentOffset + subHeader->size) - PALETTESIZE;
			FILE *file = 0;
			char tgaPath[MAX_PATH];
			
#ifdef CONVERTTOPNG
			char *extensionName = "png";
#else
			char *extensionName = "tga";
#endif
			sprintf_s(tgaPath, MAX_PATH, "%s\\out\\%s%04i.%s", pathWithoutFilename, filenameWithoutExt, i, extensionName); //Base filename on index within SFF
			//sprintf_s(tgaPath, MAX_PATH, "%s-g%05i-i%05i.png", pathWithoutFilename, subHeader->groupIndex, subHeader->imageIndex); //Base filename on group and image indices
			MakeDirectory_PathEndsWithFile(tgaPath);

			if(subHeader->size == 0) //If size is 0, then we use the image data from another frame
			{
				char pcxPathFrom[MAX_PATH];
				if(subHeader->useImgDataOfIndex >= i)
					printf("Warning: Trying to copy file that hasn't been created yet!");
				sprintf_s(pcxPathFrom, MAX_PATH, "%s\\out\\%s%04i.%s", pathWithoutFilename, filenameWithoutExt, subHeader->useImgDataOfIndex, extensionName);
				CopyFile(pcxPathFrom, tgaPath, 0);
			}
			else if(previousPaletteOffset == 0 && palFileData == 0)
			{
				printf("Warning: No palette to use for image %s", tgaPath);
			}
			else //Frame contains image data
			{
				fopen_s(&file, tgaPath, "wb");
				if(!file)
				{
					printf("Warning: Failed to open %s for writing.", tgaPath);
				}
				else
				{
					pcxHeader_s *pcxHeader = (pcxHeader_s *) &fileData[currentOffset];
					int resX = (pcxHeader->maxXCoord - pcxHeader->minXCoord) + 1, resY = (pcxHeader->maxYCoord - pcxHeader->minYCoord) + 1;

					//The final resolution might be overridden by a canvas size definition
					int finalSizeX = resX;
					int finalSizeY = resY;
					if(convertToCanvas && canvasSizeX != 0 && canvasSizeY != 0)
					{
						finalSizeX = canvasSizeX;
						finalSizeY = canvasSizeY;
					}

#ifndef CONVERTTOPNG
					//TGA header
					tgaHeader_s tgaHeader;
					memset(&tgaHeader, 0, sizeof(tgaHeader));
					tgaHeader.IDLength = 0;
					tgaHeader.ImageType = 2;
					tgaHeader.ImageWidth = finalSizeX;
					tgaHeader.ImageHeight = finalSizeY;
					tgaHeader.PixelDepth = 32;
					tgaHeader.ImageDescriptor = 0x28;
#endif

					//Convert PCX image data to uncompressed image data
					unsigned int imgDataSize = resX * resY * 4;
					unsigned char *imgData = new unsigned char[imgDataSize];
					//memset(imgData, 255, imgDataSize);
					{
						unsigned int pcxPos = currentOffset + sizeof(pcxHeader_s), tgaPos = 0;
						int stridePos = 0;
						unsigned char rleByte = 0, colourIndex = 0, length = 0, colourR = 0, colourG = 0, colourB = 0;
						bool skipPixel = 0;
						while(tgaPos < imgDataSize)
						{
							rleByte = fileData[pcxPos]; pcxPos++;
							if(rleByte & PCX_RLE && (rleByte - PCX_RLE) > 0) //Two most-significant bits are on, so this is the runlength of the following pixel index
							{
								length = rleByte - PCX_RLE;
								colourIndex = fileData[pcxPos]; pcxPos++;
							}
							else //rleByte contains colour index for one pixel
							{
								length = 1;
								colourIndex = rleByte;
							}

							if(length)
							{
								if(palFileData)
								{
									colourR = palFileData[(colourIndex * 3) + 2];
									colourG = palFileData[(colourIndex * 3) + 1];
									colourB = palFileData[(colourIndex * 3) + 0];
								}
								else if(previousPaletteOffset)
								{
									colourR = fileData[previousPaletteOffset + (colourIndex * 3) + 2];
									colourG = fileData[previousPaletteOffset + (colourIndex * 3) + 1];
									colourB = fileData[previousPaletteOffset + (colourIndex * 3) + 0];
								}
								for(int j = 0; j < length; j++)
								{
									if(!skipPixel)
									{
#ifdef CONVERTTOPNG
										imgData[tgaPos + 0] = colourB;
										imgData[tgaPos + 1] = colourG;
										imgData[tgaPos + 2] = colourR;
#else
										imgData[tgaPos + 0] = colourR;
										imgData[tgaPos + 1] = colourG;
										imgData[tgaPos + 2] = colourB;
#endif
										imgData[tgaPos + 3] = colourIndex == PCX_TRANSPARENTCOLOURINDEX ? 0 : 255;
										tgaPos += 4;
										stridePos++;
										if(tgaPos >= imgDataSize)
											break;
									}
									else
										skipPixel = 0;

									if(stridePos >= resX)
									{
										if(pcxHeader->maxXCoord + 1 != pcxHeader->stride) //Some PCX files have padding (one byte per stride) to ensure stride size is of even size
											skipPixel = 1;
										stridePos = 0;
									}
								}
							}
						}
					}

					//Potentially move image data into a new canvas
					if(finalSizeX != resX || finalSizeY != resY)
					{
						unsigned int newImgDataSize = finalSizeX * finalSizeY * 4;
						unsigned char *newImgData = new unsigned char[newImgDataSize];

						//Make the entire image data black and transparent by default
						memset(newImgData, 0, newImgDataSize);

						//Determine overall offset when moving the image to the new canvas. We go for an offset that centers the sprite nicely
						int offsetX = canvasAnchorX - subHeader->axisX;
						int offsetY = canvasAnchorY - subHeader->axisY;

						//If "offsetToFitIntoCanvas" is true, then we'll consider offsetting the image if it prevents it from going "out of bounds"
						if(offsetToFitIntoCanvas)
						{
							bool outOfBoundsLeft = (offsetX < 0);
							bool outOfBoundsRight = (offsetX + resX >= finalSizeX);
							bool outOfBoundsUp = (offsetY < 0);
							bool outOfBoundsDown = (offsetY + resY >= finalSizeY);
							if(outOfBoundsLeft && !outOfBoundsRight) offsetX = 0; //Align with left edge
							else if(outOfBoundsRight && !outOfBoundsLeft) offsetX = finalSizeX - resX; //Align with right edge
							if(outOfBoundsUp && !outOfBoundsDown) offsetY = 0; //Align with top edge
							else if(outOfBoundsDown && !outOfBoundsUp) offsetY = finalSizeY - resY; //Align with bottom edge
						}

						//Move the image data over
						for(int x = 0; x < resX; x++)
						{
							for(int y = 0; y < resY; y++)
							{
								//Calcalate target location
								int newX = x + offsetX;
								int newY = y + offsetY;

								//Out of bounds check
								if(newX < 0 || newX >= finalSizeX || newY < 0 || newY >= finalSizeY)
									continue;

								//Calcalate position within image data
								int oldImageDataPos = (x * 4) + (y * resX * 4);
								int newImageDataPos = (newX * 4) + (newY * finalSizeX * 4);

								//Another out of bounds check for sanity's sake
								if(newImageDataPos < 0 || newImageDataPos + 3 >= (int) newImgDataSize)
									continue;

								//Write pixel to new image data
								newImgData[newImageDataPos + 0] = imgData[oldImageDataPos + 0];
								newImgData[newImageDataPos + 1] = imgData[oldImageDataPos + 1];
								newImgData[newImageDataPos + 2] = imgData[oldImageDataPos + 2];
								newImgData[newImageDataPos + 3] = imgData[oldImageDataPos + 3];
							}
						}

						//Finish by swapping the image data pointers
						delete[]imgData;
						imgData = newImgData;
					}

#ifdef CONVERTTOPNG
					//Encode into PNG
					unsigned char *pngData;
					size_t pngDataSize;
					unsigned int error;
					/*if(channels == 3)
						error = lodepng_encode24(&pngData, &pngDataSize, imgData, finalSizeX, finalSizeY);
					else if(channels == 4)*/
						error = lodepng_encode32(&pngData, &pngDataSize, imgData, finalSizeX, finalSizeY);
					if(error)
					{
						printf("Lodepng error %u: %s\n", error, lodepng_error_text(error));
						fclose(file);
						delete[]imgData;
						break;
					}

					fwrite(pngData, pngDataSize, 1, file);
					free(pngData);
#else
					//Write TGA header and image data
					fwrite(&tgaHeader, sizeof(tgaHeader_s), 1, file);
					fwrite(imgData, tgaHeader.ImageWidth * tgaHeader.ImageHeight * 4, 1, file);
#endif
					delete[]imgData;
					fclose(file);
					printf("Wrote %s", tgaPath);
				}
			}
			currentOffset += subHeader->size;

			//Also save information about frame to a text file
			sprintf_s(tgaPath, MAX_PATH, "%s\\out\\%s%04i.txt", pathWithoutFilename, filenameWithoutExt, i);
			fopen_s(&file, tgaPath, "wb");
			if(file)
			{
				fprintf(file, "next entry offset = %u\n", subHeader->offset);
				fprintf(file, "image data size = %u\n", subHeader->size);
				fprintf(file, "frame pos offset = %ix%i\n", subHeader->axisX, subHeader->axisY);
				fprintf(file, "group index = %u\n", subHeader->groupIndex);
				fprintf(file, "image index = %u\n", subHeader->imageIndex);
				fprintf(file, "use other image data index = %u\n", subHeader->useImgDataOfIndex);
				fprintf(file, "palette is same as last = %s\n", subHeader->paletteIsSameAsLast ? "yes" : "no");
				fprintf(file, "comment = %s\n", subHeader->comment);
				fclose(file);
				printf("Wrote %s", tgaPath);
			}
			else
				printf("Failed to open %s for writing.", tgaPath);
		}
	}

	//Finish
finish:
	if(fileData)
		delete[]fileData;
	if(palFileData)
		delete[]palFileData;
}
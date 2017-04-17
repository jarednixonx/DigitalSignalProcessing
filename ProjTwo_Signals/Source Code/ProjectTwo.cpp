#include <iostream>
#include <sstream>
#include <fstream>														// Used only to write the summary text file.
#include <time.h>														// Used to measure the performance of the program.

#include "H_44100.h"
#include "H_22050.h"

using namespace std;

signed short* TempVarLeft_44 = new signed short[BL_44100];
signed short* TempVarRight_44 = new signed short[BL_44100];

signed short* TempVarLeft_22 = new signed short[BL_22050];
signed short* TempVarRight_22 = new signed short[BL_22050];

signed short* LeftOutArray = new signed short[1];
signed short* RightOutArray = new signed short[1];

bool filter = true;														// Filter for 44100 selected by default (checks later in program for real sampling frequency.)

// Define structure for the header.

struct headerFile
{
	char chunkID[4];													// Holds the RIFF descriptor.
	unsigned long chunkSize;											// Size of the entire file minus 8 bytes;
	char waveID[4];														// Holds the WAVE descriptor.
	char formatID[4];													// Holds the FORMAT descriptor.
	unsigned long subChunkOneSize;										// Size of sub chunk one.
	unsigned short audioFormat;											// PCM = 1 (unless compression present).
	unsigned short numberChannels;										// Mono = 1, Stereo = 2.
	unsigned long samplesPerSecond;										// Sample rate.
	unsigned long bytesPerSecond;										// = SamplesRate * NumChannels * BitsPerSamples/8.
	unsigned short blockAlign;											// = NumChannels * BitsPerSamples/8.
	unsigned short bitsPerSample;										// Number of bits per sample.
	char subChunkTwoID[4];												// Holds the DATA descriptor.
	unsigned long  subChunkTwoSize;										// = NumSamples * NumChannels * BitsPerSample/8.

																		// Actual data begins here but that is to be processed separately, outside of header.
};
void filterInit(signed short* wipeThisArray, int length);

void filterFIR(signed short* leftSamples, signed short* rightSamples, signed short* outLeft, signed short* outRight, bool filter);

void shiftArray(signed short* leftSamples, signed short* rightSamples, bool filter);

// Beginning of the main function.

int main()
{
	clock_t clockTimeStart = clock();									// Starts the clock for performance measurement.

	// Only 20 k and 44 k Hz sampling frequencies are supported.

	filterInit(TempVarLeft_44, BL_44100);								// Initialize Left Channel (44 k Hz)
	filterInit(TempVarRight_44, BL_44100);								// Initialize Right Channel(44 k Hz)
	filterInit(TempVarLeft_22, BL_22050);								// Initialize Left Channel (20 k Hz)
	filterInit(TempVarRight_22, BL_22050);								// Initialize Right Channel (20 k Hz)
	filterInit(LeftOutArray, 1);										// Initialize Output Array (Left)
	filterInit(RightOutArray, 1);										// Initialize Output Array (Right)

	FILE* fileIn;

	FILE* fileOut;

	ofstream summary;													// File stream to write diretly to the summary text file.
	summary.open("summary.txt");										// Opens the summary file.

	fileIn = fopen("Nixon_J_Mod.wav", "rb");							// Opens the file to read from.

	fileOut = fopen("Nixon_J_Lp.wav", "wb");						// Opens the output file that is being written to.

	headerFile header;

	fseek(fileIn, SEEK_SET, 0);											// Goes to the beginning of the file for safe measure.

	fread(&header, sizeof(header), 1, fileIn);							// Reads the entire header structure.

	fwrite(&header, sizeof(header), 1, fileOut);						// Writes the entire header structure to output file.

	int readSamples = 0;												// Counter to keep track of samples read.

	if (header.samplesPerSecond == 44100)
	{
		filter = true;													// True if 44 k Hz. (Bool decides which arrays to use)

		cout << "Auto selected a filter for a sampling frequency of 44100 Hz." << endl << endl;
	}
	else if(header.samplesPerSecond == 22050)
	{
		filter = false;													// False if 20 k Hz. (Bool decides which arrays to use)

		cout << "Auto selected a filter for a sampling frequency of 22050 Hz." << endl << endl;
	}
	else                                                                // Unsupported if neither of the above.
	{
		cout << "Cannot find a filter for your sampling rate because it is unsupported. Try a file with a sampling rate of 22050 or 44100 Hz." << endl << endl;
		return 0;														// End program because sampling rate is unsupported. (Error printed to user)
	}

	// These values must be signed. (8 bit is unsigned in wav file and 16 bit is signed in wav file)

	signed short bufferRightCurrent = 0;								// Right Current Sample Buffer
	signed short bufferLeftCurrent = 0;									// Left Current Sample Buffer

	fread(&bufferLeftCurrent, sizeof(short), 1, fileIn);				// Grabs first sample (left channel).
	fread(&bufferRightCurrent, sizeof(short), 1, fileIn);				// Grabs first sample (right channel).

	while (!feof(fileIn))												// Enter loop to process data samples.
	{
		if (filter == true)												// True if using 44100 Hz.
		{
			TempVarLeft_44[0] = bufferLeftCurrent;
			TempVarRight_44[0] = bufferRightCurrent;
		}
		else if (filter == false)										// False if using 22050 Hz.
		{
			TempVarLeft_22[0] = bufferLeftCurrent;
			TempVarRight_22[0] = bufferRightCurrent;
		}

		readSamples++;													// Increments number of samples processed.	

		if (filter == true)												// True if using 44100 Hz, use appropriate filter.
		{
			filterFIR(TempVarLeft_44, TempVarRight_44, LeftOutArray, RightOutArray, filter);	// Filter samples with 44100 Hz arrays.
		}
		else if (filter == false)										// False if using 22050 Hz, use appropriate filter.
		{
			filterFIR(TempVarLeft_22, TempVarRight_22, LeftOutArray, RightOutArray ,filter);	// Filter samples with 22050 Hz arrays.
		}

		fwrite(&LeftOutArray[0], sizeof(short), 1, fileOut);
		fwrite(&RightOutArray[0], sizeof(short), 1, fileOut);

		if (filter == true)												// True if using 44100 Hz, use appropriate shift.
		{
			shiftArray(TempVarLeft_44, TempVarRight_44, filter);		// Shift Array with 44100 Hz arrays.
		}
		else if (filter == false)										// False if using 22050 Hz, use appropriate shift.
		{
			shiftArray(TempVarLeft_22, TempVarRight_22, filter);		// Shift array with 22050 Hz arrays.
		}

		fread(&bufferLeftCurrent, sizeof(short), 1, fileIn);			// Grabs next sample(left channel).
		fread(&bufferRightCurrent, sizeof(short), 1, fileIn);			// Grabs next sample(right channel).
	}

	printf("Processing time: %.2fs\n", (double)(clock() - clockTimeStart) / CLOCKS_PER_SEC);		// Easiest way to print out the processing time.

	cout << endl;		
																									
	/* ********************************************* Beginning some basic calculations. ********************************************* */

	unsigned long numSamples = (header.subChunkTwoSize / ((header.bitsPerSample / 8) * header.numberChannels));

	// Duration of the file will be Samples / Frequency (in samples per second) so that the samples cancel and seconds flips and comes to the numerator.

	unsigned long waveDuration = ((numSamples * 4) + (header.subChunkOneSize + 16)) / (header.bytesPerSecond);																		// bytes / bytes per second = seconds
	
	// Need to write summary text file here.

	summary << "CPE381 project - Processing WAV File - Jared Nixon" << endl << endl;																								// Summary printing.

	summary << "The sampling frequency of this WAV file is " << header.samplesPerSecond << " Hz." << endl;																			// Summary printing.	

	summary << "The length of this file in seconds (calculated from the number of samples) is " << waveDuration << " seconds." << endl;												// Summary printing.

	summary << "This program completed the processing of the WAV file in " << (double)(((double)clock() - (double)clockTimeStart) / (double)CLOCKS_PER_SEC) << " seconds." << endl;	// Summary printing.

	fclose(fileIn);														// Closes file stream.
	fclose(fileOut);													// Closes file stream.
	summary.close();													// Closes file stream.

	system("PAUSE");

	return 0;															// End of the program.
}

void shiftArray(signed short *leftSamples, signed short *rightSamples, bool filter)
{
	// Shift for TRUE, which is 44100 Hz.

	if (filter == true)										
	{
		for (int i = (BL_44100 - 2); i >= 0; i--)
		{
			leftSamples[i + 1] = leftSamples[i];
		}
		for (int i = (BL_44100 - 2); i >= 0; i--)
		{
			rightSamples[i + 1] = rightSamples[i];
		}

		return;
	}

	// Shift for FALSE, which is 22050 Hz.

	else if (filter == false)								
	{
		for (int i = (BL_22050 - 2); i >= 0; i--)
		{
			leftSamples[i + 1] = leftSamples[i];
		}
		for (int i = (BL_22050 - 2); i >= 0; i--)
		{
			rightSamples[i + 1] = rightSamples[i];
		}

		return;
	}
}

void filterFIR(signed short *leftSamples, signed short *rightSamples, signed short *outLeft, signed short *outRight, bool filter)
{
	double tempLeft = 0;
	double tempRight = 0;

	// Filter for TRUE, which is 44100 Hz.

	if (filter == true)												
	{
		for (int i = 0; i < BL_44100; i++)
		{
			tempLeft += leftSamples[i] * B_44100[i];
		}

		for (int i = 0; i < BL_44100; i++)
		{
			tempRight += rightSamples[i] * B_44100[i];
		} 

		/* Going to check for overflow here just to be safe after computations*/

		if (tempLeft > 32767)
		{
			tempLeft = 32767;
		}
		if (tempLeft < -32768)
		{
			tempLeft = -32768;
		}
		if (tempRight < -32768)
		{
			tempRight = -32768;
		}
		if (tempRight < -32768)
		{
			tempRight = -32768;
		}

		outLeft[0] = (signed short)tempLeft;				// Write output for 44100 Hz if that is selected frequency. (left)
		outRight[0] = (signed short)tempRight;				// Write output for 44100 Hz if that is selected frequency. (right)

		return;
	}		

	// Filter for FALSE, which is 22050 Hz.

	else if (filter == false)											
	{
		for (int i = 0; i < BL_22050; i++)
		{
			tempLeft += leftSamples[i] * (B_22050[i]);
		}

		for (int i = 0; i < BL_22050; i++)
		{
			tempRight += rightSamples[i] * (B_22050[i]);
		}

		/* Going to check for overflow here just to be safe after computations*/

		if (tempLeft > 32767)
		{
			tempLeft = 32767;
		}
		if (tempLeft < -32768)
		{
			tempLeft = -32768;
		}
		if (tempRight < -32768)
		{
			tempRight = -32768;
		}
		if (tempRight < -32768)
		{
			tempRight = -32768;
		}

		outLeft[0] = (signed short)tempLeft;				// Write output for 22050 Hz if that is selected frequency. (left)
		outRight[0] = (signed short)tempRight;				// Write output for 22050 Hz if that is selected frequency. (right)

		return;
	}	
}

void filterInit(signed short *wipeThisArray, int length)					// Wipes array to all 0's.
{
	register int count;

	for (count = 0; count < length; count++)
	{
		wipeThisArray[count] = 0;
	}
}
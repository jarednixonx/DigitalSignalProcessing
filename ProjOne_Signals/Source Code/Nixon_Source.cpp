#include <iostream>
#include <sstream>
#include <fstream>														// Used only to write the summary text file.
#include <time.h>														// Used to measure the performance of the program.

using namespace std;

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


// Beginning of the main function.

int main()
{
	clock_t clockTimeStart = clock();									// Starts the clock for performance measurement.

	const double PI = 3.141592653589793;								// Go ahead and define PI for sine wave.

	FILE* fileIn;													

	FILE* fileOut;

	ofstream summary;													// File stream to write diretly to the summary text file.
	summary.open("summary.txt");										// Opens the summary file.

	fileIn = fopen("nixon.wav", "rb");									// Opens the file to read from.
	 
	fileOut = fopen("modified.wav", "wb");								// Opens the output file that is being written to.
	
	headerFile header;

	fseek(fileIn, SEEK_SET, 0);											// Goes to the beginning of the file for safe measure.

	fread(&header, sizeof(header), 1, fileIn);							// Reads the entire header structure.

	fwrite(&header, sizeof(header), 1, fileOut);						// Writes the entire header structure to output file.

	int readSamples = 0;												// Counter to keep track of samples read.

	// These values must be signed. (8 bit is unsigned in wav file and 16 bit is signed in wav file)

	signed short bufferRightCurrent;									// Right Current Sample Buffer
	signed short bufferLeftCurrent;										// Left Current Sample Buffer
	signed short bufferRightPrevious = 0;								// To keep track of previous values.
	signed short bufferLeftPrevious = 0;								// To keep track of previous values.
    signed short bufferRightOut;										// Output Buffer
	signed short bufferLeftOut;											// Output Buffer

	signed short TempVarLeft;											// Going to store the left channel average.
	signed short TempVarRight;											// Going to store the right channel average.

	int offset = 44;

	fseek(fileIn, offset, 0);											// 44 bytes for the header offset.

	fread(&bufferLeftCurrent, sizeof(short), 1, fileIn);				// Grabs first sample (left channel).
	fread(&bufferRightCurrent, sizeof(short), 1, fileIn);				// Grabs first sample (right channel).

	while (!feof(fileIn))												// Enter loop to process data samples.
	{
		offset = offset + 4;											// Moving on to next sample.

		TempVarLeft = bufferLeftPrevious + bufferLeftCurrent;			// This is to calculate the average.
		TempVarRight = bufferRightPrevious + bufferRightCurrent;		// This is to calculate the average.

		bufferLeftPrevious = bufferLeftCurrent;							// To keep track of previous samples.
		bufferRightPrevious = bufferRightCurrent;						// To keep track of previous samples.

		bufferLeftOut = (TempVarLeft);									// Shifting does not help here because it cuts my WAV file in half.
		bufferRightOut = (TempVarRight);								// Shifting does not help here because it cuts my WAV file in half.

		// Amplitude of 8000 because 1/4 of 0 to 32000.

		double x = 8000 * sin(2 * PI * 2500 * ((double)readSamples * (1 / (double)header.samplesPerSecond)));		// A * sin(2 * pi * f * t) where t = n * T -> sample counter * 1/Fs.

		bufferLeftOut = bufferLeftOut + (short)x;						// Add sine wave to left channel.
		bufferRightOut = bufferRightOut + (short)x;						// Add sine wave to right channel.

		if (bufferLeftOut > 32767)										// This accounts for overflow after sine wave is added.
		{
			bufferLeftOut = 32766;										// Save max value for current sample is max value is exceeded.
		}

		if (bufferRightOut > 32767)										// This accounts for overflow after sine wave is added.
		{
			bufferLeftOut = 32766;										// Save max value for current sample is max value is exceeded.
		}

		fwrite(&bufferLeftOut, sizeof(short), 1, fileOut);				// Writes noisy signal.
		fwrite(&bufferRightOut, sizeof(short), 1, fileOut);				// Writes noisy signal.

		readSamples++;													// Increments number of samples processed.	

		fseek(fileIn, offset, 0);										// Keeps the file on track to read the next sequential data sample.

		fread(&bufferLeftCurrent, sizeof(short), 1, fileIn);			// Grabs next sample(left channel).
		fread(&bufferRightCurrent, sizeof(short), 1, fileIn);			// Grabs next sample(right channel).
	}

	printf("Processing time: %.2fs\n", (double)(clock() - clockTimeStart) / CLOCKS_PER_SEC);		// Easiest way to print out the processing time.

	cout << endl;

	unsigned long actualSubChunkTwo = ((header.chunkSize + 8) - 44);								// Wave file says subchunk2 is like 600 million so had to manually calculate this value to use for samples.

	/* ********************************************* Beginning some basic calculations. ********************************************* */

	// Samples in the file in seconds will be -> (subChunkTwoSize / (BytesPerSample * NumChannels)).

	unsigned long numSamples = (actualSubChunkTwo / ((header.bitsPerSample / 8) * header.numberChannels));

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
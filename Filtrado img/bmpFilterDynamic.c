#include "bmpBlackWhite.h"
#include "mpi.h"

/** Enable output for filtering information */
#define DEBUG_FILTERING 0

/** Show information of input and output bitmap headers */
#define SHOW_BMP_HEADERS 1

/** Master process */
#define MASTER 0
#define END_OF_PROCESSING -1


int main(int argc, char** argv){

	tBitmapFileHeader imgFileHeaderInput;			/** BMP file header for input image */
	tBitmapInfoHeader imgInfoHeaderInput;			/** BMP info header for input image */
	tBitmapFileHeader imgFileHeaderOutput;			/** BMP file header for output image */
	tBitmapInfoHeader imgInfoHeaderOutput;			/** BMP info header for output image */
	char* sourceFileName;							/** Name of input image file */
	char* destinationFileName;						/** Name of output image file */
	int inputFile, outputFile;						/** File descriptors */
	unsigned char *outputBuffer;					/** Output buffer for filtered pixels */
	unsigned char *inputBuffer;						/** Input buffer to allocate original pixels */
	unsigned int rowSize;							/** Number of pixels per row */
	unsigned int threshold;							/** Threshold */
	unsigned int currentRow;						/** Current row being processed */
	unsigned int currentPixel;						/** Current pixel being processed */
	unsigned int readBytes;							/** Number of bytes read from input file */
	unsigned int writeBytes;						/** Number of bytes written to output file */
	unsigned int numPixels;							/** Number of neighbour pixels (including current pixel) */
	tPixelVector vector;							/** Vector of neighbour pixels */
	struct timeval tvBegin, tvEnd;					/** Structs to calculate the total processing time */
    
    
    int rank, numProc;        /** Rank and number of processes */
    int tag, flag = 0;                    /** Tag */
    MPI_Status status;            /** MPI status for receiving messages */
    int initTam, endRemainder;        /** Portion of the matrix to be processed by a process */
    int i, j, k;                /** Aux variables */
    double timeStart, timeEnd;    /** Time stamps to calculate the filtering time */
    unsigned char *ptrInputBuf;
    unsigned char *ptrOutputBuffer;
    int rowRecived;
    int totalRecived = 0;
    int numRow;
    int rowRemainder;
    int totalRequested = 0;
    int *vectorAux;
    int grano;
    
    //init MPI
    MPI_Init (&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &numProc);
    
    tag = 0;
    
    //proceso master
    if(rank == MASTER){
        // Check arguments
        if (argc != 5){
            printf ("Usage: ./bmpFilter sourceFile destinationFile threshold\n");
            exit (0);
        }

		// Get input arguments...
		sourceFileName = argv[1];
		destinationFileName = argv[2];
		threshold = atoi(argv[3]);
        
        grano = atoi(argv[4]);

		// Init seed
		srand(time(NULL));

		// Show info before processing
		printf ("Applying filter to image %s with threshold %d. Generating image %s\n", sourceFileName, threshold, destinationFileName);

		// Filter process begin
		gettimeofday(&tvBegin, NULL);

		// Read headers from input file
		readHeaders (sourceFileName, &imgFileHeaderInput, &imgInfoHeaderInput);
		readHeaders (sourceFileName, &imgFileHeaderOutput, &imgInfoHeaderOutput);

		// Write header to the output file
		writeHeaders (destinationFileName, &imgFileHeaderOutput, &imgInfoHeaderOutput);

		// Calculate row size for input and output images
		rowSize = (((imgInfoHeaderInput.biBitCount * imgInfoHeaderInput.biWidth) + 31) / 32 ) * 4;

		// Show headers...
		if (SHOW_BMP_HEADERS){
			printf ("Source BMP headers:\n");
			printBitmapHeaders (&imgFileHeaderInput, &imgInfoHeaderInput);
			printf ("Destination BMP headers:\n");
			printBitmapHeaders (&imgFileHeaderOutput, &imgInfoHeaderOutput);
		}

		// Open source image
		if((inputFile = open(sourceFileName, O_RDONLY)) < 0){
			printf("ERROR: Source file cannot be opened: %s\n", sourceFileName);
			exit(1);
		}

		// Open target image
		if((outputFile = open(destinationFileName, O_WRONLY | O_APPEND, 0777)) < 0){
			printf("ERROR: Target file cannot be open to append data: %s\n", destinationFileName);
			exit(1);
		}

		// Allocate memory to copy the bytes between the header and the image data
		outputBuffer = (unsigned char*) malloc ((imgFileHeaderInput.bfOffBits-BIMAP_HEADERS_SIZE) * sizeof(unsigned char));

		// Copy bytes between headers and pixels
		lseek (inputFile, BIMAP_HEADERS_SIZE, SEEK_SET);
		read (inputFile, outputBuffer, imgFileHeaderInput.bfOffBits-BIMAP_HEADERS_SIZE);
		write (outputFile, outputBuffer, imgFileHeaderInput.bfOffBits-BIMAP_HEADERS_SIZE);
        
        // Allocate memory for input and output buffers
        inputBuffer = (unsigned char *) malloc (rowSize * imgInfoHeaderInput.biHeight * sizeof (unsigned char));
        outputBuffer = (unsigned char*) malloc (rowSize * imgInfoHeaderInput.biHeight * sizeof(unsigned char));
        //vector aux para guardar las filas enviadas , indice+1 -> proceso
        vectorAux = (int *) malloc (sizeof(int) * (numProc-1));
        
        if ((readBytes = read (inputFile, inputBuffer, rowSize * imgInfoHeaderInput.biHeight)) != rowSize * imgInfoHeaderInput.biHeight){
            showError ("Error while reading from source file");
        }
        
        //repartimos los tamaños iniciales y el final ( puede ser cero o resto) menos 1 por el master
        int filaEnviada = 0;
        numRow = grano;
        
        
        MPI_Bcast (&rowSize, 1 , MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast (&threshold, 1, MPI_INT, 0, MPI_COMM_WORLD);
        
        ptrInputBuf = inputBuffer; // puntero auxiliar tamAux = 0
        
        for ( i = 1; i < numProc; i++) {
            MPI_Send(&numRow, 1 , MPI_INT, i, flag, MPI_COMM_WORLD);
            MPI_Send(ptrInputBuf, numRow * rowSize , MPI_CHAR, i, flag, MPI_COMM_WORLD);
            vectorAux[i-1] = filaEnviada;
            
            filaEnviada += numRow ;
            ptrInputBuf += numRow * rowSize;
            totalRequested += numRow;
        }
        ptrOutputBuffer = outputBuffer;
        while ( totalRecived < imgInfoHeaderInput.biHeight){
            MPI_Recv(&rowRecived,1, MPI_INT, MPI_ANY_SOURCE, flag,MPI_COMM_WORLD, &status);
            ptrOutputBuffer = outputBuffer + vectorAux[status.MPI_SOURCE-1] * rowSize;
            
            MPI_Recv(ptrOutputBuffer, rowRecived * rowSize, MPI_CHAR, status.MPI_SOURCE, flag,MPI_COMM_WORLD, &status);
            
            totalRecived += rowRecived;
            
            if( totalRequested < imgInfoHeaderInput.biHeight){
                numRow = (imgInfoHeaderInput.biHeight - totalRequested ) > grano? grano:(imgInfoHeaderInput.biHeight - totalRequested) ;
                MPI_Send(&numRow, 1 , MPI_INT, status.MPI_SOURCE, flag, MPI_COMM_WORLD);
                
                MPI_Send(ptrInputBuf, numRow * rowSize , MPI_CHAR, status.MPI_SOURCE, flag, MPI_COMM_WORLD);
                
                vectorAux[status.MPI_SOURCE -1] = filaEnviada;
                filaEnviada += numRow ;
                ptrInputBuf += numRow * rowSize;
                totalRequested += numRow;
            }
            else {
                numRow = END_OF_PROCESSING;
                MPI_Send(&numRow, 1 , MPI_INT, status.MPI_SOURCE, flag, MPI_COMM_WORLD);
            }
        
        }

        // Write to output file
        if ((writeBytes = write (outputFile, outputBuffer, rowSize*imgInfoHeaderInput.biHeight)) != rowSize*imgInfoHeaderInput.biHeight){
            showError ("Error while writing to destination file");
        }
        
        // Close files
        close (inputFile);
        close (outputFile);
        
        // End of processing
        gettimeofday(&tvEnd, NULL);
        
        printf("Filtering time: %ld.%06ld\n", ((tvEnd.tv_usec + 1000000 * tvEnd.tv_sec) - (tvBegin.tv_usec + 1000000 * tvBegin.tv_sec)) / 1000000,
               ((tvEnd.tv_usec + 1000000 * tvEnd.tv_sec) - (tvBegin.tv_usec + 1000000 * tvBegin.tv_sec)) % 1000000);
        
    }//fin master
    // Workers
    else{
        
        MPI_Bcast (&rowSize, 1 , MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast (&threshold, 1, MPI_INT, 0, MPI_COMM_WORLD);
        
        do {
            MPI_Recv (&numRow, 1, MPI_INT, 0,tag,  MPI_COMM_WORLD, &status);
            
            if(numRow != END_OF_PROCESSING){
                // Allocate memory for input and output buffers
                inputBuffer = (unsigned char *) malloc (rowSize * sizeof (unsigned char)*numRow);
                outputBuffer = (unsigned char*) malloc (rowSize * sizeof (unsigned char)*numRow);
                
                MPI_Recv(inputBuffer, rowSize * numRow, MPI_CHAR, 0, tag, MPI_COMM_WORLD, &status);

                for(currentRow = 0; currentRow < numRow; ++currentRow){
                    for (int currentPixel = 0; currentPixel < rowSize; ++currentPixel) {
                        // Current pixel
                        numPixels = 0;
                        vector[numPixels] = inputBuffer[(currentRow * rowSize) + currentPixel];
                        numPixels++;
                        
                        // Not the first pixel
                        if (currentPixel > 0){
                            vector[numPixels] = inputBuffer[((currentRow * rowSize) + currentPixel)-1];
                            numPixels++;
                        }
                        // Not the last pixel
                        if (currentPixel < rowSize - 1){
                            vector[numPixels] = inputBuffer[((currentRow * rowSize) + currentPixel)+1];
                            numPixels++;
                        }
                        
                        // Store current pixel value
                        outputBuffer[(currentRow * rowSize) + currentPixel] = calculatePixelValue(vector, numPixels, threshold, DEBUG_FILTERING);
                    }
                }
                
                MPI_Send(&numRow, 1 , MPI_INT, MASTER, flag, MPI_COMM_WORLD);
                
                MPI_Send(outputBuffer, rowSize * numRow, MPI_CHAR, MASTER, flag, MPI_COMM_WORLD);
            }
        } while (numRow != END_OF_PROCESSING);

    }
   
}


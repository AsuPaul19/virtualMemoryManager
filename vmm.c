/****************************************************************
 * Name        :  Paul Asu                                      *
 * Class       :  CSC 415                                       *
 * Date        :  08/08/2020                                    *
 * Description :  Writting a simple shell program               *
 *                that will execute simple commands. The main   *
 *                goal of the assignment is working with        *
 *                fork, pipes and exec system calls.            *
 ****************************************************************/


#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <alloca.h>

// function headers
void getPage(int logicaladdress);
int backingStore(int pageNum);
void TLBInsert(int pageNum, int frameNum);

// define filetypes for files
FILE    *addresses;
FILE    *backing_store;

#define BUFF_SIZE 10     
#define ADDRESS_MASK 0x0000FFFF   
#define OFFSET_MASK 0x000000FF            // buffer size for reading a line from addresses file
// #define ADDRESS_MASK 0xFFFF            // for masking all of logical_address except the address
// #define OFFSET_MASK 0xFF               // for masking the offset
#define TLB_SIZE 16                    // 16 entries in the TLB
#define PAGE_TABLE_SIZE 128            // page table of size 2^7
#define PAGE 256                       // upon page fault, read in 256-byte page from BACKING_STORE
#define FRAME_SIZE 256                 // size of each frame


int TLBEntries = 0;                    // current number of TLB entries
int hits = 0;                          // counter for TLB hits
int faults = 0;                        // counter for page faults
int currentPage = 0;                   // current number of pages
int logical_address;                   // int to store logical address
int TLBpages[TLB_SIZE];                // array to hold page numbers in TLB
bool pagesRef[PAGE_TABLE_SIZE];        // array to hold reference bits for page numbers in TLB
int pageTableNumbers[PAGE_TABLE_SIZE]; // array to hold page numbers in page table
char currentAddress[BUFF_SIZE];        // array to store addresses
signed char fromBackingStore[PAGE];    // holds reads from BACKING_STORE
signed char byte;                      // holds value of physical memory at frame number/offset
int physicalMemory[PAGE_TABLE_SIZE][FRAME_SIZE];          // physical memory array of 32,678 bytes (128 frames x 256-byte frame size)


// function to take the logical address and obtain the physical address and byte stored at that address
void getPage(int logical_address){
    
    // initialize frameNum to -1, sentinel value
    int frameNum = -1;
    
    // mask leftmost 16 bits, then shift right 8 bits to extract page number 
    int pageNum = ((logical_address & ADDRESS_MASK)>>8);

    // offset is just the rightmost bits
    int offset = (logical_address & OFFSET_MASK);
    
    // look through TLB
    int i; 
    for(i = 0; i < TLB_SIZE; i++)
    {
      // if TLB hit
      if(TLBpages[i] == pageNum)
      {   
        // extract frame number
        frameNum = i; 

	// increase number of hits
        hits++;                
      } // end if
    } // end for
    
    // if the frame number was not found in the TLB
    if(frameNum == -1)
    {
      int i;   
      for(i = 0; i < currentPage; i++)
      {
        // if page number found in page table, extract it
        if(pageTableNumbers[i] == pageNum)
	{         
          frameNum = i; 

	  // change reference bit
          pagesRef[i] = true;
        } // end if
      } // end for

      // if frame number is still -1, pageNum has not been found in TLB or page table 
      if(frameNum == -1)
      {                    
        // read from BACKING_STORE.bin
        int count = backingStore(pageNum);       

	// increase the number of page faults
        faults++;                       

	// change frame number to first available frame number
        frameNum = count; 
      } // end if
    } // end if
    
    // insert page number and frame number into TLB
    TLBInsert(pageNum, frameNum); 


    // assign the value of the signed char to byte
    byte = physicalMemory[frameNum][offset]; 
    

    // output the virtual address, physical address and byte of the signed char to the console
    printf("Virtual address: %d Physical address: %d Value: %d\n", logical_address, (frameNum << 8) | offset, byte);
} // end getPage


// function to read from backing store
int backingStore(int pageNum)
{

  int counter = 0;

  // position to read from pageNum
  // SEEK_SET reads from beginning of file
  if (fseek(backing_store, pageNum * PAGE, SEEK_SET) != 0) 
  {
    fprintf(stderr, "Error seeking in backing store\n");
  } // end if
 
 
  if (fread(fromBackingStore, sizeof(signed char), PAGE, backing_store) == 0)
  {
    fprintf(stderr, "Error reading from backing store\n");
  } // end if
 
  // boolean for while loop
  bool search = true;
  
  // second chance algorithm
  while(search)
  {
    if(currentPage == PAGE_TABLE_SIZE)
    {
      currentPage = 0;
    } // end if
 
    // if reference bit is 0
    if(pagesRef[currentPage] == false)
    {
      // replace page
      pageTableNumbers[currentPage] = pageNum;
 
      // set search to false to end loop
      search = false;
    } // end if
    // else if reference bit is 1
    else
    {
      // set reference bit to 0
      pagesRef[currentPage] = false;
    } // end else
    currentPage++;
  } // end while
  // load contents into physical memory
  int i;
  for(i = 0; i < PAGE; i++)
  {
    physicalMemory[currentPage-1][i] = fromBackingStore[i];

  } // end for
  counter = currentPage-1;

  return counter;
} // end backingStore

// insert page into TLB
void TLBInsert(int pageNum, int frameNum){
    
    int i;  // search for entry in TLB
    for(i = 0; i < TLBEntries; i++)
    {
        if(TLBpages[i] == pageNum)
	{
            break; // break if entry found
        } // end if
    } // end for
    
    // if the number of entries is equal to the index
    if(i == TLBEntries)
    {
        // if TLB is not full
        if(TLBEntries < TLB_SIZE)
	{   
	    // insert page with FIFO replacement
            TLBpages[TLBEntries] = pageNum;   
        } // end if
	// else, TLB is full
        else
	{  
	    // shift everything over
            for(i = 0; i < TLB_SIZE - 1; i++)
	    {
                TLBpages[i] = TLBpages[i + 1];
            } // end for

	    //FIFO replacement
            TLBpages[TLBEntries-1] = pageNum;
        } // end else        
    } // end if
    
    // if the number of entries is not equal to the index
    else
    {
        // move everything over up to the number of entries - 1
        for(i = i; i < TLBEntries - 1; i++)
	{     
            TLBpages[i] = TLBpages[i + 1];     
        } // end for
        
	// if still room in TLB
        if(TLBEntries < TLB_SIZE)
	{               
	    // insert page at the end
            TLBpages[TLBEntries] = pageNum;
        } // end if
	// else if TLB is full
        else
	{ 
	    // place page at number of entries - 1
            TLBpages[TLBEntries-1] = pageNum;
        } // end else
    } // end else

    // if TLB is still not full, increment the number of entries
    if(TLBEntries < TLB_SIZE)
    {                  
        TLBEntries++;
    } // end if    
} // end TLBInsert


// main opens necessary files and calls on getPage for every entry in the addresses file
int main(int argc, char *argv[])
{
    // error checking for arguments
    if (argc != 2) 
    {
        fprintf(stderr,"Usage: ./a.out [input file]\n");
        return -1;
    } // end if
    
    // set pagesRef array to 0
    for(int i = 0; i < 128; i++)
    {
      pagesRef[i] = 0;
    } // end for

    // open backing store file
    backing_store = fopen("BACKING_STORE.bin", "rb");
    
    // error checking for opening file
    if (backing_store == NULL) 
    {
        fprintf(stderr, "Error opening BACKING_STORE.bin %s\n","BACKING_STORE.bin");
        return -1;
    } // end if
    
    // open virtual addresses file
    addresses = fopen(argv[1], "r");
    
    // error checking for opening file
    if (addresses == NULL) 
    {
        fprintf(stderr, "Error opening addresses.txt %s\n",argv[1]);
        return -1;
    } // end if
    
    // define number of translated addresses
    int numberOfTranslatedAddresses = 0;


    // read through the input file and output each logical address
    while ( fgets(currentAddress, BUFF_SIZE, addresses) != NULL)
    {
        logical_address = atoi(currentAddress);
        
        // get the physical address and byte stored at that address
        getPage(logical_address);
        numberOfTranslatedAddresses++;  // increment the number of translated addresses        
    } // end while

    // calculate and print out the stats
    double pfRate = faults / (double)numberOfTranslatedAddresses;
    double TLBRate = hits / (double)numberOfTranslatedAddresses;
    
    printf("Page Faults = %d\n", faults);
    printf("Page Fault Rate = %.3f\n",pfRate);
    printf("TLB Hits = %d\n", hits);
    printf("TLB Hit Rate = %.3f\n", TLBRate);
    
    
    // close files
    fclose(addresses);
    fclose(backing_store);
    
    return 0;
} // end main



// ************************************* //

// #include <stdio.h>
// #include <stdlib.h>
// #include <sys/types.h>
// #include <string.h>
// #include <errno.h>

// //Bit mask for masking out lower 8 bits (0-7)
// #define MASK_FRAME_NUMER 0x000000FF
// //Bit mask for masking out bits 8-15
// #define MASK_PAGE_NUMBER 0x0000FF00
// //macro to extract bits 8-15 from a number, then shift right by 8 bits
// #define get_page_num(num) ((num & MASK_PAGE_NUMBER) >> 8)
// //macro to extract bits 0-15 from a number
// #define get_frame_offset(num) (num & MASK_FRAME_NUMER)

// #define PAGETABLESIZE 256
// #define PAGESIZE 256
// #define FRAMECOUNT 256
// #define FRAMESIZE 256

// #define TLBSIZE 16

// struct PageTableEntry
// {
//     int page_number;
//     int frame_number;
// };
// // array of PageTableEntry to make up page table.
// struct PageTableEntry page_table[PAGETABLESIZE];

// struct Frame
// {
//     signed char data[FRAMESIZE];
//     int last_accessed;
// };
// //                             Frame # --> Frame data
// struct Frame physical_memory[FRAMECOUNT];

// struct TLBEntry
// {
//     int page_number;
//     int frame_number;
//     int last_accessed;
// };
// struct TLBEntry tlb[TLBSIZE];

// /**
//  * THese two variables can be used to make the next available frame 
//  * or page table entry.
//  */ 
// int availableFrameIndex = 0;
// int availablePageTableIndex = 0;


// /**
//  * takes 2 commands line args
//  * 1 --> addresses.txt
//  * 2 --> BACKING_STORE.bin
//  * */
// int main(int argc, char *argv[])
// {

//     if (argc != 3)
//     {
//         printf("Error Invalid Arguments!!!\nCorrect Usage:\n\t./vmm addressfile backstorefile\n\n");
//         exit(-1);
//     }

//     //address translation variables
//     unsigned int logical_address;
//     int frame_offset; 
//     int page_number;
//     //data read from "physical memory", must be signed type
//     signed char data_read;
    
//     char *address_text_filename;
//     char *backing_store_filename;
//     //statistic variables
//     int page_fault = 0;
//     int tlb_hits = 0;
//     int num_addresses_translated = 0;
    
//     address_text_filename = argv[1];
//     backing_store_filename = argv[2];

//     //init physical memory
//     for (int i = 0; i < FRAMECOUNT; i++)
//     {
//         memset(physical_memory[i].data, 0, FRAMESIZE);
//         physical_memory[i].last_accessed = -1;
//     }

//     //init Page Table
//     for (int i = 0; i < PAGETABLESIZE; i++)
//     {
//         page_table[i].page_number = -1;
//         page_table[i].frame_number = -1;
//     }

//     //init tlb
//     for (int i = 0; i < TLBSIZE; i++)
//     {
//         tlb[i].page_number = -1;
//         tlb[i].frame_number = -1;
//         tlb[i].last_accessed = -1;
//     }

//     FILE *addresses_to_translate = fopen(address_text_filename, "r");
//     if (addresses_to_translate == NULL)
//     {
//         perror("could not open address file");
//         exit(-2);
//     }

//     FILE *backing_store_file = fopen(backing_store_filename, "rb");
//     if (backing_store_file == NULL)
//     {
//         perror("could not open backingstore");
//         exit(-2);
//     }

//     while (fscanf(addresses_to_translate, "%d\n", &logical_address) == 1)
//     {
//         num_addresses_translated++;
//         page_number = get_page_num(logical_address);
//         frame_offset = get_frame_offset(logical_address);

//         printf("Virtual Address %5d, Page Number: %3d, Fame Offset: %3d\n", logical_address, page_number, frame_offset);

//         //printf("Virtual address: %d Physical address: %d Value: %d\n", logical_address, (physical_address << 8) | frame_offset, data_read);
//     }

//     float hit_rate = (float)tlb_hits / num_addresses_translated;
//     float fault_rate = (float)page_fault / num_addresses_translated;
//     printf("Hit Rate %0.2f\nPage Fault Rate: %0.2f\n", hit_rate, fault_rate);
//     fclose(addresses_to_translate);
//     fclose(backing_store_file);
// }


// ***************************************** //
// *****************************************  //


// #include <stdio.h>
// #include <stdlib.h>
// #include <unistd.h>
// #include <string.h>
// #include <alloca.h>
// #include <math.h>


// // all these caonstants are extracted from the problem statement details

// #define FRAME_SIZE 256          //FRAME SIZE is 256 bytes 2^8
// #define FRAMES 256              //Number of frames
// #define PAGE_SIZE 256           // PAGE SIZE is 256 bytes 2^8
// #define TLB_SIZE 16             //Number of TLB Entries
// #define PAGE_MASKER 0xFFFF      // This masker will be explained later.
// #define OFFSET_MASKER 0xFF      // This masker will be explained later.
// #define ADDRESS_SIZE 10
// #define CHUNK 256

// // input files
// FILE *address_file;
// FILE *backing_store;

// // struct to store TLB and page table
// struct page_frame
// {
//    int page_number;
//    int frame_number;
// };

// // initial global variables
// int Physical_Memory [FRAMES][FRAME_SIZE];
// struct page_frame TLB[TLB_SIZE];
// struct page_frame PAGE_TABLE[FRAMES]; // the size of the page table is the number of the available frames in the system.
// int translated_addresses = 0;
// char address [ADDRESS_SIZE]; // to store the address coming from the input file
// int TLBHits = 0;
// int page_faults = 0;
// signed char buffer[CHUNK];
// int firstAvailableFrame = 0;
// int firstAvailablePageTableIndex = 0;
// signed char value;
// int TLB_Full_Entries = 0;

// // functions placeholders
// void get_page(int logical_address);
// int read_from_store(int pageNumber);
// void insert_into_TLB(int pageNumber, int frameNumber);

// // get a page for a given virtual address
// void get_page(int logical_address){

//     int pageNumber = ((logical_address & PAGE_MASKER)>>8); // this expression will mask the value of the logical address then shifting it 8 bits to the right to retireve the page number. In this case m = 16 and n = 8.
//     int offset = (logical_address & OFFSET_MASKER); // This expression retrives the last 8 bits of the address as the page offset.

//     // first try to get page from TLB
//     int frameNumber = -1; // initialized to -1 to tell if it's valid in the conditionals below

//     int i;  // look through TLB for a match
//     for(i = 0; i < TLB_SIZE; i++){
//         if(TLB[i].page_number == pageNumber){   // if the TLB key is equal to the page number.
//             frameNumber = TLB[i].frame_number;  // then the frame number value is extracted.
//             TLBHits++;                          // and the TLBHit counter is incremented.
            
//         }
//     }

//      // if the frameNumber was not int the TLB:
//     if(frameNumber == -1){
//         int i;                                                       // walk the contents of the page table
//         for(i = 0; i < firstAvailablePageTableIndex; i++){           // The size of the page table is the same number as the frames number.
//             if(PAGE_TABLE[i].page_number == pageNumber){             // if the page is found in those contents
//                 frameNumber = PAGE_TABLE[i].frame_number;            // extract the frameNumber from its twin array
//             }
//         }
        
//         if(frameNumber == -1){                                      // if the page is not found in those contents
//             frameNumber = read_from_store(pageNumber);              // page fault, call to read_from_store to get the frame into physical memory and the page table and set the frameNumber to the current firstAvailableFrame index
//             page_faults++;                                          // increment the number of page faults
//         }
//     }

//     insert_into_TLB(pageNumber, frameNumber);                       // call to function to insert the page number and frame number into the TLB
//     value = Physical_Memory[frameNumber][offset];                   // frame number and offset used to get the signed value stored at that address

//     // for testing before outputing values to the file:

//     //printf("frame number: %d\n", frameNumber);
//     //printf("offset: %d\n", offset); 
//     // output the virtual address, physical address and value of the signed char to the console
//     //printf("Virtual address: %d Physical address: %d Value: %d\n", logical_address, (frameNumber << 8) | offset, value);

//     // output final values to a file named results
//     FILE *res = fopen("results.txt","a");
//     fprintf(res,"Virtual address: %d Physical address: %d Value: %d\n", logical_address, (frameNumber << 8) | offset, value);
//     fclose(res);
// }

// int read_from_store(int pageNumber){

//     // first seek to byte CHUNK in the backing store
//     // SEEK_SET in fseek() seeks from the beginning of the file
//     if (fseek(backing_store, pageNumber * CHUNK, SEEK_SET) != 0) {
//         fprintf(stderr, "Error seeking in backing store\n");
//     }
    
//     // now read CHUNK bytes from the backing store to the buffer
//     if (fread(buffer, sizeof(signed char), CHUNK, backing_store) == 0) {
//         fprintf(stderr, "Error reading from backing store\n");        
//     }
    
//     // load the bits into the first available frame in the physical memory 2D array
//     int i;
//     for(i = 0; i < CHUNK; i++){
//         Physical_Memory[firstAvailableFrame][i] = buffer[i];
//     }
    
//     // and then load the frame number into the page table in the first available frame
//     PAGE_TABLE[firstAvailablePageTableIndex].page_number = pageNumber;
//     PAGE_TABLE[firstAvailablePageTableIndex].frame_number = firstAvailableFrame;
    
//     // increment the counters that track the next available frames
//     firstAvailableFrame++;
//     firstAvailablePageTableIndex++;
//     return PAGE_TABLE[firstAvailablePageTableIndex-1].frame_number;
// }

// void insert_into_TLB(int pageNumber, int frameNumber){

//     /*
//     The algorithm to insert a page into the TLB using FIFO replacement. It goes as follows:
//     - First check if the page is already in the TLB table. If it is in the TLB Table, then:
//         move it to the end of the TLB. "following the FIFO policies."
//     - If it is not in the TLB:
//         If there is a free cell then add it to that free cell at the end of the queue
//         else, move every thing one step to the left to free up the last cell and put the page into this last cell.
//     */
    
//    int i;  
//    // first check if it is found in the TLB. If so, then move every thing one step to the left in insert it in the last cell.
//    // remever that i iterates upto one last cell in the TLB for a maximum TLB_FULL_Entries.
//     for(i = 0; i < TLB_Full_Entries; i++){
//         if(TLB[i].page_number == pageNumber){               // if it is in the TLB, move it to the last cell.
//             for(i = i; i < TLB_Full_Entries - 1; i++)      // iterate from the current cell to one less than the number of entries
//                 TLB[i] = TLB[i + 1];                        // move everything over in the arrays
//             break;
//         }
//     }
    
//     // if the previous loop did not break, then the page is not in the TLB. We need to insert it.
//     if(i == TLB_Full_Entries){
//         int j;
//         for (j=0; j<i; j++)
//             TLB[j] = TLB[j+1];

//     }
//     TLB[i].page_number = pageNumber;        // and insert the page and frame on the end
//     TLB[i].frame_number = frameNumber;

//     if(TLB_Full_Entries < TLB_SIZE-1){     // if there is still room in the arrays, increment the number of entries
//         TLB_Full_Entries++;
//     }    
// }

// int main(int argc, char *argv[]) {
//     printf("Hello, This is the ICS431 Project\n");
//     printf("results are printed to the \"resutls.txt\" file.\n");
//     printf("The command \"diff results.txt correct.txt\" might be usefull to check whether the results of the program and the correct results are identical.\n");

//     // perform basic error checking
//     if (argc != 2) {
//         fprintf(stderr,"Usage: ./a.out [input file]\n");
//         return -1;
//     }

//     // Read Adresses from input file. The file is retrived from the command line arguments
//     address_file = fopen(argv[1], "r");

//     backing_store = fopen("BACKING_STORE.bin", "rb");

//     if (address_file == NULL) {
//         fprintf(stderr, "Error opening addresses.txt %s\n",argv[1]);
//         return -1;
//     }

//     if (backing_store == NULL) {
//         fprintf(stderr, "Error opening BACKING_STORE.bin %s\n","BACKING_STORE.bin");
//         return -1;
//     }
//     int translated_addresses = 0;
//     int logical_address;
//     // read through the input file and output each logical address
//     while (fgets(address, ADDRESS_SIZE, address_file) != NULL) {
//         logical_address = atoi(address); // this function converts numeric strings to int datatype.

//         // get the physical address and value stored at that address
//         get_page(logical_address);
//         translated_addresses++;  // increment the number of translated addresses
//     }

//      // calculate and print out the stats
//     printf("Number of translated addresses = %d\n", translated_addresses);
//     double pfRate = page_faults / (double)translated_addresses;
//     double TLBRate = TLBHits / (double)translated_addresses;
    
//     printf("Page Faults = %d\n", page_faults);
//     printf("Page Fault Rate = %.3f\n",pfRate);
//     printf("TLB Hits = %d\n", TLBHits);
//     printf("TLB Hit Rate = %.3f\n", TLBRate);
    
//     // close the input file and backing store
//     fclose(address_file);
//     fclose(backing_store);

//     return 0;
// }


//  ********************************** //
// ***********************************  //




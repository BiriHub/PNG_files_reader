/*
Systems Programming assignment : Reading Data from Portable Network Graphics Files

Write a C program named "pngq.c" that reads and extracts information from a Portable Network Graphic file (PNG).
PNG format specifications are available on the W3C documentation : https://www.w3.org/TR/2023/CR-png-3-20230921/

Solution by : 
*/


#include <stdio.h>
#include<string.h>

//Types of chunks
#define IHDR 1
#define TEXT 2


/*
    structs and global variables definitions
*/

// chunk PNG file struct definition

struct chunk{
    unsigned int num;           // chunk number

    unsigned int length;        // length of chunk data field
    unsigned char type[4];      // chunk type field , it's values are restricted between 'A'-'Z' and 'a'-'z'
    unsigned char* data;                 //chunk data field , the size depends on the chunk length field
    unsigned int crc;           // chunk checksum field
};

// define the color type enum
enum colorType {Grayscale=0, Truecolour=2, Indexed=3, GrayscaleAlpha=4, TruecolourAlpha=6};

// define the struct for the pformat output, used to store the information about the PNG file in the specified format
struct pform_output{
    unsigned int file_counter; // number of the current PNG file 

    char * _f; // file name
    unsigned int _w; // image width
    unsigned int _h; // image height
    unsigned int _d; // image bit depth
    enum colorType _c; // image color type
    unsigned int _N; //number of chunks of the PNG file
    int _C ; // boolean value, print or not print the chunks information
    int _K; // boolean value , print or not print the keywords and corrisponding text of the tEXt chunks

} pformat_output={0,0,0,0,0,0,0,0,0};

//Default formats definitions
const char *default_pformat = "_f: _w x _h, _c, _d bits per sample, _N chunks\n_C";
const char *default_cformat = "\t_n: _t (_l)\n";
const char *default_kformat = "\t_k: _t\n";


/*
    CRC32 algorithm
    The following CRC algorithm is the available implementation on the PNG format specifications website: https://www.w3.org/TR/2023/CR-png-3-20230921/#samplecrc
*/
/* Table of CRCs of all 8-bit messages. */
unsigned long crc_table[256];

/* Flag: has the table been computed? Initially false. */
int crc_table_computed = 0;

/* Make the table for a fast CRC. */
void make_crc_table(void)
{
  unsigned long c;
  int n, k;

  for (n = 0; n < 256; n++) {
    c = (unsigned long) n;
    for (k = 0; k < 8; k++) {
      if (c & 1)
        c = 0xedb88320L ^ (c >> 1);
      else
        c = c >> 1;
    }
    crc_table[n] = c;
  }
  crc_table_computed = 1;
}
/* Update a running CRC with the bytes buf[0..len-1]--the CRC
   should be initialized to all 1's, and the transmitted value
   is the 1's complement of the final running CRC (see the
   crc() routine below). */

unsigned long update_crc(unsigned long crc, unsigned char *buf,
                         int len){
  unsigned long c = crc;
  int n;

  if (!crc_table_computed)
    make_crc_table();
  for (n = 0; n < len; n++) {
    c = crc_table[(c ^ buf[n]) & 0xff] ^ (c >> 8);
  }
  return c;
}

/* Return the CRC of the bytes buf[0..len-1]. */
unsigned long crc(unsigned char *buf, int len)
{
  return update_crc(0xffffffffL, buf, len) ^ 0xffffffffL;
}


// check if the CRC of a chunk is correct using the chunk type field and the chunk data field 
unsigned int PNG_crc_check(struct chunk ch, int len){
    unsigned long crc = 0xffffffffL;

    //Calculate the CRC of chunk type field
    crc = update_crc(0xffffffffL,ch.type, 4);

    //Calculate the CRC of chunk data field
    crc = update_crc(crc, ch.data, ch.length);

    /* Finalize and complement the CRC */
    return crc ^ 0xffffffffL;
}


// return 1 if the keyword is found in the format, 0 otherwise
int find_format(const char* format,const char* keyword){
    return (strstr(format,keyword)!=0)?1:0;  //note : the keyword that are not defined in the pformat specification are ignored 
}

//Read IHDR data and print them to the standard output following the format defined by "pformat"
void print_pformat(struct chunk* IHDR_ch, const char* pformat){
    //Reading IHDR_ch data
    char* data=IHDR_ch->data;

    //Checking what the format requires to print

    //Reading the image width
    pformat_output._w=((unsigned int)data[0]) | // dobrebbe ritornare il valore numerico dei 4 byte dedicati dalla width
           ((unsigned int)data[1] << 8) |
           ((unsigned int)data[2] << 16) |
           ((unsigned int)data[3] << 24);

    //Reading the image height

    pformat_output._h=((unsigned int)data[4]) | // dobrebbe ritornare il valore numerico dei 4 byte dedicati dalla height
           ((unsigned int)data[5] << 8) |
           ((unsigned int)data[6] << 16) |
           ((unsigned int)data[7] << 24);

    //Reading the image bit depth
    pformat_output._d=data[8];

    //Reading the image color type

    pformat_output._c=data[9];

    //Increment the number of chunks of the PNG file
    pformat_output._N+=1;

    //Print the information about the PNG file in the specified format
    for(const char* p=pformat;*p!='\0';p++){
        switch(*p){
            case '_':
                p++;
                switch(*p){
                    case 'f':
                        printf("%s",pformat_output._f);
                        break;
                    case 'w':
                        printf("%u",pformat_output._w);
                        break;
                    case 'h':
                        printf("%u",pformat_output._h);
                        break;
                    case 'd':
                        printf("%u",pformat_output._d);
                        break;
                    case 'c':
                        printf("%u",pformat_output._c);
                        break;
                    case 'N':
                        printf("%u",pformat_output._N);
                        break;
                    case 'C':
                        pformat_output._C=1;
                        break;
                    case 'K':
                        pformat_output._K=1;
                        break;

                    default: //ignore the keyword that are not defined in the pformat specification
                        // printf("_%c",*p);
                        break;
                }
                break;
            default:
                printf("%c",*p);
                break;
        }
    }


}

void print_cformat(struct chunk* ch, const char* cformat){
    //Print the information about the chunk in the specified format
    for(const char* p=cformat;*p!='\0';p++){
        switch(*p){
            case '_':
                p++;
                switch(*p){
                    case 'n':
                        printf("%u",ch->num);
                        break;
                    case 't':
                        printf("%s",ch->type);
                        break;
                    case 'l':
                        printf("%u",ch->length);
                        break;
                    case 'c':
                        printf("%u",ch->crc);
                        break;
                    case 'D':
                        print_dataChunk(ch->data,ch->length); // print the data chunk
                        break;
                    default:
                        // printf("_%c",*p);
                        break;
                }
                break;
            default:
                printf("%c",*p);
                break;
        }
    }
}

// print the data chunk in the standard output in at line of at most 16 hexadecimal values (bytes)
void print_dataChunk(unsigned char* data, unsigned int length){
    int size=(length>4)?32:length; // print only the first 4 bytes of the data chunk (if the data chunk is less than 4 bytes, print all the data chunk

    for(unsigned int i=0;i<size;i+=2){
        printf("%c%c ",data[i],data[i+1]);
    }
    printf("\n");
}

void print_cformat(struct chunk* ch, const char* cformat){


    const char keyword[79+1];
    strcpy(keyword,ch->data);
    int keyword_length=strlen(keyword);

    const char* text_string=ch->data+keyword_length+1;
    int text_string_length=ch->length-keyword_length-1;


     
    //Print the information about the tEXt chunk in the specified format
    for(const char* p=cformat;*p!='\0';p++){
        switch(*p){
            case '_':
                p++;
                switch(*p){
                    case 'k': // print the keyword
                        printf("%.*s",keyword_length,keyword);
                        break;
                    case 't': // print the text string
                        printf("%.*s",text_string_length,text_string);
                        break;

                    default:
                        break;
                }
                break;
            default:
                printf("%c",*p);
                break;
        }
    }
}

void readPNGfile(FILE* png_file,const char* pformat,const char* cformat,const char* kformat){

    // check if the PNG file is valid
    //PNG signature check
    const unsigned char png_signature[8] = {0x89,0x50,0x4E,0x47,0x0D,0x0A,0x1A,0x0A}; // PNG signature
    unsigned char png_signature_read[8]; // PNG signature read from the PNG file
    
    // read the PNG signature from the PNG file
    if(fread(png_signature_read,8,1,png_file)!=1){
        printf("Error, can't read the PNG signature\n");
        fclose(png_file);
        return;
    }
    //else reading operation was successful

    if(memcmp(png_signature,png_signature_read,8)!=0){
        printf("Error, the file is not a valid PNG file\n");
        fclose(png_file);
        return;
    }

    //Reading the PNG file chunks

    unsigned int reading_chunk_group=IHDR; //define how to handle the chunk reading (IHDR, tEXt, etc...)

    while(!feof(png_file)){
    
    struct chunk read_chunk={0,0,0,0,0};

    // Read chunk information

    // Read the chunk length field
    if(fread(&read_chunk.length,4,1,png_file)!=1){
        printf("Error, can't read the chunk length field\n");
        fclose(png_file);
        return;
    }
    
    //Read the chunk type field
    if(fread(read_chunk.type,4,1,png_file)!=1){ 
        printf("Error, can't read the chunk type field\n");
        fclose(png_file);
        return;
    } 


    //Reading the IHDR chunk data
    read_chunk.data = malloc(read_chunk.length); // allocate memory for the chunk data field

    //check if the memory allocation was successful
    if(read_chunk.data==NULL){
        printf("Error, can't allocate memory for the chunk data field\n");
        fclose(png_file);
        return;
    }

    //Read the chunk data field
    if(fread(read_chunk.data,read_chunk.length,1,png_file)!=1){ // read the IHDR chunk data)
        printf("Error, can't read the chunk data field\n");
        fclose(png_file);
        free(read_chunk.data); //deallocate memory for the chunk data field
        return;
    } 

    //Reading the chunk CRC field
    if(fread(&read_chunk.crc,4,1,png_file)!=1){
        printf("Error, can't read the chunk CRC field\n");
        fclose(png_file);
        free(read_chunk.data); //deallocate memory for the chunk data field
        return;
    }
    
    //Check if the CRC is correct
    if(PNG_crc_check(read_chunk,4)!=read_chunk.crc){
        printf("Error, the chunk CRC field is not correct\n");
        fclose(png_file);
        free(read_chunk.data); //deallocate memory for the IHDR chunk data
        return;
    }

    switch (reading_chunk_group){
    case IHDR:
        print_pformat(&read_chunk,pformat);
        break;
    case TEXT:
        print_cformat(&read_chunk,cformat);
        break;
    }

    print_cformat(&read_chunk,cformat);

    }
 


}


int main(int argc, char* argv[]){
    if(argc < 2){
        printf("Error, a PNG file name is required for %s \n.Try to use: %s [options] [--] file1 [[options] file2 . . . ]\n", argv[0],argv[0]);
        return 1;
    }

    
    pformat_output; // struct to store the information about the PNG file in the specified format

    //Definition of the initial format values
    const char *pformat = default_pformat;
    const char *cformat = default_cformat;
    const char *kformat = default_kformat;

    FILE* png_file; // pointer to the PNG file

    unsigned int i; // loop counter

    int read_a_file=0; // flag to check if a PNG file is read used to flag the unique presence of "--" in the command lines


    // loop through all the line arguments
    for (i = 1; i < argc; i++){

        // check if the argument is the optional parameter "--"
        if(!read_a_file && strncmp(argv[i],"--",2)==0){
            // all_files=1;
            i++; // go to the next parameter
            goto all_files; // go to the case in which all next line arguments represent PNG files name in order to process them
            continue;
        }

        //update the reference to the format values

        // check if the argument is the optional parameter "p=" for setting up the "pformat" value
        if(strncmp(argv[i],"p=",2)==0){
            pformat = argv[i]; // set the "pformat" value to the format specified in the argument
            continue;
        }
        if(strncmp(argv[i],"c=",2)==0){
            cformat = argv[i]; // set the "cformat" value to the format specified in the argument
            continue;
        }
        if(strncmp(argv[i],"k=",2)==0){
            kformat = argv[i]; // set the "kformat" value to the format specified in the argument
            continue;
        }

        // read the PNG file name
        //TODO : implement function given the multiple equals lines of code
        
        png_file = fopen(argv[i],"rb"); // open the PNG file in read binary mode
        if(png_file == NULL){
            printf("Error, can't open the file %s\n",argv[i]);
            continue;
        }
        pformat_output.file_counter = i; // set the file counter in the pformat output struct

        pformat_output._f = argv[i]; // set the file name in the pformat output struct

        readPNGfile(png_file,pformat,cformat,kformat); // read the PNG file 


        // reset the format values to the default values for the next PNG file
        pformat = default_pformat;
        cformat = default_cformat;
        kformat = default_kformat;


    }

    all_files:
    // loop through all the line arguments and process all files
    for (i; i < argc; i++){
        png_file = fopen(argv[i],"rb"); // open the PNG file in read binary mode
        if(png_file == NULL){
            printf("Error, can't open the file %s\n",argv[i]);
            continue;
        }
        pformat_output.file_counter = i; // set the file counter in the pformat output struct
        pformat_output._f = argv[i]; // set the file name in the pformat output struct
        readPNGfile(png_file); // read the PNG file 

        // reset the format values to the default values for the next PNG file
        pformat = default_pformat;
        cformat = default_cformat;
        kformat = default_kformat;

    }
    


}
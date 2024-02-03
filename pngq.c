/*
Systems Programming assignment : Reading Data from Portable Network Graphics Files

Write a C program named "pngq.c" that reads and extracts information from a Portable Network Graphic file (PNG).
PNG format specifications are available on the W3C documentation : https://www.w3.org/TR/2023/CR-png-3-20230921/

The program is able to read and extract information from PNG files passed as parameters of command line and what it does 
is verifying the integrity and validity of PNG file looking for the PNG signature and checking if the CRC of each chunk is correct.
In particular, it also controls that the IHDR chunk fields are valid.
If all chunks of a PNG file are valid, the program prints the information about the PNG file in the specified formats (pformat,cformat,kformat).
Note : the CRC algorithm I used is a readapted version based on the one implemented on the PNG format specifications website: https://www.w3.org/TR/2023/CR-png-3-20230921/#samplecrc

Notes about the imported libraries:
    - string.h : for the string manipulation functions
    - stdlib.h : for the memory allocation functions (malloc, realloc, free)
    - arpa/inet.h : for the htonl() function (for converting the network byte order to host byte order,which means little endian to big endian translation)
    - ctype.h : for the isdigit() function call (to check if a character is a digit)

Solution by : Birindelli Leonardo
*/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <ctype.h>


/*
    Structs and global variables definitions
*/

// chunk PNG file struct definition
struct chunk{
    unsigned int num;           // chunk number
    unsigned int length;        // length of chunk data field
    unsigned char type[4];      // chunk type field , it's values are restricted between 'A'-'Z' and 'a'-'z'
    unsigned char* data;        //chunk data field , the size depends on the chunk length field
    unsigned int crc;           // chunk checksum field
};

//Define the color type enum used to store the possible color types of the PNG file
enum colorType {Greyscale=0, Truecolour=2, Indexed=3, GreyscaleAlpha=4, TruecolourAlpha=6};

//Define a mapping between the color type enum and the color type string
const char* colorTypeStrings[] = { "Greyscale", "", "Truecolor", "Indexed", "Greyscale+alpha", "", "Truecolor+alpha" };


//Define the struct for the pformat output, used to store the information about the PNG file in the specified format
struct pf_output{
    char * _f; // file name
    unsigned int _w; // image width
    unsigned int _h; // image height
    unsigned int _d; // image bit depth
    enum colorType _c; // image color type
    unsigned int _N; //number of chunks of the PNG file
    int _C ; // boolean value, print or not print the chunks information
    int _K; // boolean value , print or not print the keywords and corrisponding text of the tEXt chunks

} pformat_output;

//Default formats declaration and initialization
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

//Print the data chunk on the standard output in at line of at most 16 hexadecimal values (bytes)
void print_dataChunk(unsigned char* data, unsigned int length){
    if(length==0)
        putchar('\n');

    for(int i=0;i<length;i++){
        // printf("%02x ",data[i]);
        if(data[i] < 0x10)
            printf(" %x ",data[i]);
        else 
            printf("%x ",data[i]);
        if((i+1)%16==0 && i+1<length){
            putchar('\n');
        }
    }
}

//Read IHDR data, update the value of the struct "pformat_output" for the printing and,finally,
//print the information contained in that struct to the standard output according to the format defined by "pformat"

int print_pformat(const struct chunk* IHDR_ch, const char* pformat){
    //Reading IHDR_ch data
    unsigned char* data=IHDR_ch->data;

    //Save the image width, height, bit depth and color type in the pformat_output struct

    //Reading the image width 
    pformat_output._w=(unsigned int)data[0] << 24 | (unsigned int)data[1] << 16 | (unsigned int)data[2] << 8 
    | (unsigned int)data[3]; // convert the image width from network byte order to host byte order (little endian to big endian)

    //Reading the image height
     pformat_output._h=(unsigned int)data[4] << 24 | (unsigned int)data[5] << 16 | (unsigned int)data[6] << 8 
    | (unsigned int)data[7]; // convert the image width from network byte order to host byte order (little endian to big endian)

    //Reading the image bit depth
    pformat_output._d=data[8];

    //Reading the image color type
    pformat_output._c=data[9];
    
    //Mapping the color type value to its corresponding string
    const char* color_type_str=colorTypeStrings[pformat_output._c];

    //Check if the color type is valid 
    //Analyze if the bit depth field value assumes a valid number for the PNG image color type
    //For more information about this checking are available on the PNG format specifications website: https://www.w3.org/TR/2023/CR-png-3-20230921/#table111
    switch (pformat_output._c){
    case 0: //Greyscale
        if(!(pformat_output._d==1 || pformat_output._d==2 || pformat_output._d==4 || pformat_output._d==8|| pformat_output._d==16))
            return -1;
        break;
    case 2: //Truecolour
        if(!(pformat_output._d==8|| pformat_output._d==16))
            return -1;
        break;
    case 3: //Indexed
        if(!(pformat_output._d==1 || pformat_output._d==2 || pformat_output._d==4 || pformat_output._d==8))
            return -1;
        break;
    case 4: //GreyscaleAlpha
        if(!(pformat_output._d==8|| pformat_output._d==16))
            return -1;
        break;
    case 6: //TruecolourAlpha
        if(!(pformat_output._d==8|| pformat_output._d==16))
            return -1;
        break;
    default:
        break;
    }

    //Print the information about the PNG file in the specified format
    for(const char* p=pformat;*p!='\0';p++){
        switch(*p){
            case '\n':
                if(strcmp(p+1,"_K")!=0)
                    printf("\n"); //print the new line character
                break;
            case '_':
                p++;
                switch(*p){
                    case 'f':
                        printf("%s",pformat_output._f); //print the file name
                        break;
                    case 'w':
                        printf("%u",pformat_output._w); //print the image width
                        break;
                    case 'h':
                        printf("%u",pformat_output._h); //print the image height
                        break;
                    case 'd':
                        printf("%u",pformat_output._d); //print the image bit depth
                        break;
                    case 'c':
                        printf("%s",colorTypeStrings[pformat_output._c]); //print the image color type string
                        break;
                    case 'N':
                        printf("%u",pformat_output._N); //print the number of chunks of the PNG file
                        break;
                    case 'C':
                        pformat_output._C=1; //set the flag to 1 to indicate that the chunks information must be printed
                        break;
                    case 'K':
                        pformat_output._K=1; //set the flag to 1 to indicate that the keywords and corrisponding text of the tEXt chunks must be printed
                        break;

                    default: //ignore the keyword that are not defined in the pformat specification
                        break;
                }
                break;
            default:
                printf("%c",*p); //print the character
                break;
        }
    }

    return 0;
}

//Print the information about the chunk in the specified format specified by "cformat"
void print_cformat(const struct chunk* ch, const char* cformat){

    //Print the information about the chunk in the specified format
    for(const char* p=cformat;*p!='\0';p++){
        switch(*p){
            case '_':
                p++;
                switch(*p){
                    case 'n':
                        printf("%u",ch->num); // print the chunk number
                        break;
                    case 't':
                        printf("%.4s",ch->type); // print the chunk type string
                        break;
                    case 'l':
                        printf("%u",ch->length); // print the chunk length
                        break;
                    case 'c':
                        printf("%u",ch->crc); // print the chunk CRC
                        break;
                    case 'D':
                        print_dataChunk(ch->data,ch->length); // print the data chunk
                        break;
                    default: //ignore the keyword that are not defined in the pformat specification
                        break;
                }
                break;
            default:
                printf("%c",*p); //print the character
                break;
        }
    }
}

//Print the keyword and the text string of the tEXt chunk in the specified format specified by "kformat"
void print_kformat(const struct chunk* ch, const char* cformat){

    //Declare the Keyword characters array in which is also included the Null separator byte space for the null terminator
    unsigned char keyword[79+1]; 

    //Copy the keyword from the chunk data field to the keyword array
    unsigned i =0;
    for(;i<79 && ch->data[i]!='\0';i++){
        keyword[i]=ch->data[i];
    }
    keyword[i]='\0'; // add the null terminator to the keyword

    int keyword_length=i; // length of the keyword

    //Declare the Text string characters array and its length
    const unsigned char* text_string=ch->data+keyword_length+1;
    int text_string_length=ch->length-keyword_length-1;

    //Print the information about the tEXt chunk in the specified format
    for(const char* p=cformat;*p!='\0';p++){
        switch(*p){
            case '_':
                p++;
                switch(*p){
                    case 'k': 
                        printf("%.*s",keyword_length,keyword); // print the keyword
                        break;
                    case 't': 
                        printf("%.*s",text_string_length,text_string); // print the text string
                        break;

                    default: //ignore the keyword that are not defined in the pformat specification
                        break;
                }
                break;
            default:
                printf("%c",*p); //print the character
                break;
        }
    }
}
//Print the information about the PNG file in the specified formats (pformat,cformat,kformat)
int print_info(struct chunk* chunks,const char * pformat,const char * cformat,const char * kformat){

    const struct chunk* p = chunks;
    int flag=0; // flag to check if the IHDR chunk fields are valid

    //Print the chunk information 
    for(int i =0; i < pformat_output._N; i++,p++){
        
        if(memcmp(p->type,"IHDR",4)==0){
            flag=print_pformat(p,pformat); //print the IHDR chunk information following the format "pformat"
        }

        if(pformat_output._C){
            print_cformat(p,cformat); //print the chunk information following the format "cformat"
        }
        if(pformat_output._K && memcmp(p->type,"tEXt",4)==0){
            print_kformat(p,kformat); //print the keyword and the text string of the tEXt chunk following the format "kformat"
        }
    }

    return flag;



}

//Close the PNG file and deallocate memory for the array of chunks
struct chunk* dealloc_mem(FILE* png_file,struct chunk* chunks_array){
    //Close the PNG file
    fclose(png_file);
    //Deallocate memory allocated before with the "malloc" and "realloc" functions
    for (int i = 0; i < pformat_output._N; i++) {
        free(chunks_array[i].data);
    }
    free(chunks_array);
    
    return NULL;

}


//Read the PNG file and return an array of chunks if the PNG file is valid, otherwise return NULL
struct chunk* readPNGfile(FILE* png_file){

    //Check if the PNG file is valid

    //PNG signature check
    const unsigned char png_signature[8] = {0x89,0x50,0x4E,0x47,0x0D,0x0A,0x1A,0x0A}; // PNG signature
    unsigned char png_signature_read[8]; // PNG signature read from the PNG file
    
    //Read the PNG signature from the PNG file
    if(fread(png_signature_read,8,1,png_file)!=1){
        printf("Error, can't read the PNG signature\n");
        fclose(png_file);
        return NULL;
    }

    //Check if the PNG signature is correct
    if(memcmp(png_signature,png_signature_read,8)!=0){
        printf("Error, the file is not a valid PNG file\n");
        fclose(png_file);
        return NULL;
    }


    //Allocate memory for the array of chunks
    unsigned int size=5; // initial size of the array of chunks
    struct chunk* chunk_array=malloc(sizeof(struct chunk)*size); // allocate memory for the array of chunks

    if(chunk_array!=NULL){

    //Reading the PNG file chunks

    unsigned char check_chunk_type[4]="IHDR"; //used to flag the type of the current chunk and check if it is the IEND chunk
    unsigned i =0;

    /* Read and save chunk information till the end of file */
    while(memcmp(check_chunk_type,"IEND",4)!=0 && !feof(png_file)){

        struct chunk new_chunk;

        //Assign the chunk number to the new chunk
        new_chunk.num=i+1;
        
        // Read the chunk length field
        if(fread(&new_chunk.length,4,1,png_file)!=1){
            printf("Error, can't read the chunk length field\n");
            return dealloc_mem(png_file,chunk_array);
        }

        new_chunk.length=htonl(new_chunk.length); // convert the chunk length field from network byte order to host byte order
        
        //Read the chunk type field
        if(fread(new_chunk.type,4,1,png_file)!=1){ 
            printf("Error, can't read the chunk type field\n");
            return dealloc_mem(png_file,chunk_array);
        } 

        //Check if the chunk type field is valid
        for(unsigned i =0;i<4;i++){
            if(!((new_chunk.type[i]>='A' && new_chunk.type[i]<= 'Z') || (new_chunk.type[i]>= 'a' && new_chunk.type[i]<= 'z') || isdigit(new_chunk.data[i]))){
                printf("Error, the chunk type field is not valid\n");
                return dealloc_mem(png_file,chunk_array);
            }
        }
        
        //Reading the chunk data
        new_chunk.data = malloc(sizeof(unsigned char)*new_chunk.length); // allocate memory for the chunk data field

        //Check if the memory allocation was successful
        if(new_chunk.data==NULL){
            printf("Error, can't allocate memory for the chunk data field\n");
            return dealloc_mem(png_file,chunk_array);
        }


        
        //Read the chunk data field
        if(fread(new_chunk.data,sizeof(unsigned char),new_chunk.length,png_file)!=new_chunk.length){ // read the IHDR chunk data)
            printf("Error, can't read the chunk data field\n");
            return dealloc_mem(png_file,chunk_array);
        } 

        //Reading the chunk CRC field
        if(fread(&new_chunk.crc,4,1,png_file)!=1){
            printf("Error, can't read the chunk CRC field\n");
            fclose(png_file);
            free(new_chunk.data); //deallocate memory for the chunk data field
            free(chunk_array); //deallocate memory for the array of chunks
            return NULL;
        }
        new_chunk.crc=htonl(new_chunk.crc); // convert the chunk CRC field from network byte order to host byte order


        //Check if the CRC is correct
        if(PNG_crc_check(new_chunk,4)!=new_chunk.crc){
            printf("Error, the chunk CRC field is not correct\n");
            return dealloc_mem(png_file,chunk_array);
        }

        chunk_array[i]=new_chunk;

        //Update the check_chunk_type value
        memcpy(check_chunk_type,new_chunk.type,4);

        i++;

        //Reallocate memory for the array of chunks if needed
        if(i>=size){
            size*=2;
            chunk_array=realloc(chunk_array,sizeof(struct chunk)*size);
        }
        //Increment the number of chunks of the PNG file
        pformat_output._N+=1;
    }

    }

    return chunk_array;
}


int main(int argc, char* argv[]){
    if(argc < 2){
        printf("Error, a PNG file name is required for %s \n.Try to use: %s [options] [--] file1 [[options] file2 . . . ]\n", argv[0],argv[0]);
        return 1;
    }

    //Set to default values struct to store the information about the PNG file in the specified format
    memset(&pformat_output, 0, sizeof(pformat_output)); 
    
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
        if(strncmp(argv[i],"--",2)==0){
            if(read_a_file){
                printf("Error, the \"--\" parameter is not allowed after a PNG file name\n");
                return 0; // exit the program
            }else{
                i++;
                //compute the next arguments as PNG file names
                goto all_files;
            }
        }

        //Update the reference to the format values

        //Check if the argument is the optional parameter "p=" for setting up the "pformat" value
        if(strncmp(argv[i],"p=",2)==0){
            pformat = argv[i]+2; // set the "pformat" value to the format specified in the argument
            argv[strlen(argv[i])-1]='\0'; // remove the last character of the "pformat" value (which is a single quote)
            continue;
        }
        if(strncmp(argv[i],"c=",2)==0){
            cformat = argv[i]+2; // set the "cformat" value to the format specified in the argument
            argv[strlen(argv[i])-1]='\0'; // remove the last character of the "cformat" value (which is a single quote)
            continue;
        }
        if(strncmp(argv[i],"k=",2)==0){
            kformat = argv[i]+2; // set the "kformat" value to the format specified in the argument
            argv[strlen(argv[i])-1]='\0'; // remove the last character of the "kformat" value (which is a single quote)
            continue;
        }

        // read the PNG file name        
        png_file = fopen(argv[i],"rb"); // open the PNG file in read binary mode
        if(png_file == NULL){
            printf("Error, can't open the file %s\n",argv[i]);
            continue;
        }
        read_a_file=1; // set the flag to 1 to indicate that a PNG file is read and the "--" parameter is not allowed anymore

        pformat_output._f = argv[i]; // set the file name in the pformat output struct

        struct chunk* chunks =readPNGfile(png_file); // read the PNG file 

        if(chunks==NULL){
            printf("Error, can't read the PNG file %s\n",argv[i]);
            continue;
        }
        //print the chunks information following the formats
        if(print_info(chunks,pformat,cformat,kformat)==-1){
            printf("Error, the bit depth field value is not valid for the color type field value for the PNG file %s\n",pformat_output._f);
        }

        dealloc_mem(png_file,chunks); // deallocate memory for the array of chunks and close the file
    
        memset(&pformat_output, 0, sizeof(pformat_output)); // reset the pformat_output struct to the default values
    }

    return 0;

    all_files:
    // loop through all the line arguments and process all files
    for (; i < argc; i++){
        png_file = fopen(argv[i],"rb"); // open the PNG file in read binary mode
        if(png_file == NULL){
            printf("Error, can't open the file %s\n",argv[i]);
            continue;
        }

        pformat_output._f = argv[i]; // set the file name in the pformat output struct
        struct chunk* chunks =readPNGfile(png_file); // read the PNG file 

        if(chunks==NULL){
            printf("Error, can't read the PNG file %s\n",argv[i]);
            continue;
        }

        //print the chunks information following the formats
        if(print_info(chunks,pformat,cformat,kformat)==-1){ // in case of error
            printf("Error, the bit depth field value is not valid for the color type field value for the PNG file %s\n",pformat_output._f);
        }

        dealloc_mem(png_file,chunks); // deallocate memory for the array of chunks and close the file

        memset(&pformat_output, 0, sizeof(pformat_output)); // set the pformat_output struct to the default values*/

    } 

    return 0;
}

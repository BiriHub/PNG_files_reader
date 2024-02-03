## PNG files reader in C
The program called '**pngq.c**' reads and extracts information from one or more _Portable Network Graphics files_ (*PNG*) passed as parameters of command line and what it does is verifying the integrity and validity of each PNG file, looking for the PNG signature and checking if the CRC of each PNG chunk is correct. In particular, it also controls that the IHDR chunk fields are valid.

If all chunks of a PNG file are conform with the specification described on the [W3C documentation of the PNG format](https://www.w3.org/TR/png-3/), the program prints the information about the file in the specified formats (pformat,cformat,kformat).

> [!NOTE]
> In order to compile the code, I recomend to compile with the gcc-11 compiler 
> I used the gcc version 11.4.0
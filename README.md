# lsb
Lunar Script Builder

This program will perform one of three actions:

1.) Decode: Take binary output TEXTxxx.DAT file from Saturn/iOS/PSX Lunar SSS or SSSC and convert to a metadata file.

2.) Encode: Take a uniquely formatted input file containing metadata, parse it, and create a binary output compatible with the TEXTxxx.DAT files used in Sega Saturn Version of Lunar SSS or SSS-MPEG.

3.) Update: Take a uniquely formatted input and update metadata files, parse them, and create an updated input file that could later be used to build a binary output file.

Usage:  
   lsb.exe decode InputFname OutputFname ienc [sss]                    
   lsb.exe encode InputFname OutputFname [sss]                         
   lsb.exe update InputFname OutputFname UpdateFname                   
The table file should be named font_table.txt  
The compression table file should be named bpe.table; Another utility is used to create this.  
sss designates that the table being used is that for sssc and will be automatically altered for compatibilty  


Test Progress: 
* 2-Byte Mode & iOS Decoding to output files complete. 
* 2-Byte Mode Encoding to binary for SSS & SSS-MPEG verified. 
* 1-byte decoding / re-encoding using BPE compression verified.
* Update functionality tested & verified.  
 
  
  
     

# lsb
Lunar Script Builder

This program will perform one of two actions:

1.) Take a uniquely formatted input file containing metadata, parse it, and create a binary output compatible with the TEXTxxx.DAT files used in Sega Saturn Version of Lunar SSS or SSSC.

2.) Take a uniquely formatted input and update metadata files, parse them, and create an updated input file that could later be used to build a binary output file.

Usage:  
   lsb.exe decode InputFname OutputFname ienc [sss]                    
   lsb.exe encode InputFname OutputFname [sss]                         
   lsb.exe update InputFname OutputFname UpdateFname                   
The table file should be named font_table.txt  
sss designates that the table being used is that for sssc and will be automatically altered for compatibilty  


TBD:  Write binary output routine for Option (code 0x0007)  
      Debug (A LOT)  
      Add .csv format for better script dump output  
     
     

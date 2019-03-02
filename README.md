# lsb
Lunar Script Builder

This program will perform one of two actions:

1.) Take a uniquely formatted input file containing metadata, parse it, and create a binary output compatible with the TEXTxxx.DAT files used in Sega Saturn Version of Lunar SSS or SSSC.

2.) Take a uniquely formatted input and update metadata files, parse them, and create an updated input file that could later be used to build a binary output file.

Usage:
    lsb.exe encode InputFname OutputFname [sss]
    lsb.exe update InputFname OutputFname UpdateFname
the table file should be named newfont_table.txt
sss designates that the table being used is that for sssc and will be automatically altered for compatibilty


THIS PROGRAM IS STILL BEING DEBUGGED.
THE PROGRAM THAT GENERATES THE INPUT SCRIPT FROM EXISTING .DAT FILES IS NOT YET 100% WRITTEN.

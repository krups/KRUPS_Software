#include <iostream>
#include <fstream>
#include <string>
#include <stdio.h>

using namespace std;

//ansi color codes
#define YELLOW  ("\033[1;33m") 
#define GREEN  ("\033[0;32m")
#define RED  ("\033[0;31m")
#define NC  ("\033[0m")

//prints a message using the given color w/o newline
void printColor(string message, string color)
{
    cout << color << message << NC;
}

//print a message using the given color w newline
void printColorLn(string message, string color)
{
    cout << color << message << NC << endl;
}


int main(int argc, char** argv)
{
	bool allInOne;
	int perFile;
	if(argc == 2)
	{
		allInOne = true;
		perFile = 0;
	}
	else if(argc == 3)
	{
		allInOne = false;
		perFile = atoi(argv[2]);
	}
	else
	{
				printColorLn("Usage ./CSLtoBin <file to be converted> or ./CSLtoBin <file to be converted> <number of bytes per file>", RED);
				return -1;
	}

	printColor("Opening file: ", YELLOW);
	cout << argv[1] <<"...";
	ifstream commaSep;
	commaSep.open(argv[1]);

	if(commaSep.is_open())
	{
		printColorLn("done", GREEN);
	}
	else
	{
		printColorLn("failed", RED);
		return 1;
	}

	printColorLn("Converting...", YELLOW);

	int val;
	uint8_t conv;
	int file = 0;
	int bytesRead = 0;
	
	ofstream currFile;
	string name = "out/"+to_string(file) + ".bin";
	currFile.open(name.c_str(), ios::out | ios::binary);
	
	printColorLn("...", YELLOW);
	
	string line;
	while(!commaSep.eof())
	{
		commaSep >> val;
		conv = val;
		bytesRead++;
		currFile.write((char*)&conv, sizeof(uint8_t));
		if(bytesRead == perFile && !allInOne )
		{
			currFile.close();
			file++;
			if(!commaSep.eof())
			{
				name = "out/" + to_string(file) + ".bin";
				currFile.open(name.c_str(), ios::out | ios::binary);
				printColorLn("...", YELLOW);
			}
			bytesRead = 0;
		}
	}
	currFile.close();
	printColorLn("done", GREEN);
	return 0;
}

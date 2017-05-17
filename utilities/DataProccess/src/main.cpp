#include "compress.h"
#include <iostream>
#include <fstream>
#include <vector>
#include "quicksort.h"
#include <string>
#include "Packet.h"

#define TOTAL_ARGS (4)

//ansi color codes
#define YELLOW  ("\033[1;33m") 
#define GREEN  ("\033[1;32m")
#define RED  ("\033[1;31m")
#define NC  ("\033[0m")

//conversion constants for the sensors
#define TC_CONVERSION (1)
#define LOACCEL_CONVERSION (1)
#define HIACCEL_CONVERSION (1)
#define MAG_CONVERSION (1)
#define GYRO_CONVERSION (1)

using namespace std;

//prints a message using the given color
void printColor(string message, string color)
{
    cout << color << message << NC;
}

void printColorLn(string message, string color)
{
    cout << color << message << NC << endl;
}

//compare function for quicksort
bool sortPackets(Packet a, Packet b)
{
    //compare chronlogically by the counter in the first two bytes
    uint16_t aNum = a[0] + 256* a[1];
    uint16_t bNum = b[0] + 256* b[1];
    if(aNum < bNum)
    {
        return true;
    }
    else
    {
        return false;
    }
}


//pulls relvant info form a specifically formated .dec file 
//the three lines should look like
/*
SIZE={number}
HEADER_SIZE={number}
PACKET_ORDER={comma seperated list of instruments}
*/
void readPacketDesc(char* fileName, int& packetSize, int& headersize, string& packetOrder)
{
    ifstream desc;
    desc.open(fileName);
    string sizeinfo, headerinfo, order;
    getline(desc, sizeinfo);
    getline(desc, headerinfo);
    getline(desc, order);
    packetSize = stoi(sizeinfo.substr(5, sizeinfo.size())); //slice off the words
    string small = headerinfo.substr(12, headerinfo.size());
    headersize = stoi(small);
    packetOrder = order.substr(13, order.size());
    desc.close();
    return;
}

void CSVadd(ofstream& csv, string value)
{
    csv << value << ',';
}

void CSVadd(ofstream& csv, double value)
{
    csv << value << ',';
}

double getTCReading(Packet data, int& loc)
{
    int rawData = data[loc] + 256*data[loc+1];
    loc += 2;
    double value = rawData * TC_CONVERSION;
    return value;
}

void getGyroReadings(Packet data, int& loc, double* readings)
{
    for(int i = 0; i < 3; i++)
    {
        int rawData = data[loc] + 256*data[loc+1];
        loc += 2;
        readings[i] = rawData * GYRO_CONVERSION;
    }
}

void getLoAccelReadings(Packet data, int& loc, double* readings)
{
    for(int i = 0; i < 3; i++)
    {
        int rawData = data[loc] + 256*data[loc+1];
        loc += 2;
        readings[i] = rawData * LOACCEL_CONVERSION;
    }
}

void getHiAccelReadings(Packet data, int& loc, double* readings)
{
    for(int i = 0; i < 3; i++)
    {
        int rawData = data[loc] + 256*data[loc+1];
        loc += 2;
        readings[i] = rawData * HIACCEL_CONVERSION;
    }
}

void getMagReadings(Packet data, int& loc, double* readings)
{
    for(int i = 0; i < 3; i++)
    {
        int rawData = data[loc] + 256*data[loc+1];
        loc += 2;
        readings[i] = rawData * MAG_CONVERSION;
    }
}

int main(int argc, char** argv)
{
    //validate args
    if(argc != TOTAL_ARGS)
    {
        printColorLn("useage: ./dataProccesor <list of files> <packet description (.desc)> <name of CSV to be made>", RED);
        return 1;
    }

    printColor("Opening files list...", YELLOW);
    ifstream fileList; //all the files to proccess
    fileList.open(argv[1]);
    if(fileList.is_open())
    {
        printColorLn("done", GREEN);
    }
    else
    {
        printColorLn("failed", RED);
    }

    printColor("Reading packet info...", YELLOW);
    int packetSize, headerSize;
    string measurementOrder;
    readPacketDesc(argv[2], packetSize, headerSize, measurementOrder);
    vector<Packet> packetList; //data of files once read in
    printColorLn("done", GREEN);
    //cout << measurementOrder << endl;

    string fileName; //current file
    string dir;
    getline(fileList, dir);
    if(dir[dir.size()] != '/')
        dir += '/';

    //for each file
    printColorLn("Opening and reading each file...", YELLOW);
    while(getline(fileList, fileName))
    {
        ifstream currPacket;
        printColor(fileName + "...", YELLOW);
        currPacket.open(dir + fileName, ios::in | ios::binary); //open file
        if(!currPacket.is_open())
        {
            printColorLn("failed to open", RED);
            return 1;
        }
        char packetData[packetSize];

        //read the data in
        if(!currPacket.read(packetData, packetSize))
        {
            printColorLn("failed to be read", RED);
        }
        //if the need to be decompressed do that here

        //add to the list
        Packet p = Packet(packetData, packetSize);
        packetList.push_back(p);
        printColorLn("done", GREEN);
        currPacket.close();
    }

    //close relavant streams
    fileList.close();

    //order the packets chronologically
    printColor("organizing chronologically...", YELLOW);
    qsort(packetList, sortPackets);
    printColorLn("done", GREEN);

    //create the csv
    printColorLn("Creating the CSV... ", YELLOW);
    ofstream commaList;
    commaList.open(argv[3]);

    //add Header
    //TODO: add numbers for duplicate sensors
    printColor("Adding Header...", YELLOW);
    CSVadd(commaList, "Time (s)");
    for(int i = 0; i < measurementOrder.size(); i += 2)
    {
        switch(measurementOrder[i])
        {
            case '0': //TC
                CSVadd(commaList, "Temperature (Celcius)");
                break;
            case '1': //loacccel
                CSVadd(commaList, "Acceleration [X axis] (g-force)");
                CSVadd(commaList, "Acceleration [Y axis] (g-force)");
                CSVadd(commaList, "Acceleration [Z axis] (g-force)");
                break;
            case '2': //hiaccel
                CSVadd(commaList, "Acceleration [X axis] (g-force)");
                CSVadd(commaList, "Acceleration [Y axis] (g-force)");
                CSVadd(commaList, "Acceleration [Z axis] (g-force)");
                break;
            case '3': //mag
                CSVadd(commaList, "Magnetic Flux Density [X axis] (Gs)");
                CSVadd(commaList, "Magnetic Flux Density [Y axis] (Gs)");
                CSVadd(commaList, "Magnetic Flux Density [Z axis] (Gs)");
                break;
            case '4': //gyro
                CSVadd(commaList, "Rotation [X axis] (degrees/s)");
                CSVadd(commaList, "Rotation [Y axis] (degrees/s)");
                CSVadd(commaList, "Rotation [Z axis] (degrees/s)");
                break;
            default:
                printColorLn("fail", RED);
                printColorLn("not a known instrument, check the .desc file given", RED);
                commaList.close();
                return 1;
                break;
        }
    }
    commaList << endl;
    printColorLn("done", GREEN);

    //output data to the CSV
    //for each packet
    printColor("Getting readings...", YELLOW);
    double time = 0;
    for(int i = 0; i < packetList.size(); i++)
    {
        int loc = headerSize; //drop the header info
        Packet packet = packetList[i];
        //iterate over the length of the packet
        while(loc < packetSize)
        {
            //pulling info in for each instrument in order
            CSVadd(commaList, time);
            double readings[3];
            double tcRead;
            for(int i = 0; i < measurementOrder.size(); i += 2)
            {
                switch(measurementOrder[i])
                {
                    case '0': //TC
                        tcRead = getTCReading(packet, loc);
                        CSVadd(commaList, tcRead);
                        break;
                    case '1': //loacccel
                        getLoAccelReadings(packet, loc, readings);
                        for(int i = 0; i < 3; i++)
                        {
                            CSVadd(commaList, readings[i]);
                        }
                        break;
                    case '2': //hiaccel
                        getHiAccelReadings(packet, loc, readings);
                        for(int i = 0; i < 3; i++)
                        {
                            CSVadd(commaList, readings[i]);
                        }
                        break;
                    case '3': //mag
                        getMagReadings(packet, loc, readings);
                        for(int i = 0; i < 3; i++)
                        {
                            CSVadd(commaList, readings[i]);
                        }
                        break;
                    case '4': //gyro
                        getGyroReadings(packet, loc, readings);
                        for(int i = 0; i < 3; i++)
                        {
                            CSVadd(commaList, readings[i]);
                        }
                        break;
                    default:
                        printColorLn("fail", RED);
                        printColorLn("not a known instrument, check the .desc file given", RED);
                        commaList.close();
                        return 1;
                        break;
                }
            }
            time += .4;
            commaList << endl;
        }
    }
    printColorLn("done", GREEN);
    
    return 0;
}

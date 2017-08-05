#include "compress.h"
#include <iostream>
#include <fstream>
#include <vector>
#include "quicksort.h"
#include <string>
#include "Packet.h"
#include "brieflzDecompress.h"

#define TOTAL_ARGS (4)

//ansi color codes
#define YELLOW  ("\033[1;33m") 
#define GREEN  ("\033[1;32m")
#define RED  ("\033[1;31m")
#define NC  ("\033[0m")

//conversion constants for the sensors
#define TC_CONVERSION (.25)
#define LOACCEL_CONVERSION (0.732F) // +/- 2 g
#define HIACCEL_CONVERSION_MULTIPLIER (400.0F)
#define HIACCEL_CONVERSION_DIVISOR (4095.0F)
#define HIACCEL_ZERO_SHIFT (-200.0F)
#define MAG_CONVERSION (0.58F)  //+/- 4 gauss
#define GYRO_CONVERSION (0.07000F) //+/- 245 dps
#define TIME_CONVERSION (.001)


//unit conversion constants
#define SENSORS_GRAVITY_STANDARD  (1)//(9.80665F)      /**< Earth's gravity in m/s^2 */
#define SENSORS_MILLIGAUSS_TO_MICROTESLA  (.1F)  /**< Gauss to micro-Tesla multiplier */
#define MILLI_TO_BASE (.001F)

using namespace std;

//prints a message using the given color
void printColor(string message, string color)
{
    cout << color << message << NC;
}

//prints a message using the given color and a newline
void printColorLn(string message, string color)
{
    cout << color << message << NC << endl;
}


int getMeasureSize(string measurementOrder)
{
    int sum = 0;
    for(int i = 0; i < measurementOrder.length(); i+=2)
    {
        switch(measurementOrder[i])
        {
            case '0': //TC
                sum += 2;
                break;
            case '1': //loacccel
                sum += 6;
                break;
            case '2': //hiaccel
                sum += 6;
                break;
            case '3': //mag
                // u03bc is the mu character in unicode
                sum += 6;
                break;
            case '4': //gyro
                sum += 6;
                break;
            case '5': //time
                sum += 3;
                break;  
            default:
                printColorLn("fail", RED);
                printColorLn("not a known instrument, check the .desc file given", RED);
                return -1;
                break;
        }

    }

    return sum;
}

//pulls relvant info form a specifically formated .dec file 
//the three lines should look like
/*
SIZE={number}
HEADER_SIZE={number}
PACKET_ORDER={comma seperated list of instruments}
*/
void readPacketDesc(char* fileName, int& packetSize, int& headersize, string& packetOrder, int& measurementSize)
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
    measurementSize = getMeasureSize(packetOrder);
    return;
}


//inserts the value given into the file in a CSV format
void CSVadd(ofstream& csv, string value)
{
    csv << value << ',';
}

void CSVadd(ofstream& csv, float value)
{
    csv << value << ',';
}


int16_t twoBytesToInt(uint8_t loByte, uint8_t highByte)
{
	return (int16_t)(loByte | (highByte << 8));
}

float getTCReading(vector<uint8_t> data, size_t& loc)
{
    //cout << int(data[loc]) << " " << int(data[loc + 1]) << endl;
    int16_t rawData = twoBytesToInt(data[loc], data[loc +1]);
    loc += 2;
    float value = float(rawData) * TC_CONVERSION;
    return value;
}

float getTime(vector<uint8_t> data, size_t& loc)
{
    int rawData = data[loc] + 256*data[loc+1] + 65536*data[loc+2];
    loc += 3;
    float value = rawData * TIME_CONVERSION;
    return value;
}

void getGyroReadings(vector<uint8_t> data, size_t& loc, float* readings)
{
    for(int i = 0; i < 3; i++)
    {
        int16_t rawData = twoBytesToInt(data[loc], data[loc +1]);
        loc += 2;
        readings[i] = float(rawData) * GYRO_CONVERSION; //* MILLI_TO_BASE;
    }
}

void getHiAccelReadings(vector<uint8_t> data, size_t& loc, float* readings)
{
    for(int i = 0; i < 3; i++)
    {
        int16_t rawData = twoBytesToInt(data[loc], data[loc +1]);
        loc += 2;
        //cout << rawData << endl;
        //cout << HIACCEL_CONVERSION_MULTIPLIER << " " << HIACCEL_CONVERSION_DIVISOR << " " << HIACCEL_ZERO_SHIFT << endl;
        readings[i] = ((float(rawData) * HIACCEL_CONVERSION_MULTIPLIER)/HIACCEL_CONVERSION_DIVISOR) + HIACCEL_ZERO_SHIFT;
        //cout << readings[i] << endl;
    }
}

float getTempReading(vector<uint8_t> data, size_t& loc)
{
    int16_t rawData = twoBytesToInt(data[loc], data[loc +1]);
    loc += 2;
    float value = float(rawData) * TC_CONVERSION;
    return value;
}

void getLoAccelReadings(vector<uint8_t> data, size_t& loc, float* readings)
{
    for (int i = 0; i < 3; ++i)
    {
        uint8_t lo = data[loc];
        uint8_t hi = data[loc + 1];
        int16_t rawData = twoBytesToInt(lo, hi);
        loc += 2;
        readings[i] = float(rawData) * LOACCEL_CONVERSION * MILLI_TO_BASE * SENSORS_GRAVITY_STANDARD; //sets reading in m/s^2
    }
}

void getMagReadings(vector<uint8_t> data, size_t& loc, float* readings)
{
    for(int i = 0; i < 3; i++)
    {
        uint8_t lo = data[loc];
        uint8_t hi = data[loc + 1];
        int16_t rawData = twoBytesToInt(lo, hi);
        loc += 2;
        readings[i] = float(rawData) * MAG_CONVERSION * MILLI_TO_BASE;// * SENSORS_MILLIGAUSS_TO_MICROTESLA;
    }
}


bool sortMeasurement(vector<uint8_t> a, vector<uint8_t> b)
{
    int timeA = a[0] + 256 * a[1] + 65536 * a[2];
    int timeB = b[0] + 256 * b[1] + 65536 * b[2];
    if(timeA < timeB)
    {
        return true;
    }
    else
    {
        return false;
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
    int packetSize, headerSize, measurementSize;
    string measurementOrder;
    readPacketDesc(argv[2], packetSize, headerSize, measurementOrder, measurementSize); //get relevant info about the packet
    if(measurementSize == -1)
    {
        return -1;
    }
    cout << measurementSize << endl;
    //cout << headerSize << endl;
    vector<Packet> packetList; //data of files once read in
    printColorLn("done", GREEN);

    string fileName = ""; //current file
    string dir  = "";
    getline(fileList, dir);
    if(dir[dir.size()-1] != '/')
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
            if(currPacket.eof())
            {
            }
            else
            {
                printColorLn("failed to be read", RED);
            }
        }
        
        int charsRead = currPacket.gcount();
        cout << "Len: " << charsRead << endl;
        //add to the list
        Packet p = Packet(packetData, charsRead);
        packetList.push_back(p);
        printColorLn("done", GREEN);
        currPacket.close();
    }

    //close relavant streams
    fileList.close();

    //decompress each packet, and pull the measure reads out
    printColor("Decompressing and compiling data...", YELLOW);
    uint8_t decompressData[22000];
    vector< vector<uint8_t> > measurmentReads;
    for(int i = 0; i < packetList.size(); i++)
    {
        double check;
        //decompress the current packet
        Packet currPacket = packetList[i];
        int decompressSize = currPacket[0] + 256*currPacket[1];
        cout << "Decompress Size: " << decompressSize << endl;
        int dataOut = blz_depack(currPacket.getArrayAt(headerSize), decompressData, decompressSize);
        cout << dataOut << endl;
        if(dataOut != decompressSize)
        {
            printColorLn("decompression error", RED);
            cout << dataOut << " " << decompressSize << " " << i << endl;
            return -1;
        }

        if(dataOut % measurementSize != 0)
        {
            printColorLn("not mod measure size", RED);
            //return -1;
        }

        //iterate over the packet, pulling out each measure cyle into its own array
        for(int j = 0; j < decompressSize; j += measurementSize)
        {
            vector<uint8_t> currRead;
            for(int k = 0; k < measurementSize; k++)
            {
                currRead.push_back(decompressData[j+k]);
            }
            size_t loc = 0;
            check = getTime(currRead, loc);
            measurmentReads.push_back(currRead);
        }
        //
        //cout << check <<  endl;
        //cout << i << endl;
    }
    printColorLn("done", GREEN);

    printColor("Organizing measurment reads chronologically...", YELLOW);
    qsort(measurmentReads, sortMeasurement);
    printColorLn("done", GREEN);

    //create the csv
    printColorLn("Creating the CSV... ", YELLOW);
    ofstream commaList;
    commaList.open(argv[3]);

    //add Header
    //TODO: add numbers for duplicate sensors
    printColor("Adding Header...", YELLOW);
    for(int i = 0; i < measurementOrder.size(); i += 2)
    {
        switch(measurementOrder[i])
        {
            case '0': //TC
                CSVadd(commaList, "Temperature (Celcius)");
                break;
            case '1': //loacccel
                CSVadd(commaList, "Acceleration [X axis] (m/s^2)");
                CSVadd(commaList, "Acceleration [Y axis] (m/s^2)");
                CSVadd(commaList, "Acceleration [Z axis] (m/s^2)");
                break;
            case '2': //hiaccel
                CSVadd(commaList, "Acceleration [X axis] (m/s^2)");
                CSVadd(commaList, "Acceleration [Y axis] (m/s^2)");
                CSVadd(commaList, "Acceleration [Z axis] (m/s^2)");
                break;
            case '3': //mag
                // u03bc is the mu character in unicode
                CSVadd(commaList, "Magnetic Flux Density [X axis] (micro-Tesla)");
                CSVadd(commaList, "Magnetic Flux Density [Y axis] (micro-Tesla)");
                CSVadd(commaList, "Magnetic Flux Density [Z axis] (micro-Tesla)");
                break;
            case '4': //gyro
                CSVadd(commaList, "Rotation [X axis] (degrees/s)");
                CSVadd(commaList, "Rotation [Y axis] (degrees/s)");
                CSVadd(commaList, "Rotation [Z axis] (degrees/s)");
                break;
            case '5': //time
                CSVadd(commaList, "Time (s)");
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
    printColorLn("Analyzing sensor readings...", YELLOW);
    for(int i = 0; i < measurmentReads.size(); i++)
    {
        vector<uint8_t> currRead = measurmentReads[i];
        float readings[3];
        float reading;
        size_t loc = 0;
        for(int j  = 0; j < measurementOrder.size(); j += 2)
        {
            switch(measurementOrder[j])
            {
                case '0': //TC
                    reading = getTCReading(currRead, loc);
                    CSVadd(commaList, reading);
                    break;
                case '1': //loacccel
                    getLoAccelReadings(currRead, loc, readings);
                    for(int k = 0; k < 3; k++)
                    {
                        CSVadd(commaList, readings[k]);
                    }
                    break;
                case '2': //hiaccel
                    getHiAccelReadings(currRead, loc, readings);
                    for(int k = 0; k < 3; k++)
                    {
                        //cout << readings[k] << endl;
                        CSVadd(commaList, readings[k]);
                    }
                    break;
                case '3': //mag
                    getMagReadings(currRead, loc, readings);
                    for(int k = 0; k < 3; k++)
                    {
                        CSVadd(commaList, readings[k]);
                    }
                    break;
                case '4': //gyro
                    getGyroReadings(currRead, loc, readings);
                    for(int k = 0; k < 3; k++)
                    {
                        CSVadd(commaList, readings[k]);
                    }
                    break;
                case '5': //time
                    reading = getTime(currRead, loc);
                    CSVadd(commaList, reading);
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
        //cout << loc << endl;
    }

    printColorLn("done", GREEN);
    printColor("Saving file...", YELLOW);
    commaList.close();
    printColorLn("done", GREEN);
    return 0;
}


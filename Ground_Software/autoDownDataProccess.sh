#!/bin/bash

#three command line args, username and pass word for email and packet desc

GREEN='\033[1;32m'

BLUE='\033[1;34m'

RED='\033[1;31m'

NC='\033[0m'


TOTAL_ARGS=3
if [ $# -ne $TOTAL_ARGS ]
then
    echo -e "${RED}USAGE: $0 <Email Username> <Email Password> <Packet.desc file> $NC"
    exit
else
    echo -e "${BLUE}Downloading unread packets${NC}\n"
    ./PacketDownloader/UnreadDownload.py $1 $2 ./PacketDownloader/dwn -l
    echo -e "\n${GREEN}Done $NC"
    echo -e "${BLUE}Processing Data $NC\n"
    FILENAME=`date '+%d-%m-%Y-%H.%M.%S_%p'`.csv
    ./DataProccesor/bin/Ground_Software log.txt $3 $FILENAME
    echo -e "${GREEN}Done $NC\n"
    exit
fi

#!/usr/bin/env python3
__author__ = 'Collin'
import imaplib
import email
import sys
import os

'''
Needed Args:
username
password
download dir
'''

TOTAL_ARGS = 4
YELLOW = ("\033[1;33m")
GREEN = ("\033[1;32m")
RED = ("\033[1;31m")
NC = ("\033[0m")


def printColor(msg, color):
    print(color + msg + NC)


def main():
    if len(sys.argv) < TOTAL_ARGS: #check args 
        printColor("Correct Command Args: <username> <password> <download directory>", RED)
        exit(-1)
    #check for -l flag to decide if a log should be made or not
    if len(sys.argv) == 5 and sys.argv[4] == "-l":
    	fileIO = True;
    else:
    	fileIO = False;

    outDir = sys.argv[3] #arg three is the output file for the downloaded attachements

    if fileIO:
        fileList = open("log.txt", "w");
        currDirAppend = outDir #dir to add to the current directory
		                       #to lead data proccess to the downloaded files
        if outDir[0:2] == "./": #catch and remove leading periods
            currDirAppend = outDir[1:len(outDir)]
        fileList.write(os.getcwd() + currDirAppend + "\n"); #write the dir to the first line of the text file

    #Logging in
    m = imaplib.IMAP4_SSL('imap.mail.yahoo.com', 993)
    # args 1 and 2 are user name and password in plain text
    username = sys.argv[1]
    password = sys.argv[2]
    response = m.login(username, password)
    if response[0] == "OK":
        printColor("Logged in", GREEN)
    else:
        printColor("Failed to log in", RED)
        exit(-1)

    dir = str(sys.argv[3])
#   make sure the outDir has a trailing slash to put filenames directly on the end
    if dir[len(dir) - 1] != '/': 
        dir += '/'

    #choose inbox (i.e. unread emails)
    m.select()
    response = m.search(None, '(UNSEEN)')

    unreadEmails = str(response[1]).strip(" []b\'").split()

    if unreadEmails == []:
        printColor("No unread emails", YELLOW)

    for i in unreadEmails:
        # Downloads the packet from the current email
        response, data = m.fetch(i, "(UID BODY.PEEK[])")
        if response != 'OK':
            printColor("Failed to fetch message", RED)
            exit(-1)

        mail = email.message_from_bytes(data[0][1])
        id = str(data[0][0]).split()[2]
        for part in mail.walk():
            headerInfo = part.get('Content-Disposition')
            if str(headerInfo).split(';')[0] == 'attachment':
                attachment = part.get_payload(decode=True)
                filename = part.get_filename()
                printColor(filename, YELLOW)
                if(fileIO): fileList.write(filename + "\n"); #if logging print to file
                open(dir + filename, 'wb').write(attachment)
                m.uid('store', id, '+FLAGS', '(\\Seen)')

    # logging out procedure
    response = m.close()
    if response[0] == 'OK':
        printColor('Mailbox closed', GREEN)
    response = m.logout()
    if response[0] == 'BYE':
        printColor('Logged out', GREEN)

    if(fileIO): fileList.close(); #close fileIO

main()

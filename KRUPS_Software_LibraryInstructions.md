11:48 AM 1/7/202111:48 AM 1/7/2021to get KRUPS_Main.ino to run, had to: 

Use library manager to download 
- IridiumSBD.h
- elapsedMillis.h
- Adafruit_FONA.h
- MPU9250.h (included in KRUPS_SENSORS.h)


Ones I had to do weird things from github libraries folder to work: 
- Had to move "Compress" out of brieflz folder and rename to "breiflzCompress"

Even weirder steps to find: 
- PriorityQueue.h was in Collin Dietz's Github. Had to download from there. 
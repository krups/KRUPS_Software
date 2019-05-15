#ifndef KRUPS_IRIDIUM_H
#define KRUPS_IRIDIUM_H

#if OUTPUT_MESSAGES && DEBUG_IRIDIUM && USE_MODEM
void ISBDConsoleCallback(IridiumSBD *device, char c)
{
  Serial.write(c);
}

void ISBDDiagsCallback(IridiumSBD *device, char c)
{
  Serial.write(c);
}
#endif

#endif

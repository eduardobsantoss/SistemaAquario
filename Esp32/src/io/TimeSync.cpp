#include "io/TimeSync.h"
#include <time.h>

void syncTimeTZ(){
  const long GMT_OFFSET_SEC = -3 * 3600;
  configTime(GMT_OFFSET_SEC, 0, "pool.ntp.org", "time.nist.gov");
  time_t now = time(nullptr);
  Serial.print("Sincronizando NTP");
  int tries = 0;
  while (now < 1700000000 && tries < 60) { delay(250); Serial.print("."); now = time(nullptr); tries++; }
  Serial.println();
}

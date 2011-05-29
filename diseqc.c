/*
 * diseqc.c: DiSEqC handling
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: diseqc.c 2.5 2011/08/06 10:32:18 kls Exp $
 */

#include "diseqc.h"
#include <ctype.h>
#include "sources.h"
#include "thread.h"

// --- cDiseqc ---------------------------------------------------------------

cDiseqc::cDiseqc(void)
{
  devices = 0;
  source = 0;
  slof = 0;
  polarization = 0;
  lof = 0;
  commands = NULL;
  parsing = false;
  numCodes = 0;
  unicable = -1;
}

cDiseqc::~cDiseqc()
{
  free(commands);
}

bool cDiseqc::Parse(const char *s)
{
  bool result = false;
  char *sourcebuf = NULL;
  if (*s && s[strlen(s) - 1] == ':') {
     const char *p = s;
     while (*p && *p != ':') {
           char *t = NULL;
           int d = strtol(p, &t, 10);
           p = t;
           if (0 < d && d < 32)
              devices |= (1 << d - 1);
           else {
              esyslog("ERROR: invalid device number %d in '%s'", d, s);
              return false;
              }
           }
     return true;
     }
  int fields = sscanf(s, "%a[^ ] %d %c %d %a[^\n]", &sourcebuf, &slof, &polarization, &lof, &commands);
  if (fields == 4)
     commands = NULL; //XXX Apparently sscanf() doesn't work correctly if the last %a argument results in an empty string
  if (4 <= fields && fields <= 5) {
     source = cSource::FromString(sourcebuf);
     if (Sources.Get(source)) {
        polarization = char(toupper(polarization));
        if (polarization == 'V' || polarization == 'H' || polarization == 'L' || polarization == 'R') {
           parsing = true;
           const char *CurrentAction = NULL;
           while (Execute(&CurrentAction) != daNone)
                 ;
           parsing = false;
           result = !commands || !*CurrentAction;
           }
        else
           esyslog("ERROR: unknown polarization '%c'", polarization);
        }
     else
        esyslog("ERROR: unknown source '%s'", sourcebuf);
     }
  free(sourcebuf);
  return result;
}

const char *cDiseqc::Wait(const char *s) const
{
  char *p = NULL;
  errno = 0;
  int n = strtol(s, &p, 10);
  if (!errno && p != s && n >= 0) {
     if (!parsing)
        cCondWait::SleepMs(n);
     return p;
     }
  esyslog("ERROR: invalid value for wait time in '%s'", s - 1);
  return NULL;
}

const char *cDiseqc::Codes(const char *s) const
{
  const char *e = strchr(s, ']');
  if (e) {
     int NumCodes = 0;
     const char *t = s;
     while (t < e) {
           if (NumCodes < MaxDiseqcCodes) {
              errno = 0;
              char *p;
              int n = strtol(t, &p, 16);
              if (!errno && p != t && 0 <= n && n <= 255) {
                 if (!parsing)
                    codes[NumCodes] = uchar(n);
                 ++NumCodes;
                 t = skipspace(p);
                 }
              else {
                 esyslog("ERROR: invalid code at '%s'", t);
                 return NULL;
                 }
              }
           else {
              esyslog("ERROR: too many codes in code sequence '%s'", s - 1);
              return NULL;
              }
           }
     if (parsing)
        numCodes = NumCodes;
     return e + 1;
     }
  else
     esyslog("ERROR: missing closing ']' in code sequence '%s'", s - 1);
  return NULL;
}

const char *cDiseqc::Unicable(const char *s) const
{
  char *p = NULL;
  errno = 0;
  int n = strtol(s, &p, 10);
  if (!errno && p != s && n >= 0 && n < 8) {
     if (parsing)
        unicable = n;
     return p;
     }
  esyslog("ERROR: invalid unicable sat in '%s'", s - 1);
  return NULL;
}

unsigned int cDiseqc::UnicableFreq(unsigned int frequency, int satcr, unsigned int bpf) const
{
  unsigned int t = frequency == 0 ? 0 : (frequency + bpf + 2) / 4 - 350;
  if (t < 1024 && satcr >= 0 && satcr < 8)
  {
    codes[3] = t >> 8 | (t == 0 ? 0 : unicable << 2) | satcr << 5;
    codes[4] = t;
    return (t + 350) * 4 - frequency;
  }
  
  return 0;
}

void cDiseqc::UnicablePin(int pin) const
{
  if (pin >= 0 && pin <= 255) {
    numCodes = 6;
    codes[2] = 0x5c;
    codes[5] = pin;
    }
  else {
    numCodes = 5;
    codes[2] = 0x5a;
    }
}

cDiseqc::eDiseqcActions cDiseqc::Execute(const char **CurrentAction) const
{
  if (!*CurrentAction)
     *CurrentAction = commands;
  while (*CurrentAction && **CurrentAction) {
        switch (*(*CurrentAction)++) {
          case ' ': break;
          case 't': return daToneOff;
          case 'T': return daToneOn;
          case 'v': return daVoltage13;
          case 'V': return daVoltage18;
          case 'A': return daMiniA;
          case 'B': return daMiniB;
          case 'W': *CurrentAction = Wait(*CurrentAction); break;
          case '[': *CurrentAction = Codes(*CurrentAction); return *CurrentAction ? daCodes : daNone;
          case 'U': *CurrentAction = Unicable(*CurrentAction); return *CurrentAction ? daUnicable : daNone;
          default: return daNone;
          }
        }
  return daNone;
}

// --- cDiseqcs --------------------------------------------------------------

cDiseqcs Diseqcs;

const cDiseqc *cDiseqcs::Get(int Device, int Source, int Frequency, char Polarization) const
{
  int Devices = 0;
  for (const cDiseqc *p = First(); p; p = Next(p)) {
      if (p->Devices()) {
         Devices = p->Devices();
         continue;
         }
      if (Devices && !(Devices & (1 << Device - 1)))
         continue;
      if (p->Source() == Source && p->Slof() > Frequency && p->Polarization() == toupper(Polarization))
         return p;
      }
  return NULL;
}

// --- cUnicable --------------------------------------------------------------

cUnicable::cUnicable()
{
  satcr = -1;
  bpf = 0;
  pin = -1;
  unused = true;
}

bool cUnicable::Parse(const char *s)
{
  bool result = false;
  int fields = sscanf(s, "%d %u %d", &satcr, &bpf, &pin);
  if (fields >= 2) {
     if (satcr >= 0 && satcr < 8)
        result = true;
     else
        esyslog("Error: invalid unicable channel '%d'", satcr);
     if (result && fields == 3 && (pin < 0 || pin > 255)) {
        esyslog("Error: invalid unicable pin '%d'", pin);
        result = false;
        }
     }
  return result;
}


// --- cUnicables --------------------------------------------------------------

cUnicables Unicables;

cUnicable *cUnicables::GetUnused()
{
  for (cUnicable *p = First(); p; p = Next(p)) {
      if (p->Unused()) {
        p->Use();
        return p;
        }
      }
  return NULL;
}

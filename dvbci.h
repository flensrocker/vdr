/*
 * dvbci.h: Common Interface for DVB devices
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: dvbci.h 2.0 2006/11/26 11:19:42 kls Exp $
 */

#ifndef __DVBCI_H
#define __DVBCI_H

#include "ci.h"

class cDvbCiAdapter : public cCiAdapter {
private:
  cDevice *device;
  int fd;
  int adapter;
  int frontend;
  bool idle;

  bool OpenCa(void);
  void CloseCa(void);
protected:
  virtual int Read(uint8_t *Buffer, int MaxLength);
  virtual void Write(const uint8_t *Buffer, int Length);
  virtual bool Reset(int Slot);
  virtual eModuleStatus ModuleStatus(int Slot);
  virtual bool Assign(cDevice *Device, bool Query = false);
  cDvbCiAdapter(cDevice *Device, int Fd, int Adapter = -1, int Frontend = -1);
public:
  virtual ~cDvbCiAdapter();
  virtual bool SetIdle(bool Idle, bool TestOnly);
  virtual bool IsIdle(void) const { return idle; }
  static cDvbCiAdapter *CreateCiAdapter(cDevice *Device, int Fd, int Adapter = -1, int Frontend = -1);
  };

#endif //__DVBCI_H

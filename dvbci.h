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
  virtual cTSBuffer *GetTSBuffer(int FdDvr);
  virtual bool SetIdle(bool Idle, bool TestOnly);
  virtual bool IsIdle(void) const { return idle; }
  static int GetNumCamSlots(cDevice *Device, int Fd, cCiAdapter *CiAdapter);
   ///< Tests if the CA device is usable for vdr.
   ///< If CiAdapter is not NULL it will create the CamSlots for the given ci-adapter.
  static cDvbCiAdapter *CreateCiAdapter(cDevice *Device, int Fd, int Adapter = -1, int Frontend = -1);
  };

// A plugin that implements an external DVB ci-adapter derived from cDvbCiAdapter needs to create
// a cDvbCiAdapterProbe derived object on the heap in order to have its Probe()
// function called, where it can actually create the appropriate ci-adapter.
// The cDvbCiAdapterProbe object must be created in the plugin's constructor,
// and deleted in its destructor.
// Every plugin has to track its own list of already used device nodes.
// The Probes are always called if the base cDvbCiAdapter can't create a ci-adapter on its own.

class cDvbCiAdapterProbe : public cListObject {
public:
  cDvbCiAdapterProbe(void);
  virtual ~cDvbCiAdapterProbe();
  virtual cDvbCiAdapter *Probe(cDevice *Device) = 0;
     ///< Probes for a DVB ci-adapter for the given Device and creates the appropriate
     ///< object derived from cDvbCiAdapter if applicable.
     ///< Returns NULL if no adapter has been created.
  };

extern cList<cDvbCiAdapterProbe> DvbCiAdapterProbes;

#endif //__DVBCI_H

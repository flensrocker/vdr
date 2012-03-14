/*
 * dvbci.h: Common Interface for DVB devices
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: dvbci.c 2.0 2007/01/04 12:49:10 kls Exp $
 */

#include "dvbci.h"
#include <linux/dvb/ca.h>
#include <sys/ioctl.h>
#include "dvbdevice.h"

// --- cDvbCiAdapter ---------------------------------------------------------

cDvbCiAdapter::cDvbCiAdapter(cDevice *Device, int Fd, int Adapter, int Frontend)
{
  device = Device;
  SetDescription("CI adapter on device %d", device->DeviceNumber());
  fd = Fd;
  adapter = Adapter;
  frontend = Frontend;
  idle = false;
  GetNumCamSlots(Device, Fd, this);
  Start();
}

cDvbCiAdapter::~cDvbCiAdapter()
{
  Cancel(3);
  if (device->IsSubDevice() || device->HasSubDevice())
     CloseCa();
}

bool cDvbCiAdapter::OpenCa(void)
{
  if (fd >= 0)
     return true;
  fd = cDvbDevice::DvbOpen(DEV_DVB_CA, adapter, frontend, O_RDWR);
  return (fd >= 0);
}

void cDvbCiAdapter::CloseCa(void)
{
  if (fd < 0)
     return;
  close(fd);
  fd = -1;
}

bool cDvbCiAdapter::SetIdle(bool Idle, bool TestOnly)
{
  if ((adapter < 0) || (frontend < 0))
     return false;
  if (TestOnly || (idle == Idle))
     return true;
  if (Idle)
     CloseCa();
  else
     OpenCa();
  idle = Idle;
  return true;
}

cTSBufferBase *cDvbCiAdapter::GetTSBuffer(int FdDvr)
{
  if (device)
     return new cTSBuffer(FdDvr, MEGABYTE(2), device->CardIndex() + 1);
  return NULL;
}

int cDvbCiAdapter::Read(uint8_t *Buffer, int MaxLength)
{
  if (idle || (fd < 0))
     return 0;
  if (Buffer && MaxLength > 0) {
     struct pollfd pfd[1];
     pfd[0].fd = fd;
     pfd[0].events = POLLIN;
     if (poll(pfd, 1, CAM_READ_TIMEOUT) > 0 && (pfd[0].revents & POLLIN)) {
        int n = safe_read(fd, Buffer, MaxLength);
        if (n >= 0)
           return n;
        esyslog("ERROR: can't read from CI adapter on device %d: %m", device->DeviceNumber());
        }
     }
  return 0;
}

void cDvbCiAdapter::Write(const uint8_t *Buffer, int Length)
{
  if (idle || (fd < 0))
     return;
  if (Buffer && Length > 0) {
     if (safe_write(fd, Buffer, Length) != Length)
        esyslog("ERROR: can't write to CI adapter on device %d: %m", device->DeviceNumber());
     }
}

bool cDvbCiAdapter::Reset(int Slot)
{
  if (idle || (fd < 0))
     return false;
  if (ioctl(fd, CA_RESET, 1 << Slot) != -1)
     return true;
  else
     esyslog("ERROR: can't reset CAM slot %d on device %d: %m", Slot, device->DeviceNumber());
  return false;
}

eModuleStatus cDvbCiAdapter::ModuleStatus(int Slot)
{
  if (idle || (fd < 0))
     return msNone;
  ca_slot_info_t sinfo;
  sinfo.num = Slot;
  if (ioctl(fd, CA_GET_SLOT_INFO, &sinfo) != -1) {
     if ((sinfo.flags & CA_CI_MODULE_READY) != 0)
        return msReady;
     else if ((sinfo.flags & CA_CI_MODULE_PRESENT) != 0)
        return msPresent;
     }
  else
     esyslog("ERROR: can't get info of CAM slot %d on device %d: %m", Slot, device->DeviceNumber());
  return msNone;
}

bool cDvbCiAdapter::Assign(cDevice *Device, bool Query)
{
  // The CI is hardwired to its device, so there's not really much to do here
  if (Device)
     return Device == device;
  return true;
}

int cDvbCiAdapter::GetNumCamSlots(cDevice *Device, int Fd, cCiAdapter *CiAdapter)
{
  int NumSlots = -1;
  if (Fd >= 0) {
     ca_caps_t Caps;
     if (ioctl(Fd, CA_GET_CAP, &Caps) == 0) {
        if ((Caps.slot_type & CA_CI_LINK) != 0) {
           NumSlots = Caps.slot_num;
           if (NumSlots == 0)
              esyslog("ERROR: no CAM slots found on device %d", Device->DeviceNumber());
           else if (CiAdapter != NULL) {
              for (int i = 0; i < NumSlots; i++)
                  new cCamSlot(CiAdapter);
              }
           else
              return NumSlots;
           }
        else
           isyslog("device %d doesn't support CI link layer interface", Device->DeviceNumber());
        }
     else
        esyslog("ERROR: can't get CA capabilities on device %d", Device->DeviceNumber());
     }
  return -1;
}

cDvbCiAdapter *cDvbCiAdapter::CreateCiAdapter(cDevice *Device, int Fd, int Adapter, int Frontend)
{
  // don't create a ci-adapter if it's not useable
  if (Device && (Fd >= 0) && (GetNumCamSlots(Device, Fd, NULL) > 0))
     return new cDvbCiAdapter(Device, Fd, Adapter, Frontend);

  if (Fd >= 0)
     close(Fd);

  // try to find an external ci-adapter
  for (cDvbCiAdapterProbe *cp = DvbCiAdapterProbes.First(); cp; cp = DvbCiAdapterProbes.Next(cp)) {
      cDvbCiAdapter *ca = cp->Probe(Device);
      if (ca)
         return ca;
      }
 return NULL;
}

// --- cDvbCiAdapterProbe -------------------------------------------------------

cList<cDvbCiAdapterProbe> DvbCiAdapterProbes;

cDvbCiAdapterProbe::cDvbCiAdapterProbe(void)
{
  DvbCiAdapterProbes.Add(this);
}

cDvbCiAdapterProbe::~cDvbCiAdapterProbe()
{
  DvbCiAdapterProbes.Del(this, false);
}

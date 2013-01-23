/*
 * transfer.c: Transfer mode
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: transfer.c 2.7 2013/01/20 13:40:30 kls Exp $
 */

#include "transfer.h"

// --- cTransfer -------------------------------------------------------------

cTransfer::cTransfer(const cChannel *Channel)
:cReceiver(Channel, TRANSFERPRIORITY)
{
  patPmtGenerator.SetChannel(Channel);
  retriesExceeded = false;
}

cTransfer::~cTransfer()
{
  cReceiver::Detach();
  cPlayer::Detach();
}

void cTransfer::Activate(bool On)
{
  if (On) {
     PlayTs(patPmtGenerator.GetPat(), TS_SIZE);
     int Index = 0;
     while (uchar *pmt = patPmtGenerator.GetPmt(Index))
           PlayTs(pmt, TS_SIZE);
     }
  else
     cPlayer::Detach();
}

#define MAXRETRIES     5 // max. number of retries for a single TS packet
#define RETRYWAIT      5 // time (in ms) between two retries
#define RETRYPAUSE 10000 // time (in ms) for which to pause retrying once a packet has not been accepted

void cTransfer::Receive(uchar *Data, int Length)
{
  if (cPlayer::IsAttached()) {
     // Transfer Mode means "live tv", so there's no point in doing any additional
     // buffering here. The TS packets *must* get through here! However, every
     // now and then there may be conditions where the packet just can't be
     // handled when offered the first time, so that's why we try several times:
     for (int i = 0; i < MAXRETRIES; i++) {
         if (PlayTs(Data, Length) > 0) {
            if (retriesExceeded && timer.TimedOut())
               retriesExceeded = false;
            return;
            }
         if (retriesExceeded) // no retries once a packet has not been accepted
            return;
         cCondWait::SleepMs(RETRYWAIT);
         }
     retriesExceeded = true;
     timer.Set(RETRYPAUSE); // wait a while before retrying
     esyslog("ERROR: TS packet not accepted in Transfer Mode");
     }
}

// --- cTransferControl ------------------------------------------------------

cDevice *cTransferControl::receiverDevice = NULL;

cTransferControl::cTransferControl(cDevice *ReceiverDevice, const cChannel *Channel)
:cControl(transfer = new cTransfer(Channel), true)
{
  ReceiverDevice->AttachReceiver(transfer);
  receiverDevice = ReceiverDevice;
}

cTransferControl::~cTransferControl()
{
  receiverDevice = NULL;
  delete transfer;
}

/*
 * args.c: Read arguments from files
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: $
 */

#include "args.h"
#include <unistd.h>

cArgs::cArgs(const char *Argv0)
{
  argv0 = Argv0;
  argc = 0;
  argv = NULL;
}

cArgs::~cArgs(void)
{
  if (argv != NULL)
     delete [] argv;
}

bool cArgs::AddArg(const char *s)
{
  if (inVdrSection)
     args.Append(strdup(s));
  else if (*lastArg == NULL)
     return false;
  else
     lastArg = cString::sprintf("%s %s", *lastArg, s);
  return true;
}

bool cArgs::ReadDirectory(const char *Directory)
{
  if (argv != NULL)
     delete [] argv;
  argc = 0;
  argv = NULL;
  args.Clear();
  lastArg = NULL;
  inVdrSection = false;

  cFileNameList files(Directory, false);
  if (files.Size() == 0)
     return false;

  for (int i = 0; i < files.Size(); i++) {
      cString fileName = AddDirectory(Directory, files.At(i));
      struct stat fs;
      if ((access(*fileName, F_OK) != 0) || (stat(*fileName, &fs) != 0) || S_ISDIR(fs.st_mode))
         continue;

      bool ok = true;
      int line = 0;
      FILE *f = fopen(*fileName, "r");
      if (f) {
         char *s;
         cReadLine ReadLine;
         while ((s = ReadLine.Read(f)) != NULL) {
               line++;
               s = stripspace(skipspace(s));
               if (!isempty(s) && (s[0] != '#')) {
                  if (startswith(s, "[") && endswith(s, "]")) {
                     s[strlen(s) - 1] = 0;
                     s++;

                     if (*lastArg) {
                        args.Append(strdup(*lastArg));
                        lastArg = 0;
                        }

                     if (strcmp(s, "vdr") == 0)
                        inVdrSection = true;
                     else {
                        inVdrSection = false;
                        args.Append(strdup("-P"));
                        lastArg = s;
                        }
                     }
                  else {
                     if ((strlen(s) > 2) && (s[0] == '-') && (s[1] != '-')) { // short option, split at first space
                        char *p = strchr(s, ' ');
                        if (p == NULL) {
                           ok = AddArg(s);
                           if (!ok)
                              break;
                           }
                        else {
                           *p = 0;
                           p++;
                           ok = AddArg(s);
                           if (!ok)
                              break;
                           ok = AddArg(p);
                           if (!ok)
                              break;
                           }
                        }
                     else {
                        ok = AddArg(s);
                        if (!ok)
                           break;
                        }
                     }
                  }
               }
         fclose(f);
         }
       if (!ok) {
          esyslog("ERROR: args file %s, line %d", *fileName, line);
          return false;
          }
      }
  if (*lastArg)
     args.Append(strdup(*lastArg));

  argv = new char*[args.Size() + 1];
  argv[0] = strdup(*argv0);
  argc = 1;
  for (int i = 0; i < args.Size(); i++) {
      argv[argc] = args.At(i);
      argc++;
      }
  return true;
}


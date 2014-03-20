/*
 * args.h: Read arguments from files
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: $
 */

#ifndef __ARGS_H
#define __ARGS_H

#include "tools.h"

class cArgs {
private:
  cString argv0;
  int     argc;
  char  **argv;

public:
  cArgs(const char *Argv0);
  ~cArgs(void);

  bool ReadDirectory(const char *Directory);

  int GetArgc(void) const;
  char **GetArgv(void) const;
  };

#endif //__ARGS_H

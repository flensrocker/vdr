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

class cArgsSection : public cListObject {
private:
  cString name;
  cStringList args;

public:
  cArgsSection(const char *Name)
  {
    name = Name;
  };
  
  virtual ~cArgsSection(void)
  {
  };

  void AddArg(const char *Arg)
  {
    if (Arg != NULL)
       args.Append(strdup(Arg));
  };

  const char *Name(void)
  {
    return *name;
  };

  const cStringList &Args(void)
  {
    return args;
  };
};

class cArgsSections : public cList<cArgsSection> {
public:
  cArgsSection *FindSection(const char *Name)
  {
    if (Name == NULL)
       return NULL;

    cArgsSection *s = First();
    while ((s != NULL) && (strcmp(s->Name(), Name) != 0))
          s = Next(s);
    return s;
  };
};

cArgs::cArgs(const char *Argv0)
{
  argv0 = Argv0;
  argc = 0;
  argv = NULL;
}

cArgs::~cArgs(void)
{
  if (argv != NULL) {
     for (int i = 0; i < argc; i++)
         free(argv[i]);
     delete [] argv;
     }
}

bool cArgs::ReadDirectory(const char *Directory)
{
  if (argv != NULL) {
     for (int i = 0; i < argc; i++)
         free(argv[i]);
     delete [] argv;
     }
  argc = 0;
  argv = NULL;

  cFileNameList files(Directory, false);
  if (files.Size() == 0)
     return false;

  bool result = true;
  cArgsSections sections;
  cArgsSection *vdrSection = NULL;
  cArgsSection *currentSection = NULL;
  for (int i = 0; i < files.Size(); i++) {
      cString fileName = AddDirectory(Directory, files.At(i));
      struct stat fs;
      if ((access(*fileName, F_OK) != 0) || (stat(*fileName, &fs) != 0) || S_ISDIR(fs.st_mode))
         continue;

      FILE *f = fopen(*fileName, "r");
      if (f) {
         char *s;
         int line = 0;
         cReadLine ReadLine;
         while ((s = ReadLine.Read(f)) != NULL) {
               line++;
               s = stripspace(skipspace(s));
               if (!isempty(s) && (s[0] != '#')) {
                  if (startswith(s, "[") && endswith(s, "]")) {
                     s[strlen(s) - 1] = 0;
                     s++;
                     if (strcmp(s, "vdr") == 0) {
                        currentSection = sections.FindSection(s);
                        if (currentSection == NULL) {
                           vdrSection = currentSection = new cArgsSection(s);
                           sections.Add(currentSection);
                           }
                        }
                     else {
                        currentSection = new cArgsSection(s);
                        sections.Add(currentSection);
                        }
                     }
                  else {
                     if (!startswith(s, "-")) {
                        result = false;
                        esyslog("ERROR: args file %s, line %d must start with a hyphen (%s)", *fileName, line, s);
                        break;
                        }

                     if (currentSection == NULL) {
                        result = false;
                        esyslog("ERROR: args file %s, line %d must start with a section", *fileName, line);
                        break;
                        }

                     if ((strlen(s) > 2) && (s[1] != '-')) { // short option, split at first space
                        char *p = strchr(s, ' ');
                        if (p == NULL)
                           currentSection->AddArg(s);
                        else {
                           *p = 0;
                           p++;
                           currentSection->AddArg(s);
                           currentSection->AddArg(p);
                           }
                        }
                     else
                        currentSection->AddArg(s);
                     }
                  }
               }
         fclose(f);
         }
       if (!result)
          return false;
      }

  argc = 1; // for argv0
  if (vdrSection != NULL) {
     argc += vdrSection->Args().Size();
     argc += sections.Count() - 1;
     }
  else
     argc += sections.Count();

  argv = new char*[argc];
  argv[0] = strdup(*argv0);
  int c = 1;
  if (vdrSection != NULL) {
     for (int i = 0; i < vdrSection->Args().Size(); i++) {
         argv[c] = strdup(vdrSection->Args().At(i));
         c++;
         }
     }
  for (cArgsSection *s = sections.First(); s; s = sections.Next(s)) {
      if (s == vdrSection)
         continue;
      cString arg = cString::sprintf("'-P%s", s->Name());
      for (int i = 0; i < s->Args().Size(); i++)
          arg = cString::sprintf("%s %s", *arg, s->Args().At(i));
      arg = cString::sprintf("%s'", *arg);
      argv[c] = strdup(*arg);
      c++;
      }
  return result;
}
  
int cArgs::GetArgc(void) const
{
  return argc;
}

char **cArgs::GetArgv(void) const
{
  return argv;
}

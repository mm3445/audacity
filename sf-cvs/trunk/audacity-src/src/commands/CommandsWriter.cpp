/**********************************************************************

  Audacity: A Digital Audio Editor

  CommandsWriter.cpp

  Brian Gunlogson

  This class handles writing the default commands.cfg.

**********************************************************************/

#include "CommandsWriter.h"
#include "CommandsCfg.h"

#include <wx/file.h>
#include <wx/msgdlg.h>

//BG: Define the default commands XML
#define AUDACITY_COMMANDS_DEFAULT_XML
#include "CommandsDefaultData.h"
#undef AUDACITY_COMMANDS_DEFAULT_XML

CommandsWriter::CommandsWriter()
{
}

CommandsWriter::~CommandsWriter()
{
}

bool CommandsWriter::WriteXML()
{
   //remove existing file
   if(wxFileExists(gCommandsCfgLocation))
      wxRemoveFile(gCommandsCfgLocation);

   //open file
   FILE *fp = fopen(gCommandsCfgLocation, "wb");
   if(!fp || ferror(fp))
   {
      wxMessageBox("Couldn't write to file: " + gCommandsCfgLocation);
      return false;
   }

   for(int i = 0; ; i++)
   {
      wxString currentLine(CommandsDefaultXML[i]);

      if(!currentLine.Cmp("//EOF//"))
         break;

      fprintf(fp, "%s\n", currentLine.c_str());
   }

   fclose(fp);

   return true;
}

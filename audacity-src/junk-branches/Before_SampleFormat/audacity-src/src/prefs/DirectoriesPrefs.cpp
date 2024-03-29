/**********************************************************************

  Audacity: A Digital Audio Editor

  DirectoriesPrefs.cpp

  Joshua Haberman

**********************************************************************/

#include <math.h>

#include <wx/button.h>
#include <wx/dirdlg.h>
#include <wx/event.h>
#include <wx/filefn.h>
#include <wx/intl.h>
#include <wx/log.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/utils.h>

#include "../Prefs.h"
#include "../DiskFunctions.h"
#include "DirectoriesPrefs.h"

enum {
   TempDirID = 1000,
   ChooseButtonID
};

BEGIN_EVENT_TABLE(DirectoriesPrefs, wxPanel)
   EVT_TEXT(TempDirID, DirectoriesPrefs::UpdateFreeSpace)
   EVT_BUTTON(ChooseButtonID, DirectoriesPrefs::OnChooseTempDir)
END_EVENT_TABLE()

DirectoriesPrefs::DirectoriesPrefs(wxWindow * parent):
PrefsPanel(parent)
{
   mTempDir = gPrefs->Read("/Directories/TempDir", "");
   mOldTempDir = mTempDir;

   topSizer = new wxStaticBoxSizer(
      new wxStaticBox(this, -1, _("Directories")), wxVERTICAL );

   wxStaticBoxSizer *tempDirSizer = new wxStaticBoxSizer(
      new wxStaticBox(this, -1, _("Temp. Directory")), wxVERTICAL );

   wxFlexGridSizer *tempDirGridSizer = new wxFlexGridSizer( 0, 3, 0, 0 );

   mTempDirLabel = new wxStaticText(
      this, -1, _("Location:"), wxDefaultPosition,
      wxDefaultSize, wxALIGN_RIGHT );

   /* Order is important here: mFreeSpace must be allocated before
      mTempDirText, so that the handler doesn't try to operate on
      mFreeSpace before it exists! */
   mFreeSpace = new wxStaticText(
      this, -1, FormatSize(GetFreeDiskSpace((char *) (const char *) mTempDir)),
      wxDefaultPosition, wxDefaultSize, 0 );

   mTempDirText = NULL;
   mTempDirText = new wxTextCtrl(
      this, TempDirID, mTempDir,
      wxDefaultPosition, wxSize(160, -1), 0 );

   mFreeSpaceLabel = new wxStaticText(
      this, -1, _("Free Space:"),
      wxDefaultPosition, wxDefaultSize, 0 );

   wxButton *chooseButton =
      new wxButton(this, ChooseButtonID, _("Choose..."));
     
   tempDirGridSizer->Add( mTempDirLabel, 0, wxALIGN_LEFT|wxALL|wxALIGN_CENTER_VERTICAL, 2 );
   tempDirGridSizer->Add( mTempDirText, 1, wxGROW|wxALL|wxALIGN_CENTER_VERTICAL, 2 );
   tempDirGridSizer->Add( chooseButton, 0, wxALL|wxALIGN_CENTER_VERTICAL, 2 );

   tempDirGridSizer->Add( mFreeSpaceLabel, 0, wxALIGN_LEFT|wxALL|wxALIGN_CENTER_VERTICAL, 2 );
   tempDirGridSizer->Add( mFreeSpace, 0, wxGROW|wxALL|wxALIGN_CENTER_VERTICAL, 2 );
   tempDirGridSizer->AddGrowableCol(1);
   tempDirSizer->Add( tempDirGridSizer, 0, wxGROW|wxALL, 2 );

   topSizer->Add( tempDirSizer, 0, wxGROW|wxALL, 5 );

   SetAutoLayout(true);
   SetSizer(topSizer);

   topSizer->Fit(this);
   topSizer->SetSizeHints(this);
}

wxString DirectoriesPrefs::FormatSize(wxLongLong size)
{
   wxString sizeStr;

   /* wxLongLong contains no built-in conversion to double */
   double dSize = size.GetHi() * pow(2, 32);  // 2 ^ 32
   dSize += size.GetLo();

   if (size == -1L)
      sizeStr = _("Unable to determine");
   else {
      /* make it look nice, by formatting into k, MB, etc */
      if (size < 1024)
         sizeStr.sprintf("%ld bytes", size.GetLo());
      else if (size < 1024 * 1024) {
         sizeStr.sprintf("%.1f kB", dSize / 1024);
      }
      else if (size < 1024 * 1024 * 1024) {
         sizeStr.sprintf("%.1f MB", dSize / (1024 * 1024));
      }
      else {
         sizeStr.sprintf("%.1f GB", dSize / (1024 * 1024 * 1024));
      }
   }

   return sizeStr;
}

void DirectoriesPrefs::OnChooseTempDir(wxCommandEvent &event)
{
   wxDirDialog dlog(this, _("Choose a location to place the "
                            "temporary directory"), "");
   dlog.ShowModal();
   if (dlog.GetPath() != "") {
      mTempDirText->SetValue(dlog.GetPath() +
                             wxFILE_SEP_PATH +
                             ".audacity_temp");
      UpdateFreeSpace(event);
   }
}

void DirectoriesPrefs::UpdateFreeSpace(wxCommandEvent &event)
{
   static wxLongLong space;
   static wxString tempDir;
   static char tmp[200];

   if (!mTempDirText)
      return;

   tempDir = mTempDirText->GetValue();

#ifndef __WXMAC__  // the mac GetFreeDiskSpace routine does this automatically
   /* Try to be smart: if the directory doesn't exist, go up the
    * directory path until one is, because that's the volume that
    * the new directory would be created on */
   while(!wxDirExists(tempDir) && tempDir.Find(wxFILE_SEP_PATH) != -1)
      tempDir = tempDir.BeforeLast(wxFILE_SEP_PATH);
#endif
   strncpy(tmp, tempDir.c_str(), 200);
   space = GetFreeDiskSpace(tmp);
   
   mFreeSpace->SetLabel(FormatSize(space));
}
   
bool DirectoriesPrefs::Apply()
{
   
   mTempDir = mTempDirText->GetValue();

   if(!wxDirExists(mTempDir)) {
      int ans = wxMessageBox(
            wxString::Format(_("Directory %s does not exist. Create it?"),
                             (const char *) mTempDir),
            _("New Temporary Directory"),
            wxYES_NO|wxCENTRE|wxICON_EXCLAMATION);

      if(ans == wxYES) {
         if(!wxMkdir(mTempDir, 0600)) {
            /* wxWindows throws up a decent looking dialog */
            return false;
         }
      } 
      else {
         return false;
      }
   }
   else {
      /* If the directory already exists, make sure it is writable */
      wxLogNull logNo;
      wxString tempDir = mTempDir + wxFILE_SEP_PATH + "canicreate";
      if(!wxMkdir(tempDir)) {
         wxMessageBox(
               wxString::Format(_("Directory %s is not writable"),
                                (const char *) mTempDir),
               _("Error"), wxOK|wxICON_ERROR);
         return false;
      }
      wxRmdir(tempDir);
   }

   gPrefs->Write("/Directories/TempDir", mTempDir);

   if (mTempDir != mOldTempDir)
      wxMessageBox(
            _("Changes to temporary directory will not take effect "
              "until Audacity is restarted"),
            "Temp Directory Update", wxOK|wxCENTRE|wxICON_INFORMATION);

   return true;
}

DirectoriesPrefs::~DirectoriesPrefs()
{
}

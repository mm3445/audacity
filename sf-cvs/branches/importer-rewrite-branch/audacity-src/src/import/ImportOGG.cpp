/**********************************************************************

  Audacity: A Digital Audio Editor

  ImportOGG.cpp

  Joshua Haberman

  The Ogg format supports multiple logical bitstreams that can be chained
  within the physical bitstream. The sampling rate and number of channels
  can vary between these logical bitstreams. For the moment, we'll ignore
  all but the first logical bitstream.

  Ogg also allows for an arbitrary number of channels. Luckily, so does
  Audacity. We'll call the first channel LeftChannel, the second
  RightChannel, and all others after it MonoChannel.

**********************************************************************/

#include "../Audacity.h"
#include "ImportOGG.h"

#ifndef USE_LIBVORBIS

void GetOGGImportPlugin(ImportPluginList *importPluginList,
                        UnusableImportPluginList *unusableImportPluginList)
{
   UnusableImportPlugin* oggIsUnsupported =
      new UnusableImportPlugin("Ogg Vorbis",
                               wxStringList("ogg", NULL));

   unusableImportPluginList->Append(oggIsUnsupported);
}

#else /* USE_LIBVORBIS */

#include <wx/string.h>
#include <wx/utils.h>
#include <wx/intl.h>
/* ffile.h must be included AFTER at least one other wx header that includes
 * wx/setup.h, otherwise #ifdefs erronously collapse it to nothing. This is
 * a bug in wxWindows (ffile.h should itself include wx/setup.h), and it 
 * was a bitch to track down. */
#include <wx/ffile.h>

#include <vorbis/vorbisfile.h>

#include "../WaveTrack.h"

class OggImportPlugin : public ImportPlugin
{
public:
   OggImportPlugin():
      ImportPlugin(wxStringList("ogg", NULL)),
      mProgressCallback(NULL),
      mUserData(NULL)
   {
   }

   ~OggImportPlugin() { }

   wxString GetPluginFormatDescription();
   bool Open(wxString Filename);
   void SetProgressCallback(progress_callback_t *function,
                            void *userData);
   wxString GetFileDescription();
   int GetFileUncompressedBytes();
   bool Import(TrackFactory *trackFactory, Track ***outTracks,
               int *outNumTracks);
   void Close();
private:
   wxFFile mFile;
   OggVorbis_File mVorbisFile;
   progress_callback_t *mProgressCallback;
   void *mUserData;
};

void GetOGGImportPlugin(ImportPluginList *importPluginList,
                        UnusableImportPluginList *unusableImportPluginList)
{
   importPluginList->Append(new OggImportPlugin);
}

wxString OggImportPlugin::GetPluginFormatDescription()
{
    return "Ogg Vorbis";
}

bool OggImportPlugin::Open(wxString filename)
{

   mFile.Open(filename, "rb");

   if (!mFile.IsOpened()) {
      // No need for a message box, it's done automatically (but how?)
      return false;
   }

   int err = ov_open(mFile.fp(), &mVorbisFile, NULL, 0);

   if (err < 0) {
      wxString message;

      switch (err) {
         case OV_EREAD:
            message = _("Media read error");
            break;
         case OV_ENOTVORBIS:
            message = _("Not an Ogg Vorbis file");
            break;
         case OV_EVERSION:
            message = _("Vorbis version mismatch");
            break;
         case OV_EBADHEADER:
            message = _("Invalid Vorbis bitstream header");
            break;
         case OV_EFAULT:
            message = _("Internal logic fault");
            break;
      }

      // what to do with message?
      mFile.Close();
      return false;
   }

   return true;
}

void OggImportPlugin::SetProgressCallback(progress_callback_t progressCallback,
                                      void *userData)
{
   mProgressCallback = progressCallback;
   mUserData = userData;
}

wxString OggImportPlugin::GetFileDescription()
{
   return "Ogg Vorbis";
}

int OggImportPlugin::GetFileUncompressedBytes()
{
   // TODO:
   return 0;
}

bool OggImportPlugin::Import(TrackFactory *trackFactory, Track ***outTracks,
                         int *outNumTracks)
{
   wxASSERT(mFile.IsOpened());

   /* -1 is for the current logical bitstream */
   vorbis_info *vi = ov_info(&mVorbisFile, -1);

   *outNumTracks = vi->channels;
   WaveTrack **channels = new WaveTrack *[*outNumTracks];

   int c;
   for (c = 0; c < *outNumTracks; c++) {
      channels[c] = trackFactory->NewWaveTrack(int16Sample);
      channels[c]->SetRate(vi->rate);

      switch (c) {
         case 0:
            channels[c]->SetChannel(Track::LeftChannel);
            break;
         case 1:
            channels[c]->SetChannel(Track::RightChannel);
            break;
         default:
            channels[c]->SetChannel(Track::MonoChannel);
      }
   }

   if (*outNumTracks == 2)
      channels[0]->SetLinked(true);

/* The number of bytes to get from the codec in each run */
#define CODEC_TRANSFER_SIZE 4096

   const int bufferSize = 1048576;
   short *mainBuffer = new short[CODEC_TRANSFER_SIZE];

   short **buffers = new short *[*outNumTracks];
   for (int i = 0; i < *outNumTracks; i++) {
      buffers[i] = new short[bufferSize];
   }

   /* determine endianness (clever trick courtesy of Nicholas Devillard,
    * (http://www.eso.org/~ndevilla/endian/) */
   int testvar = 1, endian;
   if(*(char *)&testvar)
      endian = 0;  // little endian
   else
      endian = 1;  // big endian

   /* number of samples currently in each channel's buffer */
   int bufferCount = 0;
   bool cancelled = false;
   long bytesRead = 0;
   long samplesRead = 0;
   int bitstream = 0;

   do {
      bytesRead = ov_read(&mVorbisFile, (char *) mainBuffer,
                          CODEC_TRANSFER_SIZE,
                          endian,
                          2,    // word length (2 for 16 bit samples)
                          1,    // signed
                          &bitstream);
      samplesRead = bytesRead / *outNumTracks / sizeof(short);

      if (samplesRead + bufferCount > bufferSize) {
         for (c = 0; c < *outNumTracks; c++)
            channels[c]->Append((samplePtr)buffers[c],
                                   int16Sample,
                                   bufferCount);
         bufferCount = 0;
      }

      /* Un-interleave */
      for (int s = 0; s < samplesRead; s++)
         for (c = 0; c < *outNumTracks; c++)
            buffers[c][s + bufferCount] =
                mainBuffer[s * (*outNumTracks) + c];

      bufferCount += samplesRead;

      if( mProgressCallback )
         cancelled = mProgressCallback(mUserData,
                                       ov_time_tell(&mVorbisFile) /
                                       ov_time_total(&mVorbisFile, bitstream));

   } while (!cancelled && bytesRead != 0 && bitstream == 0);

   /* ...the rest is de-allocation */
   delete[]mainBuffer;

   for (c = 0; c < *outNumTracks; c++)
      delete[]buffers[c];
   delete[]buffers;

   if (cancelled) {
      for (c = 0; c < *outNumTracks; c++)
         delete channels[c];
      delete[] channels;

      return false;
   }
   else {
      *outTracks = new Track *[*outNumTracks];
      for(c = 0; c < *outNumTracks; c++)
         (*outTracks)[c] = channels[c];
      delete[] channels;

      return true;
   }
}

void OggImportPlugin::Close()
{
   ov_clear(&mVorbisFile);
   mFile.Detach();    // so that it doesn't try to close the file (ov_clear()
                      // did that already)
}

#endif                          /* USE_LIBVORBIS */

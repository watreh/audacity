/**********************************************************************
 
 Audacity: A Digital Audio Editor
 
 CrashReport.cpp
 
 *//*******************************************************************/


#include "CrashReport.h"

#if defined(HAS_CRASH_REPORT)
#include <atomic>
#include <thread>

#if defined(__WXMSW__)
#include <wx/evtloop.h>
#endif

#include "wxFileNameWrapper.h"
#include "AudacityLogger.h"
#include "AudioIOBase.h"
#include "BasicUI.h"
#include "FileNames.h"
#include "Internat.h"
#include "Languages.h"
#include "Project.h"
#include "ProjectFileIO.h"
#include "prefs/GUIPrefs.h"
#include "widgets/AudacityTextEntryDialog.h"

namespace CrashReport {

void Generate(wxDebugReport::Context ctx)
{
   wxDebugReportCompress rpt;

   // Bug 1927: Seems there problems with wxStackWalker, so don't even include
   // the stack trace or memory dump. The former is pretty much useless in Release
   // builds anyway and none of use have the skill/time/desire to fiddle with the
   // latter.
   // rpt.AddAll(ctx);

   {
      // Provides a progress dialog with indeterminate mode
      using namespace BasicUI;
      auto pd = MakeGenericProgress({},
         XO("Audacity Support Data"), XO("This may take several seconds"));
      wxASSERT(pd);

      std::atomic_bool done = {false};
      auto thread = std::thread([&]
      {
         wxFileNameWrapper fn{ FileNames::DataDir(), wxT("audacity.cfg") };
         rpt.AddFile(fn.GetFullPath(), _TS("Audacity Configuration"));
         rpt.AddFile(FileNames::PluginRegistry(), wxT("Plugin Registry"));
         rpt.AddFile(FileNames::PluginSettings(), wxT("Plugin Settings"));
   
         if (ctx == wxDebugReport::Context_Current)
         {
            auto saveLang = Languages::GetLangShort();
            GUIPrefs::SetLang( wxT("en") );
            auto cleanup = finally( [&]{ GUIPrefs::SetLang( saveLang ); } );
      
            auto gAudioIO = AudioIOBase::Get();
            rpt.AddText(wxT("audiodev.txt"), gAudioIO->GetDeviceInfo(), wxT("Audio Device Info"));
#ifdef EXPERIMENTAL_MIDI_OUT
            rpt.AddText(wxT("mididev.txt"), gAudioIO->GetMidiDeviceInfo(), wxT("MIDI Device Info"));
#endif
            auto project = GetActiveProject();
            auto &projectFileIO = ProjectFileIO::Get( *project );
            rpt.AddText(wxT("project.txt"), projectFileIO.GenerateDoc(), wxT("Active project doc"));
         }
   
         auto logger = AudacityLogger::Get();
         if (logger)
         {
            rpt.AddText(wxT("log.txt"), logger->GetLog(), _TS("Audacity Log"));
         }
   
         done = true;
      });
   
      // Wait for information to be gathered
      while (!done)
      {
         wxMilliSleep(50);
         pd->Pulse();
      }
      thread.join();
   }

   bool ok = wxDebugReportPreviewStd().Show(rpt);
   
#if defined(__WXMSW__)
   wxEventLoop::SetCriticalWindow(NULL);
#endif
   
   if (ok && rpt.Process())
   {
      AudacityTextEntryDialog dlg(nullptr,
         XO("Report generated to:"),
         XO("Audacity Support Data"),
         rpt.GetCompressedFileName(),
         wxOK | wxCENTER);
      dlg.SetName(dlg.GetTitle());
      dlg.ShowModal();
      
      wxLogMessage(wxT("Report generated to: %s"),
                   rpt.GetCompressedFileName());
      
      rpt.Reset();
   }
}

}
#endif

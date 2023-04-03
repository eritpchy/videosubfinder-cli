#pragma once

#include "wx/wx.h"
#include "DataTypes.h"
#include <execution>
#include <wx/cmdline.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/wfstream.h>
#include <wx/txtstrm.h>
#include <wx/dir.h>
#include "OCVVideoLoader.h"
#include "FFMPEGVideoLoader.h"
#include "SSAlgorithms.h"
#include "IPAlgorithms.h"

class CVideoSubFinderApp : public wxAppConsole
{
public:
	~CVideoSubFinderApp();

public:
    virtual void OnInitCmdLine(wxCmdLineParser& parser) override;

    virtual bool OnCmdLineParsed(wxCmdLineParser& parser) override;

    virtual int OnRun() override;

    void LoadSettings(wxString file_name);
};

wxIMPLEMENT_APP(CVideoSubFinderApp);

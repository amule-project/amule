//
// This file is part of the aMule Project.
//
// Copyright (c) 2024 aMule Team
//
// Any parts of this program derived from the xMule, lMule or eMule project,
// or contributed by third-party developers are copyrighted by their
// respective authors.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//

#include "MagnetGenerator.h"
#include "KnownFile.h"
#include "Logger.h"
#include "kademlia/kademlia/Kademlia.h"
#include <wx/time.h>

CMagnetGenerator::CMagnetGenerator(CKnownFile* file)
    : wxThread(wxTHREAD_DETACHED),
      m_file(file),
      m_currentProgress(0.0f),
      m_currentStage(STAGE_READY),
      m_abort(false)
{
    m_startTime = wxDateTime::Now();
}

CMagnetGenerator::~CMagnetGenerator()
{
    StopGeneration();
    if (IsRunning()) {
	Wait();
    }
}

bool CMagnetGenerator::StartGeneration()
{
    if (Create() != wxTHREAD_NO_ERROR) {
	AddDebugLogLineC(logGeneral, wxT("Failed to create magnet generation thread"));
	return false;
    }

    if (Run() != wxTHREAD_NO_ERROR) {
	AddDebugLogLineC(logGeneral, wxT("Failed to start magnet generation thread"));
	return false;
    }

    return true;
}

void CMagnetGenerator::StopGeneration()
{
    wxCriticalSectionLocker lock(m_cs);
    m_abort = true;
}

float CMagnetGenerator::GetProgress() const
{
    wxCriticalSectionLocker lock(const_cast<wxCriticalSection&>(m_cs));
    return m_currentProgress;
}

CMagnetGenerator::GenerationStage CMagnetGenerator::GetCurrentStage() const
{
    wxCriticalSectionLocker lock(const_cast<wxCriticalSection&>(m_cs));
    return m_currentStage;
}

wxString CMagnetGenerator::GetMagnetLink() const
{
    wxCriticalSectionLocker lock(const_cast<wxCriticalSection&>(m_cs));
    return m_magnetLink;
}

wxString CMagnetGenerator::GetErrorMessage() const
{
    wxCriticalSectionLocker lock(const_cast<wxCriticalSection&>(m_cs));
    return m_errorMessage;
}

wxThread::ExitCode CMagnetGenerator::Entry()
{
    AddDebugLogLineN(logGeneral, wxT("Starting magnet generation process"));

    try {
	// Stage 1: Gather metadata (20% progress)
	UpdateProgress(0.2f, STAGE_GATHERING_METADATA);
	if (!GatherMetadata()) {
	    HandleError(_("Failed to gather file metadata"));
	    return (ExitCode)1;
	}

	if (m_abort) return (ExitCode)0;

	// Stage 2: Build magnet URI (60% progress)
	UpdateProgress(0.6f, STAGE_BUILDING_MAGNET);
	if (!BuildMagnetURI()) {
	    HandleError(_("Failed to build magnet URI"));
	    return (ExitCode)1;
	}

	if (m_abort) return (ExitCode)0;

	// Stage 3: Publish to DHT (90% progress)
	UpdateProgress(0.9f, STAGE_PUBLISHING_DHT);
	if (!PublishToDHT()) {
	    AddDebugLogLineN(logGeneral, wxT("DHT publishing failed, but magnet link is ready"));
	    // Continue even if DHT publishing fails
	}

	// Stage 4: Complete (100% progress)
	UpdateProgress(1.0f, STAGE_COMPLETE);
	AddDebugLogLineN(logGeneral, wxT("Magnet generation completed successfully"));
	AddDebugLogLineN(logGeneral, wxString::Format(wxT("Generated magnet: %s"), m_magnetLink.Left(100)));

    } catch (const std::exception& e) {
	HandleError(wxString::Format(_("Generation error: %s"), e.what()));
	return (ExitCode)1;
    } catch (...) {
	HandleError(_("Unknown error during magnet generation"));
	return (ExitCode)1;
    }

    return (ExitCode)0;
}

bool CMagnetGenerator::GatherMetadata()
{
    AddDebugLogLineN(logGeneral, wxT("Gathering file metadata for magnet generation"));

    if (!m_file) {
	return false;
    }

    // Gather basic file information
    m_fileName = m_file->GetFileName().GetPrintable();
    m_fileSize = m_file->GetFileSize();
    m_fileHash = m_file->GetFileHash();

    // Gather trackers (if any)
    // TODO: Get tracker list from preferences or file metadata

    // Generate keywords from filename
    m_keywords = m_fileName;

    return true;
}

bool CMagnetGenerator::BuildMagnetURI()
{
    AddDebugLogLineN(logGeneral, wxT("Building magnet URI"));

    CMagnetURI magnet;

    // Add file name (dn parameter)
    magnet.AddField(wxT("dn"), m_fileName);

    // Add file size (xl parameter)
    magnet.AddField(wxT("xl"), wxString::Format(wxT("%llu"), (unsigned long long)m_fileSize));

    // Add ed2k hash (xt parameter)
    magnet.AddField(wxT("xt"), wxT("urn:ed2k:") + m_fileHash.Encode());

    // Add trackers if available (tr parameter)
    for (size_t i = 0; i < m_trackers.size(); ++i) {
	magnet.AddField(wxT("tr"), m_trackers[i]);
    }

    m_magnetLink = magnet.GetLink();

    if (m_magnetLink.empty()) {
	AddLogLineC(_("Failed to generate magnet URI"));
	return false;
    }

    return true;
}

bool CMagnetGenerator::PublishToDHT()
{
    AddDebugLogLineN(logGeneral, wxT("Publishing to DHT network"));

#ifdef HAVE_KADEMLIA
    if (Kademlia::CKademlia::IsConnected()) {
	// Publish the file to DHT for wider discovery
	Kademlia::CUInt128 fileID(m_fileHash.GetHash());
	Kademlia::CKademlia::PublishFile(fileID, 0, false);
	AddDebugLogLineN(logGeneral, wxT("File published to DHT network"));
	return true;
    } else {
	AddDebugLogLineN(logGeneral, wxT("Not connected to DHT, skipping publication"));
	return true; // Not an error, just not connected
    }
#else
    AddDebugLogLineN(logGeneral, wxT("DHT support not compiled in"));
    return true; // DHT not available, but magnet link is still valid
#endif
}

void CMagnetGenerator::UpdateProgress(float progress, GenerationStage stage)
{
    {
	wxCriticalSectionLocker lock(m_cs);
	m_currentProgress = progress;
	m_currentStage = stage;
    }

    // Could notify GUI here if needed
}

void CMagnetGenerator::HandleError(const wxString& errorMessage)
{
    {
	wxCriticalSectionLocker lock(m_cs);
	m_errorMessage = errorMessage;
	m_currentStage = STAGE_ERROR;
    }

    AddLogLineC(errorMessage);
}

wxString CMagnetGenerator::GenerateMagnetSync(CKnownFile* file)
{
    if (!file) {
	return wxEmptyString;
    }

    CMagnetURI magnet;

    // Add basic file information
    magnet.AddField(wxT("dn"), file->GetFileName().GetPrintable());
    magnet.AddField(wxT("xl"), wxString::Format(wxT("%llu"), (unsigned long long)file->GetFileSize()));
    magnet.AddField(wxT("xt"), wxT("urn:ed2k:") + file->GetFileHash().Encode());

    return magnet.GetLink();
}

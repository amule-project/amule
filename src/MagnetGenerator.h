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

#ifndef MAGNETGENERATOR_H
#define MAGNETGENERATOR_H

#include "MD4Hash.h"
#include "MagnetURI.h"
#include <wx/string.h>
#include <wx/thread.h>
#include <wx/event.h>

class CKnownFile;

/**
 * @class CMagnetGenerator
 * @brief Generates magnet links from completed eD2k downloads for DHT/p2p sharing
 */
class CMagnetGenerator : public wxThread
{
public:
    enum GenerationStage {
	STAGE_READY = 0,           // Ready to generate
	STAGE_GATHERING_METADATA,    // Collecting file metadata
	STAGE_BUILDING_MAGNET,       // Constructing magnet URI
	STAGE_PUBLISHING_DHT,       // Publishing to DHT network
	STAGE_COMPLETE,             // Magnet link ready
	STAGE_ERROR                // Generation failed
    };

    CMagnetGenerator(CKnownFile* file);
    virtual ~CMagnetGenerator();

    /**
     * @brief Start the magnet generation process
     */
    bool StartGeneration();

    /**
     * @brief Stop the generation process
     */
    void StopGeneration();

    /**
     * @brief Get current generation progress (0.0 to 1.0)
     */
    float GetProgress() const;

    /**
     * @brief Get current generation stage
     */
    GenerationStage GetCurrentStage() const;

    /**
     * @brief Get generated magnet link (valid only after completion)
     */
    wxString GetMagnetLink() const;

    /**
     * @brief Get error message if generation failed
     */
    wxString GetErrorMessage() const;

    /**
     * @brief Static method to generate magnet link synchronously
     */
    static wxString GenerateMagnetSync(CKnownFile* file);

protected:
    /**
     * @brief Thread entry point
     */
    virtual ExitCode Entry();

private:
    /**
     * @brief Gather file metadata for magnet generation
     */
    bool GatherMetadata();

    /**
     * @brief Build the magnet URI from gathered metadata
     */
    bool BuildMagnetURI();

    /**
     * @brief Publish to DHT network
     */
    bool PublishToDHT();

    /**
     * @brief Update progress and notify
     */
    void UpdateProgress(float progress, GenerationStage stage);

    /**
     * @brief Handle generation error
     */
    void HandleError(const wxString& errorMessage);

    CKnownFile* m_file;
    wxString m_magnetLink;
    wxString m_errorMessage;

    wxCriticalSection m_cs;
    float m_currentProgress;
    GenerationStage m_currentStage;
    bool m_abort;

    // Time tracking
    wxDateTime m_startTime;

    // Metadata collection
    wxString m_fileName;
    uint64 m_fileSize;
    CMD4Hash m_fileHash;
    std::vector<wxString> m_trackers;
    wxString m_keywords;
};

#endif // MAGNETGENERATOR_H

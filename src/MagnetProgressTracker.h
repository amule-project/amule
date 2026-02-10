//
// This file is part of the aMule Project.
//
// Copyright (c) 2024 aMule Team ( admin@amule.org / http://www.amule.org )
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

#ifndef MAGNETPROGRESSTRACKER_H
#define MAGNETPROGRESSTRACKER_H

#include "MD4Hash.h"
#include "MagnetURI.h"
#include "MagnetProtocolDetector.h"
#include <wx/thread.h>
#include <vector>
#include <wx/event.h>

class CDownloadQueue;

/**
 * @class CMagnetProgressTracker
 * @brief Tracks and manages magnet link conversion progress with real-time updates
 */
class CMagnetProgressTracker : public wxThread, public wxEvtHandler
{
public:
    enum ConversionStage {
	STAGE_PARSING = 0,
	STAGE_TRACKER_QUERY,
	STAGE_DHT_LOOKUP,
	STAGE_METADATA_FETCH,
	STAGE_CONVERSION,
	STAGE_COMPLETE,
	STAGE_ERROR
    };

    CMagnetProgressTracker(CDownloadQueue* downloadQueue, const wxString& magnetUri, const CMD4Hash& fileHash);
    virtual ~CMagnetProgressTracker();

    /**
     * @brief Start the magnet conversion process
     */
    bool StartConversion();

    /**
     * @brief Stop the conversion process
     */
    void StopConversion();

    /**
     * @brief Get current conversion progress (0.0 to 1.0)
     */
    float GetProgress() const;

    /**
     * @brief Get current conversion stage
     */
    ConversionStage GetCurrentStage() const;

	/**
	 * @brief Get error message if conversion failed
	 */
	wxString GetErrorMessage() const;

	/**
	 * @brief Get estimated time remaining for conversion
	 */
	wxTimeSpan GetEstimatedTimeRemaining() const;

	/**
	 * @brief Get current conversion rate (progress per second)
	 */
	float GetConversionRate() const;

protected:
    /**
     * @brief Thread entry point
     */
    virtual ExitCode Entry();

private:
    /**
     * @brief Parse magnet URI and extract basic information
     */
    bool ParseMagnetUri();

    /**
     * @brief Query trackers for file information
     */
    bool QueryTrackers();

    /**
     * @brief Perform DHT lookup for file information
     */
    bool PerformDHTLookup();

    /**
     * @brief Fetch metadata from peers
     */
    bool FetchMetadata();

    /**
     * @brief Detect protocol and route to appropriate handler
     */
    bool DetectProtocolAndRoute();

    /**
     * @brief Convert to preferred protocol format based on user preferences
     */
    bool ConvertToPreferredFormat();

    /**
     * @brief Convert to ED2K format
     */
    bool ConvertToED2KFormat();

    /**
     * @brief Handle BitTorrent format
     */
    bool HandleBitTorrentFormat();

    /**
     * @brief Handle hybrid protocol format
     */
    bool HandleHybridFormat(ProtocolPreference preference);

    /**
     * @brief Update progress and notify GUI
     */
    void UpdateProgress(float progress, ConversionStage stage);

    /**
     * @brief Handle conversion error
     */
    void HandleError(const wxString& errorMessage);

    CDownloadQueue* m_downloadQueue;
    wxString m_magnetUri;
    CMD4Hash m_fileHash;
    CMagnetED2KConverter m_converter;

    wxCriticalSection m_cs;
    float m_currentProgress;
    ConversionStage m_currentStage;
    wxString m_errorMessage;
    bool m_abort;

    // Time tracking for speed estimation
    wxDateTime m_startTime;
    float m_lastProgress;
    wxDateTime m_lastUpdateTime;

    // Protocol detection
    std::vector<MagnetProtocolInfo> m_detectedProtocols;
};

#endif // MAGNETPROGRESSTRACKER_H

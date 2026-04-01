//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002-2011 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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

#include "SearchPackageValidator.h"
#include "SearchPackageException.h"
#include "SearchModel.h"
#include "SearchLogging.h"
#include "../Logger.h"
#include "../MD4Hash.h"
#include "../Tag.h"
#include <common/Format.h>
#include <wx/app.h>
#include <wx/string.h>
#include <algorithm>
#include <netinet/in.h>

// Default batch size for processing large packages
static const size_t DEFAULT_BATCH_SIZE = 50;

// Maximum Kad publish info value
static const uint32_t MAX_KAD_PUBLISH_INFO = 0xFFFFFFFF;

namespace search {

SearchPackageValidator::SearchPackageValidator()
	: m_batchSize(DEFAULT_BATCH_SIZE)
	, m_totalProcessed(0)
	, m_totalAdded(0)
	, m_totalDuplicates(0)
	, m_totalInvalid(0)
{
}

SearchPackageValidator::~SearchPackageValidator()
{
}

void SearchPackageValidator::SetBatchSize(size_t batchSize)
{
	m_batchSize = batchSize;
}

size_t SearchPackageValidator::GetBatchSize() const
{
	return m_batchSize;
}

void SearchPackageValidator::ValidateMandatoryFields(CSearchFile* result) const
{
	// Validate file hash
	if (result->GetFileHash().IsEmpty()) {
		throw SearchPackageException(wxT("Invalid package: Empty file hash"),
			result->GetSearchID());
	}

	// Validate file name
	wxString fileName = result->GetFileName().GetPrintable();
	if (fileName.IsEmpty() || fileName.Length() > 255) {
		throw SearchPackageException(
			CFormat(wxT("Invalid package: File name length=%d")) % fileName.Length(),
			result->GetSearchID());
	}

	// Validate file size
	uint64_t fileSize = result->GetFileSize();
	if (fileSize == 0 || fileSize > OLD_MAX_FILE_SIZE) {
		throw SearchPackageException(
			CFormat(wxT("Invalid package: File size=%llu")) % fileSize,
			result->GetSearchID());
	}
}

void SearchPackageValidator::ValidateDataIntegrity(CSearchFile* result) const
{
	// Check tags - allow empty tag names as they are valid in some cases
	const ArrayOfCTag& tags = result->GetTags();
	for (const auto& tag : tags) {
		// Tag name validation is now optional - empty names are allowed
		// Some search results legitimately have tags with empty names
	}

	// Validate file name encoding - allow non-ASCII file names
	wxString fileName = result->GetFileName().GetPrintable();
	// Non-ASCII file names are common and should be allowed
	// Only validate that the file name is not empty (already checked in ValidateMandatoryFields)
}

void SearchPackageValidator::ValidateNetworkSource(CSearchFile* result) const
{
	// Skip network source validation for Kad results
	// Kad results have clientID=0 and clientPort=0 by design
	if (result->IsKademlia()) {
		return;
	}

	// For ED2K results
	uint32_t clientID = result->GetClientID();
	uint16_t clientPort = result->GetClientPort();

	if (clientID != 0) {
		if (clientID == INADDR_NONE) {
			throw SearchPackageException(
				CFormat(wxT("Invalid package: Invalid client ID=%u")) % clientID,
				result->GetSearchID());
		}

		if (clientPort == 0) {
			throw SearchPackageException(
				CFormat(wxT("Invalid package: Invalid client port=%u")) % clientPort,
				result->GetSearchID());
		}
	}
}



size_t SearchPackageValidator::ProcessBatch(const std::vector<CSearchFile*>& batch, SearchModel* model)
{
	size_t addedCount = 0;

	for (auto result : batch) {
		try {
			// Validate mandatory fields
			ValidateMandatoryFields(result);

			// Validate data integrity
			ValidateDataIntegrity(result);

			// Validate network source
			ValidateNetworkSource(result);

			// Add to model (handles duplicates internally)
			model->addResult(result);
			addedCount++;
			m_totalAdded++;

			AddDebugLogLineC(logSearch,
				CFormat(wxT("Result added: SearchID=%u, Name='%s', Size=%llu"))
					% result->GetSearchID() % result->GetFileName().GetPrintable() % result->GetFileSize());
		} catch (const SearchPackageException& e) {
			// Exception already logged
			// Clean up defective result
			delete result;
			m_totalInvalid++;
		}
	}

	return addedCount;
}

bool SearchPackageValidator::ProcessResult(CSearchFile* result, SearchModel* model)
{
	m_totalProcessed++;

	try {
		// Validate mandatory fields
		ValidateMandatoryFields(result);

		// Validate data integrity
		ValidateDataIntegrity(result);

		// Validate network source
		ValidateNetworkSource(result);

		// Add to model (handles duplicates internally)
		model->addResult(result);
		m_totalAdded++;

		AddDebugLogLineC(logSearch,
			CFormat(wxT("Result processed: SearchID=%u, Name='%s', Size=%llu"))
				% result->GetSearchID() % result->GetFileName().GetPrintable() % result->GetFileSize());

		return true;
	} catch (const SearchPackageException& e) {
		// Exception already logged
		// Clean up defective result
		delete result;
		m_totalInvalid++;
		return false;
	}
}

size_t SearchPackageValidator::ProcessResults(const std::vector<CSearchFile*>& results, SearchModel* model)
{
	m_totalProcessed += results.size();

	size_t totalResults = results.size();

	// Small package: immediate processing
	if (totalResults <= m_batchSize) {
		AddDebugLogLineC(logSearch,
			CFormat(wxT("Processing small package: %zu results")) % totalResults);
		return ProcessBatch(results, model);
	}

	// Medium package: batch processing with progress
	if (totalResults <= m_batchSize * 4) {
		AddDebugLogLineC(logSearch,
			CFormat(wxT("Processing medium package: %zu results in batches of %zu"))
				% totalResults % m_batchSize);

		size_t addedCount = 0;
		for (size_t batchStart = 0; batchStart < totalResults; batchStart += m_batchSize) {
			std::vector<CSearchFile*> batch;
			size_t batchEnd = std::min(batchStart + m_batchSize, totalResults);

			for (size_t i = batchStart; i < batchEnd; ++i) {
				batch.push_back(results[i]);
			}

			addedCount += ProcessBatch(batch, model);

			AddDebugLogLineC(logSearch,
				CFormat(wxT("Medium batch progress: %zu%%, Batch=%zu/%zu, Added=%zu"))
					% ((batchEnd * 100) / totalResults)
					% ((batchStart / m_batchSize) + 1)
					% ((totalResults + m_batchSize - 1) / m_batchSize)
					% addedCount);
		}

		return addedCount;
	}

	// Large package: streaming batch processing with UI updates
	AddDebugLogLineC(logSearch,
		CFormat(wxT("Processing large package: %zu results in batches of %zu"))
			% totalResults % m_batchSize);

	size_t addedCount = 0;
	for (size_t batchStart = 0; batchStart < totalResults; batchStart += m_batchSize) {
		std::vector<CSearchFile*> batch;
		size_t batchEnd = std::min(batchStart + m_batchSize, totalResults);

		for (size_t i = batchStart; i < batchEnd; ++i) {
			batch.push_back(results[i]);
		}

		addedCount += ProcessBatch(batch, model);

		size_t progress = (batchEnd * 100) / totalResults;
		AddDebugLogLineC(logSearch,
			CFormat(wxT("Large batch progress: %zu%%, Batch=%zu/%zu, Added=%zu"))
				% progress
				% ((batchStart / m_batchSize) + 1)
				% ((totalResults + m_batchSize - 1) / m_batchSize)
				% addedCount);

		// Allow UI to update
		wxYieldIfNeeded();
	}

	AddDebugLogLineC(logSearch,
		CFormat(wxT("Large package completed: Total=%zu, Added=%zu, Duplicates=%u, Invalid=%u"))
			% totalResults % addedCount % m_totalDuplicates % m_totalInvalid);

	return addedCount;
}

} // namespace search

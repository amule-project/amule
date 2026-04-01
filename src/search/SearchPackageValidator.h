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

#ifndef SEARCHPACKAGEVALIDATOR_H
#define SEARCHPACKAGEVALIDATOR_H

#include <wx/string.h>
#include <vector>
#include <stdint.h>
#include "../SearchFile.h"
#include "SearchModel.h"

// Forward declarations
class CSearchList;

// CSearchResultList is defined in SearchFile.h and SearchList.h
typedef std::vector<CSearchFile*> CSearchResultList;

namespace search {

/**
 * Search Package Validator
 *
 * This class provides validation for search result packages received from
 * both ED2K and Kademlia networks. It handles:
 * - Mandatory field validation
 * - Data integrity checking
 * - Network source validation
 * - Duplicate detection
 * - Batch processing for large packages
 */
class SearchPackageValidator {
public:
	/**
	 * Constructor
	 */
	SearchPackageValidator();

	/**
	 * Destructor
	 */
	virtual ~SearchPackageValidator();

	/**
	 * Validate and process a single search result
	 *
	 * @param result The search result to validate and process
	 * @param model The search model to add the result to
	 * @return true if result was added, false if it was a duplicate or invalid
	 */
	bool ProcessResult(CSearchFile* result, SearchModel* model);

	/**
	 * Validate and process multiple search results
	 *
	 * @param results Vector of search results to validate and process
	 * @param model The search model to add the results to
	 * @return Number of results actually added
	 */
	size_t ProcessResults(const std::vector<CSearchFile*>& results, SearchModel* model);

	/**
	 * Set batch size for processing large packages
	 *
	 * @param batchSize Number of results to process in each batch
	 */
	void SetBatchSize(size_t batchSize);

	/**
	 * Get current batch size
	 *
	 * @return Current batch size
	 */
	size_t GetBatchSize() const;

private:
	/**
	 * Validate mandatory fields of a search result
	 *
	 * @param result The search result to validate
	 * @throws SearchPackageException if validation fails
	 */
	void ValidateMandatoryFields(CSearchFile* result) const;

	/**
	 * Validate data integrity of a search result
	 *
	 * @param result The search result to validate
	 * @throws SearchPackageException if validation fails
	 */
	void ValidateDataIntegrity(CSearchFile* result) const;

	/**
	 * Validate network source of a search result
	 *
	 * @param result The search result to validate
	 * @throws SearchPackageException if validation fails
	 */
	void ValidateNetworkSource(CSearchFile* result) const;

	/**
	 * Process a batch of results
	 *
	 * @param batch Vector of results to process
	 * @param model The search model to add the results to
	 * @return Number of results actually added
	 */
	size_t ProcessBatch(const std::vector<CSearchFile*>& batch, SearchModel* model);

	// Configuration
	size_t m_batchSize;

	// Statistics
	uint32_t m_totalProcessed;
	uint32_t m_totalAdded;
	mutable uint32_t m_totalDuplicates;
	uint32_t m_totalInvalid;
};

} // namespace search

#endif // SEARCHPACKAGEVALIDATOR_H


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

#ifndef KADSEARCHCONTROLLER_H
#define KADSEARCHCONTROLLER_H

#include "SearchControllerBase.h"

// Forward declarations
namespace Kademlia {
    class CSearch;
}
#include <memory>

// Forward declarations
class CSearchFile;

namespace search {

/**
 * KadSearchController - Specialized controller for Kademlia network searches
 *
 * This controller handles Kad searches with:
 * - Optimized keyword-based searches
 * - Efficient result aggregation from Kad nodes
 * - Detailed progress reporting
 * - Automatic retry logic for failed searches
 */
class KadSearchController : public SearchControllerBase {
public:
    explicit KadSearchController();
    virtual ~KadSearchController();

    // Delete copy constructor and copy assignment operator
    KadSearchController(const KadSearchController&) = delete;
    KadSearchController& operator=(const KadSearchController&) = delete;

    // SearchController implementation
    void startSearch(const SearchParams& params) override;
    void stopSearch() override;
    void requestMoreResults() override;

    // Kad-specific methods
    void setMaxNodesToQuery(int maxNodes);
    int getMaxNodesToQuery() const;

    void setRetryCount(int retryCount);
    int getRetryCount() const;

    // Configuration validation
    bool validateConfiguration() const;

private:
    // Kad-specific settings
    int m_maxNodesToQuery;

    // Progress tracking
    int m_nodesContacted;
    static constexpr int DEFAULT_MAX_NODES = 500;
    static constexpr int PROGRESS_UPDATE_INTERVAL = 10;
    
    // Kademlia search object
    Kademlia::CSearch* m_kadSearch;

    // Helper methods
    void updateProgress();
    void initializeProgress();
    bool isValidKadNetwork() const;

    // Validation methods
    bool validatePrerequisites();

    // Kad search completion handling
    void onKadSearchComplete(uint32_t kadSearchId, bool hasResults);
    void checkKadSearchState();
};

} // namespace search

#endif // KADSEARCHCONTROLLER_H

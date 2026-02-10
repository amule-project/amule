#ifndef IGEOIPDATABASE_H
#define IGEOIPDATABASE_H

#include <wx/string.h>
#include <memory>
#include <vector>

/**
 * @brief GeoIP database types
 */
enum DatabaseType {
    DB_TYPE_UNKNOWN,      ///< Unknown format
    DB_TYPE_MAXMIND_DB,   ///< MaxMind DB binary format
    DB_TYPE_LEGACY_GEOIP, ///< Legacy GeoIP format (deprecated)
    DB_TYPE_CSV,          ///< CSV format (planned)
    DB_TYPE_SQLITE        ///< SQLite format (planned)
};

/**
 * @brief GeoIP database operations interface
 */
class IGeoIPDatabase {
public:
    virtual ~IGeoIPDatabase() = default;
    
    // Core operations
    virtual bool Open(const wxString& path) = 0;
    virtual void Close() = 0;
    virtual bool IsValid() const = 0;
    
    // Metadata
    virtual DatabaseType GetType() const = 0;
    virtual wxString GetVersion() const = 0;
    virtual wxString GetFormatName() const = 0;
    virtual wxString GetDescription() const = 0;
    
    // Lookup operations
    virtual wxString GetCountryCode(const wxString& ip) = 0;
    virtual wxString GetCountryName(const wxString& ip) = 0;
    
    // Batch operations
    virtual std::vector<wxString> BatchGetCountryCodes(
        const std::vector<wxString>& ips) = 0;
};

#endif // IGEOIPDATABASE_H
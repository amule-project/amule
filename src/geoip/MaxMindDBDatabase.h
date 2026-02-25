//
// MaxMindDBDatabase.h - MaxMind database implementation for GeoIP
//
#ifndef MAXMINDDBDATABASE_H
#define MAXMINDDBDATABASE_H

#include "IGeoIPDatabase.h"
#include <wx/string.h>

#ifdef ENABLE_MAXMINDDB
#include <maxminddb.h>
#endif

/**
 * @brief MaxMind DB database implementation
 */
class MaxMindDBDatabase : public IGeoIPDatabase
{
public:
    MaxMindDBDatabase();
    ~MaxMindDBDatabase() override;
    
    // IGeoIPDatabase implementation
    bool Open(const wxString& databasePath) override;
    void Close() override;
    bool IsValid() const override;
    DatabaseType GetType() const override { return DB_TYPE_MAXMIND_DB; }
    wxString GetFormatName() const override { return "MaxMind DB"; }
    wxString GetVersion() const override;
    wxString GetDescription() const override;
    
    wxString GetCountryCode(const wxString& ip) override;
    wxString GetCountryName(const wxString& ip) override;
    
    std::vector<wxString> BatchGetCountryCodes(
        const std::vector<wxString>& ips) override;
    
private:
    bool LookupCountry(const wxString& ip, 
                      wxString& country_code, 
                      wxString& country_name) const;
    bool IsOpen() const;  // Internal state check
    
#ifdef ENABLE_MAXMINDDB
    MMDB_s m_mmdb;        ///< MaxMind DB handle
#endif
    bool m_isOpen;        ///< Database open state
    wxString m_dbPath;    ///< Path to database file
};

#endif // MAXMINDDBDATABASE_H
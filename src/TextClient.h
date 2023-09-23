//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2003-2011 Angel Vidal ( kry@amule.org )
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

#ifndef TEXTCLIENT_H
#define TEXTCLIENT_H

#include "ExternalConnector.h"

#include <map>



class CEC_SearchFile_Tag;

class SearchFile {
   public:
      wxString sFileName;
      unsigned long lFileSize;
      CMD4Hash  nHash;
      wxString  sHash;
      long lSourceCount;
      bool bPresent;

      SearchFile(const CEC_SearchFile_Tag *);

      static class SearchInfo *GetContainerInstance();
      CMD4Hash ID() { return nHash; }
};

typedef std::map<unsigned long int,SearchFile*> CResultMap;

wxString ECv2_Response2String(CECPacket *response);

class CamulecmdApp : public CaMuleExternalConnector {
public:

   const wxString GetGreetingTitle() { return _("aMule text client"); }
   int ProcessCommand(int ID);
   void Process_Answer_v2(const CECPacket *reply);
   void OnInitCommandSet();

private:

   CECPacket * createRequest( ec_tagname_t name, const wxString & data ) const;
   CECPacket * createRequest( ec_tagname_t name, const CECTag &   tag  ) const;

   void addConnectRequest         ( const wxString & args,                             std::list<CECPacket *> & request_list ) const;
   void addSearchRequest          ( const wxString & args, EC_SEARCH_TYPE search_type, std::list<CECPacket *> & request_list ) const;
   void addRenameRequest          ( const wxString & args,                             std::list<CECPacket *> & request_list ) const;
   void addGetIpFilterRequest     (                                                    std::list<CECPacket *> & request_list ) const;
   void addSetIpFilterRequest     ( int              cmd,  bool           on,          std::list<CECPacket *> & request_list ) const;
   void addGetIpFilterLevelRequest(                                                    std::list<CECPacket *> & request_list ) const;
   int  addSetIpFilterLevelRequest( const wxString & args,                             std::list<CECPacket *> & request_list ) const;
   void addPartFileRequest        ( const wxString & args, ECOpCodes      opCode,      std::list<CECPacket *> & request_list );
   void addPriorityRequest        ( const wxString & args, uint8          priority,    std::list<CECPacket *> & request_list ) const;
   void addAddLinkRequest         ( const wxString & args,                             std::list<CECPacket *> & request_list ) const;
   void addGetBWLimitRequest      (                                                    std::list<CECPacket *> & request_list ) const;
   int  addSetBWLimitRequest      ( const wxString & args, ECTagNames     tagName,     std::list<CECPacket *> & request_list ) const;
   int  addStatTreeRequest        ( const wxString & args,                             std::list<CECPacket *> & request_list ) const;
   int  addDownloadRequest        ( const wxString & args,                             std::list<CECPacket *> & request_list );

private:

   void OnInitCmdLine  ( wxCmdLineParser & amuleweb_parser );
   bool OnCmdLineParsed( wxCmdLineParser & parser );
   void TextShell      ( const wxString &  prompt );
   void ShowResults    ( CResultMap        results_map );
   virtual int OnRun();

private:

   bool       m_HasCmdOnCmdLine;
   wxString   m_CmdString;
   int        m_last_cmd_id;
   CResultMap m_Results_map;
};

#endif // TEXTCLIENT_H
// File_checked_for_headers

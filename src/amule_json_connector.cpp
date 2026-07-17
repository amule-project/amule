#include "amule_json_connector.h"

#include "amule.h"
#include "Logger.h"
#include "PartFile.h"
#include "ServerConnect.h"
#include "Server.h"
#include "kademlia/kademlia/Kademlia.h"
#include "Statistics.h"
#include "ServerList.h"
#include "DownloadQueue.h"
#include "UploadQueue.h"
#include "SharedFileList.h"
#include "SearchList.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <thread>

using namespace hpms;
using json = nlohmann::json;

struct raw_message {
   uint32_t length;
   char      payload[64*1024];
};

template <typename... Args>
std::string buildMessage(Args&&... args) {
   std::ostringstream oss;
   (oss << ... << args); // fold expression C++17
   return oss.str();
}

JSON_Connector::JSON_Connector(unsigned short port, CamuleApp & amule) :
   _amule(amule)
{
   AddLogLineNS("JSON connector : initialisation...");
   _socket = ::socket(AF_INET, SOCK_STREAM, 0);
   if(_socket < 0) {
      AddLogLineC(buildMessage("socket: ", strerror( errno )));
   }
   else {
      int reuse = 1;
      int rc = ::setsockopt(_socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
      if(rc) {
         AddLogLineC(buildMessage("setsockopt: ", strerror( errno )));
      }
      else {
         sockaddr_in addr {};
         addr.sin_family      = AF_INET;
         addr.sin_port        = htons(port);
         addr.sin_addr.s_addr = htonl(INADDR_ANY);
         rc = ::bind(_socket, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr));
         if(rc) {
            AddLogLineC(buildMessage("bind: ", strerror( errno )));
         }
         else {
            rc = ::listen(_socket, 1); // Il n'y a qu'un seul client à la fois
            if(rc) {
               AddLogLineC(buildMessage("listen: ", strerror( errno )));
            }
            else {
               std::thread thread([this](){
                  acceptLoop();
               });
               thread.detach();
            }
         }
      }
      if(rc) {
         ::close(_socket);
      }
   }
}

JSON_Connector:: ~ JSON_Connector() {
   AddLogLineNS("JSON connector : terminé.");
   ::close(_socket);
   _socket = -1;
}

void JSON_Connector::sendJsonMsg(int socket, const json & response) {
   if(socket < 0) {
      AddLogLineC(buildMessage(__func__, ": socket invalide"));
      return;
   }
   std::string text;
   text = response.dump(3);
   if(text.size() >= sizeof(raw_message::payload)) {
      AddLogLineC(buildMessage(__func__, ": message trop grand (", text.size(), " octets)"));
      return;
   }
   raw_message msg{};
   std::memcpy(msg.payload, text.data(), text.size());
   uint32_t len = static_cast<uint32_t>(text.size());
   msg.length = htonl(len);
   const size_t total = sizeof(msg.length) + len;
   const char * data  = reinterpret_cast<const char *>(&msg);
   size_t sent = 0;
   while(sent < total) {
      ssize_t rc = ::send(socket, data + sent, total - sent, MSG_NOSIGNAL);
      if(rc < 0) {
         if(errno == EINTR) {
            continue; // interruption par un signal, on réessaie
         }
         AddLogLineC(buildMessage(__func__, ": ", strerror(errno)));
         return;
      }
      if(rc == 0) {
         AddLogLineC(buildMessage(__func__, ": connexion fermée côté TS"));
         return;
      }
      sent += static_cast<size_t>(rc);
   }
}

static std::string to_lower(std::string s) {
   std::transform(
      s.begin(), s.end(), s.begin(),
      [](unsigned char c) { return std::tolower(c); });
   return s;
}

static std::string buildSourcesString(const CPartFile* file) {
   if( ! file) {
      throw std::runtime_error("L'argument 'file' ne peut être nul !");
   }
   uint16 sc   = file->GetSourceCount();
   uint16 ncsc = file->GetNotCurrentSourcesCount();
   std::ostringstream oss;
   if(ncsc && ncsc <= sc) {
      oss << (sc - ncsc) << "/" << sc;
   }
   else {
      oss << sc;
   }
   uint16 A4AFCount = file->GetSrcA4AFCount();
   if(A4AFCount) {
      oss << "+" << A4AFCount;
   }
   uint16 transferingSrcCount = file->GetTransferingSrcCount();
   if(transferingSrcCount) {
      oss << " (" << transferingSrcCount << ")";
   }
   return oss.str();
}

static std::string partFileStatusToString(uint8 status) {
   switch (status) {
      case PS_READY:          return "ready";
      case PS_EMPTY:          return "empty";
      case PS_WAITINGFORHASH: return "waiting_for_hash";
      case PS_HASHING:        return "hashing";
      case PS_COMPLETING:     return "completing";
      case PS_COMPLETE:       return "complete";
      case PS_PAUSED:         return "paused";
      case PS_ERROR:          return "error";
      case PS_INSUFFICIENT:   return "insufficient";
      default:                return "unknown";
   }
}

static std::string priorityToString(uint8 priority) {
   switch (priority) {
      case PR_LOW:      return "low";
      case PR_NORMAL:   return "normal";
      case PR_HIGH:     return "high";
      case PR_AUTO:     return "auto"; // si applicable selon ta version
      default:          return "unknown";
   }
}

static std::string ipPort(const wxString & ip, uint16 port ) {
   std::ostringstream oss;
   oss << ip << ":" << port;
   return oss.str();
}

void JSON_Connector::status(int socket, json & response) {
   if( ! _amule.serverconnect) {
      throw std::runtime_error("serverconnect est nul !");
   }
   if( ! _amule.m_statistics) {
      throw std::runtime_error("statistics est nul !");
   }
   CServer * server = _amule.serverconnect->GetCurrentServer();
   if(server) {
      response["edk_name"] = server->GetListName().ToStdString();
      response["edk_ip"  ] = ipPort(server->GetFullIP(), server->GetPort());
   }
   else {
      response["edk_name"] = "";
      response["edk_ip"]   = "";
   }
   response["high_id" ] = _amule.serverconnect->IsConnected()&&( ! _amule.serverconnect->IsLowID());
   response["kad"     ] = Kademlia::CKademlia::IsRunning() && Kademlia::CKademlia::IsConnected();
   response["download"] = _amule.m_statistics->GetDownloadRate();
   response["upload"  ] = _amule.m_statistics->GetUploadRate();
   response["clients" ] = _amule.m_statistics->GetWaitingUserCount();
   response["sources" ] = _amule.m_statistics->GetSharedFileCount();
   sendJsonMsg(socket, response);
}

void JSON_Connector::showServers(int socket, json & response) {
   if( ! _amule.serverlist) {
      throw std::runtime_error("serverlist est nul !");
   }
   response["data"] = json::array();
   const std::vector<const CServer *> & servers = _amule.serverlist->CopySnapshot();
   for(const CServer * server : servers) {
      if(server) {
         json serverObj;
         serverObj["ip-port"] = ipPort(server->GetFullIP(), server->GetPort());
         serverObj["description"] = server->GetDescription().ToStdString();
         response["data"].push_back(serverObj);
      }
   }
   sendJsonMsg(socket, response);
}

void JSON_Connector::connect(int socket, const json & request, json & response) {
   if( ! _amule.serverlist) {
      throw std::runtime_error("serverlist est nul !");
   }
   if( ! _amule.serverconnect) {
      throw std::runtime_error("serverconnect est nul !");
   }
   const std::string & address = request.at("address");
   uint16              port    = request.at("port");
   CServer *           server  = _amule.serverlist->GetServerByAddress(address, port);
   if( ! server) {
      throw std::runtime_error(buildMessage("Aucun serveur trouvé à partir de ", address, ":", port));
   }
   _amule.serverconnect->ConnectToServer(server);
   response["data"] = true;
   sendJsonMsg(socket, response);
}

void JSON_Connector::showLog(int socket, json & response) {
   response["data"] = json::array();
   std::istringstream iss(_amule.GetLog(false).ToStdString());
   std::string        line;
   while(std::getline(iss, line, '\n')) {
      if( ! line.empty()) {
         response["data"].push_back(line);
      }
   }
   sendJsonMsg(socket, response);
}

void JSON_Connector::reset(int socket, json & response) {
   _amule.GetLog(true);
   response["data"] = true;
   sendJsonMsg(socket, response);
}

void JSON_Connector::showDownload(int socket, json & response) {
   if( ! _amule.downloadqueue) {
      throw std::runtime_error("downloadqueue est nul !");
   }
   response["data"] = json::array();
   std::vector<CPartFile *> download;
   _amule.downloadqueue->CopyFileList(download, false);
   for(const CPartFile * file : download) {
      if(file) {
         json data = {
            {"hash"    , file->GetFileHash().EncodeSTL()},
            {"name"    , file->GetFullName().GetRaw().ToStdString()},
            {"progress", file->GetPercentCompleted()},
            {"sources" , buildSourcesString(file)},
            {"status"  , partFileStatusToString(file->GetStatus())},
            {"partMet" , file->GetPartMetFileName().GetFullName().GetRaw().ToStdString()},
            {"size"    , static_cast<double>(file->GetFileSize())},
            {"priority", priorityToString(file->GetDownPriority())},
            {"rate"    , file->GetKBpsDown()}
         };
         response["data"].push_back(data);
      }
   }
   sendJsonMsg(socket, response);
}

void JSON_Connector::rename(int socket, const json & request, json & response) {
   if( ! _amule.downloadqueue) {
      throw std::runtime_error("downloadqueue est nul !");
   }
   if( ! _amule.sharedfiles) {
      throw std::runtime_error("sharedfiles est nul !");
   }
   const std::string & hashStr = request.at("hash");
   const std::string & newName = request.at("newName");
   CMD4Hash hash;
   if( ! hash.Decode(hashStr)) {
      throw std::runtime_error(buildMessage("erreur de décodage de ", hashStr));
   }
   CPartFile * partFile = _amule.downloadqueue->GetFileByID(hash);
   if( ! partFile) {
      throw std::runtime_error(buildMessage("le hash ", hashStr, " est inconnu !"));
   }
   response["data"] = _amule.sharedfiles->RenameFile(partFile, CPath(newName));
   sendJsonMsg(socket, response);
}

void JSON_Connector::cancel(int socket, const json & request, json & response) {
   if( ! _amule.downloadqueue) {
      throw std::runtime_error("downloadqueue est nul !");
   }
   const std::string & hashStr = request.at("hash");
   CMD4Hash hash;
   if( ! hash.Decode(hashStr)) {
      throw std::runtime_error(buildMessage("erreur de décodage de ", hashStr));
   }
   CPartFile * partFile = _amule.downloadqueue->GetFileByID(hash);
   if( ! partFile) {
      throw std::runtime_error(buildMessage("le hash ", hashStr, " est inconnu !"));
   }
   if(thePrefs::StartNextFile() &&(partFile->GetStatus() != PS_PAUSED)) {
      _amule.downloadqueue->StartNextFile(partFile);
   }
   partFile->Delete();
   response["data"] = true;
   sendJsonMsg(socket, response);
}

void JSON_Connector::showUpload(int socket, json & response) {
   if( ! _amule.uploadqueue) {
      throw std::runtime_error("uploadqueue est nul !");
   }
   response["data"] = json::array();
   const CClientRefList & clients = _amule.uploadqueue->GetUploadingList();
   for (const CClientRef & client : clients) {
      const CKnownFile * uploadFile  = client.GetUploadFile();
      const CKnownFile * requestFile = client.GetRequestFile();
      json data = {
         {"client_name"         , client.GetUserName().ToStdString()},
         {"client_hash"         , client.GetUserHash().EncodeSTL()},
         {"client_software"     , client.GetSoftStr().ToStdString()},
         {"client_version"      , client.GetSoftVerStr().ToStdString()},
         {"client_mod"          , client.GetClientModString().ToStdString()},
         {"client_os"           , client.GetClientOSInfo().ToStdString()},
         {"client_soft_id"      , client.GetClientSoft()},
         {"client_version_id"   , client.GetVersion()},
         {"server_name"         , client.GetServerName().ToStdString()},
         {"server_port"         , client.GetServerPort()},
         {"upload_file_name"    , uploadFile  ? uploadFile ->GetFileName().GetRaw().ToStdString() : ""},
         {"upload_file_hash"    , uploadFile  ? uploadFile ->GetFileHash().EncodeSTL()            : ""},
         {"request_file_name"   , requestFile ? requestFile->GetFileName().GetRaw().ToStdString() : ""},
         {"upload_speed_bps"    , client.GetUploadDatarate()},
         {"uploaded_total_bytes", client.GetUploadedTotal()},
         {"is_friend"           , client.IsFriend()},
         {"is_bad_guy"          , client.IsBadGuy()}
      };
      response["data"].push_back(data);
   }
   sendJsonMsg(socket, response);
}

void JSON_Connector::reloadShared(int socket, json & response) {
   if( ! _amule.sharedfiles) {
      throw std::runtime_error("sharedfiles est nul !");
   }
   _amule.sharedfiles->Reload();
   response["data"] = true;
   sendJsonMsg(socket, response);
}

void JSON_Connector::startSearch(int socket, const json & request, json & response) {
   if( ! _amule.searchlist) {
      throw std::runtime_error("searchlist est nul !");
   }
   _amule.searchlist->RemoveResults(0xffffffff);
   _searchID = 0;
   CSearchList::CSearchParams params;
   const std::string & keywords  = request.at("keywords");
   const std::string & type      = request.at("type");
   const std::string & extension = request.at("extension");
   params.searchString = keywords;
   params.strKeyword   = "";
   params.typeText     = type;
   params.extension    = extension;
   params.minSize      = request.at("minSize");
   params.maxSize      = request.at("maxSize");
   params.availability = request.at("availability");
   _amule.searchlist->StartNewSearch(&_searchID, SearchType::GlobalSearch, params);
   response["data"] = true;
   sendJsonMsg(socket, response);
}

void JSON_Connector::progress(int socket, json & response) {
   if( ! _amule.searchlist) {
      throw std::runtime_error("searchlist est nul !");
   }
   uint32_t progress = _amule.searchlist->GetSearchProgress();
   response["data"] = {
      {"completed", progress == 100},
      {"progress" , progress}
   };
   sendJsonMsg(socket, response);
}

void JSON_Connector::results(int socket, json & response) {
   if( ! _amule.searchlist) {
      throw std::runtime_error("searchlist est nul !");
   }
   const CSearchResultList & searchResults = _amule.searchlist->GetSearchResults(_searchID);
   json results = json::array();
   for(const CSearchFile * file : searchResults) {
      if(file) {
         json result = {
            {"hash"    , file->GetFileHash().EncodeSTL()},
            {"filename", file->GetFileName().GetRaw().ToStdString()},
            {"size"    , (double)file->GetFileSize()},
            {"sources" , file->GetSourceCount()}
         };
         results.push_back(result);
      }
   }
   response["data"] = results;
   sendJsonMsg(socket, response);
}

void JSON_Connector::download(int socket, const json & request, json & response) {
   if( ! _amule.searchlist) {
      throw std::runtime_error("searchlist est nul !");
   }
   const std::string & hashStr = request.at("hash");
   CMD4Hash hash;
   if( ! hash.Decode(hashStr)) {
      throw std::runtime_error(buildMessage("erreur de décodage de ", hashStr));
   }
   _amule.searchlist->AddFileToDownloadByHash(hash);
   response["data"] = true;
   sendJsonMsg(socket, response);
}

bool JSON_Connector::read_socket(int client, uint8_t * buffer, size_t length) {
   while(length) {
      ssize_t n = ::recv(client, buffer, length, 0);
      if(n < 0) {
         throw std::runtime_error(buildMessage("recv: ", strerror(errno)));
      }
      if(n == 0) {
         return false;
      }
      buffer += n;
      length -= n;
   }
   return true;
}

bool JSON_Connector::dialog(int client) {
   raw_message msg;
   if( ! read_socket(client, reinterpret_cast<uint8_t *>(&msg.length), sizeof(msg.length))) {
      return false;
   }
   msg.length = ntohl(msg.length);
   if(msg.length < 10) {
      throw std::runtime_error("message trop court !");
   }
   if(msg.length >= (sizeof(msg.payload) - 1)) {
      throw std::runtime_error("message trop grand !");
   }
   if( ! read_socket(client, reinterpret_cast<uint8_t *>(&msg.payload), msg.length)) {
      return false;
   }
   msg.payload[msg.length] = '\0';
   fprintf(stderr, "%s:%d: JSON connector : request: %s\n", __FILE__, __LINE__, msg.payload);
   json request = json::parse(msg.payload);
   const std::string command = to_lower(request.at("command"));
   json response = {
      { "command", command }
   };
   try {
      if(command == "status") {
         status(client, response);
      }
      else if(command == "show servers") {
         showServers(client, response);
      }
      else if(command == "connect") {
         connect(client, request, response);
      }
      else if(command == "show log") {
         showLog(client, response);
      }
      else if(command == "reset") {
         reset(client, response);
      }
      else if(command == "show download") {
         showDownload(client, response);
      }
      else if(command == "rename") {
         rename(client, request, response);
      }
      else if(command == "cancel") {
         cancel(client, request, response);
      }
      else if(command == "show upload") {
         showUpload(client, response);
      }
      else if(command == "reload shared") {
         reloadShared(client, response);
      }
      else if(command == "search") {
         startSearch(client, request, response);
      }
      else if(command == "progress") {
         progress(client, response);
      }
      else if(command == "results") {
         results(client, response);
      }
      else if(command == "download") {
         download(client, request, response);
      }
      else {
         throw std::runtime_error(buildMessage("commande non supportée: ", command));
      }
      return true;
   }
   catch(const std::exception & e) {
      std::string message = buildMessage(__func__, ": ", e.what());
      AddLogLineC(message);
      response["error"] = message;
      sendJsonMsg(client, response);
   }
   catch(...) {
      std::string message = buildMessage(__func__, ": exception inconnue");
      AddLogLineC(message);
      response["error"] = message;
      sendJsonMsg(client, response);
   }
   return false;
}

void JSON_Connector::acceptLoop() noexcept {
   AddLogLineNS("JSON connector : écoute.");
   while(_socket > -1) {
      int client = ::accept(_socket, nullptr, nullptr);
      if(client < 0) {
         if(_socket > -1) {
            AddLogLineC(buildMessage(__func__, ": ", strerror(errno)));
         }
      }
      else {
         try {
            AddLogLineNS("JSON connector : nouveau client connecté.");
            while((_socket > -1)&&(client > -1)&& dialog(client));
         }
         catch(const std::exception & e) {
            AddLogLineC(buildMessage(__func__, ": ", e.what()));
         }
         catch(...) {
            AddLogLineC(buildMessage(__func__, ": exception inconnue"));
         }
         ::close(client);
      }
   }
}

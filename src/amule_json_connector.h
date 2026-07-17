#include <nlohmann/json.hpp>

class CamuleApp; 

namespace hpms {

   class JSON_Connector {
   public:

      JSON_Connector(unsigned short port, CamuleApp & amule);
      JSON_Connector(const JSON_Connector&) = delete;
      JSON_Connector& operator=(const JSON_Connector&) = delete;
      ~ JSON_Connector();

   private:

      void sendJsonMsg(int socket, const nlohmann::json & response);
      void status(int socket, nlohmann::json & response);
      void showServers(int socket, nlohmann::json & response);
      void connect(int socket, const nlohmann::json & request, nlohmann::json & response);
      void showLog(int socket, nlohmann::json & response);
      void reset(int socket, nlohmann::json & response);
      void showDownload(int socket, nlohmann::json & response);
      void rename(int socket, const nlohmann::json & request, nlohmann::json & response);
      void cancel(int socket, const nlohmann::json & request, nlohmann::json & response);
      void showUpload(int socket, nlohmann::json & response);
      void reloadShared(int socket, nlohmann::json & response);
      void startSearch(int socket, const nlohmann::json & request, nlohmann::json & response);
      void progress(int socket, nlohmann::json & response);
      void results(int socket, nlohmann::json & response);
      void download(int socket, const nlohmann::json & request, nlohmann::json & response);
      bool read_socket(int client, uint8_t * buffer, size_t length);
      bool dialog(int client);
      void acceptLoop() noexcept;

   private:

      int         _socket;
      CamuleApp & _amule;
      uint32_t    _searchID;
   };
}

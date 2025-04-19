#pragma once

#include <string>
#include <vector>
#include <memory>
#include <cstdint>

namespace wfs_client
{

  // Error information structure
  struct WfsErrorInfo
  {
    int32_t code{0};
    std::string info;

    WfsErrorInfo() = default;
    WfsErrorInfo(int32_t c, const std::string &i) : code(c), info(i) {}

    bool isSet() const { return code != 0 || !info.empty(); }
  };

  // Operation result structure
  struct WfsResult
  {
    bool ok{false};
    WfsErrorInfo error;

    WfsResult() = default;
    WfsResult(bool success) : ok(success) {}
    WfsResult(bool success, const WfsErrorInfo &err) : ok(success), error(err) {}

    // Helper methods for returning operation results
    static WfsResult Success() { return WfsResult(true); }
    static WfsResult Failure(int32_t code, const std::string &message)
    {
      return WfsResult(false, WfsErrorInfo(code, message));
    }

    // Implicit conversion to boolean for direct use in conditions
    operator bool() const { return ok; }
  };

  // File data structure
  struct WfsFileData
  {
    std::string data;
    std::string name;
    int8_t compress{0};

    WfsFileData() = default;
    WfsFileData(const std::string &n, const std::string &d, int8_t c = 0)
        : data(d), name(n), compress(c) {}
  };

  // Directory item information
  struct WfsDirItem
  {
    std::string name;
    int64_t size{0};
    int64_t mtime{0};
    bool isDir{false};

    WfsDirItem() = default;
    WfsDirItem(const std::string &n, int64_t s, int64_t m, bool is_dir)
        : name(n), size(s), mtime(m), isDir(is_dir) {}
  };

  // Directory list structure
  struct WfsDirList
  {
    std::string path;
    std::vector<WfsDirItem> items;
    WfsErrorInfo error;
  };

  // Authentication information
  struct WfsAuthInfo
  {
    std::string username;
    std::string password;

    WfsAuthInfo() = default;
    WfsAuthInfo(const std::string &user, const std::string &pwd)
        : username(user), password(pwd) {}
  };

  // Connection parameters
  struct WfsConnectionParams
  {
    std::string serverIp;
    int serverPort{9090};
    int connectTimeout{10000}; // milliseconds
    int receiveTimeout{30000}; // milliseconds
    int sendTimeout{30000};    // milliseconds
    int maxRetries{3};

    WfsConnectionParams() = default;
    WfsConnectionParams(const std::string &ip, int port)
        : serverIp(ip), serverPort(port) {}
  };

} // namespace wfs_client
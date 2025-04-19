#pragma once

#include "wfs_client/datatype_.hpp"
#include "wfs_client/wfs_exports.hpp"
#include <string>
#include <memory>

namespace wfs_client
{

  // WFS Client Interface
  class IWfsClient
  {
  public:
    virtual ~IWfsClient() = default;

    // Connect to server
    virtual WfsResult Connect(const WfsConnectionParams &params) = 0;

    // Reconnect to server
    virtual WfsResult Reconnect() = 0;

    // Disconnect from server
    virtual void Disconnect() = 0;

    // Authenticate
    virtual WfsResult Authenticate(const WfsAuthInfo &authInfo) = 0;

    // Upload file
    virtual WfsResult UploadFile(const WfsFileData &fileData) = 0;

    // Download file
    virtual WfsResult DownloadFile(const std::string &remotePath, std::string &outData) = 0;

    // Delete file
    virtual WfsResult DeleteFile(const std::string &remotePath) = 0;

    // Rename file
    virtual WfsResult RenameFile(const std::string &oldPath, const std::string &newPath) = 0;

    // List directory contents
    virtual WfsResult ListDirectory(const std::string &remotePath, WfsDirList &outDirList) = 0;

    // Test connection
    virtual int8_t Ping() = 0;

    // Check if connected
    virtual bool IsConnected() const = 0;

    // Check if authenticated
    virtual bool IsAuthenticated() const = 0;

    // Get last error
    virtual WfsErrorInfo GetLastError() const = 0;
  };

  // Factory function with connection parameters and authentication
  WFS_CLIENT_API bool CreateWfsClient(std::shared_ptr<IWfsClient> &client,
                                      const WfsConnectionParams &params,
                                      const WfsAuthInfo &authInfo);

} // namespace wfs_client
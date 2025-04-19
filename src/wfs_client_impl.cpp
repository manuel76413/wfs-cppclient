#include <Windows.h>
#include <fmt/color.h>
#include <fmt/core.h>
#include <thrift/protocol/TCompactProtocol.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportException.h>
#include <thrift/transport/TTransportUtils.h>

#include <chrono>
#include <mutex>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <thread>

#include "gen-cpp/WfsIface.h"
#include "gen-cpp/wfs_types.h"
#include "wfs_client/iwfs_client.hpp"

using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;

namespace wfs_client
{

  // Forward declaration of implementation class
  class WfsClientImpl;

  // Helper function - Get readable description of exception type
  static std::string getExceptionTypeStr(TTransportException::TTransportExceptionType type)
  {
    switch (type)
    {
    case TTransportException::UNKNOWN:
      return "UNKNOWN";
    case TTransportException::NOT_OPEN:
      return "NOT_OPEN";
    case TTransportException::TIMED_OUT:
      return "TIMED_OUT";
    case TTransportException::END_OF_FILE:
      return "END_OF_FILE";
    case TTransportException::INTERRUPTED:
      return "INTERRUPTED";
    case TTransportException::BAD_ARGS:
      return "BAD_ARGS";
    case TTransportException::CORRUPTED_DATA:
      return "CORRUPTED_DATA";
    case TTransportException::INTERNAL_ERROR:
      return "INTERNAL_ERROR";
    default:
    {
      char buffer[32];
      snprintf(buffer, sizeof(buffer), "Unknown type(%d)", static_cast<int>(type));
      return buffer;
    }
    }
  }

  // Actual client implementation
  class WfsClientImpl : public IWfsClient
  {
  public:
    WfsClientImpl()
        : m_isConnected(false),
          m_isAuthenticated(false)
    {
    }

    ~WfsClientImpl() override
    {
      Disconnect();
    }

    WfsResult Connect(const WfsConnectionParams &params) override
    {
      std::lock_guard<std::mutex> lock(m_mutex);

      if (m_isConnected)
      {
        Disconnect();
      }

      m_params = params;
      return ConnectInternal();
    }

    WfsResult Reconnect() override
    {
      std::lock_guard<std::mutex> lock(m_mutex);

      if (m_isConnected)
      {
        Disconnect();
      }

      return ConnectInternal();
    }

    void Disconnect() override
    {
      std::lock_guard<std::mutex> lock(m_mutex);

      if (m_isConnected && m_client)
      {
        try
        {
          m_client->getInputProtocol()->getTransport()->close();
          fmt::print(fg(fmt::color::red), "WFS client is disconnected\n");
        }
        catch (...)
        {
          // Ignore exceptions during disconnection
        }
      }

      m_isConnected = false;
      m_isAuthenticated = false;
      m_client.reset();
    }

    WfsResult Authenticate(const WfsAuthInfo &authInfo) override
    {
      std::lock_guard<std::mutex> lock(m_mutex);

      if (!EnsureConnected())
      {
        return m_lastError;
      }

      m_authInfo = authInfo;
      return AuthenticateInternal();
    }

    WfsResult UploadFile(const WfsFileData &fileData) override
    {
      std::lock_guard<std::mutex> lock(m_mutex);

      if (!EnsureConnectedAndAuthenticated())
      {
        return m_lastError;
      }

      try
      {
        // Create WfsFile object
        WfsFile wf;
        wf.__set_data(fileData.data);
        wf.__set_name(fileData.name);
        if (fileData.compress != 0)
        {
          wf.__set_compress(fileData.compress);
        }

        // Call Append interface
        WfsAck ack;
        m_client->Append(ack, wf);

        // Process result
        if (ack.ok)
        {
          fmt::print(fg(fmt::color::green), "File upload successful: {}\n", fileData.name);
          return WfsResult::Success();
        }
        else
        {
          fmt::print(fg(fmt::color::red), "File upload failed: {} - {}\n",
                     ack.error.code, ack.error.info);
          return CreateErrorResult(ack.error);
        }
      }
      catch (const TTransportException &e)
      {
        HandleTransportException(e, "File upload");
        return m_lastError;
      }
      catch (const TException &e)
      {
        HandleThriftException(e, "File upload");
        return m_lastError;
      }
      catch (const std::exception &e)
      {
        HandleStandardException(e, "File upload");
        return m_lastError;
      }
      catch (...)
      {
        HandleUnknownException("File upload");
        return m_lastError;
      }
    }

    WfsResult DownloadFile(const std::string &remotePath, std::string &outData) override
    {
      std::lock_guard<std::mutex> lock(m_mutex);

      if (!EnsureConnectedAndAuthenticated())
      {
        return m_lastError;
      }

      try
      {
        // Call Get interface
        WfsData data;
        m_client->Get(data, remotePath);

        // Check result
        if (data.__isset.data)
        {
          outData = data.data;
          fmt::print(fg(fmt::color::green), "File download successful: {} ({} bytes)\n",
                     remotePath, outData.size());
          return WfsResult::Success();
        }
        else
        {
          fmt::print(fg(fmt::color::red), "File download failed: data is empty\n");
          m_lastError = WfsResult::Failure(-1, "Download failed: no data received");
          return m_lastError;
        }
      }
      catch (const TTransportException &e)
      {
        HandleTransportException(e, "File download");
        return m_lastError;
      }
      catch (const TException &e)
      {
        HandleThriftException(e, "File download");
        return m_lastError;
      }
      catch (const std::exception &e)
      {
        HandleStandardException(e, "File download");
        return m_lastError;
      }
      catch (...)
      {
        HandleUnknownException("File download");
        return m_lastError;
      }
    }

    WfsResult DeleteFile(const std::string &remotePath) override
    {
      std::lock_guard<std::mutex> lock(m_mutex);

      if (!EnsureConnectedAndAuthenticated())
      {
        return m_lastError;
      }

      try
      {
        // Call Delete interface
        WfsAck ack;
        m_client->Delete(ack, remotePath);

        // Process result
        if (ack.ok)
        {
          fmt::print(fg(fmt::color::green), "File deletion successful: {}\n", remotePath);
          return WfsResult::Success();
        }
        else
        {
          fmt::print(fg(fmt::color::red), "File deletion failed: {} - {}\n",
                     ack.error.code, ack.error.info);
          return CreateErrorResult(ack.error);
        }
      }
      catch (const TTransportException &e)
      {
        HandleTransportException(e, "File deletion");
        return m_lastError;
      }
      catch (const TException &e)
      {
        HandleThriftException(e, "File deletion");
        return m_lastError;
      }
      catch (const std::exception &e)
      {
        HandleStandardException(e, "File deletion");
        return m_lastError;
      }
      catch (...)
      {
        HandleUnknownException("File deletion");
        return m_lastError;
      }
    }

    WfsResult RenameFile(const std::string &oldPath, const std::string &newPath) override
    {
      std::lock_guard<std::mutex> lock(m_mutex);

      if (!EnsureConnectedAndAuthenticated())
      {
        return m_lastError;
      }

      try
      {
        // Call Rename interface
        WfsAck ack;
        m_client->Rename(ack, oldPath, newPath);

        // Process result
        if (ack.ok)
        {
          fmt::print(fg(fmt::color::green), "File rename successful: {} -> {}\n", oldPath, newPath);
          return WfsResult::Success();
        }
        else
        {
          fmt::print(fg(fmt::color::red), "File rename failed: {} - {}\n",
                     ack.error.code, ack.error.info);
          return CreateErrorResult(ack.error);
        }
      }
      catch (const TTransportException &e)
      {
        HandleTransportException(e, "File rename");
        return m_lastError;
      }
      catch (const TException &e)
      {
        HandleThriftException(e, "File rename");
        return m_lastError;
      }
      catch (const std::exception &e)
      {
        HandleStandardException(e, "File rename");
        return m_lastError;
      }
      catch (...)
      {
        HandleUnknownException("File rename");
        return m_lastError;
      }
    }

    WfsResult ListDirectory(const std::string &remotePath, WfsDirList &outDirList) override
    {
      std::lock_guard<std::mutex> lock(m_mutex);

      if (!EnsureConnectedAndAuthenticated())
      {
        return m_lastError;
      }

      try
      {
        // Call List interface
        DirList dirList;
        m_client->List(dirList, remotePath);

        // Process results and convert to local structure
        outDirList.path = dirList.path;

        if (dirList.__isset.error && dirList.error.__isset.code)
        {
          outDirList.error.code = dirList.error.code;
          outDirList.error.info = dirList.error.info;

          fmt::print(fg(fmt::color::red), "Directory listing failed: {} - {}\n",
                     dirList.error.code, dirList.error.info);
          return CreateErrorResult(dirList.error);
        }

        // Convert directory items
        outDirList.items.clear();
        for (const auto &item : dirList.items)
        {
          WfsDirItem dirItem;
          dirItem.name = item.name;
          dirItem.size = item.size;
          dirItem.mtime = item.mtime;
          dirItem.isDir = item.isDir;
          outDirList.items.push_back(dirItem);
        }

        fmt::print(fg(fmt::color::green), "Directory listing successful: {} (total {} items)\n",
                   remotePath, outDirList.items.size());
        return WfsResult::Success();
      }
      catch (const TTransportException &e)
      {
        HandleTransportException(e, "List directory");
        return m_lastError;
      }
      catch (const TException &e)
      {
        HandleThriftException(e, "List directory");
        return m_lastError;
      }
      catch (const std::exception &e)
      {
        HandleStandardException(e, "List directory");
        return m_lastError;
      }
      catch (...)
      {
        HandleUnknownException("List directory");
        return m_lastError;
      }
    }

    int8_t Ping() override
    {
      std::lock_guard<std::mutex> lock(m_mutex);

      if (!EnsureConnected())
      {
        return -1;
      }

      try
      {
        int8_t result = m_client->Ping();
        fmt::print(fg(fmt::color::green), "Ping successful, return value: {}\n", result);
        return result;
      }
      catch (const TTransportException &e)
      {
        HandleTransportException(e, "Ping");
        return -1;
      }
      catch (const TException &e)
      {
        HandleThriftException(e, "Ping");
        return -1;
      }
      catch (const std::exception &e)
      {
        HandleStandardException(e, "Ping");
        return -1;
      }
      catch (...)
      {
        HandleUnknownException("Ping");
        return -1;
      }
    }

    bool IsConnected() const override
    {
      std::lock_guard<std::mutex> lock(m_mutex);
      return m_isConnected;
    }

    bool IsAuthenticated() const override
    {
      std::lock_guard<std::mutex> lock(m_mutex);
      return m_isAuthenticated;
    }

    WfsErrorInfo GetLastError() const override
    {
      std::lock_guard<std::mutex> lock(m_mutex);
      return m_lastError.error;
    }

  private:
    // Internal connection method
    WfsResult ConnectInternal()
    {
      try
      {
        fmt::print("Connecting to server: {}:{}\n", m_params.serverIp, m_params.serverPort);

        // Create socket
        m_socket.reset(new TSocket(m_params.serverIp, m_params.serverPort));

        // Set timeout parameters
        m_socket->setConnTimeout(m_params.connectTimeout);
        m_socket->setRecvTimeout(m_params.receiveTimeout);
        m_socket->setSendTimeout(m_params.sendTimeout);

        // Create transport layer and protocol
        m_transport.reset(new TBufferedTransport(m_socket, 8192));
        m_protocol.reset(new TCompactProtocol(m_transport));

        // Create client
        m_client.reset(new WfsIfaceClient(m_protocol));

        // Open connection
        fmt::print(fg(fmt::color::yellow), "Connecting to server...\n");
        m_transport->open();
        m_isConnected = true;
        fmt::print(fg(fmt::color::green), "Connected to server successfully\n");

        return WfsResult::Success();
      }
      catch (const TTransportException &e)
      {
        HandleTransportException(e, "Connect to server");
        return m_lastError;
      }
      catch (const TException &e)
      {
        HandleThriftException(e, "Connect to server");
        return m_lastError;
      }
      catch (const std::exception &e)
      {
        HandleStandardException(e, "Connect to server");
        return m_lastError;
      }
      catch (...)
      {
        HandleUnknownException("Connect to server");
        return m_lastError;
      }
    }

    // Internal authentication method
    WfsResult AuthenticateInternal()
    {
      if (!m_isConnected)
      {
        m_lastError = WfsResult::Failure(-1, "Not connected to server");
        return m_lastError;
      }

      fmt::print(fg(fmt::color::yellow), "Authenticating (username: {})...\n", m_authInfo.username);

      try
      {
        // Create authentication request
        WfsAuth auth;
        auth.__set_name(m_authInfo.username);
        auth.__set_pwd(m_authInfo.password);

        // Send authentication request
        fmt::print("Sending authentication request...\n");
        WfsAck authResult;

        // Add retry logic
        for (int retry = 1; retry <= m_params.maxRetries; retry++)
        {
          try
          {
            m_client->Auth(authResult, auth);

            // Check authentication result
            if (authResult.ok)
            {
              fmt::print(fg(fmt::color::green), "Authentication successful\n");
              m_isAuthenticated = true;
              return WfsResult::Success();
            }
            else
            {
              fmt::print(fg(fmt::color::red), "Authentication failed: {} - {}\n",
                         authResult.error.code, authResult.error.info);
              m_isAuthenticated = false;
              return CreateErrorResult(authResult.error);
            }
          }
          catch (const TTransportException &e)
          {
            fmt::print(fg(fmt::color::yellow),
                       "Authentication attempt {}/{} failed: {}\nError type: {}\n",
                       retry, m_params.maxRetries, e.what(),
                       getExceptionTypeStr(e.getType()));

            if (retry < m_params.maxRetries)
            {
              fmt::print("Waiting 3 seconds before retry...\n");
              std::this_thread::sleep_for(std::chrono::seconds(3));

              // Try to reconnect
              try
              {
                fmt::print("Attempting to reconnect...\n");
                m_client->getInputProtocol()->getTransport()->close();
                m_client->getInputProtocol()->getTransport()->open();
                fmt::print(fg(fmt::color::green), "Reconnection successful\n");
              }
              catch (const TTransportException &recon_e)
              {
                fmt::print(fg(fmt::color::red), "Reconnection failed: {}\n", recon_e.what());
                m_isConnected = false;
              }
            }
            else
            {
              fmt::print(fg(fmt::color::red), "Authentication failed, maximum retries reached\n");
              m_isAuthenticated = false;
              m_lastError = WfsResult::Failure(-1, std::string("Authentication exception: ") + e.what());
              return m_lastError;
            }
          }
        }

        m_isAuthenticated = false;
        m_lastError = WfsResult::Failure(-1, "Authentication failed, retry limit reached");
        return m_lastError;
      }
      catch (const TException &e)
      {
        HandleThriftException(e, "Authentication");
        m_isAuthenticated = false;
        return m_lastError;
      }
      catch (const std::exception &e)
      {
        HandleStandardException(e, "Authentication");
        m_isAuthenticated = false;
        return m_lastError;
      }
      catch (...)
      {
        HandleUnknownException("Authentication");
        m_isAuthenticated = false;
        return m_lastError;
      }
    }

    // Ensure connection status
    bool EnsureConnected()
    {
      if (!m_isConnected)
      {
        fmt::print(fg(fmt::color::red), "Operation failed: Not connected to server\n");
        m_lastError = WfsResult::Failure(-1, "Not connected to server");
        return false;
      }
      return true;
    }

    // Ensure connection and authentication status
    bool EnsureConnectedAndAuthenticated()
    {
      if (!EnsureConnected())
      {
        return false;
      }

      if (!m_isAuthenticated)
      {
        fmt::print(fg(fmt::color::red), "Operation failed: Not authenticated\n");
        m_lastError = WfsResult::Failure(-1, "Not authenticated");
        return false;
      }
      return true;
    }

    // Create error result from Thrift error
    WfsResult CreateErrorResult(const WfsError &error)
    {
      m_lastError = WfsResult::Failure(error.code, error.info);
      return m_lastError;
    }

    // Exception handling methods
    void HandleTransportException(const TTransportException &e, const std::string &operation)
    {
      fmt::print(fg(fmt::color::red), "Transport exception during {}: {}\nError type: {}\n",
                 operation, e.what(), getExceptionTypeStr(e.getType()));
      m_lastError = WfsResult::Failure(-1, std::string("Transport exception: ") + e.what());

      // Connection may be broken
      if (e.getType() == TTransportException::NOT_OPEN ||
          e.getType() == TTransportException::END_OF_FILE)
      {
        m_isConnected = false;
        m_isAuthenticated = false;
      }
    }

    void HandleThriftException(const TException &e, const std::string &operation)
    {
      fmt::print(fg(fmt::color::red), "Thrift exception during {}: {}\n", operation, e.what());
      m_lastError = WfsResult::Failure(-1, std::string("Thrift exception: ") + e.what());
    }

    void HandleStandardException(const std::exception &e, const std::string &operation)
    {
      fmt::print(fg(fmt::color::red), "Standard exception during {}: {}\n", operation, e.what());
      m_lastError = WfsResult::Failure(-1, std::string("Standard exception: ") + e.what());
    }

    void HandleUnknownException(const std::string &operation)
    {
      fmt::print(fg(fmt::color::red), "Unknown exception during {}\n", operation);
      m_lastError = WfsResult::Failure(-1, "Unknown exception");
    }

  private:
    mutable std::mutex m_mutex;
    bool m_isConnected;
    bool m_isAuthenticated;
    WfsConnectionParams m_params;
    WfsAuthInfo m_authInfo;
    WfsResult m_lastError;

    // Thrift client related objects
    std::shared_ptr<TSocket> m_socket;
    std::shared_ptr<TTransport> m_transport;
    std::shared_ptr<TProtocol> m_protocol;
    std::shared_ptr<WfsIfaceClient> m_client;
  };

  bool CreateWfsClient(
      std::shared_ptr<IWfsClient> &client,
      const WfsConnectionParams &params,
      const WfsAuthInfo &authInfo)
  {
    client = std::make_shared<WfsClientImpl>();
    if (client)
    {
      fmt::print(fg(fmt::color::green), "Client creation successful\n");
    }
    else
    {
      fmt::print(fg(fmt::color::red), "Client creation failed\n");
      return false;
    }

    WfsResult wres = client->Connect(params);
    if (!wres)
      return false;
    wres = client->Authenticate(authInfo);
    return wres;
  }

} // namespace wfs_client
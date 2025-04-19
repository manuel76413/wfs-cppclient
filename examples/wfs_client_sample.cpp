#include <fmt/color.h>
#include <fmt/core.h>
#include <iostream>
#include <string>
#include <chrono>
#include <thread>

#include "wfs_client/iwfs_client.hpp"
#include "wfs_client/utils.hpp"

using namespace wfs_client;
using namespace wfs_client::utils;

// Command line help information
void showHelp(const char *programName)
{
  fmt::print(
      "Usage: {} <server_ip> <port> <username> <password> <operation> [parameters...]\n"
      "\n"
      "Operations:\n"
      "  upload <local_file_path> <remote_file_path>  - Upload file\n"
      "  download <remote_file_path> <local_file_path> - Download file\n"
      "  delete <remote_file_path>                 - Delete file\n"
      "  rename <original_file_path> <new_file_path>      - Rename file\n"
      "  list <remote_directory_path>                   - List directory contents\n"
      "  ping                                  - Test connection\n",
      programName);
}

// Upload file example
bool uploadFile(IWfsClient &client, const std::string &localPath, const std::string &remotePath)
{
  fmt::print(fg(fmt::color::cyan), "\n--- Upload File ---\n");

  try
  {
    // Read local file
    std::string fileData = readFile(localPath);
    fmt::print("Reading local file: {} ({} bytes)\n", localPath, fileData.size());

    // Create file data
    WfsFileData fileInfo{remotePath, fileData};

    // Upload file
    WfsResult result = client.UploadFile(fileInfo);

    if (result)
    {
      fmt::print(fg(fmt::color::green), "File uploaded successfully!\n");
      return true;
    }
    else
    {
      fmt::print(fg(fmt::color::red), "Upload failed: {} - {}\n",
                 result.error.code, result.error.info);
      return false;
    }
  }
  catch (const std::exception &e)
  {
    fmt::print(fg(fmt::color::red), "Exception during file upload: {}\n", e.what());
    return false;
  }
}

// Download file example
bool downloadFile(IWfsClient &client, const std::string &remotePath, const std::string &localPath)
{
  fmt::print(fg(fmt::color::cyan), "\n--- Download File ---\n");

  try
  {
    // Download file
    std::string fileData;
    WfsResult result = client.DownloadFile(remotePath, fileData);

    if (result)
    {
      // Save file
      writeFile(localPath, fileData);
      fmt::print(fg(fmt::color::green), "File downloaded successfully: {} -> {} ({} bytes)\n",
                 remotePath, localPath, fileData.size());
      return true;
    }
    else
    {
      fmt::print(fg(fmt::color::red), "Download failed: {} - {}\n",
                 result.error.code, result.error.info);
      return false;
    }
  }
  catch (const std::exception &e)
  {
    fmt::print(fg(fmt::color::red), "Exception during file download: {}\n", e.what());
    return false;
  }
}

// Delete file example
bool deleteFile(IWfsClient &client, const std::string &remotePath)
{
  fmt::print(fg(fmt::color::cyan), "\n--- Delete File ---\n");

  WfsResult result = client.DeleteFile(remotePath);

  if (result)
  {
    fmt::print(fg(fmt::color::green), "File deleted successfully: {}\n", remotePath);
    return true;
  }
  else
  {
    fmt::print(fg(fmt::color::red), "Deletion failed: {} - {}\n",
               result.error.code, result.error.info);
    return false;
  }
}

// Rename file example
bool renameFile(IWfsClient &client, const std::string &oldPath, const std::string &newPath)
{
  fmt::print(fg(fmt::color::cyan), "\n--- Rename File ---\n");

  WfsResult result = client.RenameFile(oldPath, newPath);

  if (result)
  {
    fmt::print(fg(fmt::color::green), "File renamed successfully: {} -> {}\n", oldPath, newPath);
    return true;
  }
  else
  {
    fmt::print(fg(fmt::color::red), "Rename failed: {} - {}\n",
               result.error.code, result.error.info);
    return false;
  }
}

// List directory example
bool listDirectory(IWfsClient &client, const std::string &remotePath)
{
  fmt::print(fg(fmt::color::cyan), "\n--- List Directory ---\n");

  WfsDirList dirList;
  WfsResult result = client.ListDirectory(remotePath, dirList);

  if (result)
  {
    fmt::print(fg(fmt::color::green), "Directory listing ({}):\n", remotePath);

    // Print header
    fmt::print("| {:<30} | {:<10} | {:<20} | {:<5} |\n", "Name", "Size(bytes)", "Modified Time", "Type");
    fmt::print("|--------------------------------|------------|----------------------|-------|\n");

    // Print directory contents
    for (const auto &item : dirList.items)
    {
      // Format modification time
      auto mtime = std::chrono::system_clock::time_point(
          std::chrono::seconds(item.mtime));
      auto tm = std::chrono::system_clock::to_time_t(mtime);
      std::string timeStr = std::ctime(&tm);
      if (!timeStr.empty() && timeStr.back() == '\n')
      {
        timeStr.pop_back(); // Remove newline character
      }

      fmt::print("| {:<30} | {:<10} | {:<20} | {:<5} |\n",
                 item.name,
                 item.size,
                 timeStr,
                 item.isDir ? "Dir" : "File");
    }

    return true;
  }
  else
  {
    fmt::print(fg(fmt::color::red), "Failed to get directory listing: {} - {}\n",
               result.error.code, result.error.info);
    return false;
  }
}

// Test connection example
bool pingServer(IWfsClient &client)
{
  fmt::print(fg(fmt::color::cyan), "\n--- Test Connection ---\n");

  int8_t response = client.Ping();

  if (response >= 0)
  {
    fmt::print(fg(fmt::color::green), "Server responded to ping: {}\n", response);
    return true;
  }
  else
  {
    fmt::print(fg(fmt::color::red), "Ping failed\n");
    return false;
  }
}

int main(int argc, char *argv[])
{
  // Check number of arguments
  if (argc < 6)
  {
    showHelp(argv[0]);
    return 1;
  }

  // Parse basic parameters
  std::string serverIp = argv[1];
  int serverPort = std::stoi(argv[2]);
  std::string username = argv[3];
  std::string password = argv[4];
  std::string operation = argv[5];

  fmt::print("WFS Client Example Program\n");
  fmt::print("Server: {}:{}\n", serverIp, serverPort);
  fmt::print("Username: {}\n", username);

  // Create client
  WfsConnectionParams connParams;
  connParams.serverIp = serverIp;
  connParams.serverPort = serverPort;

  WfsAuthInfo authInfo;
  authInfo.username = username;
  authInfo.password = password;

  // Create and initialize client
  std::shared_ptr<IWfsClient> client;
  bool br = CreateWfsClient(client, connParams, authInfo);
  if (br)
  {
    fmt::print(fg(fmt::color::green), "Client created successfully\n");
  }
  else
  {
    fmt::print(fg(fmt::color::red), "Failed to create client\n");
    return -1;
  }

  // Execute operation
  bool operationSuccess = false;

  if (operation == "upload")
  {
    if (argc < 8)
    {
      fmt::print(fg(fmt::color::red), "Missing parameters for upload operation\n");
      showHelp(argv[0]);
      return 1;
    }
    std::string localPath = argv[6];
    std::string remotePath = argv[7];
    operationSuccess = uploadFile(*client, localPath, remotePath);
  }
  else if (operation == "download")
  {
    if (argc < 8)
    {
      fmt::print(fg(fmt::color::red), "Missing parameters for download operation\n");
      showHelp(argv[0]);
      return 1;
    }
    std::string remotePath = argv[6];
    std::string localPath = argv[7];
    operationSuccess = downloadFile(*client, remotePath, localPath);
  }
  else if (operation == "delete")
  {
    if (argc < 7)
    {
      fmt::print(fg(fmt::color::red), "Missing parameters for delete operation\n");
      showHelp(argv[0]);
      return 1;
    }
    std::string remotePath = argv[6];
    operationSuccess = deleteFile(*client, remotePath);
  }
  else if (operation == "rename")
  {
    if (argc < 8)
    {
      fmt::print(fg(fmt::color::red), "Missing parameters for rename operation\n");
      showHelp(argv[0]);
      return 1;
    }
    std::string oldPath = argv[6];
    std::string newPath = argv[7];
    operationSuccess = renameFile(*client, oldPath, newPath);
  }
  else if (operation == "list")
  {
    if (argc < 7)
    {
      fmt::print(fg(fmt::color::red), "Missing parameters for list operation\n");
      showHelp(argv[0]);
      return 1;
    }
    std::string remotePath = argv[6];
    operationSuccess = listDirectory(*client, remotePath);
  }
  else if (operation == "ping")
  {
    operationSuccess = pingServer(*client);
  }
  else
  {
    fmt::print(fg(fmt::color::red), "Unknown operation: {}\n", operation);
    showHelp(argv[0]);
    return 1;
  }

  // Display operation result
  fmt::print("\n{}\n", operationSuccess ? "Operation completed successfully" : "Operation failed");
  return operationSuccess ? 0 : 1;
}
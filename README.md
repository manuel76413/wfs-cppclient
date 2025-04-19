# WFS C++ Client

C++ client library for interacting with WFS (Web File System) service using Apache Thrift protocol.

## Features

- Complete C++ client implementation for [WFS](https://github.com/donnie4w/wfs)
- Supports file operations: upload, download, delete, rename
- Directory listing and navigation
- Connection management and authentication
- Exception handling and error reporting
- UTF-8 encoding support
- Colorful console output with fmt library

## Requirements

- C++20 compatible compiler
- CMake 3.15+
- Dependencies:
  - Apache Thrift
  - fmt library

## Building

```bash
# Configure build
cmake -B build

# Build
cmake --build build --config Release

# Install
cmake --install build
```

## Usage Example

```cpp
#include <wfs_client/iwfs_client.hpp>

// Create connection parameters
wfs_client::WfsConnectionParams params;
params.serverIp = "127.0.0.1";
params.serverPort = 9090;

// Create authentication info
wfs_client::WfsAuthInfo auth;
auth.username = "user";
auth.password = "password";

// Create client
std::shared_ptr<wfs_client::IWfsClient> client;
bool success = wfs_client::CreateWfsClient(client, params, auth);

if (success) {
    // Upload a file
    wfs_client::WfsFileData fileData;
    fileData.name = "remote_path/file.txt";
    fileData.data = "File content";
    
    wfs_client::WfsResult result = client->UploadFile(fileData);
    if (result) {
        // Success
    }
}
```

## License

BSD-3-Clause license 
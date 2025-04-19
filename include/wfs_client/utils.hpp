#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <stdexcept>

namespace wfs_client
{
  namespace utils
  {

    // UTF-8 safe display function
    inline std::string safeUtf8(const std::string &input)
    {
      std::string result;
      result.reserve(input.size());
      for (size_t i = 0; i < input.size(); ++i)
      {
        const unsigned char c = input[i];
        if (c < 0x80)
        {
          // ASCII character
          result.push_back(c);
        }
        else if ((c & 0xE0) == 0xC0)
        {
          // 2-byte UTF-8 sequence
          if (i + 1 < input.size() && (input[i + 1] & 0xC0) == 0x80)
          {
            result.push_back(c);
            result.push_back(input[++i]);
          }
          else
          {
            result.append("?");
          }
        }
        else if ((c & 0xF0) == 0xE0)
        {
          // 3-byte UTF-8 sequence
          if (i + 2 < input.size() && (input[i + 1] & 0xC0) == 0x80 &&
              (input[i + 2] & 0xC0) == 0x80)
          {
            result.push_back(c);
            result.push_back(input[++i]);
            result.push_back(input[++i]);
          }
          else
          {
            result.append("?");
          }
        }
        else if ((c & 0xF8) == 0xF0)
        {
          // 4-byte UTF-8 sequence
          if (i + 3 < input.size() && (input[i + 1] & 0xC0) == 0x80 &&
              (input[i + 2] & 0xC0) == 0x80 && (input[i + 3] & 0xC0) == 0x80)
          {
            result.push_back(c);
            result.push_back(input[++i]);
            result.push_back(input[++i]);
            result.push_back(input[++i]);
          }
          else
          {
            result.append("?");
          }
        }
        else
        {
          // Invalid UTF-8 sequence
          result.append("?");
        }
      }
      return result;
    }

    // Read binary file
    inline std::string readFile(const std::string &filename)
    {
      std::ifstream file(filename, std::ios::binary);
      if (!file)
      {
        throw std::runtime_error("Cannot open file: " + filename);
      }

      file.seekg(0, std::ios::end);
      size_t size = file.tellg();
      std::string data(size, '\0');
      file.seekg(0);
      file.read(&data[0], size);

      return data;
    }

    // Write binary file
    inline void writeFile(const std::string &filename, const std::string &data)
    {
      std::ofstream file(filename, std::ios::binary);
      if (!file)
      {
        throw std::runtime_error("Cannot create file: " + filename);
      }

      file.write(data.data(), data.size());

      if (!file)
      {
        throw std::runtime_error("Failed to write file: " + filename);
      }
    }

    // Get file name part
    inline std::string getFileName(const std::string &path)
    {
      size_t pos = path.find_last_of("/\\");
      if (pos == std::string::npos)
      {
        return path;
      }
      return path.substr(pos + 1);
    }

    // Get directory part
    inline std::string getDirectory(const std::string &path)
    {
      size_t pos = path.find_last_of("/\\");
      if (pos == std::string::npos)
      {
        return "";
      }
      return path.substr(0, pos);
    }

    // Path combination
    inline std::string combinePath(const std::string &path1, const std::string &path2)
    {
      if (path1.empty())
      {
        return path2;
      }

      char lastChar = path1[path1.size() - 1];
      if (lastChar == '/' || lastChar == '\\')
      {
        return path1 + path2;
      }

      // Use the same separator as path1
      char separator = '/';
      if (path1.find('\\') != std::string::npos)
      {
        separator = '\\';
      }

      return path1 + separator + path2;
    }

  } // namespace utils
} // namespace wfs_client
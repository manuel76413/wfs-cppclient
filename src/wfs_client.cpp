#include <Windows.h>
#include <fmt/core.h>
#include <locale>
#include <iostream>
#include "wfs_client/iwfs_client.hpp"

// Set UTF-8 console code page
void SetUtf8Console()
{
  // Set console code page to UTF-8
  SetConsoleOutputCP(CP_UTF8);
  SetConsoleCP(CP_UTF8);

  // Apply utf-8 to std::cout
  std::locale::global(std::locale(""));
  std::ios_base::sync_with_stdio(false);

  fmt::print("Console set to UTF-8 encoding\n");
}

// Module initialization function, set console encoding
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
  switch (ul_reason_for_call)
  {
  case DLL_PROCESS_ATTACH:
    // When DLL is loaded, set UTF-8 console
    SetUtf8Console();
    break;
  case DLL_THREAD_ATTACH:
  case DLL_THREAD_DETACH:
  case DLL_PROCESS_DETACH:
    break;
  }
  return TRUE;
}

// The implementation of client factory function is in wfs_client_impl.cpp
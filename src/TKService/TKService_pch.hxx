#ifndef TKSERVICE_PCH_H
#define TKSERVICE_PCH_H

// Windows-specific headers (for MSVC)
#ifdef _WIN32
  #ifndef NOMINMAX
    #define NOMINMAX
  #endif
  #include <tchar.h>   // For Unicode/MBCS mappings
  #include <windows.h> // For Windows API functions like WideCharToMultiByte
  #ifdef GetObject
    #undef GetObject
  #endif
#endif

#include <Graphic3d_CStructure.hxx>
#include <Graphic3d_CView.hxx>
#include <Graphic3d_Structure.hxx>
#include <Graphic3d_StructureManager.hxx>

#endif // TKSERVICE_PCH_H

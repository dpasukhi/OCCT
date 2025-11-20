// Copyright (c) 2025 OPEN CASCADE SAS
//
// This file is part of Open CASCADE Technology software library.
//
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License version 2.1 as published
// by the Free Software Foundation, with special exception defined in the file
// OCCT_LGPL_EXCEPTION.txt. Consult the file LICENSE_LGPL_21.txt included in OCCT
// distribution for complete text of the license and disclaimer of any warranty.
//
// Alternatively, this file may be used under the terms of Open CASCADE
// commercial license or contractual agreement.

#include <ODE_Manifest.hxx>
#include <fstream>
#include <sstream>
#include <ctime>

IMPLEMENT_STANDARD_RTTIEXT(ODE_Manifest, Standard_Transient)

//=================================================================================================

ODE_Manifest::ODE_Manifest()
: myVersion("1.0")
{
  // Generate ISO 8601 timestamp
  std::time_t now = std::time(nullptr);
  std::tm* utc = std::gmtime(&now);
  char buffer[32];
  std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", utc);
  myCreated = TCollection_AsciiString(buffer);
}

//=================================================================================================

void ODE_Manifest::SetVersion(const TCollection_AsciiString& theVersion)
{
  myVersion = theVersion;
}

//=================================================================================================

const TCollection_AsciiString& ODE_Manifest::Version() const
{
  return myVersion;
}

//=================================================================================================

void ODE_Manifest::SetGenerator(const TCollection_AsciiString& theGenerator)
{
  myGenerator = theGenerator;
}

//=================================================================================================

const TCollection_AsciiString& ODE_Manifest::Generator() const
{
  return myGenerator;
}

//=================================================================================================

void ODE_Manifest::SetCreated(const TCollection_AsciiString& theTimestamp)
{
  myCreated = theTimestamp;
}

//=================================================================================================

const TCollection_AsciiString& ODE_Manifest::Created() const
{
  return myCreated;
}

//=================================================================================================

void ODE_Manifest::AddFile(const FileEntry& theEntry)
{
  myFiles.Append(theEntry);
}

//=================================================================================================

int ODE_Manifest::FileCount() const
{
  return myFiles.Size();
}

//=================================================================================================

const ODE_Manifest::FileEntry& ODE_Manifest::File(int theIndex) const
{
  return myFiles.Value(theIndex);
}

//=================================================================================================

void ODE_Manifest::ClearFiles()
{
  myFiles.Clear();
}

//=================================================================================================

static std::string EscapeJsonString(const TCollection_AsciiString& theStr)
{
  std::string result;
  result.reserve(theStr.Length());

  for (int i = 1; i <= theStr.Length(); ++i)
  {
    const char c = theStr.Value(i);
    switch (c)
    {
      case '"':  result += "\\\""; break;
      case '\\': result += "\\\\"; break;
      case '\b': result += "\\b";  break;
      case '\f': result += "\\f";  break;
      case '\n': result += "\\n";  break;
      case '\r': result += "\\r";  break;
      case '\t': result += "\\t";  break;
      default:   result += c;      break;
    }
  }
  return result;
}

//=================================================================================================

bool ODE_Manifest::WriteToFile(const TCollection_AsciiString& thePath) const
{
  std::ofstream file(thePath.ToCString());
  if (!file.is_open())
  {
    return false;
  }

  // Write JSON manually with proper formatting
  file << "{\n";
  file << "  \"format\": \"ODE\",\n";
  file << "  \"version\": \"" << EscapeJsonString(myVersion) << "\",\n";

  if (!myGenerator.IsEmpty())
  {
    file << "  \"generator\": \"" << EscapeJsonString(myGenerator) << "\",\n";
  }

  file << "  \"created\": \"" << EscapeJsonString(myCreated) << "\",\n";
  file << "  \"files\": [\n";

  // Write file entries
  for (int i = 1; i <= myFiles.Size(); ++i)
  {
    const FileEntry& entry = myFiles.Value(i);
    file << "    {\n";
    file << "      \"uuid\": \"" << EscapeJsonString(entry.UUID) << "\",\n";
    file << "      \"name\": \"" << EscapeJsonString(entry.Name) << "\",\n";
    file << "      \"type\": \"" << EscapeJsonString(entry.Type) << "\",\n";
    file << "      \"encoding\": \"" << EscapeJsonString(entry.Encoding) << "\",\n";
    file << "      \"objectCount\": " << entry.ObjectCount;

    if (!entry.SHA256.IsEmpty())
    {
      file << ",\n      \"sha256\": \"" << EscapeJsonString(entry.SHA256) << "\"";
    }

    file << "\n    }";
    if (i < myFiles.Size())
    {
      file << ",";
    }
    file << "\n";
  }

  file << "  ]\n";
  file << "}\n";

  file.close();
  return true;
}

//=================================================================================================

static std::string TrimWhitespace(const std::string& theStr)
{
  const size_t first = theStr.find_first_not_of(" \t\n\r");
  if (first == std::string::npos)
    return "";

  const size_t last = theStr.find_last_not_of(" \t\n\r");
  return theStr.substr(first, last - first + 1);
}

//=================================================================================================

static bool ExtractJsonString(const std::string& theLine,
                              const std::string& theKey,
                              TCollection_AsciiString& theValue)
{
  // Find "key": "value" pattern
  const size_t keyPos = theLine.find("\"" + theKey + "\"");
  if (keyPos == std::string::npos)
    return false;

  const size_t colonPos = theLine.find(':', keyPos);
  if (colonPos == std::string::npos)
    return false;

  const size_t valueStart = theLine.find('"', colonPos);
  if (valueStart == std::string::npos)
    return false;

  const size_t valueEnd = theLine.find('"', valueStart + 1);
  if (valueEnd == std::string::npos)
    return false;

  const std::string value = theLine.substr(valueStart + 1, valueEnd - valueStart - 1);
  theValue = TCollection_AsciiString(value.c_str());
  return true;
}

//=================================================================================================

static bool ExtractJsonInt(const std::string& theLine,
                           const std::string& theKey,
                           int& theValue)
{
  const size_t keyPos = theLine.find("\"" + theKey + "\"");
  if (keyPos == std::string::npos)
    return false;

  const size_t colonPos = theLine.find(':', keyPos);
  if (colonPos == std::string::npos)
    return false;

  const std::string numPart = TrimWhitespace(theLine.substr(colonPos + 1));
  const size_t commaPos = numPart.find(',');
  const std::string numStr = commaPos != std::string::npos ?
    numPart.substr(0, commaPos) : numPart;

  try
  {
    theValue = std::stoi(numStr);
    return true;
  }
  catch (...)
  {
    return false;
  }
}

//=================================================================================================

bool ODE_Manifest::ReadFromFile(const TCollection_AsciiString& thePath)
{
  std::ifstream file(thePath.ToCString());
  if (!file.is_open())
  {
    return false;
  }

  myFiles.Clear();

  std::string line;
  bool inFilesArray = false;
  bool inFileObject = false;
  FileEntry currentEntry;

  while (std::getline(file, line))
  {
    line = TrimWhitespace(line);

    // Parse top-level fields
    TCollection_AsciiString strValue;
    if (ExtractJsonString(line, "version", strValue))
    {
      myVersion = strValue;
    }
    else if (ExtractJsonString(line, "generator", strValue))
    {
      myGenerator = strValue;
    }
    else if (ExtractJsonString(line, "created", strValue))
    {
      myCreated = strValue;
    }
    else if (line.find("\"files\"") != std::string::npos)
    {
      inFilesArray = true;
    }
    else if (inFilesArray && line.find('{') != std::string::npos)
    {
      inFileObject = true;
      currentEntry = FileEntry();
    }
    else if (inFileObject)
    {
      // Parse file entry fields
      if (ExtractJsonString(line, "uuid", strValue))
      {
        currentEntry.UUID = strValue;
      }
      else if (ExtractJsonString(line, "name", strValue))
      {
        currentEntry.Name = strValue;
      }
      else if (ExtractJsonString(line, "type", strValue))
      {
        currentEntry.Type = strValue;
      }
      else if (ExtractJsonString(line, "encoding", strValue))
      {
        currentEntry.Encoding = strValue;
      }
      else if (ExtractJsonInt(line, "objectCount", currentEntry.ObjectCount))
      {
        // ObjectCount extracted
      }
      else if (ExtractJsonString(line, "sha256", strValue))
      {
        currentEntry.SHA256 = strValue;
      }
      else if (line.find('}') != std::string::npos)
      {
        // End of file object
        myFiles.Append(currentEntry);
        inFileObject = false;
      }
    }
  }

  file.close();
  return true;
}

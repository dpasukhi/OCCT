// Created on: 2025-01-20
// Created by: ODE Format Implementation
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

#ifndef _ODE_Status_HeaderFile
#define _ODE_Status_HeaderFile

//! Enumeration of ODE operation status codes.
//! Used for error handling throughout the ODE format implementation.
enum ODE_Status
{
  ODE_Status_OK,                     //!< Operation completed successfully
  ODE_Status_InvalidArchive,         //!< Archive structure is invalid or corrupted
  ODE_Status_FileNotFound,           //!< Required file not found in archive
  ODE_Status_InvalidFormat,          //!< File format is invalid or unrecognized
  ODE_Status_InvalidManifest,        //!< Manifest file is invalid or malformed
  ODE_Status_DuplicateIndex,         //!< Duplicate index detected during serialization
  ODE_Status_InvalidReference,       //!< Invalid cross-file reference encountered
  ODE_Status_MissingGeometry,        //!< Referenced geometry object not found
  ODE_Status_InvalidTopology,        //!< Topology structure is invalid or inconsistent
  ODE_Status_SerializationFailed,    //!< Cap'n Proto serialization failed
  ODE_Status_DeserializationFailed,  //!< Cap'n Proto deserialization failed
  ODE_Status_WriteError,             //!< File write operation failed
  ODE_Status_ReadError,              //!< File read operation failed
  ODE_Status_ParseError,             //!< File parsing failed
  ODE_Status_UnsupportedVersion,     //!< Unsupported format version
  ODE_Status_NotImplemented,         //!< Feature not yet implemented
  ODE_Status_Unknown                 //!< Unknown or unspecified error
};

#endif // _ODE_Status_HeaderFile

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

#ifndef _Standard_FailureRegistry_HeaderFile
#define _Standard_FailureRegistry_HeaderFile

// =======================================================================
// From: src/FoundationClasses/TKernel/Standard/Standard_ProgramError.hxx
// =======================================================================

#include <Standard_Type.hxx>
#include <Standard_DefineException.hxx>
#include <Standard_SStream.hxx>
#include <Standard_Failure.hxx>

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(Standard_ProgramError, Standard_Failure)

// =======================================================================
// From: src/ModelingData/TKBRep/TopoDS/TopoDS_FrozenShape.hxx
// =======================================================================

#include <Standard_DomainError.hxx>

//! An attempt was made to modify a Shape already
//! shared or protected.

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(TopoDS_FrozenShape, Standard_DomainError)

// =======================================================================
// From: src/Visualization/TKV3d/V3d/V3d_UnMapped.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(V3d_UnMapped, Standard_DomainError)

// =======================================================================
// From: src/Visualization/TKV3d/V3d/V3d_BadValue.hxx
// =======================================================================

#include <Standard_OutOfRange.hxx>

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(V3d_BadValue, Standard_OutOfRange)

// =======================================================================
// From: src/Visualization/TKV3d/Prs3d/Prs3d_InvalidAngle.hxx
// =======================================================================

#include <Standard_RangeError.hxx>

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(Prs3d_InvalidAngle, Standard_RangeError)

// =======================================================================
// From: src/Visualization/TKService/WNT/WNT_ClassDefinitionError.hxx
// =======================================================================

#include <Standard_ConstructionError.hxx>

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(WNT_ClassDefinitionError, Standard_ConstructionError)

// =======================================================================
// From: src/Visualization/TKService/Graphic3d/Graphic3d_StructureDefinitionError.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(Graphic3d_StructureDefinitionError, Standard_OutOfRange)

// =======================================================================
// From: src/Visualization/TKService/Graphic3d/Graphic3d_PriorityDefinitionError.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(Graphic3d_PriorityDefinitionError, Standard_OutOfRange)

// =======================================================================
// From: src/Visualization/TKService/Graphic3d/Graphic3d_MaterialDefinitionError.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(Graphic3d_MaterialDefinitionError, Standard_OutOfRange)

// =======================================================================
// From: src/Visualization/TKService/Graphic3d/Graphic3d_GroupDefinitionError.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(Graphic3d_GroupDefinitionError, Standard_OutOfRange)

// =======================================================================
// From: src/Visualization/TKService/Aspect/Aspect_WindowError.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(Aspect_WindowError, Standard_OutOfRange)

// =======================================================================
// From: src/Visualization/TKService/Aspect/Aspect_WindowDefinitionError.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(Aspect_WindowDefinitionError, Standard_OutOfRange)

// =======================================================================
// From: src/Visualization/TKService/Aspect/Aspect_IdentDefinitionError.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(Aspect_IdentDefinitionError, Standard_OutOfRange)

// =======================================================================
// From: src/Visualization/TKService/Aspect/Aspect_GraphicDeviceDefinitionError.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(Aspect_GraphicDeviceDefinitionError, Standard_OutOfRange)

// =======================================================================
// From: src/Visualization/TKService/Aspect/Aspect_DisplayConnectionDefinitionError.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(Aspect_DisplayConnectionDefinitionError, Standard_OutOfRange)

// =======================================================================
// From: src/Visualization/TKService/Aspect/Aspect_AspectMarkerDefinitionError.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(Aspect_AspectMarkerDefinitionError, Standard_OutOfRange)

// =======================================================================
// From: src/Visualization/TKService/Aspect/Aspect_AspectLineDefinitionError.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(Aspect_AspectLineDefinitionError, Standard_OutOfRange)

// =======================================================================
// From: src/Visualization/TKService/Aspect/Aspect_AspectFillAreaDefinitionError.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(Aspect_AspectFillAreaDefinitionError, Standard_OutOfRange)

// =======================================================================
// From: src/ModelingData/TKG3d/Geom/Geom_UndefinedValue.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(Geom_UndefinedValue, Standard_DomainError)

// =======================================================================
// From: src/ModelingData/TKG3d/Geom/Geom_UndefinedDerivative.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(Geom_UndefinedDerivative, Standard_DomainError)

// =======================================================================
// From: src/ModelingData/TKG3d/GProp/GProp_UndefinedAxis.hxx
// =======================================================================

//! This exception is raised when a method makes reference to
//! an undefined inertia axis of symmetry.

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(GProp_UndefinedAxis, Standard_DomainError)

// =======================================================================
// From: src/ModelingData/TKG2d/LProp/LProp_NotDefined.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(LProp_NotDefined, Standard_Failure)

// =======================================================================
// From: src/ModelingData/TKG2d/LProp/LProp_BadContinuity.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(LProp_BadContinuity, Standard_Failure)

// =======================================================================
// From: src/ModelingData/TKG2d/Geom2d/Geom2d_UndefinedValue.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(Geom2d_UndefinedValue, Standard_DomainError)

// =======================================================================
// From: src/ModelingData/TKG2d/Geom2d/Geom2d_UndefinedDerivative.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(Geom2d_UndefinedDerivative, Standard_DomainError)

// =======================================================================
// From: src/ModelingData/TKBRep/TopoDS/TopoDS_UnCompatibleShapes.hxx
// =======================================================================

//! An incorrect insertion was attempted.

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(TopoDS_UnCompatibleShapes, Standard_DomainError)

// =======================================================================
// From: src/ModelingData/TKBRep/TopoDS/TopoDS_LockedShape.hxx
// =======================================================================

//! An attempt was made to modify a geometry of Shape already
//! shared or protected.

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(TopoDS_LockedShape, Standard_DomainError)

// =======================================================================
// From: src/ModelingAlgorithms/TKTopAlgo/BRepExtrema/BRepExtrema_UnCompatibleShape.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(BRepExtrema_UnCompatibleShape, Standard_DomainError)

// =======================================================================
// From: src/ModelingAlgorithms/TKGeomAlgo/Geom2dGcc/Geom2dGcc_IsParallel.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(Geom2dGcc_IsParallel, Standard_DomainError)

// =======================================================================
// From: src/ModelingAlgorithms/TKGeomAlgo/GccEnt/GccEnt_BadQualifier.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(GccEnt_BadQualifier, Standard_DomainError)

// =======================================================================
// From: src/ModelingAlgorithms/TKGeomAlgo/GccAna/GccAna_NoSolution.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(GccAna_NoSolution, Standard_Failure)

// =======================================================================
// From: src/ModelingAlgorithms/TKExpress/ExprIntrp/ExprIntrp_SyntaxError.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(ExprIntrp_SyntaxError, Standard_Failure)

// =======================================================================
// From: src/ModelingAlgorithms/TKExpress/Expr/Expr_NotEvaluable.hxx
// =======================================================================

#include <Expr_ExprFailure.hxx>

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(Expr_NotEvaluable, Expr_ExprFailure)

// =======================================================================
// From: src/ModelingAlgorithms/TKExpress/Expr/Expr_NotAssigned.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(Expr_NotAssigned, Expr_ExprFailure)

// =======================================================================
// From: src/ModelingAlgorithms/TKExpress/Expr/Expr_InvalidOperand.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(Expr_InvalidOperand, Expr_ExprFailure)

// =======================================================================
// From: src/ModelingAlgorithms/TKExpress/Expr/Expr_InvalidFunction.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(Expr_InvalidFunction, Expr_ExprFailure)

// =======================================================================
// From: src/ModelingAlgorithms/TKExpress/Expr/Expr_InvalidAssignment.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(Expr_InvalidAssignment, Expr_ExprFailure)

// =======================================================================
// From: src/ModelingAlgorithms/TKExpress/Expr/Expr_ExprFailure.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(Expr_ExprFailure, Standard_Failure)

// =======================================================================
// From: src/FoundationClasses/TKernel/Units/Units_NoSuchUnit.hxx
// =======================================================================

#include <Standard_NoSuchObject.hxx>

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(Units_NoSuchUnit, Standard_NoSuchObject)

// =======================================================================
// From: src/FoundationClasses/TKernel/Units/Units_NoSuchType.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(Units_NoSuchType, Standard_NoSuchObject)

// =======================================================================
// From: src/FoundationClasses/TKernel/Storage/Storage_StreamWriteError.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(Storage_StreamWriteError, Standard_Failure)

// =======================================================================
// From: src/FoundationClasses/TKernel/Storage/Storage_StreamUnknownTypeError.hxx
// =======================================================================

#include <Storage_StreamReadError.hxx>

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(Storage_StreamUnknownTypeError, Storage_StreamReadError)

// =======================================================================
// From: src/FoundationClasses/TKernel/Storage/Storage_StreamTypeMismatchError.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(Storage_StreamTypeMismatchError, Storage_StreamReadError)

// =======================================================================
// From: src/FoundationClasses/TKernel/Storage/Storage_StreamReadError.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(Storage_StreamReadError, Standard_Failure)

// =======================================================================
// From: src/FoundationClasses/TKernel/Storage/Storage_StreamModeError.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(Storage_StreamModeError, Standard_Failure)

// =======================================================================
// From: src/FoundationClasses/TKernel/Storage/Storage_StreamFormatError.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(Storage_StreamFormatError, Standard_Failure)

// =======================================================================
// From: src/FoundationClasses/TKernel/Storage/Storage_StreamExtCharParityError.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(Storage_StreamExtCharParityError, Storage_StreamReadError)

// =======================================================================
// From: src/FoundationClasses/TKernel/StdFail/StdFail_UndefinedValue.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(StdFail_UndefinedValue, Standard_DomainError)

// =======================================================================
// From: src/FoundationClasses/TKernel/StdFail/StdFail_UndefinedDerivative.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(StdFail_UndefinedDerivative, Standard_DomainError)

// =======================================================================
// From: src/FoundationClasses/TKernel/StdFail/StdFail_Undefined.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(StdFail_Undefined, Standard_Failure)

// =======================================================================
// From: src/FoundationClasses/TKernel/StdFail/StdFail_NotDone.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(StdFail_NotDone, Standard_Failure)

// =======================================================================
// From: src/FoundationClasses/TKernel/StdFail/StdFail_InfiniteSolutions.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(StdFail_InfiniteSolutions, Standard_Failure)

// =======================================================================
// From: src/FoundationClasses/TKernel/Standard/Standard_Underflow.hxx
// =======================================================================

#include <Standard_NumericError.hxx>

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(Standard_Underflow, Standard_NumericError)

// =======================================================================
// From: src/FoundationClasses/TKernel/Standard/Standard_TypeMismatch.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(Standard_TypeMismatch, Standard_DomainError)

// =======================================================================
// From: src/FoundationClasses/TKernel/Standard/Standard_RangeError.hxx
// =======================================================================

class Standard_RangeError;
DEFINE_STANDARD_HANDLE(Standard_RangeError, Standard_DomainError)

#if !defined No_Exception && !defined No_Standard_RangeError
  #if (defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)))
    // suppress false-positive warnings produced by GCC optimizer
    #define Standard_RangeError_Raise_if(CONDITION, MESSAGE)                                       \
      _Pragma("GCC diagnostic push") _Pragma("GCC diagnostic ignored \"-Wstrict-overflow\"") if (  \
        CONDITION) throw Standard_RangeError(MESSAGE);                                             \
      _Pragma("GCC diagnostic pop")
  #else
    #define Standard_RangeError_Raise_if(CONDITION, MESSAGE)                                       \
      if (CONDITION)                                                                               \
        throw Standard_RangeError(MESSAGE);
  #endif
#else
  #define Standard_RangeError_Raise_if(CONDITION, MESSAGE)
#endif

DEFINE_STANDARD_EXCEPTION(Standard_RangeError, Standard_DomainError)

// =======================================================================
// From: src/FoundationClasses/TKernel/Standard/Standard_Overflow.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(Standard_Overflow, Standard_NumericError)

// =======================================================================
// From: src/FoundationClasses/TKernel/Standard/Standard_OutOfRange.hxx
// =======================================================================

class Standard_OutOfRange;
DEFINE_STANDARD_HANDLE(Standard_OutOfRange, Standard_RangeError)

#if (defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)))
  // suppress false-positive warnings produced by GCC optimizer
  #define Standard_OutOfRange_Always_Raise_if(CONDITION, MESSAGE)                                  \
    _Pragma("GCC diagnostic push") _Pragma("GCC diagnostic ignored \"-Wstrict-overflow\"") if (    \
      CONDITION) throw Standard_OutOfRange(MESSAGE);                                               \
    _Pragma("GCC diagnostic pop")
#else
  #define Standard_OutOfRange_Always_Raise_if(CONDITION, MESSAGE)                                  \
    if (CONDITION)                                                                                 \
      throw Standard_OutOfRange(MESSAGE);
#endif

#if !defined No_Exception && !defined No_Standard_OutOfRange
  #define Standard_OutOfRange_Raise_if(CONDITION, MESSAGE)                                         \
    Standard_OutOfRange_Always_Raise_if(CONDITION, MESSAGE)
#else
  #define Standard_OutOfRange_Raise_if(CONDITION, MESSAGE)
#endif

DEFINE_STANDARD_EXCEPTION(Standard_OutOfRange, Standard_RangeError)

// =======================================================================
// From: src/FoundationClasses/TKernel/Standard/Standard_NumericError.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(Standard_NumericError, Standard_Failure)

// =======================================================================
// From: src/FoundationClasses/TKernel/Standard/Standard_NullValue.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(Standard_NullValue, Standard_RangeError)

// =======================================================================
// From: src/FoundationClasses/TKernel/Standard/Standard_NullObject.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(Standard_NullObject, Standard_DomainError)

// =======================================================================
// From: src/FoundationClasses/TKernel/Standard/Standard_NotImplemented.hxx
// =======================================================================

#include <Standard_ProgramError.hxx>

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(Standard_NotImplemented, Standard_ProgramError)

// =======================================================================
// From: src/FoundationClasses/TKernel/Standard/Standard_NoSuchObject.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(Standard_NoSuchObject, Standard_DomainError)

// =======================================================================
// From: src/FoundationClasses/TKernel/Standard/Standard_NoMoreObject.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(Standard_NoMoreObject, Standard_DomainError)

// =======================================================================
// From: src/FoundationClasses/TKernel/Standard/Standard_NegativeValue.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(Standard_NegativeValue, Standard_RangeError)

// =======================================================================
// From: src/FoundationClasses/TKernel/Standard/Standard_MultiplyDefined.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(Standard_MultiplyDefined, Standard_DomainError)

// =======================================================================
// From: src/FoundationClasses/TKernel/Standard/Standard_LicenseNotFound.hxx
// =======================================================================

#include <Standard_LicenseError.hxx>

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(Standard_LicenseNotFound, Standard_LicenseError)

// =======================================================================
// From: src/FoundationClasses/TKernel/Standard/Standard_LicenseError.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(Standard_LicenseError, Standard_Failure)

// =======================================================================
// From: src/FoundationClasses/TKernel/Standard/Standard_ImmutableObject.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(Standard_ImmutableObject, Standard_DomainError)

// =======================================================================
// From: src/FoundationClasses/TKernel/Standard/Standard_DomainError.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(Standard_DomainError, Standard_Failure)

// =======================================================================
// From: src/FoundationClasses/TKernel/Standard/Standard_DivideByZero.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(Standard_DivideByZero, Standard_NumericError)

// =======================================================================
// From: src/FoundationClasses/TKernel/Standard/Standard_DimensionMismatch.hxx
// =======================================================================

#include <Standard_DimensionError.hxx>

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(Standard_DimensionMismatch, Standard_DimensionError)

// =======================================================================
// From: src/FoundationClasses/TKernel/Standard/Standard_DimensionError.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(Standard_DimensionError, Standard_DomainError)

// =======================================================================
// From: src/FoundationClasses/TKernel/Standard/Standard_ConstructionError.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(Standard_ConstructionError, Standard_DomainError)

// =======================================================================
// From: src/FoundationClasses/TKernel/Standard/Standard_AbortiveTransaction.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(Standard_AbortiveTransaction, Standard_Failure)

// =======================================================================
// From: src/FoundationClasses/TKernel/Resource/Resource_NoSuchResource.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(Resource_NoSuchResource, Standard_NoSuchObject)

// =======================================================================
// From: src/FoundationClasses/TKernel/Quantity/Quantity_PeriodDefinitionError.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(Quantity_PeriodDefinitionError, Standard_DomainError)

// =======================================================================
// From: src/FoundationClasses/TKernel/Quantity/Quantity_DateDefinitionError.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(Quantity_DateDefinitionError, Standard_DomainError)

// =======================================================================
// From: src/FoundationClasses/TKernel/Plugin/Plugin_Failure.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(Plugin_Failure, Standard_Failure)

// =======================================================================
// From: src/FoundationClasses/TKernel/OSD/OSD_Signal.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(OSD_Signal, Standard_Failure)

// =======================================================================
// From: src/FoundationClasses/TKernel/OSD/OSD_SIGSYS.hxx
// =======================================================================

#include <OSD_Signal.hxx>

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(OSD_SIGSYS, OSD_Signal)

// =======================================================================
// From: src/FoundationClasses/TKernel/OSD/OSD_SIGSEGV.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(OSD_SIGSEGV, OSD_Signal)

// =======================================================================
// From: src/FoundationClasses/TKernel/OSD/OSD_SIGQUIT.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(OSD_SIGQUIT, OSD_Signal)

// =======================================================================
// From: src/FoundationClasses/TKernel/OSD/OSD_SIGKILL.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(OSD_SIGKILL, OSD_Signal)

// =======================================================================
// From: src/FoundationClasses/TKernel/OSD/OSD_SIGINT.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(OSD_SIGINT, OSD_Signal)

// =======================================================================
// From: src/FoundationClasses/TKernel/OSD/OSD_SIGILL.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(OSD_SIGILL, OSD_Signal)

// =======================================================================
// From: src/FoundationClasses/TKernel/OSD/OSD_SIGHUP.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(OSD_SIGHUP, OSD_Signal)

// =======================================================================
// From: src/FoundationClasses/TKernel/OSD/OSD_SIGBUS.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(OSD_SIGBUS, OSD_Signal)

// =======================================================================
// From: src/FoundationClasses/TKernel/OSD/OSD_OSDError.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(OSD_OSDError, Standard_Failure)

// =======================================================================
// From: src/FoundationClasses/TKernel/OSD/OSD_Exception_STATUS_NO_MEMORY.hxx
// =======================================================================

#include <OSD_Exception.hxx>

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(OSD_Exception_STATUS_NO_MEMORY, OSD_Exception)

// =======================================================================
// From: src/FoundationClasses/TKernel/OSD/OSD_Exception_STACK_OVERFLOW.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(OSD_Exception_STACK_OVERFLOW, OSD_Exception)

// =======================================================================
// From: src/FoundationClasses/TKernel/OSD/OSD_Exception_PRIV_INSTRUCTION.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(OSD_Exception_PRIV_INSTRUCTION, OSD_Exception)

// =======================================================================
// From: src/FoundationClasses/TKernel/OSD/OSD_Exception_NONCONTINUABLE_EXCEPTION.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(OSD_Exception_NONCONTINUABLE_EXCEPTION, OSD_Exception)

// =======================================================================
// From: src/FoundationClasses/TKernel/OSD/OSD_Exception_IN_PAGE_ERROR.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(OSD_Exception_IN_PAGE_ERROR, OSD_Exception)

// =======================================================================
// From: src/FoundationClasses/TKernel/OSD/OSD_Exception_INVALID_DISPOSITION.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(OSD_Exception_INVALID_DISPOSITION, OSD_Exception)

// =======================================================================
// From: src/FoundationClasses/TKernel/OSD/OSD_Exception_INT_OVERFLOW.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(OSD_Exception_INT_OVERFLOW, OSD_Exception)

// =======================================================================
// From: src/FoundationClasses/TKernel/OSD/OSD_Exception_ILLEGAL_INSTRUCTION.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(OSD_Exception_ILLEGAL_INSTRUCTION, OSD_Exception)

// =======================================================================
// From: src/FoundationClasses/TKernel/OSD/OSD_Exception_CTRL_BREAK.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(OSD_Exception_CTRL_BREAK, OSD_Exception)

// =======================================================================
// From: src/FoundationClasses/TKernel/OSD/OSD_Exception_ARRAY_BOUNDS_EXCEEDED.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(OSD_Exception_ARRAY_BOUNDS_EXCEEDED, OSD_Exception)

// =======================================================================
// From: src/FoundationClasses/TKernel/OSD/OSD_Exception_ACCESS_VIOLATION.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(OSD_Exception_ACCESS_VIOLATION, OSD_Exception)

// =======================================================================
// From: src/FoundationClasses/TKernel/OSD/OSD_Exception.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(OSD_Exception, Standard_Failure)

// =======================================================================
// From: src/FoundationClasses/TKMath/math/math_SingularMatrix.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(math_SingularMatrix, Standard_Failure)

// =======================================================================
// From: src/FoundationClasses/TKMath/math/math_NotSquare.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(math_NotSquare, Standard_DimensionError)

// =======================================================================
// From: src/FoundationClasses/TKMath/gp/gp_VectorWithNullMagnitude.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(gp_VectorWithNullMagnitude, Standard_DomainError)

// =======================================================================
// From: src/Draw/TKDraw/Draw/Draw_Failure.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(Draw_Failure, Standard_Failure)

// =======================================================================
// From: src/DataExchange/TKXSBase/Transfer/Transfer_TransferFailure.hxx
// =======================================================================

#include <Interface_InterfaceError.hxx>

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(Transfer_TransferFailure, Interface_InterfaceError)

// =======================================================================
// From: src/DataExchange/TKXSBase/Transfer/Transfer_TransferDeadLoop.hxx
// =======================================================================

#include <Transfer_TransferFailure.hxx>

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(Transfer_TransferDeadLoop, Transfer_TransferFailure)

// =======================================================================
// From: src/DataExchange/TKXSBase/Interface/Interface_InterfaceMismatch.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(Interface_InterfaceMismatch, Interface_InterfaceError)

// =======================================================================
// From: src/DataExchange/TKXSBase/Interface/Interface_InterfaceError.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(Interface_InterfaceError, Standard_Failure)

// =======================================================================
// From: src/DataExchange/TKXSBase/Interface/Interface_CheckFailure.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(Interface_CheckFailure, Interface_InterfaceError)

// =======================================================================
// From: src/ApplicationFramework/TKCDF/PCDM/PCDM_DriverError.hxx
// =======================================================================

DEFINE_STANDARD_EXCEPTION_WITH_RAISE(PCDM_DriverError, Standard_Failure)

#endif // _Standard_FailureRegistry_HeaderFile

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

#include <Standard_Type.hxx>
#include <Standard_DefineException.hxx>
#include <Standard_SStream.hxx>
#include <Standard_Failure.hxx>

// Exception: Standard_ProgramError (base: Standard_Failure)
#define __OCCT_EXCEPTION_CLASS Standard_ProgramError
#define __OCCT_EXCEPTION_BASE Standard_Failure
#define __OCCT_EXCEPTION_DISABLE No_Standard_ProgramError
#include <Standard_DefineException.lxx>

// Core Standard exceptions

// Exception: Standard_DomainError (base: Standard_Failure)
#define __OCCT_EXCEPTION_CLASS Standard_DomainError
#define __OCCT_EXCEPTION_BASE Standard_Failure
#define __OCCT_EXCEPTION_DISABLE No_Standard_DomainError
#include <Standard_DefineException.lxx>

// Exception: Standard_NumericError (base: Standard_Failure)
#define __OCCT_EXCEPTION_CLASS Standard_NumericError
#define __OCCT_EXCEPTION_BASE Standard_Failure
#define __OCCT_EXCEPTION_DISABLE No_Standard_NumericError
#include <Standard_DefineException.lxx>

// Exception: Standard_RangeError (base: Standard_DomainError)
#define __OCCT_EXCEPTION_CLASS Standard_RangeError
#define __OCCT_EXCEPTION_BASE Standard_DomainError
#define __OCCT_EXCEPTION_DISABLE No_Standard_RangeError
#include <Standard_DefineException.lxx>

// Exception: Standard_OutOfRange (base: Standard_RangeError)
#define __OCCT_EXCEPTION_CLASS Standard_OutOfRange
#define __OCCT_EXCEPTION_BASE Standard_RangeError
#define __OCCT_EXCEPTION_DISABLE No_Standard_OutOfRange
#include <Standard_DefineException.lxx>

// Exception: Standard_NullValue (base: Standard_RangeError)
#define __OCCT_EXCEPTION_CLASS Standard_NullValue
#define __OCCT_EXCEPTION_BASE Standard_RangeError
#define __OCCT_EXCEPTION_DISABLE No_Standard_NullValue
#include <Standard_DefineException.lxx>

// Exception: Standard_NullObject (base: Standard_DomainError)
#define __OCCT_EXCEPTION_CLASS Standard_NullObject
#define __OCCT_EXCEPTION_BASE Standard_DomainError
#define __OCCT_EXCEPTION_DISABLE No_Standard_NullObject
#include <Standard_DefineException.lxx>

// Exception: Standard_TypeMismatch (base: Standard_DomainError)
#define __OCCT_EXCEPTION_CLASS Standard_TypeMismatch
#define __OCCT_EXCEPTION_BASE Standard_DomainError
#define __OCCT_EXCEPTION_DISABLE No_Standard_TypeMismatch
#include <Standard_DefineException.lxx>

// Exception: Standard_ConstructionError (base: Standard_DomainError)
#define __OCCT_EXCEPTION_CLASS Standard_ConstructionError
#define __OCCT_EXCEPTION_BASE Standard_DomainError
#define __OCCT_EXCEPTION_DISABLE No_Standard_ConstructionError
#include <Standard_DefineException.lxx>

// Exception: Standard_DimensionError (base: Standard_DomainError)
#define __OCCT_EXCEPTION_CLASS Standard_DimensionError
#define __OCCT_EXCEPTION_BASE Standard_DomainError
#define __OCCT_EXCEPTION_DISABLE No_Standard_DimensionError
#include <Standard_DefineException.lxx>

// Exception: Standard_DimensionMismatch (base: Standard_DimensionError)
#define __OCCT_EXCEPTION_CLASS Standard_DimensionMismatch
#define __OCCT_EXCEPTION_BASE Standard_DimensionError
#define __OCCT_EXCEPTION_DISABLE No_Standard_DimensionMismatch
#include <Standard_DefineException.lxx>

// Exception: Standard_DivideByZero (base: Standard_NumericError)
#define __OCCT_EXCEPTION_CLASS Standard_DivideByZero
#define __OCCT_EXCEPTION_BASE Standard_NumericError
#define __OCCT_EXCEPTION_DISABLE No_Standard_DivideByZero
#include <Standard_DefineException.lxx>

// Exception: Standard_NegativeValue (base: Standard_RangeError)
#define __OCCT_EXCEPTION_CLASS Standard_NegativeValue
#define __OCCT_EXCEPTION_BASE Standard_RangeError
#define __OCCT_EXCEPTION_DISABLE No_Standard_NegativeValue
#include <Standard_DefineException.lxx>

// Exception: Standard_MultiplyDefined (base: Standard_DomainError)
#define __OCCT_EXCEPTION_CLASS Standard_MultiplyDefined
#define __OCCT_EXCEPTION_BASE Standard_DomainError
#define __OCCT_EXCEPTION_DISABLE No_Standard_MultiplyDefined
#include <Standard_DefineException.lxx>

// Exception: Standard_NoSuchObject (base: Standard_DomainError)
#define __OCCT_EXCEPTION_CLASS Standard_NoSuchObject
#define __OCCT_EXCEPTION_BASE Standard_DomainError
#define __OCCT_EXCEPTION_DISABLE No_Standard_NoSuchObject
#include <Standard_DefineException.lxx>

// Exception: Standard_NoMoreObject (base: Standard_DomainError)
#define __OCCT_EXCEPTION_CLASS Standard_NoMoreObject
#define __OCCT_EXCEPTION_BASE Standard_DomainError
#define __OCCT_EXCEPTION_DISABLE No_Standard_NoMoreObject
#include <Standard_DefineException.lxx>

// Exception: Standard_NotImplemented (base: Standard_ProgramError)
#define __OCCT_EXCEPTION_CLASS Standard_NotImplemented
#define __OCCT_EXCEPTION_BASE Standard_ProgramError
#define __OCCT_EXCEPTION_DISABLE No_Standard_NotImplemented
#include <Standard_DefineException.lxx>

//! An attempt was made to modify a Shape already
//! shared or protected.

// Exception: TopoDS_FrozenShape (base: Standard_DomainError)
#define __OCCT_EXCEPTION_CLASS TopoDS_FrozenShape
#define __OCCT_EXCEPTION_BASE Standard_DomainError
#define __OCCT_EXCEPTION_DISABLE No_TopoDS_FrozenShape
#include <Standard_DefineException.lxx>

// Exception: V3d_UnMapped (base: Standard_DomainError)
#define __OCCT_EXCEPTION_CLASS V3d_UnMapped
#define __OCCT_EXCEPTION_BASE Standard_DomainError
#define __OCCT_EXCEPTION_DISABLE No_V3d_UnMapped
#include <Standard_DefineException.lxx>

// Exception: V3d_BadValue (base: Standard_OutOfRange)
#define __OCCT_EXCEPTION_CLASS V3d_BadValue
#define __OCCT_EXCEPTION_BASE Standard_OutOfRange
#define __OCCT_EXCEPTION_DISABLE No_V3d_BadValue
#include <Standard_DefineException.lxx>

// Exception: Prs3d_InvalidAngle (base: Standard_RangeError)
#define __OCCT_EXCEPTION_CLASS Prs3d_InvalidAngle
#define __OCCT_EXCEPTION_BASE Standard_RangeError
#define __OCCT_EXCEPTION_DISABLE No_Prs3d_InvalidAngle
#include <Standard_DefineException.lxx>

// Exception: WNT_ClassDefinitionError (base: Standard_ConstructionError)
#define __OCCT_EXCEPTION_CLASS WNT_ClassDefinitionError
#define __OCCT_EXCEPTION_BASE Standard_ConstructionError
#define __OCCT_EXCEPTION_DISABLE No_WNT_ClassDefinitionError
#include <Standard_DefineException.lxx>

// Exception: Graphic3d_StructureDefinitionError (base: Standard_OutOfRange)
#define __OCCT_EXCEPTION_CLASS Graphic3d_StructureDefinitionError
#define __OCCT_EXCEPTION_BASE Standard_OutOfRange
#define __OCCT_EXCEPTION_DISABLE No_Graphic3d_StructureDefinitionError
#include <Standard_DefineException.lxx>

// Exception: Graphic3d_PriorityDefinitionError (base: Standard_OutOfRange)
#define __OCCT_EXCEPTION_CLASS Graphic3d_PriorityDefinitionError
#define __OCCT_EXCEPTION_BASE Standard_OutOfRange
#define __OCCT_EXCEPTION_DISABLE No_Graphic3d_PriorityDefinitionError
#include <Standard_DefineException.lxx>

// Exception: Graphic3d_MaterialDefinitionError (base: Standard_OutOfRange)
#define __OCCT_EXCEPTION_CLASS Graphic3d_MaterialDefinitionError
#define __OCCT_EXCEPTION_BASE Standard_OutOfRange
#define __OCCT_EXCEPTION_DISABLE No_Graphic3d_MaterialDefinitionError
#include <Standard_DefineException.lxx>

// Exception: Graphic3d_GroupDefinitionError (base: Standard_OutOfRange)
#define __OCCT_EXCEPTION_CLASS Graphic3d_GroupDefinitionError
#define __OCCT_EXCEPTION_BASE Standard_OutOfRange
#define __OCCT_EXCEPTION_DISABLE No_Graphic3d_GroupDefinitionError
#include <Standard_DefineException.lxx>

// Exception: Aspect_WindowError (base: Standard_OutOfRange)
#define __OCCT_EXCEPTION_CLASS Aspect_WindowError
#define __OCCT_EXCEPTION_BASE Standard_OutOfRange
#define __OCCT_EXCEPTION_DISABLE No_Aspect_WindowError
#include <Standard_DefineException.lxx>

// Exception: Aspect_WindowDefinitionError (base: Standard_OutOfRange)
#define __OCCT_EXCEPTION_CLASS Aspect_WindowDefinitionError
#define __OCCT_EXCEPTION_BASE Standard_OutOfRange
#define __OCCT_EXCEPTION_DISABLE No_Aspect_WindowDefinitionError
#include <Standard_DefineException.lxx>

// Exception: Aspect_IdentDefinitionError (base: Standard_OutOfRange)
#define __OCCT_EXCEPTION_CLASS Aspect_IdentDefinitionError
#define __OCCT_EXCEPTION_BASE Standard_OutOfRange
#define __OCCT_EXCEPTION_DISABLE No_Aspect_IdentDefinitionError
#include <Standard_DefineException.lxx>

// Exception: Aspect_GraphicDeviceDefinitionError (base: Standard_OutOfRange)
#define __OCCT_EXCEPTION_CLASS Aspect_GraphicDeviceDefinitionError
#define __OCCT_EXCEPTION_BASE Standard_OutOfRange
#define __OCCT_EXCEPTION_DISABLE No_Aspect_GraphicDeviceDefinitionError
#include <Standard_DefineException.lxx>

// Exception: Aspect_DisplayConnectionDefinitionError (base: Standard_OutOfRange)
#define __OCCT_EXCEPTION_CLASS Aspect_DisplayConnectionDefinitionError
#define __OCCT_EXCEPTION_BASE Standard_OutOfRange
#define __OCCT_EXCEPTION_DISABLE No_Aspect_DisplayConnectionDefinitionError
#include <Standard_DefineException.lxx>

// Exception: Aspect_AspectMarkerDefinitionError (base: Standard_OutOfRange)
#define __OCCT_EXCEPTION_CLASS Aspect_AspectMarkerDefinitionError
#define __OCCT_EXCEPTION_BASE Standard_OutOfRange
#define __OCCT_EXCEPTION_DISABLE No_Aspect_AspectMarkerDefinitionError
#include <Standard_DefineException.lxx>

// Exception: Aspect_AspectLineDefinitionError (base: Standard_OutOfRange)
#define __OCCT_EXCEPTION_CLASS Aspect_AspectLineDefinitionError
#define __OCCT_EXCEPTION_BASE Standard_OutOfRange
#define __OCCT_EXCEPTION_DISABLE No_Aspect_AspectLineDefinitionError
#include <Standard_DefineException.lxx>

// Exception: Aspect_AspectFillAreaDefinitionError (base: Standard_OutOfRange)
#define __OCCT_EXCEPTION_CLASS Aspect_AspectFillAreaDefinitionError
#define __OCCT_EXCEPTION_BASE Standard_OutOfRange
#define __OCCT_EXCEPTION_DISABLE No_Aspect_AspectFillAreaDefinitionError
#include <Standard_DefineException.lxx>

// Exception: Geom_UndefinedValue (base: Standard_DomainError)
#define __OCCT_EXCEPTION_CLASS Geom_UndefinedValue
#define __OCCT_EXCEPTION_BASE Standard_DomainError
#define __OCCT_EXCEPTION_DISABLE No_Geom_UndefinedValue
#include <Standard_DefineException.lxx>

// Exception: Geom_UndefinedDerivative (base: Standard_DomainError)
#define __OCCT_EXCEPTION_CLASS Geom_UndefinedDerivative
#define __OCCT_EXCEPTION_BASE Standard_DomainError
#define __OCCT_EXCEPTION_DISABLE No_Geom_UndefinedDerivative
#include <Standard_DefineException.lxx>

//! This exception is raised when a method makes reference to
//! an undefined inertia axis of symmetry.

// Exception: GProp_UndefinedAxis (base: Standard_DomainError)
#define __OCCT_EXCEPTION_CLASS GProp_UndefinedAxis
#define __OCCT_EXCEPTION_BASE Standard_DomainError
#define __OCCT_EXCEPTION_DISABLE No_GProp_UndefinedAxis
#include <Standard_DefineException.lxx>

// Exception: LProp_NotDefined (base: Standard_Failure)
#define __OCCT_EXCEPTION_CLASS LProp_NotDefined
#define __OCCT_EXCEPTION_BASE Standard_Failure
#define __OCCT_EXCEPTION_DISABLE No_LProp_NotDefined
#include <Standard_DefineException.lxx>

// Exception: LProp_BadContinuity (base: Standard_Failure)
#define __OCCT_EXCEPTION_CLASS LProp_BadContinuity
#define __OCCT_EXCEPTION_BASE Standard_Failure
#define __OCCT_EXCEPTION_DISABLE No_LProp_BadContinuity
#include <Standard_DefineException.lxx>

// Exception: Geom2d_UndefinedValue (base: Standard_DomainError)
#define __OCCT_EXCEPTION_CLASS Geom2d_UndefinedValue
#define __OCCT_EXCEPTION_BASE Standard_DomainError
#define __OCCT_EXCEPTION_DISABLE No_Geom2d_UndefinedValue
#include <Standard_DefineException.lxx>

// Exception: Geom2d_UndefinedDerivative (base: Standard_DomainError)
#define __OCCT_EXCEPTION_CLASS Geom2d_UndefinedDerivative
#define __OCCT_EXCEPTION_BASE Standard_DomainError
#define __OCCT_EXCEPTION_DISABLE No_Geom2d_UndefinedDerivative
#include <Standard_DefineException.lxx>

//! An incorrect insertion was attempted.

// Exception: TopoDS_UnCompatibleShapes (base: Standard_DomainError)
#define __OCCT_EXCEPTION_CLASS TopoDS_UnCompatibleShapes
#define __OCCT_EXCEPTION_BASE Standard_DomainError
#define __OCCT_EXCEPTION_DISABLE No_TopoDS_UnCompatibleShapes
#include <Standard_DefineException.lxx>

//! An attempt was made to modify a geometry of Shape already
//! shared or protected.

// Exception: TopoDS_LockedShape (base: Standard_DomainError)
#define __OCCT_EXCEPTION_CLASS TopoDS_LockedShape
#define __OCCT_EXCEPTION_BASE Standard_DomainError
#define __OCCT_EXCEPTION_DISABLE No_TopoDS_LockedShape
#include <Standard_DefineException.lxx>

// Exception: BRepExtrema_UnCompatibleShape (base: Standard_DomainError)
#define __OCCT_EXCEPTION_CLASS BRepExtrema_UnCompatibleShape
#define __OCCT_EXCEPTION_BASE Standard_DomainError
#define __OCCT_EXCEPTION_DISABLE No_BRepExtrema_UnCompatibleShape
#include <Standard_DefineException.lxx>

// Exception: Geom2dGcc_IsParallel (base: Standard_DomainError)
#define __OCCT_EXCEPTION_CLASS Geom2dGcc_IsParallel
#define __OCCT_EXCEPTION_BASE Standard_DomainError
#define __OCCT_EXCEPTION_DISABLE No_Geom2dGcc_IsParallel
#include <Standard_DefineException.lxx>

// Exception: GccEnt_BadQualifier (base: Standard_DomainError)
#define __OCCT_EXCEPTION_CLASS GccEnt_BadQualifier
#define __OCCT_EXCEPTION_BASE Standard_DomainError
#define __OCCT_EXCEPTION_DISABLE No_GccEnt_BadQualifier
#include <Standard_DefineException.lxx>

// Exception: GccAna_NoSolution (base: Standard_Failure)
#define __OCCT_EXCEPTION_CLASS GccAna_NoSolution
#define __OCCT_EXCEPTION_BASE Standard_Failure
#define __OCCT_EXCEPTION_DISABLE No_GccAna_NoSolution
#include <Standard_DefineException.lxx>

// Exception: ExprIntrp_SyntaxError (base: Standard_Failure)
#define __OCCT_EXCEPTION_CLASS ExprIntrp_SyntaxError
#define __OCCT_EXCEPTION_BASE Standard_Failure
#define __OCCT_EXCEPTION_DISABLE No_ExprIntrp_SyntaxError
#include <Standard_DefineException.lxx>

// Exception: Expr_ExprFailure (base: Standard_Failure)
#define __OCCT_EXCEPTION_CLASS Expr_ExprFailure
#define __OCCT_EXCEPTION_BASE Standard_Failure
#define __OCCT_EXCEPTION_DISABLE No_Expr_ExprFailure
#include <Standard_DefineException.lxx>

// Exception: Expr_NotEvaluable (base: Expr_ExprFailure)
#define __OCCT_EXCEPTION_CLASS Expr_NotEvaluable
#define __OCCT_EXCEPTION_BASE Expr_ExprFailure
#define __OCCT_EXCEPTION_DISABLE No_Expr_NotEvaluable
#include <Standard_DefineException.lxx>

// Exception: Expr_NotAssigned (base: Expr_ExprFailure)
#define __OCCT_EXCEPTION_CLASS Expr_NotAssigned
#define __OCCT_EXCEPTION_BASE Expr_ExprFailure
#define __OCCT_EXCEPTION_DISABLE No_Expr_NotAssigned
#include <Standard_DefineException.lxx>

// Exception: Expr_InvalidOperand (base: Expr_ExprFailure)
#define __OCCT_EXCEPTION_CLASS Expr_InvalidOperand
#define __OCCT_EXCEPTION_BASE Expr_ExprFailure
#define __OCCT_EXCEPTION_DISABLE No_Expr_InvalidOperand
#include <Standard_DefineException.lxx>

// Exception: Expr_InvalidFunction (base: Expr_ExprFailure)
#define __OCCT_EXCEPTION_CLASS Expr_InvalidFunction
#define __OCCT_EXCEPTION_BASE Expr_ExprFailure
#define __OCCT_EXCEPTION_DISABLE No_Expr_InvalidFunction
#include <Standard_DefineException.lxx>

// Exception: Expr_InvalidAssignment (base: Expr_ExprFailure)
#define __OCCT_EXCEPTION_CLASS Expr_InvalidAssignment
#define __OCCT_EXCEPTION_BASE Expr_ExprFailure
#define __OCCT_EXCEPTION_DISABLE No_Expr_InvalidAssignment
#include <Standard_DefineException.lxx>

// Exception: Units_NoSuchUnit (base: Standard_NoSuchObject)
#define __OCCT_EXCEPTION_CLASS Units_NoSuchUnit
#define __OCCT_EXCEPTION_BASE Standard_NoSuchObject
#define __OCCT_EXCEPTION_DISABLE No_Units_NoSuchUnit
#include <Standard_DefineException.lxx>

// Exception: Units_NoSuchType (base: Standard_NoSuchObject)
#define __OCCT_EXCEPTION_CLASS Units_NoSuchType
#define __OCCT_EXCEPTION_BASE Standard_NoSuchObject
#define __OCCT_EXCEPTION_DISABLE No_Units_NoSuchType
#include <Standard_DefineException.lxx>

// Exception: Storage_StreamWriteError (base: Standard_Failure)
#define __OCCT_EXCEPTION_CLASS Storage_StreamWriteError
#define __OCCT_EXCEPTION_BASE Standard_Failure
#define __OCCT_EXCEPTION_DISABLE No_Storage_StreamWriteError
#include <Standard_DefineException.lxx>

// Exception: Storage_StreamReadError (base: Standard_Failure)
#define __OCCT_EXCEPTION_CLASS Storage_StreamReadError
#define __OCCT_EXCEPTION_BASE Standard_Failure
#define __OCCT_EXCEPTION_DISABLE No_Storage_StreamReadError
#include <Standard_DefineException.lxx>

// Exception: Storage_StreamUnknownTypeError (base: Storage_StreamReadError)
#define __OCCT_EXCEPTION_CLASS Storage_StreamUnknownTypeError
#define __OCCT_EXCEPTION_BASE Storage_StreamReadError
#define __OCCT_EXCEPTION_DISABLE No_Storage_StreamUnknownTypeError
#include <Standard_DefineException.lxx>

// Exception: Storage_StreamTypeMismatchError (base: Storage_StreamReadError)
#define __OCCT_EXCEPTION_CLASS Storage_StreamTypeMismatchError
#define __OCCT_EXCEPTION_BASE Storage_StreamReadError
#define __OCCT_EXCEPTION_DISABLE No_Storage_StreamTypeMismatchError
#include <Standard_DefineException.lxx>

// Exception: Storage_StreamModeError (base: Standard_Failure)
#define __OCCT_EXCEPTION_CLASS Storage_StreamModeError
#define __OCCT_EXCEPTION_BASE Standard_Failure
#define __OCCT_EXCEPTION_DISABLE No_Storage_StreamModeError
#include <Standard_DefineException.lxx>

// Exception: Storage_StreamFormatError (base: Standard_Failure)
#define __OCCT_EXCEPTION_CLASS Storage_StreamFormatError
#define __OCCT_EXCEPTION_BASE Standard_Failure
#define __OCCT_EXCEPTION_DISABLE No_Storage_StreamFormatError
#include <Standard_DefineException.lxx>

// Exception: Storage_StreamExtCharParityError (base: Storage_StreamReadError)
#define __OCCT_EXCEPTION_CLASS Storage_StreamExtCharParityError
#define __OCCT_EXCEPTION_BASE Storage_StreamReadError
#define __OCCT_EXCEPTION_DISABLE No_Storage_StreamExtCharParityError
#include <Standard_DefineException.lxx>

// Exception: StdFail_UndefinedValue (base: Standard_DomainError)
#define __OCCT_EXCEPTION_CLASS StdFail_UndefinedValue
#define __OCCT_EXCEPTION_BASE Standard_DomainError
#define __OCCT_EXCEPTION_DISABLE No_StdFail_UndefinedValue
#include <Standard_DefineException.lxx>

// Exception: StdFail_UndefinedDerivative (base: Standard_DomainError)
#define __OCCT_EXCEPTION_CLASS StdFail_UndefinedDerivative
#define __OCCT_EXCEPTION_BASE Standard_DomainError
#define __OCCT_EXCEPTION_DISABLE No_StdFail_UndefinedDerivative
#include <Standard_DefineException.lxx>

// Exception: StdFail_Undefined (base: Standard_Failure)
#define __OCCT_EXCEPTION_CLASS StdFail_Undefined
#define __OCCT_EXCEPTION_BASE Standard_Failure
#define __OCCT_EXCEPTION_DISABLE No_StdFail_Undefined
#include <Standard_DefineException.lxx>

// Exception: StdFail_NotDone (base: Standard_Failure)
#define __OCCT_EXCEPTION_CLASS StdFail_NotDone
#define __OCCT_EXCEPTION_BASE Standard_Failure
#define __OCCT_EXCEPTION_DISABLE No_StdFail_NotDone
#include <Standard_DefineException.lxx>

// Exception: StdFail_InfiniteSolutions (base: Standard_Failure)
#define __OCCT_EXCEPTION_CLASS StdFail_InfiniteSolutions
#define __OCCT_EXCEPTION_BASE Standard_Failure
#define __OCCT_EXCEPTION_DISABLE No_StdFail_InfiniteSolutions
#include <Standard_DefineException.lxx>

// Exception: Standard_Underflow (base: Standard_NumericError)
#define __OCCT_EXCEPTION_CLASS Standard_Underflow
#define __OCCT_EXCEPTION_BASE Standard_NumericError
#define __OCCT_EXCEPTION_DISABLE No_Standard_Underflow
#include <Standard_DefineException.lxx>

// Exception: Standard_Overflow (base: Standard_NumericError)
#define __OCCT_EXCEPTION_CLASS Standard_Overflow
#define __OCCT_EXCEPTION_BASE Standard_NumericError
#define __OCCT_EXCEPTION_DISABLE No_Standard_Overflow
#include <Standard_DefineException.lxx>

// Exception: Standard_LicenseError (base: Standard_Failure)
#define __OCCT_EXCEPTION_CLASS Standard_LicenseError
#define __OCCT_EXCEPTION_BASE Standard_Failure
#define __OCCT_EXCEPTION_DISABLE No_Standard_LicenseError
#include <Standard_DefineException.lxx>

// Exception: Standard_LicenseNotFound (base: Standard_LicenseError)
#define __OCCT_EXCEPTION_CLASS Standard_LicenseNotFound
#define __OCCT_EXCEPTION_BASE Standard_LicenseError
#define __OCCT_EXCEPTION_DISABLE No_Standard_LicenseNotFound
#include <Standard_DefineException.lxx>

// Exception: Standard_ImmutableObject (base: Standard_DomainError)
#define __OCCT_EXCEPTION_CLASS Standard_ImmutableObject
#define __OCCT_EXCEPTION_BASE Standard_DomainError
#define __OCCT_EXCEPTION_DISABLE No_Standard_ImmutableObject
#include <Standard_DefineException.lxx>

// Exception: Standard_AbortiveTransaction (base: Standard_Failure)
#define __OCCT_EXCEPTION_CLASS Standard_AbortiveTransaction
#define __OCCT_EXCEPTION_BASE Standard_Failure
#define __OCCT_EXCEPTION_DISABLE No_Standard_AbortiveTransaction
#include <Standard_DefineException.lxx>

// Exception: Resource_NoSuchResource (base: Standard_NoSuchObject)
#define __OCCT_EXCEPTION_CLASS Resource_NoSuchResource
#define __OCCT_EXCEPTION_BASE Standard_NoSuchObject
#define __OCCT_EXCEPTION_DISABLE No_Resource_NoSuchResource
#include <Standard_DefineException.lxx>

// Exception: Quantity_PeriodDefinitionError (base: Standard_DomainError)
#define __OCCT_EXCEPTION_CLASS Quantity_PeriodDefinitionError
#define __OCCT_EXCEPTION_BASE Standard_DomainError
#define __OCCT_EXCEPTION_DISABLE No_Quantity_PeriodDefinitionError
#include <Standard_DefineException.lxx>

// Exception: Quantity_DateDefinitionError (base: Standard_DomainError)
#define __OCCT_EXCEPTION_CLASS Quantity_DateDefinitionError
#define __OCCT_EXCEPTION_BASE Standard_DomainError
#define __OCCT_EXCEPTION_DISABLE No_Quantity_DateDefinitionError
#include <Standard_DefineException.lxx>

// Exception: Plugin_Failure (base: Standard_Failure)
#define __OCCT_EXCEPTION_CLASS Plugin_Failure
#define __OCCT_EXCEPTION_BASE Standard_Failure
#define __OCCT_EXCEPTION_DISABLE No_Plugin_Failure
#include <Standard_DefineException.lxx>

// Exception: OSD_Signal (base: Standard_Failure)
#define __OCCT_EXCEPTION_CLASS OSD_Signal
#define __OCCT_EXCEPTION_BASE Standard_Failure
#define __OCCT_EXCEPTION_DISABLE No_OSD_Signal
#include <Standard_DefineException.lxx>

// Exception: OSD_SIGSYS (base: OSD_Signal)
#define __OCCT_EXCEPTION_CLASS OSD_SIGSYS
#define __OCCT_EXCEPTION_BASE OSD_Signal
#define __OCCT_EXCEPTION_DISABLE No_OSD_SIGSYS
#include <Standard_DefineException.lxx>

// Exception: OSD_SIGSEGV (base: OSD_Signal)
#define __OCCT_EXCEPTION_CLASS OSD_SIGSEGV
#define __OCCT_EXCEPTION_BASE OSD_Signal
#define __OCCT_EXCEPTION_DISABLE No_OSD_SIGSEGV
#include <Standard_DefineException.lxx>

// Exception: OSD_SIGQUIT (base: OSD_Signal)
#define __OCCT_EXCEPTION_CLASS OSD_SIGQUIT
#define __OCCT_EXCEPTION_BASE OSD_Signal
#define __OCCT_EXCEPTION_DISABLE No_OSD_SIGQUIT
#include <Standard_DefineException.lxx>

// Exception: OSD_SIGKILL (base: OSD_Signal)
#define __OCCT_EXCEPTION_CLASS OSD_SIGKILL
#define __OCCT_EXCEPTION_BASE OSD_Signal
#define __OCCT_EXCEPTION_DISABLE No_OSD_SIGKILL
#include <Standard_DefineException.lxx>

// Exception: OSD_SIGINT (base: OSD_Signal)
#define __OCCT_EXCEPTION_CLASS OSD_SIGINT
#define __OCCT_EXCEPTION_BASE OSD_Signal
#define __OCCT_EXCEPTION_DISABLE No_OSD_SIGINT
#include <Standard_DefineException.lxx>

// Exception: OSD_SIGILL (base: OSD_Signal)
#define __OCCT_EXCEPTION_CLASS OSD_SIGILL
#define __OCCT_EXCEPTION_BASE OSD_Signal
#define __OCCT_EXCEPTION_DISABLE No_OSD_SIGILL
#include <Standard_DefineException.lxx>

// Exception: OSD_SIGHUP (base: OSD_Signal)
#define __OCCT_EXCEPTION_CLASS OSD_SIGHUP
#define __OCCT_EXCEPTION_BASE OSD_Signal
#define __OCCT_EXCEPTION_DISABLE No_OSD_SIGHUP
#include <Standard_DefineException.lxx>

// Exception: OSD_SIGBUS (base: OSD_Signal)
#define __OCCT_EXCEPTION_CLASS OSD_SIGBUS
#define __OCCT_EXCEPTION_BASE OSD_Signal
#define __OCCT_EXCEPTION_DISABLE No_OSD_SIGBUS
#include <Standard_DefineException.lxx>

// Exception: OSD_OSDError (base: Standard_Failure)
#define __OCCT_EXCEPTION_CLASS OSD_OSDError
#define __OCCT_EXCEPTION_BASE Standard_Failure
#define __OCCT_EXCEPTION_DISABLE No_OSD_OSDError
#include <Standard_DefineException.lxx>

// Exception: OSD_Exception (base: Standard_Failure)
#define __OCCT_EXCEPTION_CLASS OSD_Exception
#define __OCCT_EXCEPTION_BASE Standard_Failure
#define __OCCT_EXCEPTION_DISABLE No_OSD_Exception
#include <Standard_DefineException.lxx>

// Exception: OSD_Exception_STATUS_NO_MEMORY (base: OSD_Exception)
#define __OCCT_EXCEPTION_CLASS OSD_Exception_STATUS_NO_MEMORY
#define __OCCT_EXCEPTION_BASE OSD_Exception
#define __OCCT_EXCEPTION_DISABLE No_OSD_Exception_STATUS_NO_MEMORY
#include <Standard_DefineException.lxx>

// Exception: OSD_Exception_STACK_OVERFLOW (base: OSD_Exception)
#define __OCCT_EXCEPTION_CLASS OSD_Exception_STACK_OVERFLOW
#define __OCCT_EXCEPTION_BASE OSD_Exception
#define __OCCT_EXCEPTION_DISABLE No_OSD_Exception_STACK_OVERFLOW
#include <Standard_DefineException.lxx>

// Exception: OSD_Exception_PRIV_INSTRUCTION (base: OSD_Exception)
#define __OCCT_EXCEPTION_CLASS OSD_Exception_PRIV_INSTRUCTION
#define __OCCT_EXCEPTION_BASE OSD_Exception
#define __OCCT_EXCEPTION_DISABLE No_OSD_Exception_PRIV_INSTRUCTION
#include <Standard_DefineException.lxx>

// Exception: OSD_Exception_NONCONTINUABLE_EXCEPTION (base: OSD_Exception)
#define __OCCT_EXCEPTION_CLASS OSD_Exception_NONCONTINUABLE_EXCEPTION
#define __OCCT_EXCEPTION_BASE OSD_Exception
#define __OCCT_EXCEPTION_DISABLE No_OSD_Exception_NONCONTINUABLE_EXCEPTION
#include <Standard_DefineException.lxx>

// Exception: OSD_Exception_IN_PAGE_ERROR (base: OSD_Exception)
#define __OCCT_EXCEPTION_CLASS OSD_Exception_IN_PAGE_ERROR
#define __OCCT_EXCEPTION_BASE OSD_Exception
#define __OCCT_EXCEPTION_DISABLE No_OSD_Exception_IN_PAGE_ERROR
#include <Standard_DefineException.lxx>

// Exception: OSD_Exception_INVALID_DISPOSITION (base: OSD_Exception)
#define __OCCT_EXCEPTION_CLASS OSD_Exception_INVALID_DISPOSITION
#define __OCCT_EXCEPTION_BASE OSD_Exception
#define __OCCT_EXCEPTION_DISABLE No_OSD_Exception_INVALID_DISPOSITION
#include <Standard_DefineException.lxx>

// Exception: OSD_Exception_INT_OVERFLOW (base: OSD_Exception)
#define __OCCT_EXCEPTION_CLASS OSD_Exception_INT_OVERFLOW
#define __OCCT_EXCEPTION_BASE OSD_Exception
#define __OCCT_EXCEPTION_DISABLE No_OSD_Exception_INT_OVERFLOW
#include <Standard_DefineException.lxx>

// Exception: OSD_Exception_ILLEGAL_INSTRUCTION (base: OSD_Exception)
#define __OCCT_EXCEPTION_CLASS OSD_Exception_ILLEGAL_INSTRUCTION
#define __OCCT_EXCEPTION_BASE OSD_Exception
#define __OCCT_EXCEPTION_DISABLE No_OSD_Exception_ILLEGAL_INSTRUCTION
#include <Standard_DefineException.lxx>

// Exception: OSD_Exception_CTRL_BREAK (base: OSD_Exception)
#define __OCCT_EXCEPTION_CLASS OSD_Exception_CTRL_BREAK
#define __OCCT_EXCEPTION_BASE OSD_Exception
#define __OCCT_EXCEPTION_DISABLE No_OSD_Exception_CTRL_BREAK
#include <Standard_DefineException.lxx>

// Exception: OSD_Exception_ARRAY_BOUNDS_EXCEEDED (base: OSD_Exception)
#define __OCCT_EXCEPTION_CLASS OSD_Exception_ARRAY_BOUNDS_EXCEEDED
#define __OCCT_EXCEPTION_BASE OSD_Exception
#define __OCCT_EXCEPTION_DISABLE No_OSD_Exception_ARRAY_BOUNDS_EXCEEDED
#include <Standard_DefineException.lxx>

// Exception: OSD_Exception_ACCESS_VIOLATION (base: OSD_Exception)
#define __OCCT_EXCEPTION_CLASS OSD_Exception_ACCESS_VIOLATION
#define __OCCT_EXCEPTION_BASE OSD_Exception
#define __OCCT_EXCEPTION_DISABLE No_OSD_Exception_ACCESS_VIOLATION
#include <Standard_DefineException.lxx>

// Exception: math_SingularMatrix (base: Standard_Failure)
#define __OCCT_EXCEPTION_CLASS math_SingularMatrix
#define __OCCT_EXCEPTION_BASE Standard_Failure
#define __OCCT_EXCEPTION_DISABLE No_math_SingularMatrix
#include <Standard_DefineException.lxx>

// Exception: math_NotSquare (base: Standard_DimensionError)
#define __OCCT_EXCEPTION_CLASS math_NotSquare
#define __OCCT_EXCEPTION_BASE Standard_DimensionError
#define __OCCT_EXCEPTION_DISABLE No_math_NotSquare
#include <Standard_DefineException.lxx>

// Exception: gp_VectorWithNullMagnitude (base: Standard_DomainError)
#define __OCCT_EXCEPTION_CLASS gp_VectorWithNullMagnitude
#define __OCCT_EXCEPTION_BASE Standard_DomainError
#define __OCCT_EXCEPTION_DISABLE No_gp_VectorWithNullMagnitude
#include <Standard_DefineException.lxx>

// Exception: Draw_Failure (base: Standard_Failure)
#define __OCCT_EXCEPTION_CLASS Draw_Failure
#define __OCCT_EXCEPTION_BASE Standard_Failure
#define __OCCT_EXCEPTION_DISABLE No_Draw_Failure
#include <Standard_DefineException.lxx>

// Exception: Interface_InterfaceError (base: Standard_Failure)
#define __OCCT_EXCEPTION_CLASS Interface_InterfaceError
#define __OCCT_EXCEPTION_BASE Standard_Failure
#define __OCCT_EXCEPTION_DISABLE No_Interface_InterfaceError
#include <Standard_DefineException.lxx>

// Exception: Transfer_TransferFailure (base: Interface_InterfaceError)
#define __OCCT_EXCEPTION_CLASS Transfer_TransferFailure
#define __OCCT_EXCEPTION_BASE Interface_InterfaceError
#define __OCCT_EXCEPTION_DISABLE No_Transfer_TransferFailure
#include <Standard_DefineException.lxx>

// Exception: Transfer_TransferDeadLoop (base: Transfer_TransferFailure)
#define __OCCT_EXCEPTION_CLASS Transfer_TransferDeadLoop
#define __OCCT_EXCEPTION_BASE Transfer_TransferFailure
#define __OCCT_EXCEPTION_DISABLE No_Transfer_TransferDeadLoop
#include <Standard_DefineException.lxx>

// Exception: Interface_InterfaceMismatch (base: Interface_InterfaceError)
#define __OCCT_EXCEPTION_CLASS Interface_InterfaceMismatch
#define __OCCT_EXCEPTION_BASE Interface_InterfaceError
#define __OCCT_EXCEPTION_DISABLE No_Interface_InterfaceMismatch
#include <Standard_DefineException.lxx>

// Exception: Interface_CheckFailure (base: Interface_InterfaceError)
#define __OCCT_EXCEPTION_CLASS Interface_CheckFailure
#define __OCCT_EXCEPTION_BASE Interface_InterfaceError
#define __OCCT_EXCEPTION_DISABLE No_Interface_CheckFailure
#include <Standard_DefineException.lxx>

// Exception: PCDM_DriverError (base: Standard_Failure)
#define __OCCT_EXCEPTION_CLASS PCDM_DriverError
#define __OCCT_EXCEPTION_BASE Standard_Failure
#define __OCCT_EXCEPTION_DISABLE No_PCDM_DriverError
#include <Standard_DefineException.lxx>

#endif // _Standard_FailureRegistry_HeaderFile

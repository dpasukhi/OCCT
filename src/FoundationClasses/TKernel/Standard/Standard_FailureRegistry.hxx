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

// Core Standard exceptions

DEFINE_STANDARD_EXCEPTION(Standard_ProgramError, Standard_Failure)

DEFINE_STANDARD_EXCEPTION(Standard_DomainError, Standard_Failure)

DEFINE_STANDARD_EXCEPTION(Standard_NumericError, Standard_Failure)

DEFINE_STANDARD_EXCEPTION(Standard_RangeError, Standard_DomainError)

DEFINE_STANDARD_EXCEPTION(Standard_OutOfRange, Standard_RangeError)

DEFINE_STANDARD_EXCEPTION(Standard_NullValue, Standard_RangeError)

DEFINE_STANDARD_EXCEPTION(Standard_NullObject, Standard_DomainError)

DEFINE_STANDARD_EXCEPTION(Standard_TypeMismatch, Standard_DomainError)

DEFINE_STANDARD_EXCEPTION(Standard_ConstructionError, Standard_DomainError)

DEFINE_STANDARD_EXCEPTION(Standard_DimensionError, Standard_DomainError)

DEFINE_STANDARD_EXCEPTION(Standard_DimensionMismatch, Standard_DimensionError)

DEFINE_STANDARD_EXCEPTION(Standard_DivideByZero, Standard_NumericError)

DEFINE_STANDARD_EXCEPTION(Standard_NegativeValue, Standard_RangeError)

DEFINE_STANDARD_EXCEPTION(Standard_MultiplyDefined, Standard_DomainError)

DEFINE_STANDARD_EXCEPTION(Standard_NoSuchObject, Standard_DomainError)

DEFINE_STANDARD_EXCEPTION(Standard_NoMoreObject, Standard_DomainError)

DEFINE_STANDARD_EXCEPTION(Standard_NotImplemented, Standard_ProgramError)

//! An attempt was made to modify a Shape already
//! shared or protected.
DEFINE_STANDARD_EXCEPTION(TopoDS_FrozenShape, Standard_DomainError)

DEFINE_STANDARD_EXCEPTION(V3d_UnMapped, Standard_DomainError)

DEFINE_STANDARD_EXCEPTION(V3d_BadValue, Standard_OutOfRange)

DEFINE_STANDARD_EXCEPTION(Prs3d_InvalidAngle, Standard_RangeError)

DEFINE_STANDARD_EXCEPTION(WNT_ClassDefinitionError, Standard_ConstructionError)

DEFINE_STANDARD_EXCEPTION(Graphic3d_StructureDefinitionError, Standard_OutOfRange)

DEFINE_STANDARD_EXCEPTION(Graphic3d_PriorityDefinitionError, Standard_OutOfRange)

DEFINE_STANDARD_EXCEPTION(Graphic3d_MaterialDefinitionError, Standard_OutOfRange)

DEFINE_STANDARD_EXCEPTION(Graphic3d_GroupDefinitionError, Standard_OutOfRange)

DEFINE_STANDARD_EXCEPTION(Aspect_WindowError, Standard_OutOfRange)

DEFINE_STANDARD_EXCEPTION(Aspect_WindowDefinitionError, Standard_OutOfRange)

DEFINE_STANDARD_EXCEPTION(Aspect_IdentDefinitionError, Standard_OutOfRange)

DEFINE_STANDARD_EXCEPTION(Aspect_GraphicDeviceDefinitionError, Standard_OutOfRange)

DEFINE_STANDARD_EXCEPTION(Aspect_DisplayConnectionDefinitionError, Standard_OutOfRange)

DEFINE_STANDARD_EXCEPTION(Aspect_AspectMarkerDefinitionError, Standard_OutOfRange)

DEFINE_STANDARD_EXCEPTION(Aspect_AspectLineDefinitionError, Standard_OutOfRange)

DEFINE_STANDARD_EXCEPTION(Aspect_AspectFillAreaDefinitionError, Standard_OutOfRange)

DEFINE_STANDARD_EXCEPTION(Geom_UndefinedValue, Standard_DomainError)

DEFINE_STANDARD_EXCEPTION(Geom_UndefinedDerivative, Standard_DomainError)

//! This exception is raised when a method makes reference to
//! an undefined inertia axis of symmetry.
DEFINE_STANDARD_EXCEPTION(GProp_UndefinedAxis, Standard_DomainError)

DEFINE_STANDARD_EXCEPTION(LProp_NotDefined, Standard_Failure)

DEFINE_STANDARD_EXCEPTION(LProp_BadContinuity, Standard_Failure)

DEFINE_STANDARD_EXCEPTION(Geom2d_UndefinedValue, Standard_DomainError)

DEFINE_STANDARD_EXCEPTION(Geom2d_UndefinedDerivative, Standard_DomainError)

//! An incorrect insertion was attempted.
DEFINE_STANDARD_EXCEPTION(TopoDS_UnCompatibleShapes, Standard_DomainError)

//! An attempt was made to modify a geometry of Shape already
//! shared or protected.
DEFINE_STANDARD_EXCEPTION(TopoDS_LockedShape, Standard_DomainError)

DEFINE_STANDARD_EXCEPTION(BRepExtrema_UnCompatibleShape, Standard_DomainError)

DEFINE_STANDARD_EXCEPTION(Geom2dGcc_IsParallel, Standard_DomainError)

DEFINE_STANDARD_EXCEPTION(GccEnt_BadQualifier, Standard_DomainError)

DEFINE_STANDARD_EXCEPTION(GccAna_NoSolution, Standard_Failure)

DEFINE_STANDARD_EXCEPTION(ExprIntrp_SyntaxError, Standard_Failure)

DEFINE_STANDARD_EXCEPTION(Expr_ExprFailure, Standard_Failure)

DEFINE_STANDARD_EXCEPTION(Expr_NotEvaluable, Expr_ExprFailure)

DEFINE_STANDARD_EXCEPTION(Expr_NotAssigned, Expr_ExprFailure)

DEFINE_STANDARD_EXCEPTION(Expr_InvalidOperand, Expr_ExprFailure)

DEFINE_STANDARD_EXCEPTION(Expr_InvalidFunction, Expr_ExprFailure)

DEFINE_STANDARD_EXCEPTION(Expr_InvalidAssignment, Expr_ExprFailure)

DEFINE_STANDARD_EXCEPTION(Units_NoSuchUnit, Standard_NoSuchObject)

DEFINE_STANDARD_EXCEPTION(Units_NoSuchType, Standard_NoSuchObject)

DEFINE_STANDARD_EXCEPTION(Storage_StreamWriteError, Standard_Failure)

DEFINE_STANDARD_EXCEPTION(Storage_StreamReadError, Standard_Failure)

DEFINE_STANDARD_EXCEPTION(Storage_StreamUnknownTypeError, Storage_StreamReadError)

DEFINE_STANDARD_EXCEPTION(Storage_StreamTypeMismatchError, Storage_StreamReadError)

DEFINE_STANDARD_EXCEPTION(Storage_StreamModeError, Standard_Failure)

DEFINE_STANDARD_EXCEPTION(Storage_StreamFormatError, Standard_Failure)

DEFINE_STANDARD_EXCEPTION(Storage_StreamExtCharParityError, Storage_StreamReadError)

DEFINE_STANDARD_EXCEPTION(StdFail_UndefinedValue, Standard_DomainError)

DEFINE_STANDARD_EXCEPTION(StdFail_UndefinedDerivative, Standard_DomainError)

DEFINE_STANDARD_EXCEPTION(StdFail_Undefined, Standard_Failure)

DEFINE_STANDARD_EXCEPTION(StdFail_NotDone, Standard_Failure)

DEFINE_STANDARD_EXCEPTION(StdFail_InfiniteSolutions, Standard_Failure)

DEFINE_STANDARD_EXCEPTION(Standard_Underflow, Standard_NumericError)

DEFINE_STANDARD_EXCEPTION(Standard_Overflow, Standard_NumericError)

DEFINE_STANDARD_EXCEPTION(Standard_LicenseError, Standard_Failure)

DEFINE_STANDARD_EXCEPTION(Standard_LicenseNotFound, Standard_LicenseError)

DEFINE_STANDARD_EXCEPTION(Standard_ImmutableObject, Standard_DomainError)

DEFINE_STANDARD_EXCEPTION(Standard_AbortiveTransaction, Standard_Failure)

DEFINE_STANDARD_EXCEPTION(Resource_NoSuchResource, Standard_NoSuchObject)

DEFINE_STANDARD_EXCEPTION(Quantity_PeriodDefinitionError, Standard_DomainError)

DEFINE_STANDARD_EXCEPTION(Quantity_DateDefinitionError, Standard_DomainError)

DEFINE_STANDARD_EXCEPTION(Plugin_Failure, Standard_Failure)

DEFINE_STANDARD_EXCEPTION(OSD_Signal, Standard_Failure)

DEFINE_STANDARD_EXCEPTION(OSD_SIGSYS, OSD_Signal)

DEFINE_STANDARD_EXCEPTION(OSD_SIGSEGV, OSD_Signal)

DEFINE_STANDARD_EXCEPTION(OSD_SIGQUIT, OSD_Signal)

DEFINE_STANDARD_EXCEPTION(OSD_SIGKILL, OSD_Signal)

DEFINE_STANDARD_EXCEPTION(OSD_SIGINT, OSD_Signal)

DEFINE_STANDARD_EXCEPTION(OSD_SIGILL, OSD_Signal)

DEFINE_STANDARD_EXCEPTION(OSD_SIGHUP, OSD_Signal)

DEFINE_STANDARD_EXCEPTION(OSD_SIGBUS, OSD_Signal)

DEFINE_STANDARD_EXCEPTION(OSD_OSDError, Standard_Failure)

DEFINE_STANDARD_EXCEPTION(OSD_Exception, Standard_Failure)

DEFINE_STANDARD_EXCEPTION(OSD_Exception_STATUS_NO_MEMORY, OSD_Exception)

DEFINE_STANDARD_EXCEPTION(OSD_Exception_STACK_OVERFLOW, OSD_Exception)

DEFINE_STANDARD_EXCEPTION(OSD_Exception_PRIV_INSTRUCTION, OSD_Exception)

DEFINE_STANDARD_EXCEPTION(OSD_Exception_NONCONTINUABLE_EXCEPTION, OSD_Exception)

DEFINE_STANDARD_EXCEPTION(OSD_Exception_IN_PAGE_ERROR, OSD_Exception)

DEFINE_STANDARD_EXCEPTION(OSD_Exception_INVALID_DISPOSITION, OSD_Exception)

DEFINE_STANDARD_EXCEPTION(OSD_Exception_INT_OVERFLOW, OSD_Exception)

DEFINE_STANDARD_EXCEPTION(OSD_Exception_ILLEGAL_INSTRUCTION, OSD_Exception)

DEFINE_STANDARD_EXCEPTION(OSD_Exception_CTRL_BREAK, OSD_Exception)

DEFINE_STANDARD_EXCEPTION(OSD_Exception_ARRAY_BOUNDS_EXCEEDED, OSD_Exception)

DEFINE_STANDARD_EXCEPTION(OSD_Exception_ACCESS_VIOLATION, OSD_Exception)

DEFINE_STANDARD_EXCEPTION(math_SingularMatrix, Standard_Failure)

DEFINE_STANDARD_EXCEPTION(math_NotSquare, Standard_DimensionError)

DEFINE_STANDARD_EXCEPTION(gp_VectorWithNullMagnitude, Standard_DomainError)

DEFINE_STANDARD_EXCEPTION(Draw_Failure, Standard_Failure)

DEFINE_STANDARD_EXCEPTION(Interface_InterfaceError, Standard_Failure)

DEFINE_STANDARD_EXCEPTION(Transfer_TransferFailure, Interface_InterfaceError)

DEFINE_STANDARD_EXCEPTION(Transfer_TransferDeadLoop, Transfer_TransferFailure)

DEFINE_STANDARD_EXCEPTION(Interface_InterfaceMismatch, Interface_InterfaceError)

DEFINE_STANDARD_EXCEPTION(Interface_CheckFailure, Interface_InterfaceError)

DEFINE_STANDARD_EXCEPTION(PCDM_DriverError, Standard_Failure)

#endif // _Standard_FailureRegistry_HeaderFile

// Copyright (c) 2000-2014 OPEN CASCADE SAS
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

#ifndef _XSAlgo_ShapeProcessor_HeaderFile
#define _XSAlgo_ShapeProcessor_HeaderFile

#include <DE_ShapeFixParameters.hxx>
#include <ShapeProcess.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>

#include <unordered_map>

class ShapeProcess_ShapeContext;
class ShapeExtend_MsgRegistrator;
class Transfer_TransientProcess;
class Transfer_FinderProcess;
class Transfer_Binder;

//! Shape Processing module.
//! Allows to define and apply general Shape Processing as a customizable sequence of operators.
class XSAlgo_ShapeProcessor
{
public:
  using OperationsFlags = ShapeProcess::OperationsFlags;
  using ParameterMap    = std::unordered_map<std::string, std::string>;

public:
  //! Constructor.
  //! @param theParameters Pre-filled parameter map to be used in the processing.
  //! @param theShapeFixParameters Shape healing parameters to be used in the processing.
  //!        If @p theParameters has some shape healing values, they will override the
  //!        corresponding values from @p theShapeFixParameters.
  Standard_EXPORT XSAlgo_ShapeProcessor(const ParameterMap&          theParameters,
                                        const DE_ShapeFixParameters& theShapeFixParameters = {});

  //! Constructor.
  //! @param theParameters Parameters to be used in the processing.
  Standard_EXPORT XSAlgo_ShapeProcessor(const DE_ShapeFixParameters& theParameters);

  //! Process the shape by applying the specified operations.
  //! @param theShape Shape to process.
  //! @param theOperations Operations to be performed.
  //! @param theProgress Progress indicator.
  //! @return Processed shape. May be the same as the input shape if no modifications were made.
  Standard_EXPORT TopoDS_Shape ProcessShape(const TopoDS_Shape&          theShape,
                                            const OperationsFlags&       theOperations,
                                            const Message_ProgressRange& theProgress);

  //! Get the context of the last processing.
  //! Only valid after the ProcessShape() method was called.
  //! @return Shape context.
  Handle(ShapeProcess_ShapeContext) GetContext() { return myContext; }

  //! Merge the results of the shape processing with the transfer process.
  //! @param theTransientProcess Transfer process to merge with.
  //! @param theFirstTPItemIndex Index of the first item in the transfer process to merge with.
  Standard_EXPORT void MergeTransferInfo(const Handle(Transfer_TransientProcess)& theTransientProcess,
                                         const Standard_Integer                   theFirstTPItemIndex) const;

  //! Merge the results of the shape processing with the finder process.
  //! @param theFinderProcess Finder process to merge with.
  Standard_EXPORT void MergeTransferInfo(const Handle(Transfer_FinderProcess)& theFinderProcess) const;

  //! Check quality of pcurve of the edge on the given face, and correct it if necessary.
  //! @param theEdge Edge to check.
  //! @param theFace Face on which the edge is located.
  //! @param thePrecision Precision to use for checking.
  //! @param theIsSeam Flag indicating whether the edge is a seam edge.
  //! @return True if the pcurve was corrected, false if it was dropped.
  Standard_EXPORT static Standard_Boolean CheckPCurve(const TopoDS_Edge&     theEdge,
                                                      const TopoDS_Face&     theFace,
                                                      const Standard_Real    thePrecision,
                                                      const Standard_Boolean theIsSeam);

  //! Fill the parameter map with the values from the specified parameters.
  //! @param theParameters Parameters to be used in the processing.
  //! @param theMap Map to fill.
  Standard_EXPORT static void FillParameterMap(const DE_ShapeFixParameters& theParameters, ParameterMap& theMap);

  //! The function is designed to set the length unit for the application before performing a
  //! transfer operation. It ensures that the length unit is correctly configured based on the
  //! value associated with the key "xstep.cascade.unit".
  Standard_EXPORT static void PrepareForTransfer();

private:
  //! Initialize the context with the specified shape.
  //! @param theShape Shape to process.
  void initializeContext(const TopoDS_Shape& theShape);

  //! Add messages from the specified shape to the transfer binder.
  //! @param theMessages Container with messages.
  //! @param theShape Shape to get messages from.
  //! @param theBinder Transfer binder to add messages to.
  static void addMessages(const Handle(ShapeExtend_MsgRegistrator)& theMessages,
                          const TopoDS_Shape&                       theShape,
                          Handle(Transfer_Binder)&                  theBinder);

  //! Create a new edge with the same geometry as the source edge.
  //! @param theSourceEdge Source edge.
  //! @return New edge with the same geometry.
  static TopoDS_Edge MakeEdgeOnCurve(const TopoDS_Edge& aSourceEdge);

private:
  ParameterMap                      myParameters; //!< Parameters to be used in the processing.
  Handle(ShapeProcess_ShapeContext) myContext;    //!< Shape context.
};

#endif // _XSAlgo_ShapeProcessor_HeaderFile

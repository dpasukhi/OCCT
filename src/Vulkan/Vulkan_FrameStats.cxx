// Created by: Kirill GAVRILOV
// Copyright (c) 2019 OPEN CASCADE SAS
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

#include <Vulkan_FrameStats.hxx>

#include <Vulkan_DeviceMemory.hxx>
#include <Vulkan_Group.hxx>
#include <Vulkan_IndexBuffer.hxx>
#include <Vulkan_VertexBuffer.hxx>
#include <Vulkan_View.hxx>
#include <Vulkan_PrimitiveArray.hxx>
#include <Vulkan_Structure.hxx>

IMPLEMENT_STANDARD_RTTIEXT(Vulkan_FrameStats, Graphic3d_FrameStats)

namespace
{
  //! Return estimated data size.
  static Standard_Size estimatedDataSize (const Handle(Vulkan_Buffer)& theRes)
  {
    if (theRes.IsNull()
     || theRes->DeviceMemory().IsNull())
    {
      return 0;
    }
    const Vulkan_DeviceMemoryInfo aMemInfo = theRes->DeviceMemory()->DeviceMemoryInfo();
    return aMemInfo.Size;
  }
}

// =======================================================================
// function : Vulkan_FrameStats
// purpose  :
// =======================================================================
Vulkan_FrameStats::Vulkan_FrameStats()
{
  //
}

// =======================================================================
// function : ~Vulkan_FrameStats
// purpose  :
// =======================================================================
Vulkan_FrameStats::~Vulkan_FrameStats()
{
  //
}

// =======================================================================
// function : IsFrameUpdated
// purpose  :
// =======================================================================
bool Vulkan_FrameStats::IsFrameUpdated (Handle(Vulkan_FrameStats)& thePrev) const
{
  const Graphic3d_FrameStatsData& aFrame = LastDataFrame();
  if (thePrev.IsNull())
  {
    thePrev = new Vulkan_FrameStats();
  }
  // check just a couple of major counters
  else if (myLastFrameIndex == thePrev->myLastFrameIndex
        && Abs (aFrame.FrameRate()    - thePrev->myCountersTmp.FrameRate())    <= 0.001
        && Abs (aFrame.FrameRateCpu() - thePrev->myCountersTmp.FrameRateCpu()) <= 0.001
        && aFrame[Graphic3d_FrameStatsCounter_NbLayers]           == thePrev->myCountersTmp[Graphic3d_FrameStatsCounter_NbLayers]
        && aFrame[Graphic3d_FrameStatsCounter_NbLayersNotCulled]  == thePrev->myCountersTmp[Graphic3d_FrameStatsCounter_NbLayersNotCulled]
        && aFrame[Graphic3d_FrameStatsCounter_NbStructs]          == thePrev->myCountersTmp[Graphic3d_FrameStatsCounter_NbStructs]
        && aFrame[Graphic3d_FrameStatsCounter_NbStructsNotCulled] == thePrev->myCountersTmp[Graphic3d_FrameStatsCounter_NbStructsNotCulled])
  {
    return false;
  }

  thePrev->myLastFrameIndex = myLastFrameIndex;
  thePrev->myCountersTmp = aFrame;
  return true;
}

// =======================================================================
// function : updateStatistics
// purpose  :
// =======================================================================
void Vulkan_FrameStats::updateStatistics (const Handle(Graphic3d_CView)& theView,
                                          bool theIsImmediateOnly)
{
  const Vulkan_View* aView = dynamic_cast<const Vulkan_View*> (theView.get());
  if (aView == NULL)
  {
    myCounters.SetValue (myLastFrameIndex, myCountersTmp);
    myCountersTmp.Reset();
    return;
  }

  const Graphic3d_RenderingParams::PerfCounters aBits = theView->RenderingParams().CollectedStats;
  const Standard_Boolean toCountMem     = (aBits & Graphic3d_RenderingParams::PerfCounters_EstimMem)  != 0;
  const Standard_Boolean toCountTris    = (aBits & Graphic3d_RenderingParams::PerfCounters_Triangles) != 0
                                       || (aBits & Graphic3d_RenderingParams::PerfCounters_Points)    != 0;
  const Standard_Boolean toCountElems   = (aBits & Graphic3d_RenderingParams::PerfCounters_GroupArrays) != 0 || toCountTris || toCountMem;
  const Standard_Boolean toCountGroups  = (aBits & Graphic3d_RenderingParams::PerfCounters_Groups)      != 0 || toCountElems;
  const Standard_Boolean toCountStructs = (aBits & Graphic3d_RenderingParams::PerfCounters_Structures)  != 0
                                       || (aBits & Graphic3d_RenderingParams::PerfCounters_Layers)      != 0 || toCountGroups;

  /// TODO
  ///myCountersTmp[Graphic3d_FrameStatsCounter_NbLayers] = aView->LayerList().Layers().Size();
  if (toCountStructs
   || (aBits & Graphic3d_RenderingParams::PerfCounters_Layers)    != 0)
  {
    const Standard_Integer aViewId = aView->Identification();
    /// TODO
    /*for (OpenGl_SequenceOfLayers::Iterator aLayerIter (aView->LayerList().Layers()); aLayerIter.More(); aLayerIter.Next())
    {
      const Handle(OpenGl_Layer)& aLayer = aLayerIter.Value();
      myCountersTmp[Graphic3d_FrameStatsCounter_NbStructs] += aLayer->NbStructures();
      if (theIsImmediateOnly && !aLayer->LayerSettings().IsImmediate())
      {
        continue;
      }

      if (!aLayer->IsCulled())
      {
        ++myCountersTmp[Graphic3d_FrameStatsCounter_NbLayersNotCulled];
      }
      myCountersTmp[Graphic3d_FrameStatsCounter_NbStructsNotCulled] += aLayer->NbStructuresNotCulled();
      if (toCountGroups)
      {
        updateStructures (aViewId, aLayer->CullableStructuresBVH().Structures(), toCountElems, toCountTris, toCountMem);
        updateStructures (aViewId, aLayer->CullableTrsfPersStructuresBVH().Structures(), toCountElems, toCountTris, toCountMem);
        updateStructures (aViewId, aLayer->NonCullableStructures(), toCountElems, toCountTris, toCountMem);
      }
    }*/
  }
  if (toCountMem)
  {
    // TODO
    /**for (OpenGl_Context::OpenGl_ResourcesMap::Iterator aResIter (aView->GlWindow()->GetGlContext()->SharedResources());
         aResIter.More(); aResIter.Next())
    {
      myCountersTmp[Graphic3d_FrameStatsCounter_EstimatedBytesTextures] += aResIter.Value()->EstimatedDataSize();
    }

    {
      Standard_Size& aMemFbos = myCountersTmp[Graphic3d_FrameStatsCounter_EstimatedBytesFbos];
      // main FBOs
      aMemFbos += estimatedDataSize (aView->myMainSceneFbos[0]);
      aMemFbos += estimatedDataSize (aView->myMainSceneFbos[1]);
      aMemFbos += estimatedDataSize (aView->myImmediateSceneFbos[0]);
      aMemFbos += estimatedDataSize (aView->myImmediateSceneFbos[1]);
      // OIT FBOs
      aMemFbos += estimatedDataSize (aView->myMainSceneFbosOit[0]);
      aMemFbos += estimatedDataSize (aView->myMainSceneFbosOit[1]);
      aMemFbos += estimatedDataSize (aView->myImmediateSceneFbosOit[0]);
      aMemFbos += estimatedDataSize (aView->myImmediateSceneFbosOit[1]);
      // dump FBO
      aMemFbos += estimatedDataSize (aView->myFBO);
      // RayTracing FBO
      aMemFbos += estimatedDataSize (aView->myOpenGlFBO);
      aMemFbos += estimatedDataSize (aView->myOpenGlFBO2);
      aMemFbos += estimatedDataSize (aView->myRaytraceFBO1[0]);
      aMemFbos += estimatedDataSize (aView->myRaytraceFBO1[1]);
      aMemFbos += estimatedDataSize (aView->myRaytraceFBO2[0]);
      aMemFbos += estimatedDataSize (aView->myRaytraceFBO2[1]);
      // also RayTracing
      aMemFbos += estimatedDataSize (aView->myRaytraceOutputTexture[0]);
      aMemFbos += estimatedDataSize (aView->myRaytraceOutputTexture[1]);
      aMemFbos += estimatedDataSize (aView->myRaytraceVisualErrorTexture[0]);
      aMemFbos += estimatedDataSize (aView->myRaytraceVisualErrorTexture[1]);
      aMemFbos += estimatedDataSize (aView->myRaytraceTileOffsetsTexture[0]);
      aMemFbos += estimatedDataSize (aView->myRaytraceTileOffsetsTexture[1]);
      aMemFbos += estimatedDataSize (aView->myRaytraceTileSamplesTexture[0]);
      aMemFbos += estimatedDataSize (aView->myRaytraceTileSamplesTexture[1]);
    }*/
  }
}

// =======================================================================
// function : updateStructures
// purpose  :
// =======================================================================
void Vulkan_FrameStats::updateStructures (Standard_Integer theViewId,
                                          const NCollection_IndexedMap<const Graphic3d_CStructure*>& theStructures,
                                          Standard_Boolean theToCountElems,
                                          Standard_Boolean theToCountTris,
                                          Standard_Boolean theToCountMem)
{
  for (Vulkan_Structure::StructIterator aStructIter (theStructures); aStructIter.More(); aStructIter.Next())
  {
    const Vulkan_Structure* aStruct = aStructIter.Value();
    const bool isStructHidden = aStruct->IsCulled()
                            || !aStruct->IsVisible (theViewId);
    for (; aStruct != NULL; aStruct = aStruct->InstancedStructure())
    {
      if (isStructHidden)
      {
        if (theToCountMem)
        {
          for (Vulkan_Structure::GroupIterator aGroupIter (aStruct->Groups()); aGroupIter.More(); aGroupIter.Next())
          {
            const Vulkan_Group* aGroup = aGroupIter.Value();
            for (Vulkan_ListOfElements::Iterator aNodeIter (aGroup->Elements()); aNodeIter.More(); aNodeIter.Next())
            {
              if (const Vulkan_PrimitiveArray* aPrim = dynamic_cast<const Vulkan_PrimitiveArray*> (aNodeIter.Value().get()))
              {
                myCountersTmp[Graphic3d_FrameStatsCounter_EstimatedBytesGeom] += estimatedDataSize (aPrim->AttributesVbo());
                myCountersTmp[Graphic3d_FrameStatsCounter_EstimatedBytesGeom] += estimatedDataSize (aPrim->IndexVbo());
              }
            }
          }
        }
        continue;
      }

      myCountersTmp[Graphic3d_FrameStatsCounter_NbGroupsNotCulled] += aStruct->Groups().Size();
      if (!theToCountElems)
      {
        continue;
      }

      for (Vulkan_Structure::GroupIterator aGroupIter (aStruct->Groups()); aGroupIter.More(); aGroupIter.Next())
      {
        const Vulkan_Group* aGroup = aGroupIter.Value();
        for (Vulkan_ListOfElements::Iterator aNodeIter (aGroup->Elements()); aNodeIter.More(); aNodeIter.Next())
        {
          if (const Vulkan_PrimitiveArray* aPrim = dynamic_cast<const Vulkan_PrimitiveArray*> (aNodeIter.Value().get()))
          {
            ++myCountersTmp[Graphic3d_FrameStatsCounter_NbElemsNotCulled];
            if (theToCountMem)
            {
              myCountersTmp[Graphic3d_FrameStatsCounter_EstimatedBytesGeom] += estimatedDataSize (aPrim->AttributesVbo());
              myCountersTmp[Graphic3d_FrameStatsCounter_EstimatedBytesGeom] += estimatedDataSize (aPrim->IndexVbo());
            }

            if (aPrim->IsFillDrawMode())
            {
              ++myCountersTmp[Graphic3d_FrameStatsCounter_NbElemsFillNotCulled];
              if (!theToCountTris)
              {
                continue;
              }

              // TODO
              /**const Handle(OpenGl_VertexBuffer)& anAttribs = aPrim->AttributesVbo();
              if (anAttribs.IsNull()
              || !anAttribs->IsValid())
              {
                continue;
              }

              const Handle(OpenGl_VertexBuffer)& anIndices = aPrim->IndexVbo();
              const Standard_Integer aNbIndices = !anIndices.IsNull() ? anIndices->GetElemsNb() : anAttribs->GetElemsNb();
              const Standard_Integer aNbBounds  = !aPrim->Bounds().IsNull() ? aPrim->Bounds()->NbBounds : 1;
              switch (aPrim->DrawMode())
              {
                case GL_TRIANGLES:
                {
                  myCountersTmp[Graphic3d_FrameStatsCounter_NbTrianglesNotCulled] += aNbIndices / 3;
                  break;
                }
                case GL_TRIANGLE_STRIP:
                case GL_TRIANGLE_FAN:
                {
                  myCountersTmp[Graphic3d_FrameStatsCounter_NbTrianglesNotCulled] += aNbIndices - 2 * aNbBounds;
                  break;
                }
                case GL_TRIANGLES_ADJACENCY:
                {
                  myCountersTmp[Graphic3d_FrameStatsCounter_NbTrianglesNotCulled] += aNbIndices / 6;
                  break;
                }
                case GL_TRIANGLE_STRIP_ADJACENCY:
                {
                  myCountersTmp[Graphic3d_FrameStatsCounter_NbTrianglesNotCulled] += aNbIndices - 4 * aNbBounds;
                  break;
                }
              #if !defined(GL_ES_VERSION_2_0)
                case GL_QUADS:
                {
                  myCountersTmp[Graphic3d_FrameStatsCounter_NbTrianglesNotCulled] += aNbIndices / 2;
                  break;
                }
                case GL_QUAD_STRIP:
                {
                  myCountersTmp[Graphic3d_FrameStatsCounter_NbTrianglesNotCulled] += (aNbIndices / 2 - aNbBounds) * 2;
                  break;
                }
              #endif
              }*/
            }
            /*else if (aPrim->DrawMode() == GL_POINTS)
            {
              ++myCountersTmp[Graphic3d_FrameStatsCounter_NbElemsPointNotCulled];
              if (theToCountTris)
              {
                const Handle(OpenGl_VertexBuffer)& anAttribs = aPrim->AttributesVbo();
                if (!anAttribs.IsNull()
                  && anAttribs->IsValid())
                {
                  const Handle(OpenGl_VertexBuffer)& anIndices = aPrim->IndexVbo();
                  const Standard_Integer aNbIndices = !anIndices.IsNull() ? anIndices->GetElemsNb() : anAttribs->GetElemsNb();
                  myCountersTmp[Graphic3d_FrameStatsCounter_NbPointsNotCulled] += aNbIndices;
                }
              }
            }
            else
            {
              ++myCountersTmp[Graphic3d_FrameStatsCounter_NbElemsLineNotCulled];
            }*/
          }
          /*else if (const Vulkan_Text* aText = dynamic_cast<const Vulkan_Text*> (aNodeIter.Value().get()))
          {
            (void )aText;
            ++myCountersTmp[Graphic3d_FrameStatsCounter_NbElemsNotCulled];
            ++myCountersTmp[Graphic3d_FrameStatsCounter_NbElemsTextNotCulled];
          }*/
        }
      }
    }
  }
}

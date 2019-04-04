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

#ifndef _Vulkan_View_HeaderFile
#define _Vulkan_View_HeaderFile

#include <Graphic3d_CView.hxx>
#include <Graphic3d_Layer.hxx>

class Vulkan_Caps;
class Vulkan_Context;
class Vulkan_Fence;
class Vulkan_GraphicDriver;
class Vulkan_Surface;

//! Implementation of Vulkan view.
class Vulkan_View : public Graphic3d_CView
{
  DEFINE_STANDARD_RTTIEXT(Vulkan_View, Graphic3d_CView)
public:

  //! Constructor.
  Standard_EXPORT Vulkan_View (const Handle(Graphic3d_StructureManager)& theMgr,
                               const Handle(Vulkan_GraphicDriver)& theDriver);

  //! Default destructor.
  Standard_EXPORT virtual ~Vulkan_View();

  //! Release GPU resources.
  Standard_EXPORT void ReleaseVkResources();

  //! Deletes and erases the view.
  Standard_EXPORT virtual void Remove() Standard_OVERRIDE;

  //! @param theDrawToFrontBuffer Advanced option to modify rendering mode:
  //! 1. TRUE.  Drawing immediate mode structures directly to the front buffer over the scene image.
  //! Fast, so preferred for interactive work (used by default).
  //! However these extra drawings will be missed in image dump since it is performed from back buffer.
  //! Notice that since no pre-buffering used the V-Sync will be ignored and rendering could be seen
  //! in run-time (in case of slow hardware) and/or tearing may appear.
  //! So this is strongly recommended to draw only simple (fast) structures.
  //! 2. FALSE. Drawing immediate mode structures to the back buffer.
  //! The complete scene is redrawn first, so this mode is slower if scene contains complex data and/or V-Sync
  //! is turned on. But it works in any case and is especially useful for view dump because the dump image is read
  //! from the back buffer.
  //! @return previous mode.
  Standard_EXPORT virtual Standard_Boolean SetImmediateModeDrawToFront (const Standard_Boolean theDrawToFrontBuffer) Standard_OVERRIDE;

  //! Creates and maps rendering window to the view.
  //! @param theWindow [in] the window.
  //! @param theContext [in] the rendering context. If NULL the context will be created internally.
  Standard_EXPORT virtual void SetWindow (const Handle(Aspect_Window)&  theWindow,
                                          const Aspect_RenderingContext theContext) Standard_OVERRIDE;

  //! Returns window associated with the view.
  virtual Handle(Aspect_Window) Window() const Standard_OVERRIDE { return myPlatformWindow; }

  //! Returns True if the window associated to the view is defined.
  virtual Standard_Boolean IsDefined() const Standard_OVERRIDE { return !myPlatformWindow.IsNull(); }

  //! Handle changing size of the rendering window.
  Standard_EXPORT virtual void Resized() Standard_OVERRIDE;

  //! Redraw content of the view.
  Standard_EXPORT virtual void Redraw() Standard_OVERRIDE;

  //! Redraw immediate content of the view.
  Standard_EXPORT virtual void RedrawImmediate() Standard_OVERRIDE;

  //! Marks BVH tree for given priority list as dirty and marks primitive set for rebuild.
  Standard_EXPORT virtual void Invalidate() Standard_OVERRIDE;

  //! Return true if view content cache has been invalidated.
  virtual Standard_Boolean IsInvalidated() Standard_OVERRIDE { return !myBackBufferRestored; }

  //! Dump active rendering buffer into specified memory buffer.
  //! In Ray-Tracing allow to get a raw HDR buffer using Graphic3d_BT_RGB_RayTraceHdrLeft buffer type,
  //! only Left view will be dumped ignoring stereoscopic parameter.
  Standard_EXPORT virtual Standard_Boolean BufferDump (Image_PixMap& theImage,
                                                       const Graphic3d_BufferType& theBufferType) Standard_OVERRIDE;

  //! Marks BVH tree and the set of BVH primitives of correspondent priority list with id theLayerId as outdated.
  Standard_EXPORT virtual void InvalidateBVHData (const Graphic3d_ZLayerId theLayerId) Standard_OVERRIDE;

  //! Insert a new top-level z layer with the given ID.
  Standard_EXPORT virtual void AddZLayer (const Graphic3d_ZLayerId theLayerId) Standard_OVERRIDE;

  //! Remove a z layer with the given ID.
  Standard_EXPORT virtual void RemoveZLayer (const Graphic3d_ZLayerId theLayerId) Standard_OVERRIDE;

  //! Sets the settings for a single Z layer of specified view.
  Standard_EXPORT virtual void SetZLayerSettings (const Graphic3d_ZLayerId theLayerId,
                                                  const Graphic3d_ZLayerSettings& theSettings) Standard_OVERRIDE;

  //! Returns the maximum Z layer ID.
  //! First layer ID is Graphic3d_ZLayerId_Default, last ID is ZLayerMax().
  Standard_EXPORT virtual Standard_Integer ZLayerMax() const Standard_OVERRIDE;

  //! Returns the bounding box of all structures displayed in the Z layer.
  //! Never fails. If Z layer does not exist nothing happens.
  Standard_EXPORT virtual void InvalidateZLayerBoundingBox (const Graphic3d_ZLayerId theLayerId) const Standard_OVERRIDE;

  //! Returns the bounding box of all structures displayed in the Z layer.
  //! If Z layer does not exist the empty box is returned.
  //! @param theLayerId            layer identifier
  //! @param theCamera             camera definition
  //! @param theWindowWidth        viewport width  (for applying transformation-persistence)
  //! @param theWindowHeight       viewport height (for applying transformation-persistence)
  //! @param theToIncludeAuxiliary consider also auxiliary presentations (with infinite flag or with trihedron transformation persistence)
  //! @return computed bounding box
  Standard_EXPORT virtual Bnd_Box ZLayerBoundingBox (const Graphic3d_ZLayerId        theLayerId,
                                                     const Handle(Graphic3d_Camera)& theCamera,
                                                     const Standard_Integer          theWindowWidth,
                                                     const Standard_Integer          theWindowHeight,
                                                     const Standard_Boolean          theToIncludeAuxiliary) const Standard_OVERRIDE;

  //! Returns pointer to an assigned framebuffer object.
  Standard_EXPORT virtual Handle(Standard_Transient) FBO() const Standard_OVERRIDE;

  //! Sets framebuffer object for offscreen rendering.
  Standard_EXPORT virtual void SetFBO (const Handle(Standard_Transient)& theFbo) Standard_OVERRIDE;

  //! Generate offscreen FBO in the graphic library.
  //! If not supported on hardware returns NULL.
  Standard_EXPORT virtual Handle(Standard_Transient) FBOCreate (const Standard_Integer theWidth,
                                                                const Standard_Integer theHeight) Standard_OVERRIDE;

  //! Remove offscreen FBO from the graphic library
  Standard_EXPORT virtual void FBORelease (Handle(Standard_Transient)& theFbo) Standard_OVERRIDE;

  //! Read offscreen FBO configuration.
  Standard_EXPORT virtual void FBOGetDimensions (const Handle(Standard_Transient)& theFbo,
                                                 Standard_Integer& theWidth,
                                                 Standard_Integer& theHeight,
                                                 Standard_Integer& theWidthMax,
                                                 Standard_Integer& theHeightMax) Standard_OVERRIDE;

  //! Change offscreen FBO viewport.
  Standard_EXPORT virtual void FBOChangeViewport (const Handle(Standard_Transient)& theFbo,
                                                  const Standard_Integer theWidth,
                                                  const Standard_Integer theHeight) Standard_OVERRIDE;

public:

  //! Returns gradient background fill colors.
  virtual Aspect_GradientBackground GradientBackground() const { return Aspect_GradientBackground(); }

  //! Sets gradient background fill colors.
  virtual void SetGradientBackground (const Aspect_GradientBackground& ) {}

  //! Returns background image texture file path.
  virtual TCollection_AsciiString BackgroundImage() Standard_OVERRIDE { return myBackgroundImagePath; }

  //! Sets background image texture file path.
  virtual void SetBackgroundImage (const TCollection_AsciiString& ) Standard_OVERRIDE {}

  //! Returns background image fill style.
  virtual Aspect_FillMethod BackgroundImageStyle() const Standard_OVERRIDE { return Aspect_FM_NONE; }

  //! Sets background image fill style.
  virtual void SetBackgroundImageStyle (const Aspect_FillMethod ) Standard_OVERRIDE {}

  //! Returns environment texture set for the view.
  virtual Handle(Graphic3d_TextureEnv) TextureEnv() const Standard_OVERRIDE { return myTextureEnvData; }

  //! Sets environment texture for the view.
  Standard_EXPORT virtual void SetTextureEnv (const Handle(Graphic3d_TextureEnv)& theTextureEnv) Standard_OVERRIDE;

  //! Return backfacing model used for the view.
  virtual Graphic3d_TypeOfBackfacingModel BackfacingModel() const Standard_OVERRIDE { return myBackfacing; }

  //! Sets backfacing model for the view.
  virtual void SetBackfacingModel (const Graphic3d_TypeOfBackfacingModel theModel) Standard_OVERRIDE { myBackfacing = theModel; }

  //! Returns local camera origin currently set for rendering, might be modified during rendering.
  const gp_XYZ& LocalOrigin() const { return myLocalOrigin; }

  //! Setup local camera origin currently set for rendering.
  Standard_EXPORT void SetLocalOrigin (const gp_XYZ& theOrigin);

  //! Returns list of lights of the view.
  virtual const Handle(Graphic3d_LightSet)& Lights() const Standard_OVERRIDE { return myLights; }

  //! Sets list of lights for the view.
  virtual void SetLights (const Handle(Graphic3d_LightSet)& theLights) Standard_OVERRIDE
  {
    myLights = theLights;
    ///myCurrLightSourceState = myStateCounter->Increment();
  }

  //! Returns list of clip planes set for the view.
  virtual const Handle(Graphic3d_SequenceOfHClipPlane)& ClipPlanes() const Standard_OVERRIDE { return myClipPlanes; }

  //! Sets list of clip planes for the view.
  virtual void SetClipPlanes (const Handle(Graphic3d_SequenceOfHClipPlane)& thePlanes) Standard_OVERRIDE { myClipPlanes = thePlanes; }

  //! Fill in the dictionary with diagnostic info.
  //! Should be called within rendering thread.
  //!
  //! This API should be used only for user output or for creating automated reports.
  //! The format of returned information (e.g. key-value layout)
  //! is NOT part of this API and can be changed at any time.
  //! Thus application should not parse returned information to weed out specific parameters.
  Standard_EXPORT virtual void DiagnosticInformation (TColStd_IndexedDataMapOfStringString& theDict,
                                                      Graphic3d_DiagnosticInfo theFlags) const Standard_OVERRIDE;

  //! Returns string with statistic performance info.
  Standard_EXPORT virtual TCollection_AsciiString StatisticInformation() const Standard_OVERRIDE;

  //! Fills in the dictionary with statistic performance info.
  Standard_EXPORT virtual void StatisticInformation (TColStd_IndexedDataMapOfStringString& theDict) const Standard_OVERRIDE;

public:

  //! Returns background color.
  /**const Quantity_ColorRGBA& BackgroundColor() const { return myBgColor; }

  void SetBackgroundTextureStyle (const Aspect_FillMethod FillStyle);

  void SetBackgroundGradient (const Quantity_Color& AColor1, const Quantity_Color& AColor2, const Aspect_GradientFillMethod AType);

  void SetBackgroundGradientType (const Aspect_GradientFillMethod AType);

  //! Returns list of OpenGL Z-layers.
  const OpenGl_LayerList& LayerList() const { return myZLayers; }

  //! Returns OpenGL window implementation.
  const Handle(OpenGl_Window)& GlWindow() const { return myWindow; }

  //! Returns OpenGL environment map.
  const Handle(OpenGl_TextureSet)& GlTextureEnv() const { return myTextureEnv; }

  //! Returns selector for BVH tree, providing a possibility to store information
  //! about current view volume and to detect which objects are overlapping it.
  const OpenGl_BVHTreeSelector& BVHTreeSelector() const { return myBVHSelector; }

  //! Returns true if there are immediate structures to display
  bool HasImmediateStructures() const
  {
    return myZLayers.NbImmediateStructures() != 0;
  }*/

protected: //! @name low-level redrawing sub-routines

  //! Redraws view for the given monographic camera projection, or left/right eye.
 /** Standard_EXPORT virtual void redraw (const Graphic3d_Camera::Projection theProjection,
                                       OpenGl_FrameBuffer*                theReadDrawFbo,
                                       OpenGl_FrameBuffer*                theOitAccumFbo);

  //! Redraws view for the given monographic camera projection, or left/right eye.
  //!
  //! Method will blit snapshot containing main scene (myMainSceneFbos or BackBuffer)
  //! into presentation buffer (myMainSceneFbos -> offscreen FBO or
  //! myMainSceneFbos -> BackBuffer or BackBuffer -> FrontBuffer),
  //! and redraw immediate structures on top.
  //!
  //! When scene caching is disabled (myTransientDrawToFront, no double buffer in window, etc.),
  //! the first step (blitting) will be skipped.
  //!
  //! @return false if immediate structures has been rendered directly into FrontBuffer
  //! and Buffer Swap should not be called.
  Standard_EXPORT virtual bool redrawImmediate (const Graphic3d_Camera::Projection theProjection,
                                                OpenGl_FrameBuffer* theReadFbo,
                                                OpenGl_FrameBuffer* theDrawFbo,
                                                OpenGl_FrameBuffer* theOitAccumFbo,
                                                const Standard_Boolean theIsPartialUpdate = Standard_False);

  //! Blit image from/to specified buffers.
  Standard_EXPORT bool blitBuffers (OpenGl_FrameBuffer*    theReadFbo,
                                    OpenGl_FrameBuffer*    theDrawFbo,
                                    const Standard_Boolean theToFlip = Standard_False);

  //! Setup default FBO.
  Standard_EXPORT void bindDefaultFbo (OpenGl_FrameBuffer* theCustomFbo = NULL);*/

protected: //! @name Rendering of GL graphics (with prepared drawing buffer).

  //! Renders the graphical contents of the view into the preprepared window or framebuffer.
  //! @param theProjection [in] the projection that should be used for rendering.
  //! @param theReadDrawFbo [in] the framebuffer for rendering graphics.
  //! @param theOitAccumFbo [in] the framebuffer for accumulating color and coverage for OIT process.
  //! @param theToDrawImmediate [in] the flag indicates whether the rendering performs in immediate mode.
  /**Standard_EXPORT virtual void render (Graphic3d_Camera::Projection theProjection,
                                       OpenGl_FrameBuffer*          theReadDrawFbo,
                                       OpenGl_FrameBuffer*          theOitAccumFbo,
                                       const Standard_Boolean       theToDrawImmediate);

  //! Renders the graphical scene.
  //! @param theProjection [in] the projection that is used for rendering.
  //! @param theReadDrawFbo [in] the framebuffer for rendering graphics.
  //! @param theOitAccumFbo [in] the framebuffer for accumulating color and coverage for OIT process.
  //! @param theToDrawImmediate [in] the flag indicates whether the rendering performs in immediate mode.
  Standard_EXPORT virtual void renderScene (Graphic3d_Camera::Projection theProjection,
                                            OpenGl_FrameBuffer*    theReadDrawFbo,
                                            OpenGl_FrameBuffer*    theOitAccumFbo,
                                            const Standard_Boolean theToDrawImmediate);

  //! Draw background (gradient / image)
  Standard_EXPORT virtual void drawBackground (const Handle(OpenGl_Workspace)& theWorkspace);

  //! Render set of structures presented in the view.
  //! @param theProjection [in] the projection that is used for rendering.
  //! @param theReadDrawFbo [in] the framebuffer for rendering graphics.
  //! @param theOitAccumFbo [in] the framebuffer for accumulating color and coverage for OIT process.
  //! @param theToDrawImmediate [in] the flag indicates whether the rendering performs in immediate mode.
  Standard_EXPORT virtual void renderStructs (Graphic3d_Camera::Projection theProjection,
                                              OpenGl_FrameBuffer*    theReadDrawFbo,
                                              OpenGl_FrameBuffer*    theOitAccumFbo,
                                              const Standard_Boolean theToDrawImmediate);*/

private:

  //! Initialize swapchain.
  Standard_EXPORT bool initSwapChain (const Handle(Aspect_Window)& theWindow,
                                      const Aspect_RenderingContext theContext);

  //! Adds the structure to display lists of the view.
  Standard_EXPORT virtual void displayStructure (const Handle(Graphic3d_CStructure)& theStructure,
                                                 const Standard_Integer thePriority) Standard_OVERRIDE;

  //! Erases the structure from display lists of the view.
  Standard_EXPORT virtual void eraseStructure (const Handle(Graphic3d_CStructure)& theStructure) Standard_OVERRIDE;

  //! Change Z layer of a structure already presented in view.
  Standard_EXPORT virtual void changeZLayer (const Handle(Graphic3d_CStructure)& theCStructure,
                                             const Graphic3d_ZLayerId theNewLayerId) Standard_OVERRIDE;

  //! Changes the priority of a structure within its Z layer in the specified view.
  Standard_EXPORT virtual void changePriority (const Handle(Graphic3d_CStructure)& theCStructure,
                                               const Standard_Integer theNewPriority) Standard_OVERRIDE;

  //! Returns zoom-scale factor.
  Standard_EXPORT virtual Standard_Real considerZoomPersistenceObjects (const Graphic3d_ZLayerId        theLayerId,
                                                                        const Handle(Graphic3d_Camera)& theCamera,
                                                                        const Standard_Integer          theWindowWidth,
                                                                        const Standard_Integer          theWindowHeight) const Standard_OVERRIDE;

private:

  //! Copy content of Back buffer to the Front buffer.
  /**bool copyBackToFront();

  //! Initialize blit quad.
  OpenGl_VertexBuffer* initBlitQuad (const Standard_Boolean theToFlip);

  //! Blend together views pair into stereo image.
  void drawStereoPair (OpenGl_FrameBuffer* theDrawFbo);

  //! Check and update OIT compatibility with current OpenGL context's state.
  bool checkOitCompatibility (const Handle(OpenGl_Context)& theGlContext,
                              const Standard_Boolean theMSAA);

  //! Chooses compatible internal color format for OIT frame buffer.
  bool chooseOitColorConfiguration (const Handle(OpenGl_Context)& theGlContext,
                                    const Standard_Integer theConfigIndex,
                                    OpenGl_ColorFormats& theFormats);*/

protected:

  Graphic3d_Layer myLayer; /// TODO
  Standard_Integer       mySwapInterval;   //!< last assigned swap interval (VSync) for this window

  Vulkan_GraphicDriver*    myDriver;
  Handle(Vulkan_Context)   myContext;
  Handle(Vulkan_Surface)   mySurface;
  Handle(Aspect_Window)    myPlatformWindow; //!< software platform window wrapper
  Standard_Boolean         myWasRedrawnGL;

  Graphic3d_TypeOfBackfacingModel myBackfacing;
  Handle(Graphic3d_SequenceOfHClipPlane) myClipPlanes;
  gp_XYZ                          myLocalOrigin;
  //Handle(OpenGl_FrameBuffer)      myFBO;
  TCollection_AsciiString         myBackgroundImagePath;
  Handle(Graphic3d_TextureEnv)    myTextureEnvData;

  Handle(Graphic3d_LightSet)      myNoShadingLight;
  Handle(Graphic3d_LightSet)      myLights;
  ///OpenGl_LayerList                myZLayers; //!< main list of displayed structure, sorted by layers

  Graphic3d_WorldViewProjState    myWorldViewProjState; //!< camera modification state
  ///OpenGl_StateCounter*            myStateCounter;
  ///Standard_Size                   myCurrLightSourceState;
  ///Standard_Size                   myLightsRevision;

  //typedef std::pair<Standard_Size, Standard_Size> StateInfo;
  //StateInfo myLastOrientationState;
  //StateInfo myLastViewMappingState;
  //StateInfo myLastLightSourceState;

  //! Is needed for selection of overlapping objects and storage of the current view volume
  ///OpenGl_BVHTreeSelector myBVHSelector;

  ///OpenGl_FrameStatsPrs      myFrameStatsPrs;

  ///Handle(OpenGl_TextureSet) myTextureEnv;

  //! Framebuffers for OpenGL output.
  ///Handle(OpenGl_FrameBuffer) myOpenGlFBO;
  ///Handle(OpenGl_FrameBuffer) myOpenGlFBO2;

protected: //! @name Rendering properties

  //! Two framebuffers (left and right views) store cached main presentation
  //! of the view (without presentation of immediate layers).
  int                        myFboColorFormat;        //!< sized format for color attachments
  int                        myFboDepthFormat;        //!< sized format for depth-stencil attachments
  //OpenGl_ColorFormats        myFboOitColorConfig;     //!< selected color format configuration for OIT color attachments
  //Handle(OpenGl_FrameBuffer) myMainSceneFbos[2];
  //Handle(OpenGl_FrameBuffer) myMainSceneFbosOit[2];      //!< Additional buffers for transparent draw of main layer.
  //Handle(OpenGl_FrameBuffer) myImmediateSceneFbos[2];    //!< Additional buffers for immediate layer in stereo mode.
  //Handle(OpenGl_FrameBuffer) myImmediateSceneFbosOit[2]; //!< Additional buffers for transparency draw of immediate layer.
  ///OpenGl_VertexBuffer        myFullScreenQuad;        //!< Vertices for full-screen quad rendering.
  ///OpenGl_VertexBuffer        myFullScreenQuadFlip;
  ///Standard_Boolean           myToFlipOutput;          //!< Flag to draw result image upside-down
  unsigned int               myFrameCounter;          //!< redraw counter, for debugging
  Standard_Boolean           myHasFboBlit;            //!< disable FBOs on failure
  Standard_Boolean           myToDisableOIT;          //!< disable OIT on failure
  Standard_Boolean           myToDisableOITMSAA;      //!< disable OIT with MSAA on failure
  Standard_Boolean           myToDisableMSAA;         //!< disable MSAA after failure
  Standard_Boolean           myTransientDrawToFront; //!< optimization flag for immediate mode (to render directly to the front buffer)
  Standard_Boolean           myBackBufferRestored;
  Standard_Boolean           myIsImmediateDrawn;     //!< flag indicates that immediate mode buffer contains some data

protected: //! @name Background parameters

  ///OpenGl_Aspects*         myTextureParams;   //!< Stores texture and its parameters for textured background
  ///OpenGl_BackgroundArray* myBgGradientArray; //!< Primitive array for gradient background
  ///OpenGl_BackgroundArray* myBgTextureArray;  //!< Primitive array for texture  background

};

#endif // _Vulkan_View_HeaderFile

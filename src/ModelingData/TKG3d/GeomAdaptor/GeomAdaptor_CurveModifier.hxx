// Copyright (c) 2024 OPEN CASCADE SAS
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

#ifndef GeomAdaptor_CurveModifier_HeaderFile
#define GeomAdaptor_CurveModifier_HeaderFile

#include <GeomAdaptor_TrsfModifier.hxx>
#include <GeomAdaptor_CurveOnSurfaceModifier.hxx>
#include <GeomAdaptor_IsoCurveModifier.hxx>

#include <variant>

//! Variant type holding all possible curve modifiers.
//!
//! This type uses std::variant to store one of the available curve modifiers
//! in a type-safe manner. The modifiers transform curve evaluation results:
//!
//! - std::monostate: No modifier (identity transformation)
//! - GeomAdaptor_TrsfModifier: Applies gp_Trsf transformation to evaluation results
//! - GeomAdaptor_CurveOnSurfaceModifier: Evaluates 2D curve on 3D surface
//! - GeomAdaptor_IsoCurveModifier: Evaluates isoparametric curve on surface
//!
//! Usage in GeomAdaptor_Curve:
//! @code
//! GeomAdaptor_CurveModifierVariant myModifier;
//!
//! // Check for specific modifier type
//! if (auto* pTrsf = std::get_if<GeomAdaptor_TrsfModifier>(&myModifier)) {
//!   pTrsf->Transform(aPoint);
//! }
//!
//! // Use std::visit for polymorphic dispatch
//! std::visit([&](auto& mod) {
//!   if constexpr (!std::is_same_v<std::decay_t<decltype(mod)>, std::monostate>) {
//!     mod.D0(theU, theP);
//!   }
//! }, myModifier);
//! @endcode
//!
//! Memory semantics:
//! - std::monostate and GeomAdaptor_TrsfModifier are lightweight and copyable
//! - GeomAdaptor_CurveOnSurfaceModifier and GeomAdaptor_IsoCurveModifier are move-only
//! - The entire variant is therefore move-only when containing a move-only type
using GeomAdaptor_CurveModifierVariant = std::variant<std::monostate,
                                                       GeomAdaptor_TrsfModifier,
                                                       GeomAdaptor_CurveOnSurfaceModifier,
                                                       GeomAdaptor_IsoCurveModifier>;

//! Helper to check if modifier is empty (std::monostate).
//! @param theModifier the modifier variant to check
//! @return true if modifier is std::monostate (no modification)
inline bool IsEmptyModifier(const GeomAdaptor_CurveModifierVariant& theModifier)
{
  return std::holds_alternative<std::monostate>(theModifier);
}

//! Helper to check if modifier is a transformation only.
//! @param theModifier the modifier variant to check
//! @return true if modifier is GeomAdaptor_TrsfModifier
inline bool IsTrsfModifier(const GeomAdaptor_CurveModifierVariant& theModifier)
{
  return std::holds_alternative<GeomAdaptor_TrsfModifier>(theModifier);
}

//! Helper to check if modifier is a curve-on-surface.
//! @param theModifier the modifier variant to check
//! @return true if modifier is GeomAdaptor_CurveOnSurfaceModifier
inline bool IsCurveOnSurfaceModifier(const GeomAdaptor_CurveModifierVariant& theModifier)
{
  return std::holds_alternative<GeomAdaptor_CurveOnSurfaceModifier>(theModifier);
}

//! Helper to check if modifier is an iso curve.
//! @param theModifier the modifier variant to check
//! @return true if modifier is GeomAdaptor_IsoCurveModifier
inline bool IsIsoCurveModifier(const GeomAdaptor_CurveModifierVariant& theModifier)
{
  return std::holds_alternative<GeomAdaptor_IsoCurveModifier>(theModifier);
}

#endif // GeomAdaptor_CurveModifier_HeaderFile

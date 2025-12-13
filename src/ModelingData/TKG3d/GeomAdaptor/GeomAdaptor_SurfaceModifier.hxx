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

#ifndef GeomAdaptor_SurfaceModifier_HeaderFile
#define GeomAdaptor_SurfaceModifier_HeaderFile

#include <GeomAdaptor_TrsfModifier.hxx>

#include <variant>

//! Variant type holding all possible surface modifiers.
//!
//! This type uses std::variant to store one of the available surface modifiers
//! in a type-safe manner. The modifiers transform surface evaluation results:
//!
//! - std::monostate: No modifier (identity transformation)
//! - GeomAdaptor_TrsfModifier: Applies gp_Trsf transformation to evaluation results
//!
//! Usage in GeomAdaptor_Surface:
//! @code
//! GeomAdaptor_SurfaceModifierVariant myModifier;
//!
//! // Check for transformation modifier
//! if (auto* pTrsf = std::get_if<GeomAdaptor_TrsfModifier>(&myModifier)) {
//!   pTrsf->Transform(aPoint);
//!   pTrsf->Transform(aNormal);
//! }
//!
//! // Use std::visit for polymorphic dispatch
//! std::visit([&](auto& mod) {
//!   if constexpr (std::is_same_v<std::decay_t<decltype(mod)>, GeomAdaptor_TrsfModifier>) {
//!     mod.TransformD0(theP);
//!   }
//! }, myModifier);
//! @endcode
//!
//! Memory semantics:
//! - Both std::monostate and GeomAdaptor_TrsfModifier are lightweight and copyable
//! - The entire variant is therefore copyable and movable
using GeomAdaptor_SurfaceModifierVariant = std::variant<std::monostate, GeomAdaptor_TrsfModifier>;

//! Helper to check if modifier is empty (std::monostate).
//! @param theModifier the modifier variant to check
//! @return true if modifier is std::monostate (no modification)
inline bool IsEmptyModifier(const GeomAdaptor_SurfaceModifierVariant& theModifier)
{
  return std::holds_alternative<std::monostate>(theModifier);
}

//! Helper to check if modifier is a transformation.
//! @param theModifier the modifier variant to check
//! @return true if modifier is GeomAdaptor_TrsfModifier
inline bool IsTrsfModifier(const GeomAdaptor_SurfaceModifierVariant& theModifier)
{
  return std::holds_alternative<GeomAdaptor_TrsfModifier>(theModifier);
}

#endif // GeomAdaptor_SurfaceModifier_HeaderFile

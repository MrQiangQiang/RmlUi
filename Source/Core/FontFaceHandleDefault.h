/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019 The RmlUi Team, and contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#ifndef RMLUICOREFONTFACEHANDLE_H
#define RMLUICOREFONTFACEHANDLE_H

#include "../../Include/RmlUi/Core/Traits.h"
#include "../../Include/RmlUi/Core/FontEffect.h"
#include "../../Include/RmlUi/Core/FontGlyph.h"
#include "../../Include/RmlUi/Core/Geometry.h"
#include "../../Include/RmlUi/Core/Texture.h"

namespace Rml {
namespace Core {

#ifndef RMLUI_NO_FONT_INTERFACE_DEFAULT

class FontFaceLayer;

/**
	@author Peter Curry
 */

struct FontMetrics {
	// The average advance (in pixels) of all of this face's glyphs.
	int average_advance;

	int size;
	int x_height;
	int line_height;
	int baseline;

	float underline_position;
	float underline_thickness;
};

class FontFaceHandleDefault : public NonCopyMoveable
{
public:
	FontFaceHandleDefault();
	virtual ~FontFaceHandleDefault();

	/// Returns the average advance of all glyphs in this font face.
	/// @return An approximate width of the characters in this font face.
	int GetCharacterWidth() const;

	/// Returns the point size of this font face.
	/// @return The face's point size.
	int GetSize() const;
	/// Returns the pixel height of a lower-case x in this font face.
	/// @return The height of a lower-case x.
	int GetXHeight() const;
	/// Returns the default height between this font face's baselines.
	/// @return The default line height.
	int GetLineHeight() const;

	/// Returns the font's baseline, as a pixel offset from the bottom of the font.
	/// @return The font's baseline.
	int GetBaseline() const;

	/// Returns the font's underline, as a pixel offset from the bottom of the font.
	/// @return The font's underline thickness.
	float GetUnderline(float* thickness) const;

	/// Returns the font's glyphs.
	/// @return The font's glyphs.
	const FontGlyphMap& GetGlyphs() const;

	/// Returns the width a string will take up if rendered with this handle.
	/// @param[in] string The string to measure.
	/// @param[in] prior_character The optionally-specified character that immediately precedes the string. This may have an impact on the string width due to kerning.
	/// @return The width, in pixels, this string will occupy if rendered with this handle.
	int GetStringWidth(const String& string, CodePoint prior_character = CodePoint::Null);

	/// Generates, if required, the layer configuration for a given array of font effects.
	/// @param[in] font_effects The list of font effects to generate the configuration for.
	/// @return The index to use when generating geometry using this configuration.
	int GenerateLayerConfiguration(const FontEffectList& font_effects);
	/// Generates the texture data for a layer (for the texture database).
	/// @param[out] texture_data The pointer to be set to the generated texture data.
	/// @param[out] texture_dimensions The dimensions of the texture.
	/// @param[in] layer_id The id of the layer to request the texture data from.
	/// @param[in] texture_id The index of the texture within the layer to generate.
	/// @param[in] handle_version The version of the handle data. Function returns false if out of date.
	bool GenerateLayerTexture(UniquePtr<const byte[]>& texture_data, Vector2i& texture_dimensions, FontEffect* layer_id, int texture_id, int handle_version);

	/// Generates the geometry required to render a single line of text.
	/// @param[out] geometry An array of geometries to generate the geometry into.
	/// @param[in] string The string to render.
	/// @param[in] position The position of the baseline of the first character to render.
	/// @param[in] colour The colour to render the text.
	/// @return The width, in pixels, of the string geometry.
	int GenerateString(GeometryList& geometry, const String& string, const Vector2f& position, const Colourb& colour, int layer_configuration = 0);


	int GetVersion() const;

protected:

	FontGlyphMap& GetGlyphs();
	FontMetrics& GetMetrics();

	void GenerateBaseLayer();

	// Build and append glyph to 'glyphs'
	virtual bool AppendGlyph(CodePoint code_point) = 0;

	virtual int GetKerning(CodePoint lhs, CodePoint rhs) const = 0;

private:

	bool UpdateLayersOnDirty();

	// Note: can modify code_point to change character to e.g. replacement character.
	const FontGlyph* GetOrAppendGlyph(CodePoint& code_point, bool look_in_fallback_fonts = true);

	// Create a new layer from the given font effect if it does not already exist.
	FontFaceLayer* GetOrCreateLayer(const SharedPtr<const FontEffect>& font_effect);

	// (Re-)generate a layer in this font face handle.
	bool GenerateLayer(FontFaceLayer* layer);

	FontGlyphMap glyphs;

	using FontLayerMap = SmallUnorderedMap< const FontEffect*, UniquePtr<FontFaceLayer> >;
	using FontLayerCache = SmallUnorderedMap< size_t, FontFaceLayer* >;
	using LayerConfiguration = std::vector< FontFaceLayer* >;
	using LayerConfigurationList = std::vector< LayerConfiguration >;

	// The list of all font layers, index by the effect that instanced them.
	FontFaceLayer* base_layer;
	FontLayerMap layers;
	// Each font layer that generated geometry or textures, indexed by the respective generation key.
	FontLayerCache layer_cache;

	int version = 0;
	bool is_layers_dirty = false;

	// All configurations currently in use on this handle. New configurations will be generated as required.
	LayerConfigurationList layer_configurations;

	FontMetrics metrics;
};

#endif

}
}

#endif

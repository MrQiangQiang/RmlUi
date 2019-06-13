/*
 * This source file is part of libRocket, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://www.librocket.com
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
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

#include "precompiled.h"
#include "../../Include/Rocket/Core/PropertySpecification.h"
#include "PropertyShorthandDefinition.h"
#include "../../Include/Rocket/Core/Log.h"
#include "../../Include/Rocket/Core/PropertyDefinition.h"
#include "../../Include/Rocket/Core/PropertyDictionary.h"
#include "../../Include/Rocket/Core/StyleSheetSpecification.h"

namespace Rocket {
namespace Core {

PropertySpecification::PropertySpecification(size_t reserve_num_properties, size_t reserve_num_shorthands) 
	: properties(reserve_num_properties, nullptr), shorthands(reserve_num_shorthands, nullptr), property_map(reserve_num_properties), shorthand_map(reserve_num_shorthands)
{
}

PropertySpecification::~PropertySpecification()
{
	for (PropertyDefinition* p : properties)
		delete p;
	for (ShorthandDefinition* s : shorthands)
		delete s;
}

// Registers a property with a new definition.
PropertyDefinition& PropertySpecification::RegisterProperty(const String& property_name, const String& default_value, bool inherited, bool forces_layout, PropertyId id)
{
	if (id == PropertyId::Invalid)
		id = property_map.GetOrCreateId(property_name);
	else
		property_map.AddPair(id, property_name);

	size_t index = (size_t)id;
	ROCKET_ASSERT(index < (size_t)INT16_MAX);

	if (index < properties.size())
	{
		// We don't want to owerwrite an existing entry.
		if (properties[index])
		{
			Log::Message(Log::LT_ERROR, "While registering property '%s': The property is already registered, ignoring.", StyleSheetSpecification::GetPropertyName(id).c_str());
			return *properties[index];
		}
	}
	else
	{
		// Resize vector to hold the new index
		properties.resize((index*3)/2 + 1, nullptr);
	}

	// Create and insert the new property
	PropertyDefinition* property_definition = new PropertyDefinition(id, default_value, inherited, forces_layout);

	properties[index] = property_definition;

	return *property_definition;
}

// Returns a property definition.
const PropertyDefinition* PropertySpecification::GetProperty(PropertyId id) const
{
	if (id == PropertyId::Invalid || (size_t)id >= properties.size())
		return nullptr;

	return properties[(size_t)id];
}

const PropertyDefinition* PropertySpecification::GetProperty(const String& property_name) const
{
	return GetProperty(property_map.GetId(property_name));
}

// Fetches a list of the names of all registered property definitions.
const PropertyNameList& PropertySpecification::GetRegisteredProperties(void) const
{
	return property_names;
}

// Fetches a list of the names of all registered property definitions.
const PropertyNameList& PropertySpecification::GetRegisteredInheritedProperties(void) const
{
	return inherited_property_names;
}

// Registers a shorthand property definition.
bool PropertySpecification::RegisterShorthand(const String& shorthand_name, const String& property_names, ShorthandType type, ShorthandId id)
{
	if (id == ShorthandId::Invalid)
		id = shorthand_map.GetOrCreateId(shorthand_name);
	else
		shorthand_map.AddPair(id, shorthand_name);

	StringList property_list;
	StringUtilities::ExpandString(property_list, ToLower(property_names));
	ShorthandItemIdList id_list;
	id_list.reserve(property_list.size());

	for (auto& name : property_list)
	{
		PropertyId property_id = StyleSheetSpecification::GetPropertyId(name);
		if (property_id != PropertyId::Invalid)
			id_list.emplace_back(property_id);
		else
		{
			ShorthandId shorthand_id = StyleSheetSpecification::GetShorthandId(name);
			if (shorthand_id != ShorthandId::Invalid)
				id_list.emplace_back(shorthand_id);
		}
	}

	if (id_list.empty() || id_list.size() != property_list.size())
		return false;

	if (id_list.empty())
		return false;

	// Construct the new shorthand definition and resolve its properties.
	ShorthandDefinition* property_shorthand = new ShorthandDefinition();
	for (size_t i = 0; i < id_list.size(); i++)
	{
		ShorthandItem item;
		if (id_list[i].type == ShorthandItemType::Property)
		{
			const PropertyDefinition* property = GetProperty(id_list[i].property_id);
			if(property)
				item = ShorthandItem(id_list[i].property_id, property);
		}
		else if (id_list[i].type == ShorthandItemType::Shorthand && type == ShorthandType::Recursive)
		{
			// The recursive type (and only that) can hold other shorthands
			const ShorthandDefinition* shorthand = GetShorthand(id_list[i].shorthand_id);
			if (shorthand)
				item = ShorthandItem(id_list[i].shorthand_id, shorthand);
		}

		if (item.type == ShorthandItemType::Invalid)
		{
			Log::Message(Log::LT_ERROR, "Shorthand property '%s' was registered with invalid property '%s'.", StyleSheetSpecification::GetShorthandName(id).c_str(), property_list[i].c_str());
			delete property_shorthand;

			return false;
		}
		property_shorthand->items.push_back(item);
	}

	property_shorthand->type = type;

	const size_t index = (size_t)id;
	ROCKET_ASSERT(index < (size_t)INT16_MAX);

	if (index < shorthands.size())
	{
		// We don't want to owerwrite an existing entry.
		if (shorthands[index])
		{
			Log::Message(Log::LT_ERROR, "While registering shorthand '%s': The shorthand is already registered, ignoring.", StyleSheetSpecification::GetShorthandName(id).c_str());
			return false;
		}
	}
	else
	{
		// Resize vector to hold the new index
		shorthands.resize((index * 3) / 2 + 1, nullptr);
	}

	shorthands[index] = property_shorthand;
	return true;
}

// Returns a shorthand definition.
const ShorthandDefinition* PropertySpecification::GetShorthand(ShorthandId id) const
{
	if (id == ShorthandId::Invalid || (size_t)id >= shorthands.size())
		return nullptr;

	return shorthands[(size_t)id];
}

const ShorthandDefinition* PropertySpecification::GetShorthand(const String& shorthand_name) const
{
	return GetShorthand(shorthand_map.GetId(shorthand_name));
}

bool PropertySpecification::ParsePropertyDeclaration(PropertyDictionary& dictionary, const String& property_name, const String& property_value, const String& source_file, int source_line_number) const
{
	// Try as a property first
	PropertyId property_id = property_map.GetId(property_name);
	if (property_id != PropertyId::Invalid)
		return ParsePropertyDeclaration(dictionary, property_id, property_value, source_file, source_line_number);

	// Then, as a shorthand
	ShorthandId shorthand_id = shorthand_map.GetId(property_name);
	if (shorthand_id != ShorthandId::Invalid)
		return ParseShorthandDeclaration(dictionary, shorthand_id, property_value, source_file, source_line_number);

	return false;
}

bool PropertySpecification::ParsePropertyDeclaration(PropertyDictionary& dictionary, PropertyId property_id, const String& property_value, const String& source_file, int source_line_number) const
{
	// Parse as a single property.
	const PropertyDefinition* property_definition = GetProperty(property_id);
	if (!property_definition)
		return false;

	StringList property_values;
	if (!ParsePropertyValues(property_values, property_value, false) || property_values.size() == 0)
		return false;

	Property new_property;
	new_property.source = source_file;
	new_property.source_line_number = source_line_number;
	if (!property_definition->ParseValue(new_property, property_values[0]))
		return false;
	
	dictionary.SetProperty(property_id, new_property);
	return true;
}

// Parses a property declaration, setting any parsed and validated properties on the given dictionary.
bool PropertySpecification::ParseShorthandDeclaration(PropertyDictionary& dictionary, ShorthandId shorthand_id, const String& property_value, const String& source_file, int source_line_number) const
{
	StringList property_values;
	if (!ParsePropertyValues(property_values, property_value, true) || property_values.size() == 0)
		return false;

	// Parse as a shorthand.
	const ShorthandDefinition* shorthand_definition = GetShorthand(shorthand_id);
	if (!shorthand_definition)
		return false;

	// If this definition is a 'box'-style shorthand (x-top, x-right, x-bottom, x-left, etc) and there are fewer
	// than four values
	if (shorthand_definition->type == ShorthandType::Box &&
		property_values.size() < 4)
	{
		// This array tells which property index each side is parsed from
		std::array<int, 4> box_side_to_value_index = { 0,0,0,0 };
		switch (property_values.size())
		{
		case 1:
			// Only one value is defined, so it is parsed onto all four sides.
			box_side_to_value_index = { 0,0,0,0 };
			break;
		case 2:
			// Two values are defined, so the first one is parsed onto the top and bottom value, the second onto
			// the left and right.
			box_side_to_value_index = { 0,1,0,1 };
			break;
		case 3:
			// Three values are defined, so the first is parsed into the top value, the second onto the left and
			// right, and the third onto the bottom.
			box_side_to_value_index = { 0,1,2,1 };
			break;
		default:
			ROCKET_ERROR;
			break;
		}

		for (int i = 0; i < 4; i++)
		{
			ROCKET_ASSERT(shorthand_definition->items[i].type == ShorthandItemType::Property);
			Property new_property;
			int value_index = box_side_to_value_index[i];
			if (!shorthand_definition->items[i].property_definition->ParseValue(new_property, property_values[value_index]))
				return false;

			new_property.source = source_file;
			new_property.source_line_number = source_line_number;
			dictionary.SetProperty(shorthand_definition->items[i].property_definition->GetId(), new_property);
		}
	}
	else if (shorthand_definition->type == ShorthandType::Recursive)
	{
		bool result = true;

		for (size_t i = 0; i < shorthand_definition->items.size(); i++)
		{
			const ShorthandItem& item = shorthand_definition->items[i];
			if (item.type == ShorthandItemType::Property)
				result &= ParsePropertyDeclaration(dictionary, item.property_id, property_value, source_file, source_line_number);
			else if (item.type == ShorthandItemType::Shorthand)
				result &= ParseShorthandDeclaration(dictionary, item.shorthand_id, property_value, source_file, source_line_number);
			else
				result = false;
		}

		if (!result)
			return false;
	}
	else
	{
		size_t value_index = 0;
		size_t property_index = 0;

		for (; value_index < property_values.size() && property_index < shorthand_definition->items.size(); property_index++)
		{
			Property new_property;
			new_property.source = source_file;
			new_property.source_line_number = source_line_number;

			if (!shorthand_definition->items[property_index].property_definition->ParseValue(new_property, property_values[value_index]))
			{
				// This definition failed to parse; if we're falling through, try the next property. If there is no
				// next property, then abort!
				if (shorthand_definition->type == ShorthandType::FallThrough)
				{
					if (property_index + 1 < shorthand_definition->items.size())
						continue;
				}
				return false;
			}

			dictionary.SetProperty(shorthand_definition->items[property_index].property_id, new_property);

			// Increment the value index, unless we're replicating the last value and we're up to the last value.
			if (shorthand_definition->type != ShorthandType::Replicate ||
				value_index < property_values.size() - 1)
				value_index++;
		}
	}

	return true;
}

// Sets all undefined properties in the dictionary to their defaults.
void PropertySpecification::SetPropertyDefaults(PropertyDictionary& dictionary) const
{
	for (PropertyDefinition* property : properties)
	{
		if (property && dictionary.GetProperty(property->GetId()) == NULL)
			dictionary.SetProperty(property->GetId(), *property->GetDefaultValue());
	}
}


bool PropertySpecification::ParsePropertyValues(StringList& values_list, const String& values, bool split_values) const
{
	String value;

	enum ParseState { VALUE, VALUE_PARENTHESIS, VALUE_QUOTE };
	ParseState state = VALUE;
	int open_parentheses = 0;

	size_t character_index = 0;
	char previous_character = 0;
	while (character_index < values.size())
	{
		char character = values[character_index];
		character_index++;

		switch (state)
		{
			case VALUE:
			{
				if (character == ';')
				{
					value = StringUtilities::StripWhitespace(value);
					if (value.size() > 0)
					{
						values_list.push_back(value);
						value.clear();
					}
				}
				else if (StringUtilities::IsWhitespace(character))
				{
					if (split_values)
					{
						value = StringUtilities::StripWhitespace(value);
						if (value.size() > 0)
						{
							values_list.push_back(value);
							value.clear();
						}
					}
					else
						value += character;
				}
				else if (character == '"')
				{
					if (split_values)
					{
						value = StringUtilities::StripWhitespace(value);
						if (value.size() > 0)
						{
							values_list.push_back(value);
							value.clear();
						}
						state = VALUE_QUOTE;
					}
					else
					{
						value += ' ';
						state = VALUE_QUOTE;
					}
				}
				else if (character == '(')
				{
					open_parentheses = 1;
					value += character;
					state = VALUE_PARENTHESIS;
				}
				else
				{
					value += character;
				}
			}
			break;

			case VALUE_PARENTHESIS:
			{
				if (previous_character == '/')
				{
					if (character == ')' || character == '(')
						value += character;
					else
					{
						value += '/';
						value += character;
					}
				}
				else
				{
					if (character == '(')
					{
						open_parentheses++;
						value += character;
					}
					else if (character == ')')
					{
						open_parentheses--;
						value += character;
						if (open_parentheses == 0)
							state = VALUE;
					}
					else if (character != '/')
					{
						value += character;
					}
				}
			}
			break;

			case VALUE_QUOTE:
			{
				if (previous_character == '/')
				{
					if (character == '"')
						value += character;
					else
					{
						value += '/';
						value += character;
					}
				}
				else
				{
					if (character == '"')
					{
						if (split_values)
						{
							value = StringUtilities::StripWhitespace(value);
							if (value.size() > 0)
							{
								values_list.push_back(value);
								value.clear();
							}
						}
						else
							value += ' ';
						state = VALUE;
					}
					else if (character != '/')
					{
						value += character;
					}
				}
			}
		}

		previous_character = character;
	}

	if (state == VALUE)
	{
		value = StringUtilities::StripWhitespace(value);
		if (value.size() > 0)
			values_list.push_back(value);
	}

	return true;
}

}
}

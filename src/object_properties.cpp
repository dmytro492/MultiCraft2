/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 3.0 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "object_properties.h"
#include "irrlichttypes_bloated.h"
#include "exceptions.h"
#include "util/serialize.h"
#include "util/basic_macros.h"
#include <sstream>

static const video::SColor NULL_BGCOLOR{0, 1, 1, 1};

ObjectProperties::ObjectProperties()
{
	textures.emplace_back("unknown_object.png");
	colors.emplace_back(255,255,255,255);
}

std::string ObjectProperties::dump()
{
	std::ostringstream os(std::ios::binary);
	os << "hp_max=" << hp_max;
	os << ", breath_max=" << breath_max;
	os << ", physical=" << physical;
	os << ", collideWithObjects=" << collideWithObjects;
	os << ", collisionbox=" << PP(collisionbox.MinEdge) << "," << PP(collisionbox.MaxEdge);
	os << ", visual=" << visual;
	os << ", mesh=" << mesh;
	os << ", visual_size=" << PP(visual_size);
	os << ", textures=[";
	for (const std::string &texture : textures) {
		os << "\"" << texture << "\" ";
	}
	os << "]";
	os << ", colors=[";
	for (const video::SColor &color : colors) {
		os << "\"" << color.getAlpha() << "," << color.getRed() << ","
			<< color.getGreen() << "," << color.getBlue() << "\" ";
	}
	os << "]";
	os << ", spritediv=" << PP2(spritediv);
	os << ", initial_sprite_basepos=" << PP2(initial_sprite_basepos);
	os << ", is_visible=" << is_visible;
	os << ", makes_footstep_sound=" << makes_footstep_sound;
	os << ", automatic_rotate="<< automatic_rotate;
	os << ", backface_culling="<< backface_culling;
	os << ", glow=" << glow;
	os << ", nametag=" << nametag;
	os << ", nametag_color=" << "\"" << nametag_color.getAlpha() << "," << nametag_color.getRed()
			<< "," << nametag_color.getGreen() << "," << nametag_color.getBlue() << "\" ";

	if (nametag_bgcolor)
		os << ", nametag_bgcolor=" << "\"" << nametag_color.getAlpha() << "," << nametag_color.getRed()
		   << "," << nametag_color.getGreen() << "," << nametag_color.getBlue() << "\" ";
	else
		os << ", nametag_bgcolor=null ";

	os << ", selectionbox=" << PP(selectionbox.MinEdge) << "," << PP(selectionbox.MaxEdge);
	os << ", pointable=" << pointable;
	os << ", static_save=" << static_save;
	os << ", eye_height=" << eye_height;
	os << ", zoom_fov=" << zoom_fov;
	os << ", use_texture_alpha=" << use_texture_alpha;
	os << ", damage_texture_modifier=" << damage_texture_modifier;
	os << ", shaded=" << shaded;
	os << ", show_on_minimap=" << show_on_minimap;
	return os.str();
}

bool ObjectProperties::validate()
{
	const char *func = "ObjectProperties::validate(): ";
	bool ret = true;

	// cf. where serializeString16 is used below
	for (u32 i = 0; i < textures.size(); i++) {
		if (textures[i].size() > U16_MAX) {
			warningstream << func << "texture " << (i+1) << " has excessive length, "
				"clearing it." << std::endl;
			textures[i].clear();
			ret = false;
		}
	}
	if (nametag.length() > U16_MAX) {
		warningstream << func << "nametag has excessive length, clearing it." << std::endl;
		nametag.clear();
		ret = false;
	}
	if (infotext.length() > U16_MAX) {
		warningstream << func << "infotext has excessive length, clearing it." << std::endl;
		infotext.clear();
		ret = false;
	}
	if (wield_item.length() > U16_MAX) {
		warningstream << func << "wield_item has excessive length, clearing it." << std::endl;
		wield_item.clear();
		ret = false;
	}

	return ret;
}

void ObjectProperties::serialize(std::ostream &os, u16 protocol_version) const
{
	if (protocol_version > 36)
		writeU8(os, 4); // PROTOCOL_VERSION >= 37
	else
		writeU8(os, 1);
	writeU16(os, hp_max);
	writeU8(os, physical);
	writeF(os, 0.f, protocol_version); // Removed property (weight)
	if (protocol_version > 36) {
		writeV3F32(os, collisionbox.MinEdge);
		writeV3F32(os, collisionbox.MaxEdge);
		writeV3F32(os, selectionbox.MinEdge);
		writeV3F32(os, selectionbox.MaxEdge);
		writeU8(os, pointable);
	} else if (pointable) {
		writeV3F1000(os, selectionbox.MinEdge);
		writeV3F1000(os, selectionbox.MaxEdge);
	} else {
		// A hack to emulate unpointable objects
		for (u8 i = 0; i < 6; i++)
			writeF1000(os, 0);
	}

	// The "wielditem" type isn't exactly the same as "item", however this
	// is the most similar compatible option
	if (visual == "item" && protocol_version < 37)
		os << serializeString16("wielditem");
	else
		os << serializeString16(visual);

	if (protocol_version > 36) {
		writeV3F32(os, visual_size);
	} else {
		writeF1000(os, visual_size.X);
		writeF1000(os, visual_size.Y);
	}

	// MT 0.4.15 and below don't have the wield_item property and expect
	// wield_item to be in textures[0].
	if (protocol_version < 37 && (visual == "item" || visual == "wielditem") &&
			!wield_item.empty()) {
		writeU16(os, 1);
		os << serializeString16(wield_item);
	} else {
		writeU16(os, textures.size());
		for (const std::string &texture : textures) {
			os << serializeString16(texture);
		}
	}

	writeV2S16(os, spritediv);
	writeV2S16(os, initial_sprite_basepos);
	writeU8(os, is_visible);
	writeU8(os, makes_footstep_sound);
	writeF(os, automatic_rotate, protocol_version);
	os << serializeString16(mesh);
	writeU16(os, colors.size());
	for (video::SColor color : colors) {
		writeARGB8(os, color);
	}
	writeU8(os, collideWithObjects);
	writeF(os, stepheight, protocol_version);
	writeU8(os, automatic_face_movement_dir);
	writeF(os, automatic_face_movement_dir_offset, protocol_version);
	writeU8(os, backface_culling);
	os << serializeString16(nametag);
	writeARGB8(os, nametag_color);
	writeF(os, automatic_face_movement_max_rotation_per_sec, protocol_version);
	os << serializeString16(infotext);
	os << serializeString16(wield_item);
	writeS8(os, glow);

	// Everything after this can use writeF32().
	if (protocol_version < 37)
		return;

	writeU16(os, breath_max);
	writeF32(os, eye_height);
	writeF32(os, zoom_fov);
	writeU8(os, use_texture_alpha);
	os << serializeString16(damage_texture_modifier);
	writeU8(os, shaded);
	writeU8(os, show_on_minimap);

	if (!nametag_bgcolor)
		writeARGB8(os, NULL_BGCOLOR);
	else if (nametag_bgcolor.value().getAlpha() == 0)
		writeARGB8(os, video::SColor(0, 0, 0, 0));
	else
		writeARGB8(os, nametag_bgcolor.value());

	// Add stuff only at the bottom.
	// Never remove anything, because we don't want new versions of this
}

void ObjectProperties::deSerialize(std::istream &is)
{
	int version = readU8(is);
	if (version != 1 && version != 4)
		throw SerializationError("unsupported ObjectProperties version");

	// Another approximate protocol version.
	const u16 protocol_version = version == 1 ? 32 : 37;

	try {
		hp_max = readU16(is);
		physical = readU8(is);
		readU32(is); // removed property (weight)
		collisionbox.MinEdge = readV3F(is, protocol_version);
		collisionbox.MaxEdge = readV3F(is, protocol_version);
		if (version >= 4) {
			selectionbox.MinEdge = readV3F32(is);
			selectionbox.MaxEdge = readV3F32(is);
			pointable = readU8(is);
		} else {
			selectionbox.MinEdge = collisionbox.MinEdge;
			selectionbox.MaxEdge = collisionbox.MaxEdge;
			pointable = true;
		}
		visual = deSerializeString16(is);
		if (version == 1) {
			v2f size = readV2F1000(is);
			visual_size = v3f(size.X, size.Y, size.X);
		} else {
			visual_size = readV3F32(is);
		}
		textures.clear();
		u32 texture_count = readU16(is);
		for (u32 i = 0; i < texture_count; i++){
			textures.push_back(deSerializeString16(is));
		}
		spritediv = readV2S16(is);
		initial_sprite_basepos = readV2S16(is);
		is_visible = readU8(is);
		makes_footstep_sound = readU8(is);
		automatic_rotate = readF(is, protocol_version);
		mesh = deSerializeString16(is);
		colors.clear();
		u32 color_count = readU16(is);
		for (u32 i = 0; i < color_count; i++){
			colors.push_back(readARGB8(is));
		}
		collideWithObjects = readU8(is);
		stepheight = readF(is, protocol_version);
		automatic_face_movement_dir = readU8(is);
		automatic_face_movement_dir_offset = readF(is, protocol_version);
		backface_culling = readU8(is);
		nametag = deSerializeString16(is);
		nametag_color = readARGB8(is);
		automatic_face_movement_max_rotation_per_sec = readF(is,
			protocol_version);
		infotext = deSerializeString16(is);
		wield_item = deSerializeString16(is);

		// The "glow" property exists in MultiCraft 1.
		glow = readS8(is);
		if (version == 1)
			return;

		breath_max = readU16(is);
		eye_height = readF32(is);
		zoom_fov = readF32(is);
		use_texture_alpha = readU8(is);

		damage_texture_modifier = deSerializeString16(is);
		u8 tmp = readU8(is);
		if (is.eof())
			return;
		shaded = tmp;
		tmp = readU8(is);
		if (is.eof())
			return;
		show_on_minimap = tmp;

		auto bgcolor = readARGB8(is);
		if (bgcolor != NULL_BGCOLOR)
			nametag_bgcolor = bgcolor;
		else
			nametag_bgcolor = nullopt;
	} catch (SerializationError &e) {}
}

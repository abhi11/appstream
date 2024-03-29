/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*-
 *
 * Copyright (C) 2012-2014 Matthias Klumpp <matthias@tenstral.net>
 *
 * Licensed under the GNU Lesser General Public License Version 2.1
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 2.1 of the license, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "as-component.h"
#include "as-component-private.h"

#include <glib.h>
#include <glib-object.h>
#include <stdlib.h>
#include <string.h>
#include <libxml/tree.h>
#include <libxml/parser.h>

#include "as-utils.h"
#include "as-utils-private.h"

/**
 * SECTION:as-component
 * @short_description: Object representing a software component
 * @include: appstream.h
 *
 * This object represents an Appstream software component which is associated
 * to a package in the distribution's repositories.
 * A component can be anything, ranging from an application to a font, a codec or
 * even a non-visual software project providing libraries and python-modules for
 * other applications to use.
 *
 * The type of the component is stored as #AsComponentKind and can be queried to
 * find out which kind of component we're dealing with.
 *
 * See also: #AsProvidesKind, #AsDatabase
 */

struct _AsComponentPrivate {
	AsComponentKind kind;
	gchar *id;
	gchar **pkgnames;
	gchar *name;
	gchar *name_original;
	gchar *summary;
	gchar *description;
	gchar **keywords;
	gchar *icon;
	gchar *icon_url;
	gchar **categories;
	gchar *project_license;
	gchar *project_group;
	gchar *developer_name;
	gchar **compulsory_for_desktops;
	GPtrArray *screenshots; /* of AsScreenshot elements */
	GPtrArray *provided_items; /* of utf8:string */
	GPtrArray *releases; /* of AsRelease */
	GHashTable *urls; /* of key:utf8 */
	GPtrArray *extends; /* of utf8:string */
	GHashTable *languages; /* of key:utf8 */
	int priority; /* used internally */
};

static gpointer as_component_parent_class = NULL;

#define AS_COMPONENT_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), AS_TYPE_COMPONENT, AsComponentPrivate))

enum  {
	AS_COMPONENT_DUMMY_PROPERTY,
	AS_COMPONENT_KIND,
	AS_COMPONENT_PKGNAMES,
	AS_COMPONENT_ID,
	AS_COMPONENT_NAME,
	AS_COMPONENT_NAME_ORIGINAL,
	AS_COMPONENT_SUMMARY,
	AS_COMPONENT_DESCRIPTION,
	AS_COMPONENT_KEYWORDS,
	AS_COMPONENT_ICON,
	AS_COMPONENT_ICON_URL,
	AS_COMPONENT_URLS,
	AS_COMPONENT_CATEGORIES,
	AS_COMPONENT_PROJECT_LICENSE,
	AS_COMPONENT_PROJECT_GROUP,
	AS_COMPONENT_DEVELOPER_NAME,
	AS_COMPONENT_SCREENSHOTS
};

static void as_component_finalize (GObject* obj);
static void as_component_get_property (GObject * object, guint property_id, GValue * value, GParamSpec * pspec);
static void as_component_set_property (GObject * object, guint property_id, const GValue * value, GParamSpec * pspec);

/**
 * as_component_kind_get_type:
 *
 * Defines registered component types.
 */
GType
as_component_kind_get_type (void)
{
	static volatile gsize as_component_kind_type_id__volatile = 0;
	if (g_once_init_enter (&as_component_kind_type_id__volatile)) {
		static const GEnumValue values[] = {
					{AS_COMPONENT_KIND_UNKNOWN, "AS_COMPONENT_KIND_UNKNOWN", "unknown"},
					{AS_COMPONENT_KIND_GENERIC, "AS_COMPONENT_KIND_GENERIC", "generic"},
					{AS_COMPONENT_KIND_DESKTOP_APP, "AS_COMPONENT_KIND_DESKTOP_APP", "desktop"},
					{AS_COMPONENT_KIND_FONT, "AS_COMPONENT_KIND_FONT", "font"},
					{AS_COMPONENT_KIND_CODEC, "AS_COMPONENT_KIND_CODEC", "codec"},
					{AS_COMPONENT_KIND_INPUTMETHOD, "AS_COMPONENT_KIND_INPUTMETHOD", "inputmethod"},
					{AS_COMPONENT_KIND_LAST, "AS_COMPONENT_KIND_LAST", "last"},
					{0, NULL, NULL}
		};
		GType as_component_type_type_id;
		as_component_type_type_id = g_enum_register_static ("AsComponentKind", values);
		g_once_init_leave (&as_component_kind_type_id__volatile, as_component_type_type_id);
	}
	return as_component_kind_type_id__volatile;
}

/**
 * as_component_kind_to_string:
 * @kind: the %AsComponentKind.
 *
 * Converts the enumerated value to an text representation.
 *
 * Returns: string version of @kind
 **/
const gchar *
as_component_kind_to_string (AsComponentKind kind)
{
	if (kind == AS_COMPONENT_KIND_GENERIC)
		return "generic";
	if (kind == AS_COMPONENT_KIND_DESKTOP_APP)
		return "desktop";
	if (kind == AS_COMPONENT_KIND_FONT)
		return "font";
	if (kind == AS_COMPONENT_KIND_CODEC)
		return "codec";
	if (kind == AS_COMPONENT_KIND_INPUTMETHOD)
		return "inputmethod";
	if (kind == AS_COMPONENT_KIND_ADDON)
		return "addon";
	return "unknown";
}

/**
 * as_component_kind_from_string:
 * @kind_str: the string.
 *
 * Converts the text representation to an enumerated value.
 *
 * Returns: a #AsComponentKind or %AS_COMPONENT_KIND_UNKNOWN for unknown
 **/
AsComponentKind
as_component_kind_from_string (const gchar *kind_str)
{
	if (g_strcmp0 (kind_str, "generic") == 0)
		return AS_COMPONENT_KIND_GENERIC;
	if (g_strcmp0 (kind_str, "desktop") == 0)
		return AS_COMPONENT_KIND_DESKTOP_APP;
	if (g_strcmp0 (kind_str, "font") == 0)
		return AS_COMPONENT_KIND_FONT;
	if (g_strcmp0 (kind_str, "codec") == 0)
		return AS_COMPONENT_KIND_CODEC;
	if (g_strcmp0 (kind_str, "inputmethod") == 0)
		return AS_COMPONENT_KIND_INPUTMETHOD;
	if (g_strcmp0 (kind_str, "addon") == 0)
		return AS_COMPONENT_KIND_ADDON;
	return AS_COMPONENT_KIND_UNKNOWN;
}

/**
 * as_component_construct:
 *
 * Construct an #AsComponent.
 *
 * Returns: (transfer full): a new #AsComponent
 **/
AsComponent*
as_component_construct (GType object_type)
{
	AsComponent * self = NULL;
	gchar** strv;
	self = (AsComponent*) g_object_new (object_type, NULL);
	as_component_set_id (self, "");
	as_component_set_name_original (self, "");
	as_component_set_summary (self, "");
	as_component_set_description (self, "");
	as_component_set_icon (self, "");
	as_component_set_icon_url (self, "");
	as_component_set_project_license (self, "");
	as_component_set_project_group (self, "");
	self->priv->keywords = NULL;

	strv = g_new0 (gchar*, 1 + 1);
	strv[0] = NULL;
	self->priv->categories = strv;
	self->priv->screenshots = g_ptr_array_new_with_free_func (g_object_unref);
	self->priv->provided_items = g_ptr_array_new_with_free_func (g_free);
	self->priv->releases = g_ptr_array_new_with_free_func (g_object_unref);
	self->priv->urls = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
	self->priv->extends = g_ptr_array_new_with_free_func (g_free);
	self->priv->languages = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

	as_component_set_priority (self, 0);

	return self;
}

static void
as_component_finalize (GObject* obj)
{
	AsComponent *self;
	self = G_TYPE_CHECK_INSTANCE_CAST (obj, AS_TYPE_COMPONENT, AsComponent);
	g_free (self->priv->id);
	g_strfreev (self->priv->pkgnames);
	g_free (self->priv->name);
	g_free (self->priv->name_original);
	g_free (self->priv->summary);
	g_free (self->priv->description);
	g_free (self->priv->icon);
	g_free (self->priv->icon_url);
	g_free (self->priv->project_license);
	g_free (self->priv->project_group);
	g_strfreev (self->priv->keywords);
	g_strfreev (self->priv->categories);
	g_strfreev (self->priv->compulsory_for_desktops);
	g_ptr_array_unref (self->priv->screenshots);
	g_ptr_array_unref (self->priv->provided_items);
	g_ptr_array_unref (self->priv->releases);
	g_hash_table_unref (self->priv->urls);
	g_ptr_array_unref (self->priv->extends);
	g_hash_table_unref (self->priv->languages);
	G_OBJECT_CLASS (as_component_parent_class)->finalize (obj);
}

/**
 * as_component_new:
 *
 * Creates a new #AsComponent.
 *
 * Returns: (transfer full): a new #AsComponent
 **/
AsComponent*
as_component_new (void)
{
	return as_component_construct (AS_TYPE_COMPONENT);
}

/**
 * as_component_is_valid:
 *
 * Check if the essential properties of this Component are
 * populated with useful data.
 *
 * Returns: TRUE if the component data was validated successfully.
 */
gboolean
as_component_is_valid (AsComponent* self)
{
	gboolean ret = FALSE;
	AsComponentKind ctype;

	g_return_val_if_fail (self != NULL, FALSE);

	ctype = self->priv->kind;
	if (ctype == AS_COMPONENT_KIND_UNKNOWN)
		return FALSE;

	if ((self->priv->pkgnames != NULL) &&
		(g_strcmp0 (self->priv->id, "") != 0) &&
		(g_strcmp0 (self->priv->name, "") != 0) &&
		(g_strcmp0 (self->priv->name_original, "") != 0)) {
		ret = TRUE;
		}

#if 0
	if ((ret) && ctype == AS_COMPONENT_KIND_DESKTOP_APP) {
		ret = g_strcmp0 (self->priv->desktop_file, "") != 0;
	}
#endif

	return ret;
}

/**
 * as_component_to_string:
 *
 * Returns a string identifying this component.
 * (useful for debugging)
 *
 * Returns: (transfer full): A descriptive string
 **/
gchar*
as_component_to_string (AsComponent* self)
{
	gchar* res = NULL;
	const gchar *name;
	gchar *pkgs;
	g_return_val_if_fail (self != NULL, NULL);

	if (self->priv->pkgnames == NULL)
		pkgs = g_strdup ("?");
	else
		pkgs = g_strjoinv (",", self->priv->pkgnames);
	name = as_component_get_name (self);

	switch (self->priv->kind) {
		case AS_COMPONENT_KIND_DESKTOP_APP:
		{
			res = g_strdup_printf ("[DesktopApp::%s]> name: %s | package: %s | summary: %s", self->priv->id, name, pkgs, self->priv->summary);
			break;
		}
		default:
		{
			res = g_strdup_printf ("[Component::%s]> name: %s | package: %s | summary: %s", self->priv->id, name, pkgs, self->priv->summary);
			break;
		}
	}

	g_free (pkgs);
	return res;
}

/**
 * as_component_add_screenshot:
 * @self: a #AsComponent instance.
 * @sshot: The #AsScreenshot to add
 *
 * Add an #AsScreenshot to this component.
 **/
void
as_component_add_screenshot (AsComponent* self, AsScreenshot* sshot)
{
	GPtrArray* sslist;
	g_return_if_fail (self != NULL);
	g_return_if_fail (sshot != NULL);

	sslist = as_component_get_screenshots (self);
	g_ptr_array_add (sslist, g_object_ref (sshot));
}

/**
 * as_component_add_release:
 * @self: a #AsComponent instance.
 * @release: The #AsRelease to add
 *
 * Add an #AsRelease to this component.
 **/
void
as_component_add_release (AsComponent* self, AsRelease* release)
{
	GPtrArray* releases;
	g_return_if_fail (self != NULL);
	g_return_if_fail (release != NULL);

	releases = as_component_get_releases (self);
	g_ptr_array_add (releases, g_object_ref (release));
}

/**
 * as_component_get_urls:
 * @self: a #AsComponent instance.
 *
 * Gets the URLs set for the component.
 *
 * Returns: (transfer none): URLs
 *
 * Since: 0.6.2
 **/
GHashTable*
as_component_get_urls (AsComponent *self)
{
	g_return_if_fail (self != NULL);
	return self->priv->urls;
}

/**
 * as_component_get_url:
 * @self: a #AsComponent instance.
 * @url_kind: the URL kind, e.g. %AS_URL_KIND_HOMEPAGE.
 *
 * Gets a URL.
 *
 * Returns: string, or %NULL if unset
 *
 * Since: 0.6.2
 **/
const gchar *
as_component_get_url (AsComponent *self, AsUrlKind url_kind)
{
	g_return_if_fail (self != NULL);
	return g_hash_table_lookup (self->priv->urls,
				    as_url_kind_to_string (url_kind));
}

/**
 * as_component_add_url:
 * @self: a #AsComponent instance.
 * @url_kind: the URL kind, e.g. %AS_URL_KIND_HOMEPAGE
 * @url: the full URL.
 *
 * Adds some URL data to the component.
 *
 * Since: 0.6.2
 **/
void
as_component_add_url (AsComponent *self,
					  AsUrlKind url_kind,
					  const gchar *url)
{
	g_return_if_fail (self != NULL);
	g_hash_table_insert (self->priv->urls,
			     g_strdup (as_url_kind_to_string (url_kind)),
			     g_strdup (url));
}

 /**
  * as_component_get_extends:
  * @cpt: an #AsComponent instance.
  *
  * Returns a string list of IDs of components which
  * are extended by this addon.
  *
  * Returns: (element-type utf8) (transfer none): an array
  *
  * Since: 0.7.0
**/
GPtrArray*
as_component_get_extends (AsComponent *cpt)
{
	AsComponentPrivate *priv = cpt->priv;
	return priv->extends;
}

/**
 * as_component_add_extends:
 * @cpt: a #AsComponent instance.
 * @cpt_id: The id of a component which is extended by this component
 *
 * Add a reference to the extended component
 **/
void
as_component_add_extends (AsComponent* cpt, const gchar* cpt_id)
{
	AsComponentPrivate *priv = cpt->priv;
	g_ptr_array_add (priv->extends, g_strdup (cpt_id));
}

static void
_as_component_serialize_image (AsImage *img, xmlNode *subnode)
{
	xmlNode* n_image = NULL;
	gchar *size;
	g_return_if_fail (img != NULL);
	g_return_if_fail (subnode != NULL);

	n_image = xmlNewTextChild (subnode, NULL, (xmlChar*) "image", (xmlChar*) as_image_get_url (img));
	if (as_image_get_kind (img) == AS_IMAGE_KIND_THUMBNAIL)
		xmlNewProp (n_image, (xmlChar*) "type", (xmlChar*) "thumbnail");
	else
		xmlNewProp (n_image, (xmlChar*) "type", (xmlChar*) "source");

	size = g_strdup_printf("%i", as_image_get_width (img));
	xmlNewProp (n_image, (xmlChar*) "width", (xmlChar*) size);
	g_free (size);

	size = g_strdup_printf("%i", as_image_get_height (img));
	xmlNewProp (n_image, (xmlChar*) "height", (xmlChar*) size);
	g_free (size);

	xmlAddChild (subnode, n_image);
}

/**
 * as_component_dump_screenshot_data_xml:
 *
 * Internal function to create XML which gets stored in the AppStream database
 * for screenshots
 */
gchar*
as_component_dump_screenshot_data_xml (AsComponent* self)
{
	GPtrArray* sslist;
	xmlDoc *doc;
	xmlNode *root;
	guint i;
	AsScreenshot *sshot;
	gchar *xmlstr = NULL;
	g_return_val_if_fail (self != NULL, NULL);

	sslist = as_component_get_screenshots (self);
	if (sslist->len == 0) {
		return g_strdup ("");
	}

	doc = xmlNewDoc ((xmlChar*) NULL);
	root = xmlNewNode (NULL, (xmlChar*) "screenshots");
	xmlDocSetRootElement (doc, root);

	for (i = 0; i < sslist->len; i++) {
		xmlNode *subnode;
		const gchar *str;
		GPtrArray *images;
		sshot = (AsScreenshot*) g_ptr_array_index (sslist, i);

		subnode = xmlNewTextChild (root, NULL, (xmlChar*) "screenshot", (xmlChar*) "");
		if (as_screenshot_get_kind (sshot) == AS_SCREENSHOT_KIND_DEFAULT)
			xmlNewProp (subnode, (xmlChar*) "type", (xmlChar*) "default");

		str = as_screenshot_get_caption (sshot);
		if (g_strcmp0 (str, "") != 0) {
			xmlNode* n_caption;
			n_caption = xmlNewTextChild (subnode, NULL, (xmlChar*) "caption", (xmlChar*) str);
			xmlAddChild (subnode, n_caption);
		}

		images = as_screenshot_get_images (sshot);
		g_ptr_array_foreach (images, (GFunc) _as_component_serialize_image, subnode);
	}

	xmlDocDumpMemory (doc, (xmlChar**) (&xmlstr), NULL);
	xmlFreeDoc (doc);

	return xmlstr;
}

/**
 * as_component_load_screenshots_from_internal_xml:
 *
 * Internal function to load the screenshot list
 * using the database-internal XML data.
 */
void
as_component_load_screenshots_from_internal_xml (AsComponent* self, const gchar* xmldata)
{
	xmlDoc* doc = NULL;
	xmlNode* root = NULL;
	xmlNode *iter;
	g_return_if_fail (self != NULL);
	g_return_if_fail (xmldata != NULL);

	if (as_str_empty (xmldata)) {
		return;
	}

	doc = xmlParseDoc ((xmlChar*) xmldata);
	root = xmlDocGetRootElement (doc);

	if (root == NULL) {
		xmlFreeDoc (doc);
		return;
	}

	for (iter = root->children; iter != NULL; iter = iter->next) {
		if (g_strcmp0 ((gchar*) iter->name, "screenshot") == 0) {
			AsScreenshot* sshot;
			gchar *typestr;
			xmlNode *iter2;

			sshot = as_screenshot_new ();
			typestr = (gchar*) xmlGetProp (iter, (xmlChar*) "type");
			if (g_strcmp0 (typestr, "default") == 0)
				as_screenshot_set_kind (sshot, AS_SCREENSHOT_KIND_DEFAULT);
			else
				as_screenshot_set_kind (sshot, AS_SCREENSHOT_KIND_NORMAL);
			g_free (typestr);
			for (iter2 = iter->children; iter2 != NULL; iter2 = iter2->next) {
				const gchar *node_name;
				gchar *content;

				node_name = (const gchar*) iter2->name;
				content = (gchar*) xmlNodeGetContent (iter2);
				if (g_strcmp0 (node_name, "image") == 0) {
					AsImage *img;
					gchar *str;
					guint64 width;
					guint64 height;
					gchar *imgtype;
					if (content == NULL)
						continue;
					img = as_image_new ();

					str = (gchar*) xmlGetProp (iter2, (xmlChar*) "width");
					if (str == NULL) {
						g_object_unref (img);
						continue;
					}
					width = g_ascii_strtoll (str, NULL, 10);
					g_free (str);

					str = (gchar*) xmlGetProp (iter2, (xmlChar*) "height");
					if (str == NULL) {
						g_object_unref (img);
						continue;
					}
					height = g_ascii_strtoll (str, NULL, 10);
					g_free (str);

					as_image_set_width (img, width);
					as_image_set_height (img, height);

					/* discard invalid elements */
					if ((width == 0) || (height == 0)) {
						g_object_unref (img);
						continue;
					}
					as_image_set_url (img, content);

					imgtype = (gchar*) xmlGetProp (iter2, (xmlChar*) "type");
					if (g_strcmp0 (imgtype, "thumbnail") == 0) {
						as_image_set_kind (img, AS_IMAGE_KIND_THUMBNAIL);
					} else {
						as_image_set_kind (img, AS_IMAGE_KIND_SOURCE);
					}
					g_free (imgtype);

					as_screenshot_add_image (sshot, img);
				} else if (g_strcmp0 (node_name, "caption") == 0) {
					if (content != NULL)
						as_screenshot_set_caption (sshot, content);
				}
				g_free (content);
			}
			as_component_add_screenshot (self, sshot);
		}
	}
}

/**
 * as_component_complete:
 *
 * Private function to complete a AsComponent with
 * additional data found on the system.
 *
 * @scr_base_url Base url for screenshot-service, obtain via #AsDistroDetails
 */
void
as_component_complete (AsComponent* self, gchar *scr_base_url)
{
	AsComponentPrivate *priv = self->priv;

	/* we want screenshot data from 3rd-party screenshot servers, if the component doesn't have screenshots defined already */
	if ((priv->screenshots->len == 0) && (priv->pkgnames != NULL)) {
		gchar *url;
		AsImage *img;
		AsScreenshot *sshot;

		url = g_build_filename (scr_base_url, "screenshot", priv->pkgnames[0], NULL);

		/* screenshots.debian.net-like services dont specify a size, so we choose the default sizes
		 * (800x600 for source-type images, 160x120 for thumbnails)
		 */

		/* add main image */
		img = as_image_new ();
		as_image_set_url (img, url);
		as_image_set_width (img, 800);
		as_image_set_height (img, 600);
		as_image_set_kind (img, AS_IMAGE_KIND_SOURCE);

		sshot = as_screenshot_new ();
		as_screenshot_add_image (sshot, img);
		as_screenshot_set_kind (sshot, AS_SCREENSHOT_KIND_DEFAULT);

		g_object_unref (img);
		g_free (url);

		/* add thumbnail */
		url = g_build_filename (scr_base_url, "thumbnail", priv->pkgnames[0], NULL);
		img = as_image_new ();
		as_image_set_url (img, url);
		as_image_set_width (img, 160);
		as_image_set_height (img, 120);
		as_image_set_kind (img, AS_IMAGE_KIND_THUMBNAIL);
		as_screenshot_add_image (sshot, img);

		/* add screenshot to component */
		as_component_add_screenshot (self, sshot);

		g_object_unref (img);
		g_object_unref (sshot);
		g_free (url);
	}
}

/**
 * as_component_dump_releases_data_xml:
 *
 * Internal function to create XML which gets stored in the AppStream database
 * for releases
 */
gchar*
as_component_dump_releases_data_xml (AsComponent* self)
{
	GPtrArray* releases;
	xmlDoc *doc;
	xmlNode *root;
	guint i;
	AsRelease *release;
	gchar *xmlstr = NULL;
	g_return_val_if_fail (self != NULL, NULL);

	releases = as_component_get_releases (self);
	if (releases->len == 0) {
		return g_strdup ("");
	}

	doc = xmlNewDoc ((xmlChar*) NULL);
	root = xmlNewNode (NULL, (xmlChar*) "releases");
	xmlDocSetRootElement (doc, root);

	for (i = 0; i < releases->len; i++) {
		xmlNode *subnode;
		const gchar *str;
		gchar *timestamp;
		release = (AsRelease*) g_ptr_array_index (releases, i);

		subnode = xmlNewTextChild (root, NULL, (xmlChar*) "release", (xmlChar*) "");
		xmlNewProp (subnode, (xmlChar*) "version",
					(xmlChar*) as_release_get_version (release));
		timestamp = g_strdup_printf ("%ld", as_release_get_timestamp (release));
		xmlNewProp (subnode, (xmlChar*) "timestamp",
					(xmlChar*) timestamp);
		g_free (timestamp);

		str = as_release_get_description (release);
		if (g_strcmp0 (str, "") != 0) {
			xmlNode* n_desc;
			n_desc = xmlNewTextChild (subnode, NULL, (xmlChar*) "description", (xmlChar*) str);
			xmlAddChild (subnode, n_desc);
		}
	}

	xmlDocDumpMemory (doc, (xmlChar**) (&xmlstr), NULL);
	xmlFreeDoc (doc);

	return xmlstr;
}

/**
 * as_component_load_releases_from_internal_xml:
 *
 * Internal function to load the releases list
 * using the database-internal XML data.
 */
void
as_component_load_releases_from_internal_xml (AsComponent* self, const gchar* xmldata)
{
	xmlDoc* doc = NULL;
	xmlNode* root = NULL;
	xmlNode *iter;
	g_return_if_fail (self != NULL);
	g_return_if_fail (xmldata != NULL);

	if (as_str_empty (xmldata)) {
		return;
	}

	doc = xmlParseDoc ((xmlChar*) xmldata);
	root = xmlDocGetRootElement (doc);

	if (root == NULL) {
		xmlFreeDoc (doc);
		return;
	}

	for (iter = root->children; iter != NULL; iter = iter->next) {
		if (g_strcmp0 ((gchar*) iter->name, "release") == 0) {
			AsRelease *release;
			gchar *prop;
			guint64 timestamp;
			xmlNode *iter2;
			release = as_release_new ();

			prop = (gchar*) xmlGetProp (iter, (xmlChar*) "version");
			as_release_set_version (release, prop);
			g_free (prop);

			prop = (gchar*) xmlGetProp (iter, (xmlChar*) "timestamp");
			timestamp = g_ascii_strtoll (prop, NULL, 10);
			as_release_set_timestamp (release, timestamp);
			g_free (prop);

			for (iter2 = iter->children; iter2 != NULL; iter2 = iter2->next) {
				if (iter->type != XML_ELEMENT_NODE)
					continue;

				if (g_strcmp0 ((gchar*) iter->name, "description") == 0) {
					gchar *content;
					content = (gchar*) xmlNodeGetContent (iter2);
					as_release_set_description (release, content);
					g_free (content);
					break;
				}
			}

			as_component_add_release (self, release);
			g_object_unref (release);
		}
	}
}

/**
 * as_component_provides_item:
 *
 * Checks if this component provides an item of the specified type
 *
 * @self a valid #AsComponent
 * @kind the kind of the provides-item
 * @value the value of the provides-item
 *
 * Returns: %TRUE if an item was found
 */
gboolean
as_component_provides_item (AsComponent *self, AsProvidesKind kind, const gchar *value)
{
	guint i;
	gboolean ret = FALSE;
	gchar *item;
	AsComponentPrivate *priv = self->priv;

	item = as_provides_item_create (kind, value, "");
	for (i = 0; i < priv->provided_items->len; i++) {
		gchar *cval;
		cval = (gchar*) g_ptr_array_index (priv->provided_items, i);
		if (g_strcmp0 (item, cval) == 0) {
			ret = TRUE;
			break;
		}
	}

	g_free (item);
	return ret;
}

/**
 * as_component_get_kind:
 *
 * Returns the #AsComponentKind of this component.
 */
AsComponentKind
as_component_get_kind (AsComponent* self)
{
	g_return_val_if_fail (self != NULL, 0);
	return self->priv->kind;
}

/**
 * as_component_set_kind:
 *
 * Sets the #AsComponentKind of this component.
 */
void
as_component_set_kind (AsComponent* self, AsComponentKind value)
{
	g_return_if_fail (self != NULL);

	self->priv->kind = value;
	g_object_notify ((GObject *) self, "kind");
}

/**
 * as_component_get_pkgnames:
 *
 * Get a list of package names which this component consists of.
 * This usually is just one package name.
 *
 * Returns: (transfer none): String array of package names
 */
gchar**
as_component_get_pkgnames (AsComponent* self)
{
	g_return_val_if_fail (self != NULL, NULL);
	return self->priv->pkgnames;
}

/**
 * as_component_set_pkgnames:
 * @value: (array zero-terminated=1):
 *
 * Set a list of package names this component consists of.
 * (This should usually be just one package name)
 */
void
as_component_set_pkgnames (AsComponent* self, gchar** value)
{
	g_return_if_fail (self != NULL);

	g_strfreev (self->priv->pkgnames);
	self->priv->pkgnames = g_strdupv (value);
	g_object_notify ((GObject *) self, "pkgnames");
}

/**
 * as_component_get_id:
 *
 * Set the unique identifier for this component.
 */
const gchar*
as_component_get_id (AsComponent* self)
{
	g_return_val_if_fail (self != NULL, NULL);
	return self->priv->id;
}

/**
 * as_component_set_id:
 *
 * Set the unique identifier for this component.
 */
void
as_component_set_id (AsComponent* self, const gchar* value)
{
	g_return_if_fail (self != NULL);

	g_free (self->priv->id);
	self->priv->id = g_strdup (value);
	g_object_notify ((GObject *) self, "id");
}

const gchar*
as_component_get_name (AsComponent* self)
{
	g_return_val_if_fail (self != NULL, NULL);
	if (as_str_empty (self->priv->name)) {
		self->priv->name = g_strdup (self->priv->name_original);
	}

	return self->priv->name;
}

void
as_component_set_name (AsComponent* self, const gchar* value)
{
	g_return_if_fail (self != NULL);

	g_free (self->priv->name);
	self->priv->name = g_strdup (value);
	g_object_notify ((GObject *) self, "name");
}

const gchar*
as_component_get_name_original (AsComponent* self)
{
	g_return_val_if_fail (self != NULL, NULL);
	return self->priv->name_original;
}

void
as_component_set_name_original (AsComponent* self, const gchar* value)
{
	g_return_if_fail (self != NULL);

	g_free (self->priv->name_original);
	self->priv->name_original = g_strdup (value);
	g_object_notify ((GObject *) self, "name-original");
}

const gchar*
as_component_get_summary (AsComponent* self)
{
	g_return_val_if_fail (self != NULL, NULL);

	return self->priv->summary;
}

void
as_component_set_summary (AsComponent* self, const gchar* value)
{
	g_return_if_fail (self != NULL);

	g_free (self->priv->summary);
	self->priv->summary = g_strdup (value);
	g_object_notify ((GObject *) self, "summary");
}

const gchar*
as_component_get_description (AsComponent* self)
{
	const gchar* result;
	const gchar* _tmp0_ = NULL;
	g_return_val_if_fail (self != NULL, NULL);
	_tmp0_ = self->priv->description;
	result = _tmp0_;
	return result;
}

void
as_component_set_description (AsComponent* self, const gchar* value)
{
	g_return_if_fail (self != NULL);

	g_free (self->priv->description);
	self->priv->description = g_strdup (value);
	g_object_notify ((GObject *) self, "description");
}

/**
 * as_component_get_keywords:
 *
 * Returns: (transfer none): String array of keywords
 */
gchar**
as_component_get_keywords (AsComponent* self)
{
	g_return_val_if_fail (self != NULL, NULL);
	return self->priv->keywords;
}

/**
 * as_component_set_keywords:
 * @value: (array zero-terminated=1):
 */
void
as_component_set_keywords (AsComponent* self, gchar** value)
{
	g_return_if_fail (self != NULL);

	g_strfreev (self->priv->keywords);
	self->priv->keywords = g_strdupv (value);
	g_object_notify ((GObject *) self, "keywords");
}

/**
 * as_component_get_icon:
 * @self: an #AsComponent instance
 *
 * Returns the icon name for this component. This is usually
 * a stock icon name, e.g. "applications-science"
 */
const gchar*
as_component_get_icon (AsComponent* self)
{
	g_return_val_if_fail (self != NULL, NULL);
	return self->priv->icon;
}

/**
 * as_component_set_icon:
 * @self: an #AsComponent instance
 *
 * Set a stock icon name for this component,
 * e.g. "applications-science"
 */
void
as_component_set_icon (AsComponent* self, const gchar* value)
{
	g_return_if_fail (self != NULL);

	g_free (self->priv->icon);
	self->priv->icon = g_strdup (value);
	g_object_notify ((GObject *) self, "icon");
}

/**
 * as_component_get_icon_url:
 * @self: an #AsComponent instance
 *
 * Returns the full url of this icon, e.g.
 * "/usr/share/icons/hicolor/64x64/foobar.png"
 * This might also be an http url pointing at a remote location.
 */
const gchar*
as_component_get_icon_url (AsComponent* self)
{
	g_return_val_if_fail (self != NULL, NULL);
	return self->priv->icon_url;
}

/**
 * as_component_set_icon_url:
 * @self: an #AsComponent instance
 *
 * Set an icon url for this component, which can be a remote
 * or local location.
 * The icon size pointed to should be 64x64 and the icon should ideally be
 * a PNG icon.
 */
void
as_component_set_icon_url (AsComponent* self, const gchar* value)
{
	g_return_if_fail (self != NULL);

	g_free (self->priv->icon_url);
	self->priv->icon_url = g_strdup (value);
	g_object_notify ((GObject *) self, "icon-url");
}

/**
 * as_component_get_categories:
 *
 * Returns: (transfer none): String array of categories
 */
gchar**
as_component_get_categories (AsComponent* self)
{
	g_return_val_if_fail (self != NULL, NULL);

	return self->priv->categories;
}

/**
 * as_component_set_categories:
 * @value: (array zero-terminated=1):
 */
void
as_component_set_categories (AsComponent* self, gchar** value)
{
	g_return_if_fail (self != NULL);

	g_strfreev (self->priv->categories);
	self->priv->categories = g_strdupv (value);
	g_object_notify ((GObject *) self, "categories");
}

/**
 * as_component_set_categories_from_str:
 *
 * Set the categories list from a string
 *
 * @self a valid #AsComponent instance
 * @categories_str Semicolon-separated list of category-names
 */
void
as_component_set_categories_from_str (AsComponent* self, const gchar* categories_str)
{
	gchar** cats = NULL;

	g_return_if_fail (self != NULL);
	g_return_if_fail (categories_str != NULL);

	cats = g_strsplit (categories_str, ";", 0);
	as_component_set_categories (self, cats);
	g_strfreev (cats);
}

/**
 * as_component_has_category:
 * @self an #AsComponent object
 *
 * Check if component is in the specified category.
 **/
gboolean
as_component_has_category (AsComponent* self, const gchar* category)
{
	gchar **categories;
	guint i;
	g_return_val_if_fail (self != NULL, FALSE);

	categories = self->priv->categories;
	for (i = 0; categories[i] != NULL; i++) {
		if (g_strcmp0 (categories[i], category) == 0)
			return TRUE;
	}

	return FALSE;
}

/**
 * as_component_get_project_license:
 *
 * Get the license of the project this component belongs to.
 */
const gchar*
as_component_get_project_license (AsComponent* self)
{
	g_return_val_if_fail (self != NULL, NULL);
	return self->priv->project_license;
}

/**
 * as_component_set_project_license:
 *
 * Set the project license.
 */
void
as_component_set_project_license (AsComponent* self, const gchar* value)
{
	g_return_if_fail (self != NULL);

	g_free (self->priv->project_license);
	self->priv->project_license = g_strdup (value);
	g_object_notify ((GObject *) self, "project-license");
}

/**
 * as_component_get_project_group:
 *
 * Get the component's project group.
 */
const gchar*
as_component_get_project_group (AsComponent* self)
{
	g_return_val_if_fail (self != NULL, NULL);
	return self->priv->project_group;
}

/**
 * as_component_set_project_group:
 *
 * Set the component's project group.
 */
void
as_component_set_project_group (AsComponent* self, const gchar* value)
{
	g_return_if_fail (self != NULL);

	g_free (self->priv->project_group);
	self->priv->project_group = g_strdup (value);
}

/**
 * as_component_get_developer_name:
 *
 * Get the component's developer or development team name.
 */
const gchar*
as_component_get_developer_name (AsComponent* self)
{
	g_return_val_if_fail (self != NULL, NULL);
	return self->priv->developer_name;
}

/**
 * as_component_set_developer_name:
 *
 * Set the the component's developer or development team name.
 */
void
as_component_set_developer_name (AsComponent* self, const gchar* value)
{
	g_return_if_fail (self != NULL);

	g_free (self->priv->developer_name);
	self->priv->developer_name = g_strdup (value);
}

/**
 * as_component_get_screenshots:
 *
 * Get a list of associated screenshots.
 *
 * Returns: (element-type AsScreenshot) (transfer none): an array of #AsScreenshot instances
 */
GPtrArray*
as_component_get_screenshots (AsComponent* self)
{
	g_return_val_if_fail (self != NULL, NULL);

	return self->priv->screenshots;
}

/**
 * as_component_get_compulsory_for_desktops:
 *
 * Return value: (transfer none): A list of desktops where this component is compulsory
 **/
gchar **
as_component_get_compulsory_for_desktops (AsComponent* self)
{
	g_return_val_if_fail (self != NULL, NULL);

	return self->priv->compulsory_for_desktops;
}

/**
 * as_component_set_compulsory_for_desktops:
 *
 * Set a list of desktops where this component is compulsory.
 **/
void
as_component_set_compulsory_for_desktops (AsComponent* self, gchar** value)
{
	g_return_if_fail (self != NULL);

	g_strfreev (self->priv->compulsory_for_desktops);
	self->priv->compulsory_for_desktops = g_strdupv (value);
}

/**
 * as_component_is_compulsory_for_desktop:
 * @self an #AsComponent object
 * @desktop the desktop-id to test for
 *
 * Check if this component is compulsory for the given desktop.
 *
 * Returns: %TRUE if compulsory, %FALSE otherwise.
 **/
gboolean
as_component_is_compulsory_for_desktop (AsComponent* self, const gchar* desktop)
{
	gchar **compulsory_for_desktops;
	guint i;
	g_return_val_if_fail (self != NULL, FALSE);

	compulsory_for_desktops = self->priv->compulsory_for_desktops;
	for (i = 0; compulsory_for_desktops[i] != NULL; i++) {
		if (g_strcmp0 (compulsory_for_desktops[i], desktop) == 0)
			return TRUE;
	}

	return FALSE;
}

/**
 * as_component_get_provided_items:
 *
 * Get an array of the provides-items this component is
 * associated with.
 *
 * Return value: (element-type utf8) (transfer none): A list of desktops where this component is compulsory
 **/
GPtrArray*
as_component_get_provided_items (AsComponent* self)
{
	g_return_val_if_fail (self != NULL, NULL);

	return self->priv->provided_items;
}

/**
 * as_component_add_provided_item:
 * @self: a #AsComponent instance.
 * @kind: the kind of the provided item (e.g. %AS_PROVIDES_KIND_MIMETYPE)
 * @data: additional data associated with this item, or %NULL.
 *
 * Adds a provided item to the component.
 *
 * Since: 0.6.2
 **/
void
as_component_add_provided_item (AsComponent *self, AsProvidesKind kind, const gchar *value, const gchar *data)
{
	g_return_if_fail (self != NULL);
	/* we just skip empty items */
	if (as_str_empty (value))
		return;
	g_ptr_array_add (self->priv->provided_items,
			     as_provides_item_create (kind, value, data));
}

/**
 * as_component_get_releases:
 *
 * Get an array of the #AsRelease items this component
 * provides.
 *
 * Return value: (element-type AsRelease) (transfer none): A list of releases
 **/
GPtrArray*
as_component_get_releases (AsComponent* self)
{
	g_return_val_if_fail (self != NULL, NULL);

	return self->priv->releases;
}

/**
 * as_component_get_priority:
 *
 * Returns the priority of this component.
 * This method is used internally.
 *
 * Since: 0.6.1
 */
int
as_component_get_priority (AsComponent* self)
{
	g_return_val_if_fail (self != NULL, 0);
	return self->priv->priority;
}

/**
 * as_component_set_priority:
 *
 * Sets the priority of this component.
 * This method is used internally.
 *
 * Since: 0.6.1
 */
void
as_component_set_priority (AsComponent* self, int priority)
{
	g_return_if_fail (self != NULL);
	self->priv->priority = priority;
}

/**
 * as_component_add_language:
 * @self: an #AsComponent instance.
 * @locale: the locale, or %NULL. e.g. "en_GB"
 * @percentage: the percentage completion of the translation, 0 for locales with unknown amount of translation
 *
 * Adds a language to the component.
 *
 * Since: 0.7.0
 **/
void
as_component_add_language (AsComponent *self, const gchar *locale, gint percentage)
{
	if (locale == NULL)
		locale = "C";
	g_hash_table_insert (self->priv->languages,
						 g_strdup (locale),
						 GINT_TO_POINTER (percentage));
}

/**
 * as_component_get_language:
 * @self: an #AsComponent instance.
 * @locale: the locale, or %NULL. e.g. "en_GB"
 *
 * Gets the translation coverage in percent for a specific locale
 *
 * Returns: a percentage value, -1 if locale was not found
 *
 * Since: 0.7.0
 **/
gint
as_component_get_language (AsComponent *self, const gchar *locale)
{
	gboolean ret;
	gpointer value = NULL;

	if (locale == NULL)
		locale = "C";
	ret = g_hash_table_lookup_extended (self->priv->languages,
					    locale, NULL, &value);
	if (!ret)
		return -1;
	return GPOINTER_TO_INT (value);
}

/**
 * as_component_get_languages:
 * @self: an #AsComponent instance.
 *
 * Get a list of all languages.
 *
 * Returns: (transfer container) (element-type utf8): list of locales
 *
 * Since: 0.7.0
 **/
GList*
as_component_get_languages (AsComponent *self)
{
	return g_hash_table_get_keys (self->priv->languages);
}

/**
 * as_component_get_languages_map:
 * @self: an #AsComponent instance.
 *
 * Get a HashMap mapping languages to their completion percentage
 *
 * Returns: (transfer none): locales map
 *
 * Since: 0.7.0
 **/
GHashTable*
as_component_get_languages_map (AsComponent *self)
{
	return self->priv->languages;
}

static void
as_component_class_init (AsComponentClass * klass)
{
	as_component_parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (AsComponentPrivate));
	G_OBJECT_CLASS (klass)->get_property = as_component_get_property;
	G_OBJECT_CLASS (klass)->set_property = as_component_set_property;
	G_OBJECT_CLASS (klass)->finalize = as_component_finalize;
	g_object_class_install_property (G_OBJECT_CLASS (klass), AS_COMPONENT_KIND, g_param_spec_enum ("kind", "kind", "kind", AS_TYPE_COMPONENT_KIND, 0, G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_READABLE | G_PARAM_WRITABLE));
	g_object_class_install_property (G_OBJECT_CLASS (klass), AS_COMPONENT_PKGNAMES, g_param_spec_boxed ("pkgnames", "pkgnames", "pkgnames", G_TYPE_STRV, G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_READABLE | G_PARAM_WRITABLE));
	g_object_class_install_property (G_OBJECT_CLASS (klass), AS_COMPONENT_ID, g_param_spec_string ("id", "id", "id", NULL, G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_READABLE | G_PARAM_WRITABLE));
	g_object_class_install_property (G_OBJECT_CLASS (klass), AS_COMPONENT_NAME, g_param_spec_string ("name", "name", "name", NULL, G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_READABLE | G_PARAM_WRITABLE));
	g_object_class_install_property (G_OBJECT_CLASS (klass), AS_COMPONENT_NAME_ORIGINAL, g_param_spec_string ("name-original", "name-original", "name-original", NULL, G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_READABLE | G_PARAM_WRITABLE));
	g_object_class_install_property (G_OBJECT_CLASS (klass), AS_COMPONENT_SUMMARY, g_param_spec_string ("summary", "summary", "summary", NULL, G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_READABLE | G_PARAM_WRITABLE));
	g_object_class_install_property (G_OBJECT_CLASS (klass), AS_COMPONENT_DESCRIPTION, g_param_spec_string ("description", "description", "description", NULL, G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_READABLE | G_PARAM_WRITABLE));
	g_object_class_install_property (G_OBJECT_CLASS (klass), AS_COMPONENT_KEYWORDS, g_param_spec_boxed ("keywords", "keywords", "keywords", G_TYPE_STRV, G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_READABLE | G_PARAM_WRITABLE));
	g_object_class_install_property (G_OBJECT_CLASS (klass), AS_COMPONENT_ICON, g_param_spec_string ("icon", "icon", "icon", NULL, G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_READABLE | G_PARAM_WRITABLE));
	g_object_class_install_property (G_OBJECT_CLASS (klass), AS_COMPONENT_ICON_URL, g_param_spec_string ("icon-url", "icon-url", "icon-url", NULL, G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_READABLE | G_PARAM_WRITABLE));
	g_object_class_install_property (G_OBJECT_CLASS (klass), AS_COMPONENT_URLS, g_param_spec_boxed ("urls", "urls", "urls", G_TYPE_HASH_TABLE, G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_READABLE));
	g_object_class_install_property (G_OBJECT_CLASS (klass), AS_COMPONENT_CATEGORIES, g_param_spec_boxed ("categories", "categories", "categories", G_TYPE_STRV, G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_READABLE | G_PARAM_WRITABLE));
	g_object_class_install_property (G_OBJECT_CLASS (klass), AS_COMPONENT_PROJECT_LICENSE, g_param_spec_string ("project-license", "project-license", "project-license", NULL, G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_READABLE | G_PARAM_WRITABLE));
	g_object_class_install_property (G_OBJECT_CLASS (klass), AS_COMPONENT_PROJECT_GROUP, g_param_spec_string ("project-group", "project-group", "project-group", NULL, G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_READABLE | G_PARAM_WRITABLE));
	g_object_class_install_property (G_OBJECT_CLASS (klass), AS_COMPONENT_DEVELOPER_NAME, g_param_spec_string ("developer-name", "developer-name", "developer-name", NULL, G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_READABLE | G_PARAM_WRITABLE));
	g_object_class_install_property (G_OBJECT_CLASS (klass), AS_COMPONENT_SCREENSHOTS, g_param_spec_boxed ("screenshots", "screenshots", "screenshots", G_TYPE_PTR_ARRAY, G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_READABLE));
}

static void
as_component_instance_init (AsComponent * self)
{
	self->priv = AS_COMPONENT_GET_PRIVATE (self);
}

/**
 * as_component_get_type:
 *
 * Class to store data describing a component in AppStream
 */
GType
as_component_get_type (void)
{
	static volatile gsize as_component_type_id__volatile = 0;
	if (g_once_init_enter (&as_component_type_id__volatile)) {
		static const GTypeInfo g_define_type_info = {
					sizeof (AsComponentClass),
					(GBaseInitFunc) NULL,
					(GBaseFinalizeFunc) NULL,
					(GClassInitFunc) as_component_class_init,
					(GClassFinalizeFunc) NULL,
					NULL,
					sizeof (AsComponent),
					0,
					(GInstanceInitFunc) as_component_instance_init,
					NULL
		};
		GType as_component_type_id;
		as_component_type_id = g_type_register_static (G_TYPE_OBJECT, "AsComponent", &g_define_type_info, 0);
		g_once_init_leave (&as_component_type_id__volatile, as_component_type_id);
	}
	return as_component_type_id__volatile;
}

static void
as_component_get_property (GObject * object, guint property_id, GValue * value, GParamSpec * pspec)
{
	AsComponent * self;
	self = G_TYPE_CHECK_INSTANCE_CAST (object, AS_TYPE_COMPONENT, AsComponent);
	switch (property_id) {
		case AS_COMPONENT_KIND:
			g_value_set_enum (value, as_component_get_kind (self));
			break;
		case AS_COMPONENT_PKGNAMES:
			g_value_set_boxed (value, as_component_get_pkgnames (self));
			break;
		case AS_COMPONENT_ID:
			g_value_set_string (value, as_component_get_id (self));
			break;
		case AS_COMPONENT_NAME:
			g_value_set_string (value, as_component_get_name (self));
			break;
		case AS_COMPONENT_NAME_ORIGINAL:
			g_value_set_string (value, as_component_get_name_original (self));
			break;
		case AS_COMPONENT_SUMMARY:
			g_value_set_string (value, as_component_get_summary (self));
			break;
		case AS_COMPONENT_DESCRIPTION:
			g_value_set_string (value, as_component_get_description (self));
			break;
		case AS_COMPONENT_KEYWORDS:
			g_value_set_boxed (value, as_component_get_keywords (self));
			break;
		case AS_COMPONENT_ICON:
			g_value_set_string (value, as_component_get_icon (self));
			break;
		case AS_COMPONENT_ICON_URL:
			g_value_set_string (value, as_component_get_icon_url (self));
			break;
		case AS_COMPONENT_URLS:
			g_value_set_boxed (value, as_component_get_urls (self));
			break;
		case AS_COMPONENT_CATEGORIES:
			g_value_set_boxed (value, as_component_get_categories (self));
			break;
		case AS_COMPONENT_PROJECT_LICENSE:
			g_value_set_string (value, as_component_get_project_license (self));
			break;
		case AS_COMPONENT_PROJECT_GROUP:
			g_value_set_string (value, as_component_get_project_group (self));
			break;
		case AS_COMPONENT_DEVELOPER_NAME:
			g_value_set_string (value, as_component_get_developer_name (self));
			break;
		case AS_COMPONENT_SCREENSHOTS:
			g_value_set_boxed (value, as_component_get_screenshots (self));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}


static void
as_component_set_property (GObject * object, guint property_id, const GValue * value, GParamSpec * pspec)
{
	AsComponent * self;
	self = G_TYPE_CHECK_INSTANCE_CAST (object, AS_TYPE_COMPONENT, AsComponent);
	switch (property_id) {
		case AS_COMPONENT_KIND:
			as_component_set_kind (self, g_value_get_enum (value));
			break;
		case AS_COMPONENT_PKGNAMES:
			as_component_set_pkgnames (self, g_value_get_boxed (value));
			break;
		case AS_COMPONENT_ID:
			as_component_set_id (self, g_value_get_string (value));
			break;
		case AS_COMPONENT_NAME:
			as_component_set_name (self, g_value_get_string (value));
			break;
		case AS_COMPONENT_NAME_ORIGINAL:
			as_component_set_name_original (self, g_value_get_string (value));
			break;
		case AS_COMPONENT_SUMMARY:
			as_component_set_summary (self, g_value_get_string (value));
			break;
		case AS_COMPONENT_DESCRIPTION:
			as_component_set_description (self, g_value_get_string (value));
			break;
		case AS_COMPONENT_KEYWORDS:
			as_component_set_keywords (self, g_value_get_boxed (value));
			break;
		case AS_COMPONENT_ICON:
			as_component_set_icon (self, g_value_get_string (value));
			break;
		case AS_COMPONENT_ICON_URL:
			as_component_set_icon_url (self, g_value_get_string (value));
			break;
		case AS_COMPONENT_CATEGORIES:
			as_component_set_categories (self, g_value_get_boxed (value));
			break;
		case AS_COMPONENT_PROJECT_LICENSE:
			as_component_set_project_license (self, g_value_get_string (value));
			break;
		case AS_COMPONENT_PROJECT_GROUP:
			as_component_set_project_group (self, g_value_get_string (value));
			break;
		case AS_COMPONENT_DEVELOPER_NAME:
			as_component_set_developer_name (self, g_value_get_string (value));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

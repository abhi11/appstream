/* settings.vala
 *
 * Copyright (C) 2012 Matthias Klumpp
 *
 * Licensed under the GNU General Public License Version 3
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

using GLib;
using Config;

internal static const string DB_SCHEMA_VERSION = "6";

internal static string SOFTWARE_CENTER_DATABASE_PATH; // usually "/var/cache/software-center/xapian"

private static const string APPSTREAM_BASE_PATH = DATADIR + "/share/app-info";

public static const string ICON_PATH = APPSTREAM_BASE_PATH + "/icons";

private static const string APPSTREAM_XML_PATH = APPSTREAM_BASE_PATH + "/xmls";
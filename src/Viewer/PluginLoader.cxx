/*
 * SPDX-FileComment: Load plugins from statically-linked OSG and SimGear.
 * SPDX-FileCopyrightText: Copyright (C) 2024 Fernando García Liñán
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <osgDB/Registry>

// Static linking of OSG needs special macros to force plugins to be included
#ifdef OSG_LIBRARY_STATIC
USE_GRAPHICSWINDOW();
// Image formats
USE_OSGPLUGIN(dds);
USE_OSGPLUGIN(rgb);
USE_OSGPLUGIN(tga);
#ifdef OSG_JPEG_ENABLED
USE_OSGPLUGIN(jpeg);
#endif
#ifdef OSG_PNG_ENABLED
USE_OSGPLUGIN(png);
#endif
#ifdef OSG_TIFF_ENABLED
USE_OSGPLUGIN(tiff);
#endif
// Model formats
USE_OSGPLUGIN(ive);
USE_OSGPLUGIN(osg);
#endif

// Do the same thing with SimGear. A corresponding REGISTER_OSGPLUGIN() must be
// placed on each ReaderWriter.
//
// This is only needed if SimGear is statically linked, but since this is the
// case 99% of the time, just do it (it won't harm the shared case).

// Model formats
USE_OSGPLUGIN(gltf)
USE_OSGPLUGIN(ac)
// Scenery formats
USE_OSGPLUGIN(btg)
USE_OSGPLUGIN(stg)
USE_OSGPLUGIN(spt)

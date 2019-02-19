#ifndef COMMON_CAMERAGROUP_HXX
#define COMMON_CAMERAGROUP_HXX

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef ENABLE_COMPOSITOR
#  include "CameraGroup_compositor.hxx"
#else
#  include "CameraGroup_legacy.hxx"
#endif

#endif /* COMMON_CAMERAGROUP_HXX */

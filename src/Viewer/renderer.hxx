#ifndef COMMON_RENDERER_HXX
#define COMMON_RENDERER_HXX

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef ENABLE_COMPOSITOR
#  include "renderer_compositor.hxx"
#else
#  include "renderer_legacy.hxx"
#endif

#endif /* COMMON_RENDERER_HXX */

#!/bin/sh

if [ "x$1" = "x--full" ]; then
    echo "Running full screen ..."
    shift
    WINDOW=NO
else
    echo "Running in a window, use --full to run full screen."
    WINDOW=YES
fi
echo

if [ $WINDOW = "YES" ]; then
    # in a window (slow hack)
    export MESA_GLX_FX=window

    export SST_VGA_PASS=1
    export SST_NOSHUTDOWN=1

    export SSTV2_VGA_PASS=1
    export SSTV2_NOSHUTDOWN=1
else 
    # full screen
    export MESA_GLX_FX=fullscreen

    unset SST_VGA_PASS
    unset SST_NOSHUTDOWN

    unset SSTV2_VGA_PASS
    unset SSTV2_NOSHUTDOWN
fi

export FX_GLIDE_NO_SPLASH=1
export FX_GLIDE_SWAPINTERVAL=0

export SST_FASTMEM=1
export SST_FASTPCIRD=1
export SST_GRXCLK=57
export SST_GAMMA=1.0
export SST_SCREENREFRESH=60

export SSTV2_FASTMEM=1
export SSTV2_FASTPCIRD=1
export SSTV2_GRXCLK=57
export SSTV2_GAMMA=1.0
export SSTV2_SCREENREFRESH=60

# Enable this if you wand solid vswap - disable to measure speeds
export SST_SWAP_EN_WAIT_ON_VSYNC=0
export SSTV2_SWAP_EN_WAIT_ON_VSYNC=0

# export SST_SWA_EN_WAIT_ON_VSYNC=1
# export SSTV2_SWA_EN_WAIT_ON_VSYNC=1

echo executing $*

$*

// FGTileLoader - Queue scenery tiles for loading.
//
// Written by Bernie Bright, started March 2001.
//
// Copyright (C) 2001  Bernard Bright - bbright@bigpond.net.au
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Id$

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <Main/globals.hxx>
#include "FGTileLoader.hxx"
#include "tileentry.hxx"
#include "tilemgr.hxx"

extern ssgBranch *terrain;
extern ssgBranch *ground;

/**
 * 
 */
FGTileLoader::FGTileLoader()
{
#ifdef ENABLE_THREADS
    // Create and start the loader threads.
    for (int i = 0; i < MAX_THREADS; ++i)
    {
	threads[i] = new LoaderThread(this);
	threads[i]->start();
    }
#endif // ENABLE_THREADS
}

/**
 * Terminate all threads.
 */
FGTileLoader::~FGTileLoader()
{
#ifdef ENABLE_THREADS
    // Wake up its time to die.
    // queue_cond.broadcast();

    for (int i = 0; i < MAX_THREADS; ++i)
    {
	threads[i]->cancel();
	threads[i]->join();
    }    
#endif // ENABLE_THREADS
}

/**
 * 
 */
void
FGTileLoader::add( FGTileEntry* tile )
{
    /**
     * Initialise tile_path here and not in ctor to avoid problems
     * with the initialastion order of global objects.
     */
    static bool beenhere = false;
    if (!beenhere)
    {
	if ( globals->get_fg_scenery() != (string)"" ) {
	    tile_path.set( globals->get_fg_scenery() );
	} else {
	    tile_path.set( globals->get_fg_root() );
	    tile_path.append( "Scenery" );
	}
	beenhere = true;
    }

#ifdef ENABLE_THREADS
    tile_queue.push( tile );
#else
    tile->load( tile_path, true );
    tile->add_ssg_nodes( terrain, ground );
#endif // ENABLE_THREADS
}

/**
 * 
 */
void
FGTileLoader::update()
{
#ifdef ENABLE_THREADS
    mutex.lock();
    frame_cond.signal();
    mutex.unlock();
#endif // ENABLE_THREADS
}


#ifdef ENABLE_THREADS
/**
 * 
 */
void
FGTileLoader::LoaderThread::run()
{
    pthread_cleanup_push( cleanup_handler, loader );
    while ( true ) {
	// Wait for a load request to be placed in the queue.
	FGTileEntry* tile = loader->tile_queue.pop();

        // Wait for the next frame signal before we load a tile from the queue
        // loader->mutex.lock();
        // loader->frame_cond.wait( loader->mutex );
        // loader->mutex.unlock();

  	set_cancel( SGThread::CANCEL_DISABLE );
	tile->load( loader->tile_path, true );
  	set_cancel( SGThread::CANCEL_DEFERRED );

  	FGTileMgr::loaded( tile );
    }
    pthread_cleanup_pop(1);
}

/**
 * Ensure mutex is unlocked.
 */
void 
cleanup_handler( void* arg )
{
    FGTileLoader* loader = (FGTileLoader*) arg;
    loader->mutex.unlock();
}
#endif // ENABLE_THREADS

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
    cond.broadcast();

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
    mutex.lock();
    tile_queue.push( tile );
    // Signal waiting working threads.
    cond.signal();
    mutex.unlock();
#else
    tile->load( tile_path, true );
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
	loader->mutex.lock();
	while (loader->empty())
	{
	    loader->cond.wait( loader->mutex );
	}

	// Have we been canceled - exits if yes.
	//pthread_testcancel();
	if (loader->empty())
	{
	    loader->mutex.unlock();
	    pthread_exit( PTHREAD_CANCELED );
	}

	// Grab the tile to load and release the mutex.
	FGTileEntry* tile = loader->tile_queue.front();
	loader->tile_queue.pop();
	loader->mutex.unlock();

  	set_cancel( SGThread::CANCEL_DISABLE );
	tile->load( loader->tile_path, true );
  	set_cancel( SGThread::CANCEL_DEFERRED );
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

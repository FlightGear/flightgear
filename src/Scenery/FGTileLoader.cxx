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

#include <simgear/compiler.h>

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
#if defined(ENABLE_THREADS) && ENABLE_THREADS
    // Create and start the loader threads.
    for (int i = 0; i < MAX_THREADS; ++i)
    {
	threads[i] = new LoaderThread(this);
	threads[i]->start( 1 );
    }
#endif // ENABLE_THREADS
}

/**
 * Terminate all threads.
 */
FGTileLoader::~FGTileLoader()
{
#if defined(ENABLE_THREADS) && ENABLE_THREADS
    // Wake up its time to die.
    // queue_cond.broadcast();

    for (int i = 0; i < MAX_THREADS; ++i)
    {
	threads[i]->cancel();
	threads[i]->join();
    }    
#endif // ENABLE_THREADS
}


#if 0 // we don't ever want to do this I don't think
/**
 * 
 */
void FGTileLoader::reinit() {
    while ( !tile_load_queue.empty() ) {
	tile_load_queue.pop();
    }
}
#endif


/**
 * 
 */
void
FGTileLoader::add( FGTileEntry* tile )
{
    /**
     * Initialise tile_path here and not in ctor to avoid problems
     * with the initialisation order of global objects.
     */
    static bool beenhere = false;
    if (!beenhere) {
        tile_path = globals->get_fg_scenery();
        beenhere = true;
    }

    tile_load_queue.push( tile );
}

#ifdef WISH_PLIB_WAS_THREADED // but it isn't
/**
 * 
 */
void
FGTileLoader::remove( FGTileEntry* tile )
{
    tile_free_queue.push( tile );
}
#endif

/**
 * 
 */
void
FGTileLoader::update()
{

#if defined(ENABLE_THREADS) && ENABLE_THREADS
    // send a signal to the pager thread that it is allowed to load
    // another tile
    mutex.lock();
    frame_cond.signal();
    mutex.unlock();
#else
    if ( !tile_load_queue.empty() ) {
        // cout << "loading next tile ..." << endl;
        // load the next tile in the queue
        FGTileEntry* tile = tile_load_queue.front();
        tile_load_queue.pop();

        tile->load( tile_path, true );

        FGTileMgr::ready_to_attach( tile );
    }

#ifdef WISH_PLIB_WAS_THREADED // but it isn't
    if ( !tile_free_queue.empty() ) {
        // cout << "freeing next tile ..." << endl;
        // free the next tile in the queue
        FGTileEntry* tile = tile_free_queue.front();
        tile_free_queue.pop();
	tile->free_tile();
	delete tile;
    }
#endif

#endif // ENABLE_THREADS

}


#if defined(ENABLE_THREADS) && ENABLE_THREADS
/**
 * 
 */
void
FGTileLoader::LoaderThread::run()
{
    pthread_cleanup_push( cleanup_handler, loader );
    while ( true ) {
	// Wait for a load request to be placed in the queue.
	FGTileEntry* tile = loader->tile_load_queue.pop();

        // Wait for the next frame signal before we load a tile from the queue
        loader->mutex.lock();
        loader->frame_cond.wait( loader->mutex );
        loader->mutex.unlock();

  	set_cancel( SGThread::CANCEL_DISABLE );
	tile->load( loader->tile_path, true );
  	set_cancel( SGThread::CANCEL_DEFERRED );

  	FGTileMgr::ready_to_attach( tile );

#ifdef WISH_PLIB_WAS_THREADED // but it isn't
	// Handle and pending removals
	while ( !loader->tile_free_queue.empty() ) {
	    // cout << "freeing next tile ..." << endl;
	    // free the next tile in the queue
	    FGTileEntry* tile = loader->tile_free_queue.pop();
	    tile->free_tile();
	    delete tile;
	}
#endif

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

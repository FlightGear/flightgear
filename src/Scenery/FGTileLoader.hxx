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


#ifndef FG_TILE_LOADER_HXX
#define FG_TILE_LOADER_HXX

#include <simgear/bucket/newbucket.hxx>
#include <simgear/misc/sg_path.hxx>

#if defined(ENABLE_THREADS) && ENABLE_THREADS
#  include <simgear/threads/SGThread.hxx>
#  include <simgear/threads/SGQueue.hxx>
#else
#  include <queue>
   SG_USING_STD( queue );
#endif


// Forward reference.
class FGTileEntry;

/**
 * Queues tiles for loading, possibly by a separate thread.
 */
class FGTileLoader
{
public:

    /**
     * Constructor.
     */
    FGTileLoader();

    /**
     * Destructor.
     */
    ~FGTileLoader();

#if 0 // we don't ever want to do this I don't think
    /**
     * Flush anything in pending load queue without doing the work
     * Leave the free queue intact since that's are only record of
     * things we need to remove.
     */
    void reinit();
#endif
    
    /**
     * Add a tile to the end of the load queue.
     * @param tile The tile to be loaded from disk.
     */
    void add( FGTileEntry* tile );

#ifdef WISH_PLIB_WAS_THREADED // but it isn't
    /**
     * Remove a tile from memory.
     * @param tile The tile to be removed from memory.
     */
    void remove( FGTileEntry* tile );
#endif

    /**
     * The tile loader thread will only load one tile per call to the
     * update() method.  This is a way to spread out the work of the
     * tile loader and slow it down so it is less intrusive.  For
     * systems built without thead support this is a no-op.
     */
    void update();

    /**
     * Returns whether the load queue is empty (contains no elements).
     * @return true if load queue is empty otherwise returns false.
     */
    // bool empty() const { return tile_load_queue.empty(); }

private:

private:

#if defined(ENABLE_THREADS) && ENABLE_THREADS
    /**
     * FIFO queue of tiles to load from data files.
     */
    SGBlockingQueue< FGTileEntry * > tile_load_queue;
    // SGBlockingQueue< FGTileEntry * > tile_free_queue;
#else
    queue< FGTileEntry * > tile_load_queue;
    // queue< FGTileEntry * > tile_free_queue;
#endif

    /**
     * Base names of directories containing tile data files.
     */
    string_list tile_path;

#if defined(ENABLE_THREADS) && ENABLE_THREADS
    /**
     * Maximum number of threads to create for loading tiles.
     */
    enum { MAX_THREADS = 1 };

    /**
     * This class represents the thread of execution responsible for
     * loading a tile.
     */
    class LoaderThread : public SGThread
    {
    public:
	LoaderThread( FGTileLoader* l ) : loader(l) {}
	~LoaderThread() {}

	/**
	 * Reads the tile from disk.
	 */
	void run();

    private:
	FGTileLoader* loader;

    private:
	// not implemented.
	LoaderThread();
	LoaderThread( const LoaderThread& );
	LoaderThread& operator=( const LoaderThread& );
    };

    friend class LoaderThread;

    /**
     * Array of loading threads.
     */
    LoaderThread* threads[ MAX_THREADS ];
    
    /**
     * Lock and synchronize access to tile queue.
     */
    SGMutex mutex;
    SGPthreadCond frame_cond;

    /**
     * Thread cleanup handler.
     */
    friend void cleanup_handler( void* );
#endif // ENABLE_THREADS
};

#endif // FG_TILE_LOADER_HXX

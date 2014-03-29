// jpg-httpd.cxx -- FGFS jpg-http interface
//
// Written by Curtis Olson, started June 2001.
//
// Copyright (C) 2001  Curtis L. Olson - http://www.flightgear.org/~curt
//
// Jpeg Image Support added August 2001
//  by Norman Vine - nhv@cape.com
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id$


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>

#include <cstdlib>        // atoi() atof()
#include <cstring>

#include <osgDB/Registry>
#include <osgDB/ReaderWriter>
#include <osgUtil/SceneView>
#include <osgViewer/Viewer>

#include <simgear/debug/logstream.hxx>
#include <simgear/io/iochannel.hxx>
#include <simgear/math/sg_types.hxx>
#include <simgear/props/props.hxx>
#include <simgear/io/sg_netChat.hxx>
#include <simgear/threads/SGThread.hxx>
#include <simgear/threads/SGGuard.hxx>

#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include <Viewer/renderer.hxx>

#include "jpg-httpd.hxx"

#define HTTP_GET_STRING           "GET "
#define HTTP_TERMINATOR             "\r\n"

using std::string;

//////////////////////////////////////////////////////////////
// class CompressedImageBuffer
//////////////////////////////////////////////////////////////

class CompressedImageBuffer : public SGReferenced
{
public:
    CompressedImageBuffer(osg::Image* img) :
        _image(img)
    {
    }
    
    bool compress(const std::string& extension)
    {
        osgDB::ReaderWriter* writer =
            osgDB::Registry::instance()->getReaderWriterForExtension(extension);
        
        std::stringstream outputStream;
        osgDB::ReaderWriter::WriteResult wr;
        wr = writer->writeImage(*_image,outputStream, NULL);
        
        if(wr.success()) {
            _compressedData = outputStream.str();
            return true;
        } else {
            return false;
        }
    }
    
    size_t size() const
    {
        return _compressedData.length();
    }
    
    const char* data() const
    {
        return _compressedData.data();
    }
private:
    osg::ref_ptr<osg::Image> _image;
    std::string _compressedData;
};

typedef SGSharedPtr<CompressedImageBuffer> ImageBufferPtr;

//////////////////////////////////////////////////////////////

class HttpdThread : public SGThread, private simgear::NetChannel
{
public:
    HttpdThread (int port, int frameHz, const std::string& type) :
        _done(false),
        _port(port),
        _hz(frameHz),
        _imageType(type)
    {
    }
    
    bool init();
    void shutdown();
    
    virtual void run();
    
    void setNewFrameImage(osg::Image* frameImage)
    {
        SGGuard<SGMutex> g(_newFrameLock);
        _newFrame = frameImage;
    }
    
    int getFrameHz() const
    { return _hz; }
    
    void setDone()
    {
        _done = true;
    }
private:
    virtual bool writable (void) { return false; }
    
    virtual void handleAccept (void);
    
    bool _done;
    simgear::NetChannelPoller _poller;
    int _port, _hz;
    std::string _imageType;
    
    SGMutex _newFrameLock;
    osg::ref_ptr<osg::Image> _newFrame;
    /// current frame we're serving to new connections
    ImageBufferPtr _currentFrame;
};


///////////////////////////////////////////////////////////////////////////

class WindowCaptureCallback : public osg::Camera::DrawCallback
{
public:
    
    enum Mode
    {
        READ_PIXELS,
        SINGLE_PBO,
        DOUBLE_PBO,
        TRIPLE_PBO
    };
    
    enum FramePosition
    {
        START_FRAME,
        END_FRAME
    };
    
    struct ContextData : public osg::Referenced
    {
        ContextData(osg::GraphicsContext* gc, Mode mode, GLenum readBuffer, HttpdThread* httpd):
            _gc(gc),
            _mode(mode),
            _readBuffer(readBuffer),
            _pixelFormat(GL_RGB),
            _type(GL_UNSIGNED_BYTE),
            _width(0),
            _height(0),
            _currentImageIndex(0),
            _httpd(httpd)
        {
            _previousFrameTick = osg::Timer::instance()->tick();
            
            if (gc->getTraits() && gc->getTraits()->alpha) {
                _pixelFormat = GL_RGBA;
            }
            
            getSize(gc, _width, _height);
            
            // single buffered image
            _imageBuffer.push_back(new osg::Image);
        }
        
        void getSize(osg::GraphicsContext* gc, int& width, int& height)
        {
            if (gc->getTraits())
            {
                width = gc->getTraits()->width;
                height = gc->getTraits()->height;
            }
        }
        
        void readPixels();
        
        typedef std::vector< osg::ref_ptr<osg::Image> >  ImageBuffer;
        
        osg::GraphicsContext*   _gc;
        Mode                    _mode;
        GLenum                  _readBuffer;
        GLenum                  _pixelFormat;
        GLenum                  _type;
        int                     _width;
        int                     _height;
        unsigned int            _currentImageIndex;
        ImageBuffer             _imageBuffer;
        osg::Timer_t            _previousFrameTick;
        HttpdThread*            _httpd;
    };
    
    WindowCaptureCallback(HttpdThread *thread, Mode mode, FramePosition position, GLenum readBuffer):
        _mode(mode),
        _position(position),
        _readBuffer(readBuffer),
        _thread(thread)
    {
    }
    
    FramePosition getFramePosition() const { return _position; }
    
    ContextData* createContextData(osg::GraphicsContext* gc) const
    {
        return new ContextData(gc, _mode, _readBuffer, _thread);
    }
    
    ContextData* getContextData(osg::GraphicsContext* gc) const
    {
        OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_mutex);
        osg::ref_ptr<ContextData>& data = _contextDataMap[gc];
        if (!data) data = createContextData(gc);
        
        return data.get();
    }
    
    virtual void operator () (osg::RenderInfo& renderInfo) const
    {
        glReadBuffer(_readBuffer);
        
        osg::GraphicsContext* gc = renderInfo.getState()->getGraphicsContext();
        osg::ref_ptr<ContextData> cd = getContextData(gc);
        cd->readPixels();
    }
    
    typedef std::map<osg::GraphicsContext*, osg::ref_ptr<ContextData> > ContextDataMap;
    
    Mode                        _mode;
    FramePosition               _position;
    GLenum                      _readBuffer;
    mutable OpenThreads::Mutex  _mutex;
    mutable ContextDataMap      _contextDataMap;
    HttpdThread*                _thread;
};

void WindowCaptureCallback::ContextData::readPixels()
{
    osg::Timer_t n = osg::Timer::instance()->tick();
    double dt = osg::Timer::instance()->delta_s(_previousFrameTick,n);
    double frameInterval = 1.0 / _httpd->getFrameHz();
    if (dt < frameInterval)
        return;
    
    _previousFrameTick = n;
    unsigned int nextImageIndex = (_currentImageIndex+1)%_imageBuffer.size();
    
    int width=0, height=0;
    getSize(_gc, width, height);
    if (width!=_width || _height!=height)
    {
        _width = width;
        _height = height;
    }
    
    osg::Image* image = _imageBuffer[_currentImageIndex].get();
    image->readPixels(0,0,_width,_height,
                      _pixelFormat,_type);
    
    _httpd->setNewFrameImage(image);
    _currentImageIndex = nextImageIndex;
}

osg::Camera* findLastCamera(osgViewer::ViewerBase& viewer)
{
    osgViewer::ViewerBase::Windows windows;
    viewer.getWindows(windows);
    for(osgViewer::ViewerBase::Windows::iterator itr = windows.begin();
        itr != windows.end();
        ++itr)
    {
        osgViewer::GraphicsWindow* window = *itr;
        osg::GraphicsContext::Cameras& cameras = window->getCameras();
        osg::Camera* lastCamera = 0;
        for(osg::GraphicsContext::Cameras::iterator cam_itr = cameras.begin();
            cam_itr != cameras.end();
            ++cam_itr)
        {
            if (lastCamera)
            {
                if ((*cam_itr)->getRenderOrder() > lastCamera->getRenderOrder())
                {
                    lastCamera = (*cam_itr);
                }
                if ((*cam_itr)->getRenderOrder() == lastCamera->getRenderOrder() &&
                    (*cam_itr)->getRenderOrderNum() >= lastCamera->getRenderOrderNum())
                {
                    lastCamera = (*cam_itr);
                }
            }
            else
            {
                lastCamera = *cam_itr;
            }
        }
        
        return lastCamera;
    }
    
    return NULL;
}

//////////////////////////////////////////////////////////////
// class HttpdImageChannel
//////////////////////////////////////////////////////////////

class HttpdImageChannel : public simgear::NetChannel
{
public:
    HttpdImageChannel(SGSharedPtr<CompressedImageBuffer> img) :
        _imageData(img),
        _bytesToSend(0)
    {
    }

    void setMimeType(const std::string& s)
    {
        _mimeType = s;
    }

    virtual void collectIncomingData (const char* s, int n)
    {
        _request += string(s, n);
    }
    
    virtual void handleRead()
    {
        char data[512];
        int num_read = recv(data, 512);
        if (num_read > 0) {
            _request += std::string(data, num_read);
        }
        
        if (_request.find(HTTP_TERMINATOR) != std::string::npos) {
            // have complete first line of request
            if (_request.find(HTTP_GET_STRING) == 0) {
                processRequest();
            }
        }
    }

    virtual void handleWrite()
    {
        sendResponse();
    }
    
private:
    void processRequest();
    void sendResponse();

    std::string _request;
    ImageBufferPtr _imageData;
    std::string _mimeType;
    size_t _bytesToSend;
};

//////////////////////////////////////////////////////////////
// class HttpdThread
//////////////////////////////////////////////////////////////

bool HttpdThread::init()
{
    if (!open()) {
        SG_LOG( SG_IO, SG_ALERT, "Failed to open HttpdImage port.");
        return false;
    }
    
    if (0 != bind( "", _port )) {
        SG_LOG( SG_IO, SG_ALERT, "Failed to bind HttpdImage port.");
        return false;
    }
    
    if (0 != listen( 5 )) {
        SG_LOG( SG_IO, SG_ALERT, "Failed to listen on HttpdImage port.");
        return false;
    }
    
    _poller.addChannel(this);
    
    osgViewer::Viewer* v = globals->get_renderer()->getViewer();
    osg::Camera* c = findLastCamera(*v);
    if (!c) {
        return false;
    }
    
    c->setFinalDrawCallback(new WindowCaptureCallback(this,
                                                      WindowCaptureCallback::READ_PIXELS,
                                                      WindowCaptureCallback::END_FRAME,
                                                      GL_BACK));

    
    SG_LOG(SG_IO, SG_INFO, "HttpdImage server started on port " << _port);
    return true;
}

void HttpdThread::shutdown()
{
    setDone();
    join();
    
    osgViewer::Viewer* v = globals->get_renderer()->getViewer();
    osg::Camera* c = findLastCamera(*v);
    c->setFinalDrawCallback(NULL);
    
    SG_LOG(SG_IO, SG_INFO, "HttpdImage server shutdown on port " << _port);
}

void HttpdThread::run()
{
    while (!_done) {
        _poller.poll(10); // 10msec sleep
        
        // locked section to check for a new raw frame from the callback
        osg::ref_ptr<osg::Image> frameToWriteOut;
        {
            SGGuard<SGMutex> g(_newFrameLock);
            if (_newFrame) {
                frameToWriteOut = _newFrame;
                _newFrame = NULL;
            }
        } // of locked section
        
        // no locking needed on currentFrame; channels run in this thread
        if (frameToWriteOut) {
            _currentFrame = ImageBufferPtr(new CompressedImageBuffer(frameToWriteOut));
            _currentFrame->compress(_imageType);
        }
    } // of thread poll loop
}
    
void HttpdThread::handleAccept (void)
{
    simgear::IPAddress addr;
    int handle = accept ( &addr );
    SG_LOG( SG_IO, SG_DEBUG, "Client " << addr.getHost() << ":" << addr.getPort() << " connected" );
    
    HttpdImageChannel *hc = new HttpdImageChannel(_currentFrame);
    hc->setMimeType("image/" + _imageType);
    hc->setHandle ( handle );
    
    _poller.addChannel( hc );
}

//////////////////////////////////////////////////////////////
// class FGJpegHttpd
//////////////////////////////////////////////////////////////

FGJpegHttpd::FGJpegHttpd( int p, int hz, const std::string& type )
{
    _imageServer.reset(new HttpdThread( p, hz, type ));
}

FGJpegHttpd::~FGJpegHttpd()
{
}

bool FGJpegHttpd::open()
{
    if ( is_enabled() ) {
        SG_LOG( SG_IO, SG_ALERT, "This shouldn't happen, but the channel "
        << "is already in use, ignoring" );
        return false;
    }

    if (!_imageServer->init()) {
        SG_LOG( SG_IO, SG_ALERT, "FGJpegHttpd: failed to init Http sevrer thread");
        return false;
    }
    
    _imageServer->start();
    set_enabled( true );

    return true;
}

bool FGJpegHttpd::process()
{
    return true;
}

bool FGJpegHttpd::close()
{
    _imageServer->shutdown();
    return true;
}

void HttpdImageChannel::processRequest()
{
    std::ostringstream ss;
    _bytesToSend = _imageData->size();
    if (_bytesToSend <= 0) {
        return;
    }

    // assemble HTTP 1.1 headers. Connection: close is important since we
    // don't attempt pipelining at all
    ss << "HTTP/1.1 200 OK" << HTTP_TERMINATOR;
    ss << "Content-Type: " << _mimeType << HTTP_TERMINATOR;
    ss << "Content-Length: " << _bytesToSend << HTTP_TERMINATOR;
    ss << "Connection: close" << HTTP_TERMINATOR;
    ss << HTTP_TERMINATOR; // end of headers
    
    if( getHandle() == -1 )
    {
        SG_LOG( SG_IO, SG_DEBUG, "<<<<<<<<< Invalid socket handle. Ignoring request.\n" );
        return;
    }
    
    // send headers out
    string headersData(ss.str());
    if (send(headersData.c_str(), headersData.length()) <= 0 )
    {
        SG_LOG( SG_IO, SG_DEBUG, "<<<<<<<<< Error to send HTTP response. Ignoring request.\n" );
        return;
    }
    
    _bytesToSend = _imageData->size();
    sendResponse();
}

void HttpdImageChannel::sendResponse()
{
    const char* ptr = _imageData->data();

    ptr += (_imageData->size() - _bytesToSend);
    size_t sent = send(ptr, _bytesToSend);
    if (sent <= 0) {
        SG_LOG( SG_IO, SG_DEBUG, "<<<<<<<<< Error to send HTTP response. Ignoring request.\n" );
        return;
    }
    
    _bytesToSend -= sent;
    if (_bytesToSend == 0) {
        close();
        shouldDelete(); // use NetChannelPoller delete mechanism
    }
}


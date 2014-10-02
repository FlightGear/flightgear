// ScreenshotUriHandler.cxx -- Provide screenshots via http
//
// Started by Curtis Olson, started June 2001.
// osg support written by James Turner
// Ported to new httpd infrastructure by Torsten Dreyer
//
// Copyright (C) 2014  Torsten Dreyer
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

#include "ScreenshotUriHandler.hxx"

#include <osgDB/Registry>
#include <osgDB/ReaderWriter>
#include <osgUtil/SceneView>
#include <osgViewer/Viewer>

#include <simgear/threads/SGQueue.hxx>
#include <simgear/structure/Singleton.hxx>
#include <Main/globals.hxx>
#include <Viewer/renderer.hxx>

#include <queue>
#include <boost/lexical_cast.hpp>

using std::string;
using std::vector;
using std::list;

namespace flightgear {
namespace http {

///////////////////////////////////////////////////////////////////////////

class ImageReadyListener {
public:
  virtual void imageReady(osg::ref_ptr<osg::Image>) = 0;
  virtual ~ImageReadyListener()
  {
  }
};

class StringReadyListener {
public:
  virtual void stringReady(const std::string &) = 0;
  virtual ~StringReadyListener()
  {
  }
};

struct ImageCompressionTask {
  StringReadyListener * stringReadyListener;
  string format;
  osg::ref_ptr<osg::Image> image;

  ImageCompressionTask()
  {
    stringReadyListener = NULL;
  }

  ImageCompressionTask(const ImageCompressionTask & other)
  {
    stringReadyListener = other.stringReadyListener;
    format = other.format;
    image = other.image;
  }

  ImageCompressionTask & operator =(const ImageCompressionTask & other)
  {
    stringReadyListener = other.stringReadyListener;
    format = other.format;
    image = other.image;
    return *this;
  }

};

class ImageCompressor: public OpenThreads::Thread {
public:
  ImageCompressor()
  {
  }
  virtual void run();
  void addTask(ImageCompressionTask & task);
  private:
  typedef SGBlockingQueue<ImageCompressionTask> TaskList;
  TaskList _tasks;
};

typedef simgear::Singleton<ImageCompressor> ImageCompressorSingleton;

void ImageCompressor::run()
{
  osg::ref_ptr<osgDB::ReaderWriter::Options> options = new osgDB::ReaderWriter::Options("JPEG_QUALITY 80 PNG_COMPRESSION 9");

  SG_LOG(SG_NETWORK, SG_DEBUG, "ImageCompressor is running");
  for (;;) {
    ImageCompressionTask task = _tasks.pop();
    SG_LOG(SG_NETWORK, SG_DEBUG, "ImageCompressor has an image");
    if ( NULL != task.stringReadyListener) {
      SG_LOG(SG_NETWORK, SG_DEBUG, "ImageCompressor checking for writer for " << task.format);
      osgDB::ReaderWriter* writer = osgDB::Registry::instance()->getReaderWriterForExtension(task.format);
      if (!writer)
      continue;

      SG_LOG(SG_NETWORK, SG_DEBUG, "ImageCompressor compressing to " << task.format);
      std::stringstream outputStream;
      osgDB::ReaderWriter::WriteResult wr;
      wr = writer->writeImage(*task.image, outputStream, options);

      if (wr.success()) {
        SG_LOG(SG_NETWORK, SG_DEBUG, "ImageCompressor compressed to  " << task.format);
        task.stringReadyListener->stringReady(outputStream.str());
      }
      SG_LOG(SG_NETWORK, SG_DEBUG, "ImageCompressor done for this image" << task.format);
    }
  }
  SG_LOG(SG_NETWORK, SG_DEBUG, "ImageCompressor exiting");
}

void ImageCompressor::addTask(ImageCompressionTask & task)
{
  _tasks.push(task);
}

/**
 * Based on <a href="http://code.google.com/p/osgworks">osgworks</a> ScreenCapture.cpp
 *
 */
class ScreenshotCallback: public osg::Camera::DrawCallback {
public:
  ScreenshotCallback()
      : _min_delta_tick(1.0/8.0)
  {
    _previousFrameTick = osg::Timer::instance()->tick();
  }

  virtual void operator ()(osg::RenderInfo& renderInfo) const
  {
    osg::Timer_t n = osg::Timer::instance()->tick();
    double dt = osg::Timer::instance()->delta_s(_previousFrameTick,n);
    if (dt < _min_delta_tick)
        return;
    _previousFrameTick = n;

    bool hasSubscribers = false;
    {
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_lock);
      hasSubscribers = !_subscribers.empty();

    }
    if (hasSubscribers) {
      osg::ref_ptr<osg::Image> image = new osg::Image;
      const osg::Viewport* vp = renderInfo.getState()->getCurrentViewport();
      image->readPixels(vp->x(), vp->y(), vp->width(), vp->height(), GL_RGB, GL_UNSIGNED_BYTE);
      {
        OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_lock);
        while (!_subscribers.empty()) {
          try {
            _subscribers.back()->imageReady(image);
          }
          catch (...) {
          }
          _subscribers.pop_back();

        }
      }
    }
  }

  void subscribe(ImageReadyListener * subscriber)
  {
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_lock);
    _subscribers.push_back(subscriber);
  }

  void unsubscribe(ImageReadyListener * subscriber)
  {
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_lock);
    _subscribers.remove( subscriber );
  }

private:
  mutable list<ImageReadyListener*> _subscribers;
  mutable OpenThreads::Mutex _lock;
  mutable double _previousFrameTick;
  double _min_delta_tick;
};

///////////////////////////////////////////////////////////////////////////

class ScreenshotRequest: public ConnectionData, public ImageReadyListener, StringReadyListener {
public:
  ScreenshotRequest(const string & window, const string & type, bool stream)
      : _type(type), _stream(stream)
  {
    if ( NULL == osgDB::Registry::instance()->getReaderWriterForExtension(_type))
    throw sg_format_exception("Unsupported image type: " + type, type);

    osg::Camera * camera = findLastCamera(globals->get_renderer()->getViewer(), window);
    if ( NULL == camera)
    throw sg_error("Can't find a camera for window '" + window + "'");

    // add our ScreenshotCallback to the camera
    if ( NULL == camera->getFinalDrawCallback()) {
      //TODO: are we leaking the Callback on reinit?
      camera->setFinalDrawCallback(new ScreenshotCallback());
    }

    _screenshotCallback = dynamic_cast<ScreenshotCallback*>(camera->getFinalDrawCallback());
    if ( NULL == _screenshotCallback)
    throw sg_error("Can't find ScreenshotCallback");

    requestScreenshot();
  }

  virtual ~ScreenshotRequest()
  {
    _screenshotCallback->unsubscribe(this);
  }

  virtual void imageReady(osg::ref_ptr<osg::Image> rawImage)
  {
    // called from a rendering thread, not from the main loop
    ImageCompressionTask task;
    task.image = rawImage;
    task.format = _type;
    task.stringReadyListener = this;
    ImageCompressorSingleton::instance()->addTask(task);
  }

  void requestScreenshot()
  {
    _screenshotCallback->subscribe(this);
  }

  mutable OpenThreads::Mutex _lock;

  virtual void stringReady(const string & s)
  {
    // called from the compressor thread
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_lock);
    _compressedData = s;
  }

  string getScreenshot()
  {
    string reply;
    {
      // called from the main loop
      OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_lock);
      reply = _compressedData;
      _compressedData.clear();
    }
    return reply;
  }

  osg::Camera* findLastCamera(osgViewer::ViewerBase * viewer, const string & windowName)
  {
    osgViewer::ViewerBase::Windows windows;
    viewer->getWindows(windows);

    osgViewer::GraphicsWindow* window = NULL;

    if (false == windowName.empty()) {
      for (osgViewer::ViewerBase::Windows::iterator itr = windows.begin(); itr != windows.end(); ++itr) {
        if ((*itr)->getTraits()->windowName == windowName) {
          window = *itr;
          break;
        }
      }
    }

    if ( NULL == window) {
      if (false == windowName.empty()) {
        SG_LOG(SG_NETWORK, SG_INFO, "requested window " << windowName << " not found, using first window");
      }
      window = *windows.begin();
    }

    SG_LOG(SG_NETWORK, SG_DEBUG, "Looking for last Camera of window '" << window->getTraits()->windowName << "'");

    osg::GraphicsContext::Cameras& cameras = window->getCameras();
    osg::Camera* lastCamera = 0;
    for (osg::GraphicsContext::Cameras::iterator cam_itr = cameras.begin(); cam_itr != cameras.end(); ++cam_itr) {
      if (lastCamera) {
        if ((*cam_itr)->getRenderOrder() > lastCamera->getRenderOrder()) {
          lastCamera = (*cam_itr);
        }
        if ((*cam_itr)->getRenderOrder() == lastCamera->getRenderOrder()
            && (*cam_itr)->getRenderOrderNum() >= lastCamera->getRenderOrderNum()) {
          lastCamera = (*cam_itr);
        }
      } else {
        lastCamera = *cam_itr;
      }
    }

    return lastCamera;
  }

  bool isStream() const
  {
    return _stream;
  }

  const string & getType() const
  {
    return _type;
  }

private:
  string _type;
  bool _stream;
  string _compressedData;
  ScreenshotCallback * _screenshotCallback;
};

ScreenshotUriHandler::ScreenshotUriHandler(const char * uri)
    : URIHandler(uri)
{
}

ScreenshotUriHandler::~ScreenshotUriHandler()
{
  ImageCompressorSingleton::instance()->cancel();
  //ImageCompressorSingleton::instance()->join();
}

const static string KEY("ScreenshotUriHandler::ScreenshotRequest");
#define BOUNDARY "--fgfs-screenshot-boundary"

bool ScreenshotUriHandler::handleGetRequest(const HTTPRequest & request, HTTPResponse & response, Connection * connection)
{
  if (!ImageCompressorSingleton::instance()->isRunning())
  ImageCompressorSingleton::instance()->start();

  string type = request.RequestVariables.get("type");
  if (type.empty()) type = "jpg";

  //  string camera = request.RequestVariables.get("camera");
  string window = request.RequestVariables.get("window");

  bool stream = (false == request.RequestVariables.get("stream").empty());

  SGSharedPtr<ScreenshotRequest> screenshotRequest;
  try {
    SG_LOG(SG_NETWORK, SG_DEBUG, "new ScreenshotRequest("<<window<<","<<type<<"," << stream << ")");
    screenshotRequest = new ScreenshotRequest(window, type, stream);
  }
  catch (sg_format_exception & ex)
  {
    SG_LOG(SG_NETWORK, SG_INFO, ex.getFormattedMessage());
    response.Header["Content-Type"] = "text/plain";
    response.StatusCode = 410;
    response.Content = ex.getFormattedMessage();
    return true;
  }
  catch (sg_error & ex)
  {
    SG_LOG(SG_NETWORK, SG_INFO, ex.getFormattedMessage());
    response.Header["Content-Type"] = "text/plain";
    response.StatusCode = 500;
    response.Content = ex.getFormattedMessage();
    return true;
  }

  if (false == stream) {
    response.Header["Content-Type"] = string("image/").append(type);
    response.Header["Content-Disposition"] = string("inline; filename=\"fgfs-screen.").append(type).append("\"");
  } else {
    response.Header["Content-Type"] = string("multipart/x-mixed-replace; boundary=" BOUNDARY);

  }

  connection->put(KEY, screenshotRequest);
  return false; // call me again thru poll
}

bool ScreenshotUriHandler::poll(Connection * connection)
{

  SGSharedPtr<ConnectionData> data = connection->get(KEY);
  ScreenshotRequest * screenshotRequest = dynamic_cast<ScreenshotRequest*>(data.get());
  if ( NULL == screenshotRequest) return true; // Should not happen, kill the connection

  const string & screenshot = screenshotRequest->getScreenshot();
  if (screenshot.empty()) {
    SG_LOG(SG_NETWORK, SG_DEBUG, "No screenshot available.");
    return false; // not ready yet, call again.
  }

  SG_LOG(SG_NETWORK, SG_DEBUG, "Screenshot is ready, size=" << screenshot.size());

  if (screenshotRequest->isStream()) {
    string s( BOUNDARY "\r\nContent-Type: image/");
    s.append(screenshotRequest->getType()).append("\r\nContent-Length:");
    s += boost::lexical_cast<string>(screenshot.size());
    s += "\r\n\r\n";
    connection->write(s.c_str(), s.length());
  }

  connection->write(screenshot.data(), screenshot.size());

  if (screenshotRequest->isStream()) {
    screenshotRequest->requestScreenshot();
    // continue until user closes connection
    return false;
  }

  // single screenshot, send terminating chunk
  connection->remove(KEY);
  connection->write("", 0);
  return true; // done.
}

} // namespace http
} // namespace flightgear


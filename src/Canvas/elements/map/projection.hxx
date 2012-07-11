/*
 * projection.hxx
 *
 *  Created on: 12.07.2012
 *      Author: tom
 */

#ifndef CANVAS_MAP_PROJECTION_HXX_
#define CANVAS_MAP_PROJECTION_HXX_

const double DEG2RAD = M_PI / 180.0;

namespace canvas
{

  /**
   * Base class for all projections
   */
  class Projection
  {
    public:
      struct ScreenPosition
      {
        ScreenPosition() {}

        ScreenPosition(double x, double y):
          x(x),
          y(y)
        {}

        double x, y;
      };

      virtual ~Projection() {}

      void setScreenRange(double range)
      {
        _screen_range = range;
      }

      virtual ScreenPosition worldToScreen(double x, double y) = 0;

    protected:

      double _screen_range;
  };

  /**
   * Base class for horizontal projections
   */
  class HorizontalProjection:
    public Projection
  {
    public:

      HorizontalProjection():
        _cos_rot(1),
        _sin_rot(0),
        _range(5)
      {
        setScreenRange(200);
      }

      /**
       * Set world position of center point used for the projection
       */
      void setWorldPosition(double lat, double lon)
      {
        _ref_lat = lat * DEG2RAD;
        _ref_lon = lon * DEG2RAD;
      }

      /**
       * Set up heading
       */
      void setOrientation(float hdg)
      {
        hdg *= DEG2RAD;
        _sin_rot = sin(hdg);
        _cos_rot = cos(hdg);
      }

      void setRange(double range)
      {
        _range = range;
      }

      /**
       * Transform given world position to screen position
       *
       * @param lat   Latitude in degrees
       * @param lon   Longitude in degrees
       */
      ScreenPosition worldToScreen(double lat, double lon)
      {
        lat *= DEG2RAD;
        lon *= DEG2RAD;
        ScreenPosition pos = project(lat, lon);
        double scale = _screen_range / _range;
        pos.x *= scale;
        pos.y *= scale;
        return ScreenPosition
        (
          _cos_rot * pos.x - _sin_rot * pos.y,
         -_sin_rot * pos.x - _cos_rot * pos.y
        );
      }

    protected:

      /**
       * Project given geographic world position to screen space
       *
       * @param lat   Latitude in radians
       * @param lon   Longitude in radians
       */
      virtual ScreenPosition project(double lat, double lon) const = 0;

      double  _ref_lat,
              _ref_lon,
              _cos_rot,
              _sin_rot,
              _range;
  };

  /**
   * Sanson-Flamsteed projection, relative to the projection center
   */
  class SansonFlamsteedProjection:
    public HorizontalProjection
  {
    protected:

      virtual ScreenPosition project(double lat, double lon) const
      {
        double d_lat = lat - _ref_lat,
               d_lon = lon - _ref_lon;
        double r = getEarthRadius(lat);

        ScreenPosition pos;

        pos.x = r * cos(lat) * d_lon;
        pos.y = r * d_lat;

        return pos;
      }

      /**
       * Returns Earth radius at a given latitude (Ellipsoide equation with two
       * equal axis)
       */
      float getEarthRadius(float lat) const
      {
        const float rec  = 6378137.f / 1852;      // earth radius, equator (?)
        const float rpol = 6356752.314f / 1852;   // earth radius, polar   (?)

        double a = cos(lat) / rec;
        double b = sin(lat) / rpol;
        return 1.0f / sqrt( a * a + b * b );
      }
  };

} // namespace canvas

#endif /* CANVAS_MAP_PROJECTION_HXX_ */

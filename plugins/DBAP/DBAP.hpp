// PluginDBAP.hpp
// Jacob Sundstrom (jacob.sundstrom@gmail.com)

#pragma once

#include "SC_PlugIn.hpp"
#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/polygon.hpp>

#define MAX_SPEAKERS 50

namespace DBAP {

class DBAP : public SCUnit {
  // for the convex hull
  typedef boost::geometry::model::d2::point_xy<float> point;
  typedef boost::geometry::model::polygon<point> polygon;
  typedef boost::geometry::model::segment<point> segment;

public:
    DBAP(); // constructor

    struct speaker {
      point pos;
      float ampCorrection;
      float gain = 0;
      float weight;
      float dist = 10.0; // distance from the source
    };

    struct convexHullStruct {
      polygon perimeter;
      segment segments[MAX_SPEAKERS];
      point projectedPoint;
      float projectedDist = 0;
      bool isOutside = false;
      float gainCorrection = 1; // gain correction when the source is outside the hull
    };

    int numSpeakers;
    speaker speakers[MAX_SPEAKERS];
    point* sourcePos; // x, y. Default to the origin
    point realSourcePos = point(0,0);
    convexHullStruct convexHull;

    // Destructor
    // ~DBAP();

private:
    // functions
    void next(int nSamples);
    void calcA();
    void calcK();
    void getDists();
    float calcGain(const speaker &speaker);
    bool insideConvexHull(const point &source);
    point getNearestPoint();
    point projectPoint(const point &pos, const segment &seg);

    // convenience functions
    float getX(const point &point) {
      return (float) boost::geometry::get<0>(point);
    }

    float getY(const point &point) {
      return (float) boost::geometry::get<1>(point);
    }

    // Member variables
    float k, a;
    float rolloff, blur;
    float m_fbufnum;
    SndBuf* m_buf;
    float* data;

};

} // namespace DBAP

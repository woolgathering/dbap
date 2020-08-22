// PluginDBAP.hpp
// Jacob Sundstrom (jacob.sundstrom@gmail.com)

#pragma once

#include "SC_PlugIn.hpp"
#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/polygon.hpp>

#define MAX_SPEAKERS 50
#define R_20LOG 0.16609640474 // reciprocal of (20*log10(2))

// #define DefineSimpleCantAliasUnit(name) (*ft->fDefineUnit)(#name, sizeof(name), (UnitCtorFunc)&name##, 0, kUnitDef_CantAliasInputsToOutputs);

namespace DBAP {

class DBAP : public SCUnit {
  // for the convex hull
  typedef boost::geometry::model::d2::point_xy<double> point;
  typedef boost::geometry::model::polygon<point> polygon;
  typedef boost::geometry::model::segment<point> segment;

public:
    DBAP(); // constructor

    struct speaker {
      point pos;
      double gain = 0;
      double weight;
      double projectedDist = 0.f; // distance from the source (if outside the hull, from the projected source)
      double realDist = 0.f; // real distance from the source (realDist=dist if source is inside the hull)
    };

    struct convexHullStruct {
      polygon perimeter;
      segment segments[MAX_SPEAKERS];
      point projectedPoint;
      double projectedDist = 0.0;
    };

    int numSpeakers;
    speaker speakers[MAX_SPEAKERS];
    point realSourcePos = point(0,0);
    point projectedSourcePos = point(0,0);
    convexHullStruct convexHull;
    segment* nearestSegment;
    bool outside = false;

    // Destructor
    // ~DBAP();

private:
    // functions
    void next(int nSamples);
    void calcA();
    void calcK();
    void getDists(bool outsideHull);
    double calcGain(const speaker &speaker, const bool outsideHull);
    double calcGainWithK(const double &dist, const double &weight);
    double calcGainWithoutK(const double &dist, const double &weight);
    bool insideConvexHull(const point &source);
    point getNearestPoint();
    point projectPoint(const point &pos, const segment &seg);
    int checkIfNewArgs(const float &x, const float &y, const float &b, const float &r);
    double scale(const double &realDist, const double &projDist, const double &absProjDist, double maxDist);

    // convenience functions
    double getX(const point &point) {
      return (double) boost::geometry::get<0>(point);
    }
    double getY(const point &point) {
      return (double) boost::geometry::get<1>(point);
    }

    // Member variables
    double k, a;
    double rolloff, blur;
    double sqrtSumOfDists;
    float m_fbufnum;
    SndBuf* m_buf;

};

} // namespace DBAP

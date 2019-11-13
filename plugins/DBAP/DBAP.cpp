// PluginDBAP.cpp
// Jacob Sundstrom (jacob.sundstrom@gmail.com)

#include "SC_PlugIn.hpp"
#include "DBAP.hpp"
#include <cmath>
#include <algorithm>
#include <boost/geometry.hpp>
namespace bg = boost::geometry;

static InterfaceTable* ft;

namespace DBAP {

DBAP::DBAP() {
  auto* unit = this;
  int idx = 0;
  mCalcFunc = make_calc_function<DBAP, &DBAP::next>();

  // get the buffer
  float fbufnum = ZIN0(1);
  uint32 ibufnum = (uint32)fbufnum;
  World *world = unit->mWorld;
  SndBuf *buf;
  if (ibufnum >= world->mNumSndBufs) {
    int localBufNum = ibufnum - world->mNumSndBufs;
    Graph *parent = unit->mParent;
    if(localBufNum <= parent->localBufNum) {
      buf = parent->mLocalSndBufs + localBufNum;
    } else {
      buf = world->mSndBufs;
    }
  } else {
    buf = world->mSndBufs + ibufnum;
  }

  ////////////////////////////////////////////////////////
  // get values from the buffer
  numSpeakers = (int) buf->data[idx++]; // get it only once
  for(int i=0; i<numSpeakers; i++) {
    float x = buf->data[idx++];
    float y = buf->data[idx++];
    speakers[i].pos = point(x, y); // (buf->data[idx++], buf->data[idx++]) results in a reversed array??
  }
  // get the weights
  for(int i=0; i<numSpeakers; i++) {
    speakers[i].weight = buf->data[idx++];
  }
  // get the convex hull
  short int numVert = buf->data[idx++]; // only get it once
  short int lastVert, firstVert; // save some points
  for(int i=0; i<numVert; i++) {
    int vert = buf->data[idx++];
    bg::append(convexHull.perimeter.outer(), speakers[vert].pos); // append a point
    if(i==0) {
      firstVert = vert;
      lastVert = vert;
    } else {
      convexHull.segments[i-1] = segment(speakers[lastVert].pos, speakers[vert].pos);
      lastVert = vert;
    }
  }
  convexHull.segments[numVert-1] = segment(speakers[lastVert].pos, speakers[firstVert].pos); // add the last one
  ////////////////////////////////////////////////////////

  // initialize stuff
  rolloff = in0(4);
  blur = in0(5) + 0.0001;
  bg::assign_values(realSourcePos, in0(2), in0(3));
  calcA();
  calcK();
  getDists(false);
  next(1);
}

/////////////////////////////////////////////
// DBAP functions ///////////////////////////
/////////////////////////////////////////////
// calculate a
void DBAP::calcA() {
  a = rolloff*R_20LOG;
}

// calculate k
void DBAP::calcK() {
  k = 1/sqrtSumOfDists;
}

// get distance between the source and the speakers
void DBAP::getDists(bool outsideHull) {
  float sumOfDists = 0; // reset

  for(int i=0; i<numSpeakers; i++) {
    float xDiff, yDiff;
    xDiff = (float) pow(getX(speakers[i].pos) - getX(realSourcePos), 2);
    yDiff = (float) pow(getY(speakers[i].pos) - getY(realSourcePos), 2);

    speakers[i].realDist = sqrt(xDiff + yDiff + pow(blur, 2));
    sumOfDists += pow(speakers[i].weight, 2) / pow(speakers[i].realDist, 2*a);

    // if it's outside the hull, also get the distances from the projection for biasing
    if(outsideHull) {
      xDiff = (float) pow(getX(speakers[i].pos) - getX(projectedSourcePos), 2);
      yDiff = (float) pow(getY(speakers[i].pos) - getY(projectedSourcePos), 2);
      speakers[i].projectedDist = sqrt(xDiff + yDiff + pow(blur, 2));
    }
  }

  sqrtSumOfDists = sqrt(sumOfDists); // only do it when we need to
}

float DBAP::calcGain(const speaker &speaker, const bool outsideHull) {
  float gain;
  // if outside, bias the amplitudes
  if(outsideHull) {
    // return (calcRealGain(speaker) * calcAbsoluteGain(speaker));
    // return (calcGainWithK(speaker.projectedDist, speaker.weight) * calcGainWithoutK(speaker.realDist, speaker.weight));
    gain = (k*pow(speaker.weight,2)) / pow(speaker.realDist*speaker.projectedDist, a);
  } else {
    // return calcRealGain(speaker);
    gain = calcGainWithK(speaker.realDist, speaker.weight);
  }
  return std::min(gain, 1.f);
}

// calculate the real gain for a speaker
float DBAP::calcGainWithK(const float &dist, const float &weight) {
  float gain = (k*weight) / pow(dist, a);
  if(dist <= 0.f) {
    return 1;
  } else {
    if(gain < 1.f) {
      return gain;
    } else {
      return 1;
    }
  }
}

// get the absolute gain (bias gain for sources outside the hull)
float DBAP::calcGainWithoutK(const float &dist, const float &weight) {
  float gain = weight/pow(dist, a);
  if(gain < 1.f) {
    return gain;
  } else {
    return 1;
  }
}

// do distance manually to each segment and keep the segment of the convex hull which is closest to the source
DBAP::point DBAP::getNearestPoint() {
  float dist;
  float lastDist = 99999.0; // a very big number to check against first

  for(int i=0; i<bg::num_points(convexHull.perimeter)-1; i++) {
    dist = bg::distance(convexHull.segments[i], realSourcePos);
    if(dist < lastDist) {
      nearestSegment = &convexHull.segments[i];
      lastDist = dist;
    }
  }

  // get the projected point
  // dist should get passed out
  return projectPoint(realSourcePos, *nearestSegment);
}

// project a point orthogonally onto a segment and return the closest point
// the MAJOR problem with this method is that an orthogonal segment must be able to be drawn
// from the point to the segment. If not, it returns a point that does not exist on any segment.
DBAP::point DBAP::projectPoint(const point &pos, const segment &seg) {
  float sourceX, sourceY, seg1X, seg1Y, seg2X, seg2Y, frac;

  // make it easier to debug
  sourceX = getX(pos);
  sourceY = getY(pos);
  seg1X = bg::get<0,0>(seg);
  seg1Y = bg::get<0,1>(seg);
  seg2X = bg::get<1,0>(seg);
  seg2Y = bg::get<1,1>(seg);

  // do math
  frac = ((sourceX - seg1X) * (seg2X - seg1X) + (sourceY - seg1Y) * (seg2Y - seg1Y)) /
    (pow(seg2X - seg1X, 2) + pow(seg2Y - seg1Y, 2));

  // this is necessary since the below will instead return a point not on the segment
  // if frac>=1, the nearest point is one of the end points of the segment. Return the nearest one.
  if(frac>=0.f && frac<=1.f) {
    return point(seg1X + (frac *  (seg2X - seg1X)), seg1Y + (frac *  (seg2Y - seg1Y)));
  } else {
    point end1, end2;
    end1 = point(seg1X, seg1Y);
    end2 = point(seg2X, seg2Y);
    if(bg::distance(pos, end1) > bg::distance(pos, end2)) {
      return end2;
    } else {
      return end1;
    }
  }
}

/////////////////////////////////////////////
/////////////////////////////////////////////

void DBAP::next(int nSamples) {
  auto* unit = this;
  const float* input = in(0); // get the mono input signal

  // get stuff
  blur = in0(5) + 0.0001;
  rolloff = in0(4);
  calcA(); // we need a before dists

  // only recalculate when the source moves
  if((getX(realSourcePos) != in0(2)) || (getY(realSourcePos) != in0(3))) {
    bg::assign_values(realSourcePos, in0(2), in0(3));

    // if the source is outside the convex hull...
    if(bg::within(realSourcePos, convexHull.perimeter)) {
      outside = false;
    } else {
      outside = true;
      convexHull.projectedDist = (float) bg::distance(realSourcePos, convexHull.perimeter); // this should be elimiated and instead returned by getNearestPoint()
      projectedSourcePos = getNearestPoint();
    }
  }

  // get this after getDists() since sumOfDists is set inside
  getDists(outside); // get the distances
  calcK();

  // iterate through each speaker BACKWARDS. See https://scsynth.org/t/plugin-multichannel-output/1353/2
  for(short int spkr=numSpeakers-1; spkr>=0; spkr--) {
    float* outbuf = out(spkr);
    float gain, diff, inc;

    // calculate the gain and find the increment;
    gain = calcGain(speakers[spkr], outside);
    diff = gain - speakers[spkr].gain;
    inc = diff/(nSamples-1);

    for(short int i=0; i<nSamples; i++) {
      outbuf[i] = input[i] * (speakers[spkr].gain + (inc*i));
    }
    speakers[spkr].gain = gain; // save the new gain as the old
  }

}

} // namespace DBAP

PluginLoad(DBAPUGens) {
    // Plugin magic
    ft = inTable;
    registerUnit<DBAP::DBAP>(ft, "DBAP");
}

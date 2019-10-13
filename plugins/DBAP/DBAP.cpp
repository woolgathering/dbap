// PluginDBAP.cpp
// Jacob Sundstrom (jacob.sundstrom@gmail.com)

#include "SC_PlugIn.hpp"
#include "DBAP.hpp"
#include <cmath>
#include <iostream>
#include <boost/geometry.hpp>
namespace bg = boost::geometry;

static InterfaceTable* ft;

namespace DBAP {

DBAP::DBAP() {
  auto* unit = this;
  mCalcFunc = make_calc_function<DBAP, &DBAP::next>();
  // GET_BUF

  // get the buffer
  float fbufnum = ZIN0(3);
  if (fbufnum < 0.f) {
      fbufnum = 0.f;
  }
  if (fbufnum != unit->m_fbufnum) {
    uint32 bufnum = (int)fbufnum;
    World* world = unit->mWorld;
    if (bufnum >= world->mNumSndBufs) {
      int localBufNum = bufnum - world->mNumSndBufs;
      Graph* parent = unit->mParent;
      if (localBufNum <= parent->localBufNum) {
        unit->m_buf = parent->mLocalSndBufs + localBufNum;
      } else {
        bufnum = 0;
        unit->m_buf = world->mSndBufs + bufnum;
      }
    } else {
      unit->m_buf = world->mSndBufs + bufnum;
    }
    unit->m_fbufnum = fbufnum;
  }
  SndBuf* buf = unit->m_buf;
  LOCK_SNDBUF(buf);
  data = unit->m_buf->data;

  ////////////////////////////////////////////////////////
  // get values by incrementing the pointer
  numSpeakers = (int) *data++; // get it only once
  for(int i=0; i<numSpeakers; i++) {
    // speakerPos[i][0] = (double) *data++; // x pos
    // speakerPos[i][1] = (double) *data++;  // y pos
    float x = *data++;
    float y = *data++;
    speakers[i].pos = point(x, y); // (*data++, *data++) results in a reversed array??
  };
  // get the weights
  for(int i=0; i<numSpeakers; i++) {
    speakers[i].weight = *data++;
  };
  int numVert = *data++; // only get it once
  for(int i=0; i<numVert; i++) {
    int idx = *data++;
    bg::append(convexHull.perimeter.outer(), speakers[idx].pos);

    if(idx!=0) {
      convexHull.segments[i] = segment(speakers[idx-1].pos, speakers[idx].pos); // make the segments
    };
  };
  ////////////////////////////////////////////////////////

  // print
  std::cout << "numSpeakers: " << numSpeakers << ", ";
  for(int i=0; i<numSpeakers; i++) {
    // std::cout << "Speaker " << i << ": [" << speakerPos[i][0] << ", " << speakerPos[i][1] << "], ";
    // std::cout << i << ": [" << getX(speakerPos[i]) << ", " << getY(speakerPos[i]) << "] weight: " << weights[i] << ", ";
    std::cout << i << ": [" << getX(speakers[i].pos) << ", " << getY(speakers[i].pos) << "] weight: " << speakers[i].weight << ", ";
  };
  std::cout << "Convex Hull: " << bg::wkt(convexHull.perimeter) << "\n"; //getting the vertices back
  // std::cout << "\n" << bg::distance(speakerPos[0], speakerPos[1]) << "\n";


  // initialize stuff
  rolloff = in0(4);
  blur = in0(5);
  bg::set<0>(realSourcePos, in0(1));
  bg::set<1>(realSourcePos, in0(2));
  sourcePtr = &realSourcePos; // use an address
  getDists();



  // std::cout << rolloff << "\n";
  std::cout << "Dists: ";
  for(int i=0; i<numSpeakers; i++) {
    std::cout << speakers[i].dist << ", ";
  };
  std::cout << "\n";


  next(1);
}


/////////////////////////////////////////////
// DBAP functions ///////////////////////////
/////////////////////////////////////////////
// calculate a
void DBAP::calcA() {
  a = 10 * ((-1 * rolloff) / 20);
}

// calculate k
void DBAP::calcK() {
  float sumOfDists = 0;
  for(int i=0; i < numSpeakers; i++) {
    sumOfDists += pow(speakers[i].weight, 2) / pow(speakers[i].dist, 2); // sum
  };
  k = (2*a) / sqrt(sumOfDists);
}

// get distance manually to include the blur parameter
void DBAP::getDists() {
  for(int i=0; i<numSpeakers; i++) {
    float xDiff, yDiff;
    xDiff = (float) pow(getX(speakers[i].pos) - getX(*sourcePtr), 2);
    yDiff = (float) pow(getY(speakers[i].pos) - getY(*sourcePtr), 2);
    speakers[i].dist = sqrt(xDiff + yDiff + pow(blur, 2));
  };
}

// calculate the gain for a speaker
float DBAP::calcGain(const speaker &speaker) {
  float dB = (k * speaker.dist) / pow(speaker.dist, a); // returns amplitude in dB
  return pow(10, dB/20); // convert to linear amplitude
}

// do distance manually to each segment and keep the segment of the convex hull which is closest to the source
DBAP::point DBAP::getNearestPoint() {
  segment* nearestSegment;
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
  // projected = projectedPoint(sourcePtr, nearestSegment);
  // dist should get passed out
  return projectPoint(realSourcePos, *nearestSegment);
}

// project a point orthogonally onto a segment and return the closest point
DBAP::point DBAP::projectPoint(const point &pos, const segment &seg) {
  float sourceX, sourceY, seg1X, seg1Y, seg2X, seg2Y;
  float frac;

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
  return point(seg1X + (frac *  (seg2X - seg1X)), seg1Y + (frac *  (seg2Y - seg1Y)));
}
/////////////////////////////////////////////
/////////////////////////////////////////////

void DBAP::next(int nSamples) {
  auto* unit = this;
  const float* input = in(0); // get the mono input signal

  // get stuff
  blur = in0(5);
  rolloff = in0(4);
  calcA();
  calcK();

  // only recalculate when the source moves
  if((getX(realSourcePos) != in0(1)) || (getY(realSourcePos) != in0(2))) {
    bg::set<0>(realSourcePos, in0(1)); // just use the inputs
    bg::set<1>(realSourcePos, in0(2));
    // std::cout << "Source is now [" << getX(realSourcePos)  << ", " << getY(realSourcePos) << "]\n";

    // if the source is outside the convex hull...
    if(!bg::within(realSourcePos, convexHull.perimeter)) {
      convexHull.isOutside = true;
      convexHull.projectedDist = (float) bg::distance(realSourcePos, convexHull.perimeter); // this should be elimiated
      convexHull.projectedPoint = getNearestPoint();
      sourcePtr = &convexHull.projectedPoint; // get the address instead of copying

      convexHull.gainCorrection = 1/pow(convexHull.projectedDist, 2); // fall off 1/d^2
      if(convexHull.gainCorrection > 1) {
        convexHull.gainCorrection  = 1;
      };

      // std::cout << "nearest point: " << bg::wkt(convexHull.projectedPoint) << "\tdistance: " << convexHull.projectedDist << "\n";
    } else {
      convexHull.isOutside = false;
      sourcePtr = &realSourcePos; // get the address instead of copying
      convexHull.gainCorrection = 1; // reset it to 1
    };

    getDists(); // get distances after we've corrected for anything outside the hull
  };


  // iterate through each speaker
  for(int i=0; i<numSpeakers; i++) {
    float* outbuf = out(i);
    float gain, diff, inc;

    // calculate the gain and find the increment
    gain = calcGain(speakers[i]) * convexHull.gainCorrection;
    diff = gain - speakers[i].gain;
    inc = diff/nSamples;

    for(int j=0; j < nSamples; j++) {
      // float tmpGain = speakers[i].gain + (diff*inc*j);
      // outbuf[j] = tmpGain; // straight gain factor (for debugging)
      outbuf[j] = input[j] * (speakers[i].gain + (diff*inc*j));
    };
    speakers[i].gain = gain; // save the new gain as the old
  }

}

} // namespace DBAP

PluginLoad(DBAPUGens) {
    // Plugin magic
    ft = inTable;
    registerUnit<DBAP::DBAP>(ft, "DBAP");
}

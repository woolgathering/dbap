// PluginDBAP.cpp
// Jacob Sundstrom (jacob.sundstrom@gmail.com)

#include "SC_PlugIn.hpp"
#include "DBAP.hpp"
#include <cmath>
#include <boost/geometry.hpp>
namespace bg = boost::geometry;

static InterfaceTable* ft;

namespace DBAP {

DBAP::DBAP() {
  auto* unit = this;
  int idx = 0;
  mCalcFunc = make_calc_function<DBAP, &DBAP::next>();

  // get the buffer ///////////////////////////
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
  ///////////////////////////////////////////////

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
  blur = in0(5);
  bg::assign_values(realSourcePos, in0(2), in0(3));
  sourcePtr = &realSourcePos; // use an address
  getDists();
  next(1);
}


/////////////////////////////////////////////
// DBAP functions ///////////////////////////
/////////////////////////////////////////////
// calculate a
void DBAP::calcA() {
  a = -10*rolloff*R_20; // simplify
}

// calculate k
void DBAP::calcK() {
  k = (2*a) / sqrtSumOfDists;
}

// get distance manually to include the blur parameter
void DBAP::getDists() {
  sumOfDists = 0; // reset

  for(int i=0; i<numSpeakers; i++) {
    float xDiff, yDiff;
    xDiff = (float) pow(getX(speakers[i].pos) - getX(*sourcePtr), 2);
    yDiff = (float) pow(getY(speakers[i].pos) - getY(*sourcePtr), 2);

    speakers[i].dist = sqrt(xDiff + yDiff + pow(blur, 2));
    sumOfDists += pow(speakers[i].weight, 2) / pow(speakers[i].dist, 2);
    sqrtSumOfDists = sqrt(sumOfDists); // only do it when we need to
  }
}

// calculate the gain for a speaker
float DBAP::calcGain(const speaker &speaker) {
  float dB = (k * speaker.dist) / pow(speaker.dist, a); // returns amplitude in dB
  return pow(10, dB*R_20); // convert to linear amplitude
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

  // get the projected point; dist should get passed out
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

  // only recalculate when the source moves
  if((getX(realSourcePos) != in0(2)) || (getY(realSourcePos) != in0(3))) {
    bg::assign_values(realSourcePos, in0(2), in0(3));

    // if the source is outside the convex hull...
    if(bg::within(realSourcePos, convexHull.perimeter)) {
      sourcePtr = &realSourcePos; // get the address instead of copying
      convexHull.gainCorrection = 1; // reset it to 1
    } else {
      convexHull.projectedDist = (float) bg::distance(realSourcePos, convexHull.perimeter); // this should be elimiated and instead returned by getNearestPoint()
      convexHull.projectedPoint = getNearestPoint();
      sourcePtr = &convexHull.projectedPoint; // get the address instead of copying

      if(convexHull.projectedDist > 5.0) {
        convexHull.gainCorrection = 1/(convexHull.projectedDist - 5.0); // fall off 1/d AFTER 5m
      }
      if(convexHull.gainCorrection > 1) {
        convexHull.gainCorrection  = 1;
      }
    }
    getDists(); // get distances after we've corrected for anything outside the hull
  }

  // get these after getDists() since sumOfDists is set inside
  calcA();
  calcK(); // this has a sqrt inside... see if it can be eliminated with an if

  // iterate through each speaker BACKWARDS. See https://scsynth.org/t/plugin-multichannel-output/1353/2
  for(short int spkr=numSpeakers-1; spkr>=0; spkr--) {
    float* outbuf = out(spkr);
    float gain, diff, inc;

    // calculate the gain and find the increment
    gain = calcGain(speakers[spkr]) * convexHull.gainCorrection;
    diff = gain - speakers[spkr].gain;
    inc = diff/nSamples;

    for(short int i=0; i<nSamples; i++) {
      outbuf[i] = input[i] * (speakers[spkr].gain + (diff*inc*i));
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

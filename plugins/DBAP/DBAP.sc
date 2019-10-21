DBAP : MultiOutUGen {
	*ar { |numSpeakers, source, buff, sourceX = 0, sourceY = 0, rolloff = 0.50118723362727, blur = 0|
		^this.multiNew('audio', numSpeakers, source, buff, sourceX, sourceY, rolloff, blur);
	}

  init {|numSpeakers ... theInputs|
		inputs = theInputs;
		^this.initOutputs(numSpeakers, rate);
	}

  checkInputs {
    ^this.checkValidInputs;
  }

}


DBAPSpeakerArray {
  var <positions, <weights, <convexHull, convexHullVerticies, <buff, <buffArray, sources, window;

  *new {|positions, weights|
    ^super.newCopyArgs(positions, weights).init;
  }

  init {
    var posRound;
    positions = positions ?? {"No positions given".error; ^this};
    weights = weights ?? {Array.fill(positions.size, {1.0})}; // give an array of 1's if no weights are given
    sources = List.new(0); // an optional list for source positions

    // note that because of a precision error, we can use only two digits after the decimal
    // otherwise .indexOfEqual() can return nil values! This needs to be mitigated in the future.
    // it may not present a problem since if we use meters, speakers must be no closer than 1cm!
    posRound = positions.round(0.01);
    convexHull = this.grahamScan2D(posRound); // get the convex hull
    convexHullVerticies = convexHull.collect{|vert| posRound.indexOfEqual(vert.round(0.01))}; // get the speakers that form the hull
    convexHull = convexHullVerticies.collect{|idx| positions[idx]}; // get back our original precision

  }

  makeBuffer {
    var array = [];
    if(buff.isNil) {
      array = array ++ positions.size ++ positions ++ weights ++ convexHull.size ++ convexHullVerticies;
      array = array.flat;
      if(array.size < 32) {
        array = array ++ Array.fill(array.size.nextPowerOfTwo - array.size, {0}); // extend with zeros since SC breaks if size < 32
      };
      buffArray = array.flat; // for debugging
      buff = Buffer.loadCollection(Server.default, array.flat, action: {
        "DBAP buffer loaded".postln;
        }); // make the buffer
    } {
      "Buffer is already loaded!"
    }
  }

  free {
    buff.free;
    buff = nil;
  }

  // a crude (for now) visualizer
  plot {
    var chTrans, posTrans, sourceTrans;

    // sloppy but it's only run once ¯\_(ツ)_/¯
    defer {
      window = Window.new.front;
      window.view.background_(Color.white);

      chTrans = convexHull.collect{|vert| this.correctForPlot(vert)};
      posTrans = positions.collect{|vert| this.correctForPlot(vert)};

      window.drawFunc = {
        // outline the convex hull
        Pen.strokeColor = Color.red;
        Pen.moveTo(chTrans[0]);
        chTrans[1..].do{|vert|
          Pen.lineTo(vert);
        };
        Pen.lineTo(chTrans[0]);
        Pen.stroke;

        // draw the speakers
        Pen.strokeColor = Color.blue;
        posTrans.do{|spkr, i|
          Pen.addOval(Rect(spkr.x-5, spkr.y-5, 10,10));
          Pen.stringAtPoint(i.asString, Point(spkr.x+5, spkr.y+5), Font(size: 10), Color.black);
        };
        Pen.stroke;

        // draw the sources
        Pen.fillColor = Color.green;
        sourceTrans = sources.collect{|source| this.correctForPlot(source)};
        sourceTrans.do{|source, i|
          Pen.addOval(Rect(source.x-5, source.y-5, 10,10));
          Pen.stringAtPoint(i.asString, Point(source.x+5, source.y+5), Font(size: 10), Color.black);
        };
        Pen.fill;

        // put in a scale
        Pen.strokeColor = Color.gray(0.4, 0.5);
        Pen.width = 1;
        Pen.moveTo(Point(0, window.bounds.height/2));
        Pen.lineTo(Point(window.bounds.width, window.bounds.height/2));

        Pen.moveTo(Point(window.bounds.width/2, 0));
        Pen.lineTo(Point(window.bounds.width/2, window.bounds.height));
        Pen.stroke;

        // add a label
        Pen.stringAtPoint("%m x %m".format(window.bounds.width/10, window.bounds.height/10), Point(10,10), Font(size: 12)), Color.black;
      };
      window.refresh;
      ^window;
    };
  }

  correctForPlot {|point|
    point = point*[10,-10];
    point = point + [window.bounds.width/2, window.bounds.height / 2];
    ^point.asPoint
  }

  addSource {|source|
    sources.add(source);
    if(window.notNil) {defer {window.refresh}};
  }

  modifySource {|idx, source|
    if(idx>(sources.size-1)) {
      "No such source exists! Sources: %".format(sources.asArray).error;
    } {
      sources[idx] = source;
      if(window.notNil) {defer {window.refresh}};
    };
  }

  // Graham scan to find the convex hull of a set of points
  // can only be used right now in two dimensions
  grahamScan2D {|speakerPositions|
    var p0, tmpArray, stack, calcDirection, sortByAngle;

    // function to calculate the direction
    calcDirection = {|vec1, vec2, vec3|
      ((vec2[0] - vec1[0])*(vec3[1] - vec1[1])) - ((vec2[1] - vec1[1])*(vec3[0] - vec1[0]));
    };
    // function to sort by angle and remove points with the same angle
    sortByAngle = {|array|
      var angles = [], points = [], combined, dist;
      dist = {|pointA, pointB| ((pointA[0] - pointB[0]).pow(2) + (pointA[1] - pointB[1]).pow(2)).sqrt};
      array.do{|p|
        var ang = atan2(p[1], p[0]), idxOfEqual; // could be made faster by replacing atan2
        if(angles.includes(ang)) {
          // there's already a point with the same angle. Find the farthest one.
          idxOfEqual = angles.indexOf(ang); // get the index of the point with the equal angle
          if(dist.(p0, p) > dist.(p0, points[idxOfEqual])) {
            // if this point is further, replace the old one
            points[idxOfEqual] = p;
          }; // otherwise keep the old one
        } {
          angles = angles.add(ang); // add it
          points = points.add(p); // add it
        }
      };

      // then sort
      combined = points.collect{|p, i| p ++ angles[i]};
      combined.sort{|a, b| a[2] < b[2]}; // sort by angle
      combined.collect{|p| p[..1]}; // keep and return the coordinates only
    };

    // get P0
    p0 = {
      var tmp;
      tmp = speakerPositions.flop[1]; // get the y values
      tmp = tmp.indicesOfEqual(tmp.minItem).collect{|i|
        speakerPositions[i]; // get the right points
      };
      if(speakerPositions .size > 1)
        {tmp[tmp.flop[0].minIndex]} // return the lowest x val
        {speakerPositions[tmp[0]]}; // return the point
    }.value;

    // transpose by P0 (makes getting angles easier)
    tmpArray = speakerPositions.copy; // make a copy so as not to destroy the original
    tmpArray.remove(p0); // remove P0 from the list
    tmpArray = tmpArray.collect{|p| p - p0}; // transpose

    // sort by angle and remove points with the same angle
    tmpArray = sortByAngle.(tmpArray);

    // now we can get the convex hull
    stack = [[0,0], tmpArray[0], tmpArray[1]]; // a stack with P0 and the first two points of the remainder of the points
    tmpArray[2..].do{|point|
      var ang;
      ang = calcDirection.(stack[stack.size-2], stack.last, point);
      while ({ang < 0}) {
        // if ang is less than 0, then the turn is clockwise which is wrong
        stack.pop; // remove the last point
        ang = calcDirection.(stack[stack.size-2], stack.last, point); // recalculate
      };
      stack = stack.add(point); // add the point when the loop terminates (we turn ccw)
    };

    ^stack.collect{|p| p+p0}; // transpose back and return the stack
  }

  /////////////////////////////////////////
  // special getter methods
  /////////////////////////////////////////
  sources {
    ^sources.asArray;
  }

}

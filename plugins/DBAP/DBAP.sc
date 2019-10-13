DBAP : MultiOutUGen {
// DBAP : UGen {
	// *ar { |buf, numSpeakers = 4, sourcePos = 0, rolloff = 0, blur = 0|
	// 	^this.multiNew('audio', buf, numSpeakers, sourcePos, rolloff, blur);
	// }
	*ar { |numSpeakers, source, sourceX = 0, sourceY = 0, buf, rolloff = 0, blur = 0|
		^this.multiNew('audio', numSpeakers, source, sourceX, sourceY, buf, rolloff, blur);
	}

  init { arg numSpeakers ... theInputs;
		inputs = theInputs;
    // check to make sure numSpeakers and the first value in the buffer are the same!!
		^this.initOutputs(numSpeakers, rate);
	}

  checkInputs {
    ^this.checkValidInputs;
  }

}


DBAPSpeakerArray {
  var <positions, <weights, <convexHull, convexHullVerticies, <buff, <buffArray, <sources;

  *new {|positions, weights|
    ^super.newCopyArgs(positions, weights).init;
  }

  init {
    positions = positions ?? {"No positions given".error; ^this};
    weights = weights ?? {Array.fill(positions.size, {1})}; // give an array of 1's if no weights are given
    convexHull = positions.grahamScan2D; // get the convex hull
    convexHullVerticies = convexHull.collect{|vert| positions.indexOfEqual(vert)};
    sources = List.new(0);
  }

  makeBuffer {
    var array = [];
    array = array ++ positions.size ++ positions ++ weights ++ convexHull.size ++ convexHullVerticies;
    buffArray = array;
    buff = Buffer.loadCollection(Server.default, array.flat, action: {
      "DBAP buffer loaded".postln;
    }); // make the buffer
  }

  //
  plot {
    ^this.notYetImplemented;

    // plot a display of the speakers. iterate through the sources and plot them, too.
  }

  addSource {|source|
    sources.add(source);
  }


  sources {
    ^sources.asArray;
  }

}

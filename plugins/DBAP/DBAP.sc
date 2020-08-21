DBAP : MultiOutUGen {
	*ar { |numSpeakers, source, buff, sourceX = 0, sourceY = 0, rolloff = 3, blur = 0|
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
  var <positions, <weights, <convexHull, convexHullVerticies, <buff, <buffArray, sources, window, <scale;
	var chTrans, posTrans;

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
    // convexHull = this.grahamScan2D(posRound); // get the convex hull
    convexHull = posRound.grahamScan2D; // get the convex hull
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
    var sourceTrans;

    // sloppy but it's only run once ¯\_(ツ)_/¯
    defer {
      window = Window.new.front;
      window.view.background_(Color.white);
			scale = 1;

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
        Pen.stringAtPoint("% x %".format((window.bounds.width/10)/scale, (window.bounds.height/10)/scale), Point(10,10), Font(size: 12), Color.black);
      };

			window.view.onResize = this.prOnResize;
      window.refresh;
      ^window;
    };
  }

  correctForPlot {|point|
    point = point*([10,-10]*scale);
    point = point + [window.bounds.width/2, window.bounds.height/2];
    ^point.asPoint;
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

  removeSource {|idx|
    if (idx.isNil) {
      sources.pop; // if no arg, remove the last BoidUnit
      } {
        sources.removeAt(idx); // else, remove at the index
      };
  }

	// scale the plot
	scale_ {|val|
		scale = val;
		chTrans = convexHull.collect{|vert| this.correctForPlot(vert)};
		posTrans = positions.collect{|vert| this.correctForPlot(vert)};
		{window.refresh}.defer;
	}

	prOnResize {
		chTrans = convexHull.collect{|vert| this.correctForPlot(vert)};
		posTrans = positions.collect{|vert| this.correctForPlot(vert)};
		{window.refresh}.defer;
	}

  /////////////////////////////////////////
  // special getter methods
  /////////////////////////////////////////
  sources {
    ^sources.asArray;
  }

}

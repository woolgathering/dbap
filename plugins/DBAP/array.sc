+ Array {

  // Graham scan to find the convex hull of a set of points
  // can only be used right now in two dimensions
  grahamScan2D {
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
      tmp = this.flop[1]; // get the y values
      tmp = tmp.indicesOfEqual(tmp.minItem).collect{|i|
        this[i]; // get the right points
      };
      if(this .size > 1)
        {tmp[tmp.flop[0].minIndex]} // return the lowest x val
        {this[tmp[0]]}; // return the point
    }.value;

    // transpose by P0 (makes getting angles easier)
    tmpArray = this.copy; // make a copy so as not to destroy the original
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

}

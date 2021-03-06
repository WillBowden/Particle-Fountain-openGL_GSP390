#pragma once
#include <GL/freeglut.h>
#include "codegiraffe/v2.h"
#include "codegiraffe/circle.h"

template <typename TYPE>
class Cone {
public:
	V2<TYPE> origin;
	TYPE radius;
private:
	TYPE startAngle;
	TYPE endAngle;
	/** the point at the circle edge of the cone at startAngle, relative to the origin*/
	V2<TYPE> startPoint;
	/** the point at the circle edge of the cone at endAngle, relative to the origin */
	V2<TYPE> endPoint;
public:
	Cone(V2<TYPE> origin, TYPE rad, TYPE startAngle, TYPE endAngle)
		:origin(origin), radius(rad), startAngle(startAngle), endAngle(endAngle)
	{
		startPoint = V2<TYPE>(startAngle) * radius;
		endPoint = V2<TYPE>(endAngle) * radius;
	}

	V2<TYPE> getCenter() const { 
		TYPE totalAngle = endAngle - startAngle;
		TYPE middleAngle = totalAngle / 2 + startAngle;
		V2<TYPE> midRad = V2<TYPE>(middleAngle);
		TYPE rad = radius / 2;
		if (totalAngle > V_PI) {
			rad -= (float)((totalAngle - V_PI) / V_PI * (radius / 2));
		}
		return midRad * rad;
	}

	static bool isInAngle(V2<TYPE> testPoint, V2<TYPE> origin, TYPE startAngle, TYPE endAngle) {
		V2<TYPE> relativeP = p - origin;
		V2<TYPE> startRad = V2<TYPE>(startAngle);
		V2<TYPE> endRad = V2<TYPE>(endAngle);
		return isInAngle(relativeP, startRad, endRad);
	}

	/**
	 * @param relativeP point relative to the origin
	 * @param startRad line from the origin that is CCW from the endRad
	 * @param endRad line from the origin that is CW from the startRad
	 */
	static bool isInAngle(V2<TYPE> relativeP, V2<TYPE> startRad, V2<TYPE> endRad) {
		bool cwStart = relativeP.sign(startRad) <= 0, cwEnd = relativeP.sign(endRad) <= 0;
		return (cwStart && !cwEnd);
	}

	bool isInAngle(V2<TYPE> const & point) const {
		V2<TYPE> relativeP = point - origin;
		return isInAngle(relativeP, startPoint, endPoint);
	}

	bool contains(V2<TYPE> const & p) const { 
		V2<TYPE> relativeP = p - origin;
		if (isInAngle(relativeP, startPoint, endPoint)) {
			TYPE distance = relativeP.magnitude();
			return distance < radius;
		}
		return false;
	}

	int indexOfContained(const V2<TYPE> * points, const int numPoints, const int startIndex) const {
		for (int i = startIndex; i < numPoints; ++i) {
			V2<TYPE> relativeP = points[i] - origin;
			if (isInAngle(relativeP, startPoint, endPoint)) {
				TYPE distance = relativeP.magnitude();
				if (distance < radius)
					return i;
			}
		}
		return -1;
	}

	static int indexOfSmallestPositive(const TYPE * list, const int listSize) {
		int closest = -1;
		for (int i = 0; i < listSize; ++i) {
			if (list[i] >= 0 && ((closest == -1) || (list[i] < list[closest])))
				closest = i;
		}
		return closest;
	}

	bool raycast(V2<TYPE> const & rayStart, V2<TYPE> const & rayDirection,
		TYPE & out_dist, V2<TYPE> & out_point, V2<TYPE> & out_normal) const {
		CircF circ(origin, radius);
		V2<TYPE> arcPoint, arcNormal;
		TYPE distances[3] = { -1, -1, -1 };
		const int numDistances = sizeof(distances) / sizeof(distances[0]);
		bool hit = circ.raycast(rayStart, rayDirection, distances[0], arcPoint, arcNormal);
		V2<TYPE> relativeP = arcPoint - origin;
		if(!hit || !isInAngle(relativeP, startPoint, endPoint)){
			distances[0] = -1;
		}
		TYPE ray, line;
		hit = V2<TYPE>::rayIntersection(rayStart, rayStart+rayDirection, origin, origin + startPoint, ray, line);
		V2<TYPE> startRad_point, startRad_normal;
		if(hit && ray > 0 && line > 0 && line < 1) {
			startRad_point = startPoint * line + origin;
			startRad_normal = -V2<TYPE>(startAngle).perp();
			distances[1] = (rayDirection * ray).magnitude();
		} else distances[1] = -1;
		hit = V2<TYPE>::rayIntersection(rayStart, rayStart+rayDirection, origin, origin + endPoint, ray, line);
		V2<TYPE> endRad_point, endRad_normal;
		if (hit && ray > 0 && line > 0 && line < 1) {
			endRad_point = endPoint * line + origin;
			endRad_normal = V2<TYPE>(endAngle).perp();
			distances[2] = (rayDirection * ray).magnitude();
		} else distances[2] = -1;
		int closest = indexOfSmallestPositive(distances, numDistances);
		if (closest != -1) {
			out_dist = distances[closest];
			switch (closest) {
			case 0:	out_point = arcPoint;	out_normal = arcNormal;	break;
			case 1:	out_point = startRad_point;	out_normal = startRad_normal;	break;
			case 2:	out_point = endRad_point;	out_normal = endRad_normal;	break;
			}
		}
		return closest != -1;
	}

	V2<TYPE> getClosestPointOnEdge(const V2<TYPE> point, V2<TYPE> & out_normal) const {
		CircF circ(origin, radius);
		V2<TYPE> arcPoint = circ.getClosestPointOnEdge(point, out_normal);
		V2<TYPE> relativeP = arcPoint - origin;
		V2<TYPE> startRadCorner = origin + startPoint;
		V2<TYPE> endRadCorner = origin + endPoint;
		V2<TYPE> onStart, onEnd, p;
		bool arcPointValid = isInAngle(relativeP, startPoint, endPoint);
		TYPE line, ray;
		bool startLineHit = V2<TYPE>::rayIntersection(origin, startRadCorner, point, point + startPoint.perp(), line, ray);
		if (startLineHit && line <= 0 || line >= 1) {
			startLineHit = false;
		} else { onStart = origin + startPoint * line; }
		bool endLineHit = V2<TYPE>::rayIntersection(origin, endRadCorner, point, point + endPoint.perp(), line, ray);
		if (endLineHit && line <= 0 || line >= 1) {
			endLineHit = false;
		} else { onEnd = origin + endPoint * line; }
		TYPE distances[] = {
			(point - origin).magnitude(), // origin corner
			(point - startRadCorner).magnitude(), // start corner
			(point - endRadCorner).magnitude(), // end corner
			arcPointValid ? (point - arcPoint).magnitude() : distances[0], // arc line
			startLineHit ? (point - onStart).magnitude() : distances[1], // start line
			endLineHit ? (point - onEnd).magnitude() : distances[2], // end line
		};
		const int numDistances = sizeof(distances) / sizeof(distances[0]);
		int smallest = indexOfSmallestPositive(distances, numDistances);
		switch (smallest) {
		case 0:	out_normal = (point - origin).normal();	p = origin;	break;
		case 1:	out_normal = (point - startRadCorner).normal();	p = startRadCorner;	break;
		case 2:	out_normal = (point - endRadCorner).normal();	p = endRadCorner;	break;
		case 3: p = arcPoint; break;
		case 4: out_normal = -V2<TYPE>(startAngle).perp();	p = onStart;	break;
		case 5: out_normal = V2<TYPE>(endAngle).perp();	p = onEnd;	break;
		}
		return p;
	}
	void glDraw(bool filled) const {
		glDrawCircle(origin, radius, false);
		V2<TYPE> start(startAngle);
		V2<TYPE> points[32];
		const int numPoints = sizeof(points)/sizeof(points[0]);
		TYPE anglePerPoint = (endAngle-startAngle) / (numPoints-1);
		V2<TYPE>::arc(start, V2<TYPE>(anglePerPoint), points, numPoints);
		V2<TYPE> zero = V2<TYPE>::ZERO();
		glPushMatrix();
		glTranslatef(origin.x, origin.y, 0);
		if(!filled) {
			zero.glDrawTo(startPoint);
			zero.glDrawTo(endPoint);
			glScalef(radius, radius, 1);
			glBegin(GL_LINE_STRIP);
		} else {
			glScalef(radius, radius, 1);
			glBegin(GL_TRIANGLE_FAN);
			zero.glVertex();
		}
		for(int i = 0; i < numPoints; ++i) {
			points[i].glVertex();
		}
		glEnd();
		glPopMatrix();
	}

	bool intersectCircle(const V2<TYPE> & circCenter, const TYPE circRadius) const {
		V2<TYPE> delta = circCenter - origin;
		// check the easy stuff: against the 3 corners
		float dist = delta.magnitude();
		if (dist < circRadius)return true;
		V2<TYPE> relativeP = circCenter - origin;
		// if it is in the circle range, check the harder stuff
		if (dist < circRadius + radius) {
			if (isInAngle(relativeP, startPoint, endPoint)) {
				return true;
			}
			V2<TYPE> startperp(startAngle + (float)V_PI/2);
			V2<TYPE> endperp(endAngle + (float)V_PI / 2);
			V2<TYPE> points[] = {
				circCenter + startperp * circRadius,
				circCenter - startperp * circRadius,
				circCenter + endperp * circRadius,
				circCenter - endperp * circRadius,
			};
			const int numPoints = sizeof(points) / sizeof(points[0]);
			for (int i = 0; i < numPoints; ++i) {
				if (isInAngle(points[i] - origin, startPoint, endPoint))
					return true;
			}
		}
		dist = (relativeP - startPoint).magnitude();
		if (dist < circRadius) return true;
		dist = (relativeP - endPoint).magnitude();
		if (dist < circRadius) return true;
		return false;
	}

	bool intersectsBox(Box<TYPE> box) const {
		// check radius intersect, to see if collision is possible
		TYPE boxRad = box.size.magnitude() / 2;
		V2<TYPE> delta = box.center - origin;
		TYPE dist = delta.magnitude();
		if (dist < radius + boxRad) {
			if (box.contains(origin)) return true;
			// check if box corners are in the cone
			const int numPoints = 4;
			V2<TYPE> points[numPoints];
			box.writePoints(points, numPoints);
			// check if cone lines cross box lines
			V2<TYPE> startP = origin + startPoint;
			if (box.contains(startP)) return true;
			V2<TYPE> endP = origin + endPoint;
			if (box.contains(endP)) return true;
			return intersectsPolygonLineList(points, numPoints);
		}
		return false;
	}

	bool intersectsPolygonLineList(const V2<TYPE> * const & points, const int numPoints) const {
		if (indexOfContained(points, numPoints, 0) >= 0) {
			return true;
		}
		V2<TYPE> startP = origin + startPoint;
		V2<TYPE> endP = origin + endPoint;
		for (int i = 0; i < numPoints; ++i) {
			V2<TYPE> polyA = points[i], polyB = points[(i + 1) % numPoints];
			if (intersectsLine(polyA, polyB)) return true;
		}
		return false;
	}

	bool intersectsLine(V2<TYPE> const & a, V2<TYPE> const & b) const {
		V2<TYPE> cross;
		// check if the line is in range of the cone
		if (V2<TYPE>::lineCrossesCircle(a, b, origin, radius, cross) && isInAngle(cross))
			return true;
		// check if the cone ends cross the line
		if (V2<TYPE>::lineIntersection(origin, startPoint, a, b)
		||  V2<TYPE>::lineIntersection(origin, endPoint, a, b)) {
			return true;
		}
		return false;
	}

	private: static bool checkOneCone(Cone<TYPE> const & a, Cone<TYPE> const & b, const float dist, 
		V2f const & startRad, V2f const & endRad) {
		// check if cone b is in cone a's angles
		if (dist < a.radius && isInAngle(b.origin - a.origin, startRad, endRad)) return true;

		V2<TYPE> point;
		point = b.origin + V2<TYPE>(b.startAngle) * b.radius;
		float d = (point - a.origin).magnitude();
		if (d < a.radius && isInAngle(point - a.origin, startRad, endRad)) return true;

		point = b.origin + V2<TYPE>(b.endAngle) * b.radius;
		d = (point - a.origin).magnitude();
		if (d < a.radius && isInAngle(point - a.origin, startRad, endRad)) return true;

		// check if lines are close enough to origins
		if (V2<TYPE>::closestPointOnLine(a.origin, startRad + a.origin, b.origin, point)
			&& b.contains(point)) {
			return true;
		}
		if (V2<TYPE>::closestPointOnLine(a.origin, endRad + a.origin, b.origin, point)
			&& (point - b.origin).magnitude() < b.radius 
			&& b.contains(point)) {
			return true;
		}
		return false;
	}

	public: bool intersectsCone(Cone<TYPE> const & otherCone) const {
		// check radius intersect
		V2<TYPE> delta = otherCone.origin - origin;
		TYPE dist = delta.magnitude();
		if (dist < radius + otherCone.radius) {
			// if they are looking at each other...
			if (isInAngle(otherCone.origin) && otherCone.isInAngle(origin)) return true;
			if (checkOneCone(*this, otherCone, dist, startPoint, endPoint)) {
				return true;
			}
			if (checkOneCone(otherCone, *this, dist, otherCone.startPoint, otherCone.endPoint)) {
				return true;
			}
			if (intersectsLine(otherCone.origin, otherCone.origin + otherCone.startPoint)
			|| intersectsLine(otherCone.origin, otherCone.origin + otherCone.endPoint)) return true;
		}
		return false;
	}
};

typedef Cone<float> ConeF;

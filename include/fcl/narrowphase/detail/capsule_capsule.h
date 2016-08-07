/*
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2011-2014, Willow Garage, Inc.
 *  Copyright (c) 2014-2016, Open Source Robotics Foundation
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of Open Source Robotics Foundation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

/** \author Jia Pan */

#ifndef FCL_NARROWPHASE_DETAIL_CAPSULECAPSULE_H
#define FCL_NARROWPHASE_DETAIL_CAPSULECAPSULE_H

#include "fcl/collision_data.h"

namespace fcl
{

namespace details
{

// Clamp n to lie within the range [min, max]
template <typename Scalar>
Scalar clamp(Scalar n, Scalar min, Scalar max);

// Computes closest points C1 and C2 of S1(s)=P1+s*(Q1-P1) and
// S2(t)=P2+t*(Q2-P2), returning s and t. Function result is squared
// distance between between S1(s) and S2(t)
template <typename Scalar>
Scalar closestPtSegmentSegment(
    Vector3<Scalar> p1, Vector3<Scalar> q1, Vector3<Scalar> p2, Vector3<Scalar> q2,
    Scalar &s, Scalar &t, Vector3<Scalar> &c1, Vector3<Scalar> &c2);

// Computes closest points C1 and C2 of S1(s)=P1+s*(Q1-P1) and
// S2(t)=P2+t*(Q2-P2), returning s and t. Function result is squared
// distance between between S1(s) and S2(t)
template <typename Scalar>
bool capsuleCapsuleDistance(const Capsule<Scalar>& s1, const Transform3<Scalar>& tf1,
          const Capsule<Scalar>& s2, const Transform3<Scalar>& tf2,
          Scalar* dist, Vector3<Scalar>* p1_res, Vector3<Scalar>* p2_res);

//============================================================================//
//                                                                            //
//                              Implementations                               //
//                                                                            //
//============================================================================//

//==============================================================================
template <typename Scalar>
Scalar clamp(Scalar n, Scalar min, Scalar max)
{
  if (n < min) return min;
  if (n > max) return max;
  return n;
}

//==============================================================================
template <typename Scalar>
Scalar closestPtSegmentSegment(
    Vector3<Scalar> p1, Vector3<Scalar> q1, Vector3<Scalar> p2, Vector3<Scalar> q2,
    Scalar &s, Scalar &t, Vector3<Scalar> &c1, Vector3<Scalar> &c2)
{
  const Scalar EPSILON = 0.001;
  Vector3<Scalar> d1 = q1 - p1; // Direction vector of segment S1
  Vector3<Scalar> d2 = q2 - p2; // Direction vector of segment S2
  Vector3<Scalar> r = p1 - p2;
  Scalar a = d1.dot(d1); // Squared length of segment S1, always nonnegative

  Scalar e = d2.dot(d2); // Squared length of segment S2, always nonnegative
  Scalar f = d2.dot(r);
  // Check if either or both segments degenerate into points
  if (a <= EPSILON && e <= EPSILON) {
    // Both segments degenerate into points
    s = t = 0.0;
    c1 = p1;
    c2 = p2;
    Vector3<Scalar> diff = c1-c2;
    Scalar res = diff.dot(diff);
    return res;
  }
  if (a <= EPSILON) {
    // First segment degenerates into a point
    s = 0.0;
    t = f / e; // s = 0 => t = (b*s + f) / e = f / e
    t = clamp(t, (Scalar)0.0, (Scalar)1.0);
  } else {
    Scalar c = d1.dot(r);
    if (e <= EPSILON) {
      // Second segment degenerates into a point
      t = 0.0;
      s = clamp(-c / a, (Scalar)0.0, (Scalar)1.0); // t = 0 => s = (b*t - c) / a = -c / a
    } else {
      // The general nondegenerate case starts here
      Scalar b = d1.dot(d2);
      Scalar denom = a*e-b*b; // Always nonnegative
      // If segments not parallel, compute closest point on L1 to L2 and
      // clamp to segment S1. Else pick arbitrary s (here 0)
      if (denom != 0.0) {
        std::cerr << "denominator equals zero, using 0 as reference" << std::endl;
        s = clamp((b*f - c*e) / denom, (Scalar)0.0, (Scalar)1.0);
      } else s = 0.0;
      // Compute point on L2 closest to S1(s) using
      // t = Dot((P1 + D1*s) - P2,D2) / Dot(D2,D2) = (b*s + f) / e
      t = (b*s + f) / e;

      //
      //If t in [0,1] done. Else clamp t, recompute s for the new value
      //of t using s = Dot((P2 + D2*t) - P1,D1) / Dot(D1,D1)= (t*b - c) / a
      //and clamp s to [0, 1]
      if(t < 0.0) {
        t = 0.0;
        s = clamp(-c / a, (Scalar)0.0, (Scalar)1.0);
      } else if (t > 1.0) {
        t = 1.0;
        s = clamp((b - c) / a, (Scalar)0.0, (Scalar)1.0);
      }
    }
  }
  c1 = p1 + d1 * s;
  c2 = p2 + d2 * t;
  Vector3<Scalar> diff = c1-c2;
  Scalar res = diff.dot(diff);
  return res;
}

//==============================================================================
template <typename Scalar>
bool capsuleCapsuleDistance(const Capsule<Scalar>& s1, const Transform3<Scalar>& tf1,
          const Capsule<Scalar>& s2, const Transform3<Scalar>& tf2,
          Scalar* dist, Vector3<Scalar>* p1_res, Vector3<Scalar>* p2_res)
{

  Vector3<Scalar> p1(tf1.translation());
  Vector3<Scalar> p2(tf2.translation());

  // line segment composes two points. First point is given by the origin, second point is computed by the origin transformed along z.
  // extension along z-axis means transformation with identity matrix and translation vector z pos
  Transform3<Scalar> transformQ1 = tf1 * Translation3<Scalar>(Vector3<Scalar>(0,0,s1.lz));
  Vector3<Scalar> q1 = transformQ1.translation();

  Transform3<Scalar> transformQ2 = tf2 * Translation3<Scalar>(Vector3<Scalar>(0,0,s2.lz));
  Vector3<Scalar> q2 = transformQ2.translation();

  // s and t correspont to the length of the line segment
  Scalar s, t;
  Vector3<Scalar> c1, c2;

  Scalar result = closestPtSegmentSegment(p1, q1, p2, q2, s, t, c1, c2);
  *dist = sqrt(result)-s1.radius-s2.radius;

  // getting directional unit vector
  Vector3<Scalar> distVec = c2 -c1;
  distVec.normalize();

  // extend the point to be border of the capsule.
  // Done by following the directional unit vector for the length of the capsule radius
  *p1_res = c1 + distVec*s1.radius;

  distVec = c1-c2;
  distVec.normalize();

  *p2_res = c2 + distVec*s2.radius;

  return true;
}

} // namespace details

} // namespace fcl

#endif

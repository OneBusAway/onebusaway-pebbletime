#include "location.h"
#include "utility.h"

#define max(x,y) (x > y ? x : y)
#define min(x,y) (x > y ? y : x)

// sll functions not provided by math-sll.h
//   sll slldeg2rad(sll);
//   sll sllabs(sll);
//   sll sllatan2(sll,sll);

sll slldeg2rad(sll deg) {
  return (slldiv(sllmul(deg,CONST_PI), int2sll(180)));
}

sll sllabs(sll x) {
  sll ret = x;
  if (x < CONST_0) {
    ret = sllneg(x);
  }
  return ret;
}

// http://math.stackexchange.com/questions/1098487/atan2-faster-approximation
// atan2(y,x)
// a := min (|x|, |y|) / max (|x|, |y|)
// s := a * a
// r := ((-0.0464964749 * s + 0.15931422) * s - 0.327622764) * s * a + a
// if |y| > |x| then r := 1.57079637 - r
// if x < 0 then r := 3.14159274 - r
// if y < 0 then r := -r
sll sllatan2(sll y, sll x) {
  sll abs_x = sllabs(x);
  sll abs_y = sllabs(y);
  sll maxyx = max(abs_x, abs_y);
  sll minyx = min(abs_x, abs_y);

  sll a = slldiv(minyx, maxyx);
  sll s = sllmul(a, a);
  sll r_1 = sllmul(dbl2sll((-0.0464964749)), s);
  sll r_2 = slladd(r_1, dbl2sll(0.15931422));
  sll r_3 = sllsub(sllmul(r_2, s), dbl2sll(0.327622764));
  sll r = slladd(sllmul(sllmul(r_3, s), a), a);

  if(sllabs(y) > sllabs(x)) {
    r = sllsub(CONST_PI_2, r);
  }
  if(x < CONST_0) {
    r = sllsub(CONST_PI, r);
  }
  if(y < CONST_0) {
    r = sllneg(r);
  }

  return r;
}

// distance() testing - result - pretty good results above 300M distance.
//  - below that, most distances come back as '0 KM'
//
// distance(48.6816800, -121.3098810, 47.6816800, -122.3098810);
// APP_LOG(APP_LOG_LEVEL_INFO, "133.6 KM");
// distance(47.6843, -122.3088840000, 47.6816800, -122.3098810);
// APP_LOG(APP_LOG_LEVEL_INFO, "0.3007 KM");
// distance(47.684112999, -122.3088840000, 47.6816800, -122.3098810);
// APP_LOG(APP_LOG_LEVEL_INFO, "0.2806 KM");
// distance(47.6826800, -122.3088840000, 47.6816800, -122.3098810);
// APP_LOG(APP_LOG_LEVEL_INFO, "0.1339 KM");
// distance(47.6816800, -122.3088840000, 47.6816800, -122.3098810);
// APP_LOG(APP_LOG_LEVEL_INFO, "0.07464 KM");

// Calculates the distance between two GPS lat/lon coordinates
// returns units: KM
// From: http://www.movable-type.co.uk/scripts/latlong.html
sll DistanceBetweenSLL(sll lat1, sll lon1, sll lat2, sll lon2) {  
  sll R = int2sll(6371); // kilometres
  sll lat1_rad = slldeg2rad(lat1);
  sll lon1_rad = slldeg2rad(lon1);
  sll lat2_rad = slldeg2rad(lat2);
  sll lon2_rad = slldeg2rad(lon2);
  sll dist_lat = sllsub(lat2_rad, lat1_rad);
  sll dist_lon = sllsub(lon2_rad, lon1_rad);

  dist_lat = slldiv2(dist_lat);
  dist_lon = slldiv2(dist_lon);

  dist_lat = sllsin(dist_lat);
  dist_lon = sllsin(dist_lon);

  sll a = sllmul(dist_lat, dist_lat);
  sll b = sllmul(dist_lon, dist_lon);
  sll c = sllmul(sllcos(lat1_rad), sllcos(lat2_rad));
  sll d = slladd(a, sllmul(b,c));

  sll d_sqrt = sllsqrt(d);
  sll d_sqrt_1 = sllsqrt(sllsub(CONST_1,d));

  sll result = sllmul2(sllatan2(d_sqrt, d_sqrt_1));

  return sllmul(R,result);
}
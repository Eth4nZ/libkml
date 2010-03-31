// Copyright 2008, Google Inc. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without 
// modification, are permitted provided that the following conditions are met:
//
//  1. Redistributions of source code must retain the above copyright notice, 
//     this list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//  3. Neither the name of Google Inc. nor the names of its contributors may be
//     used to endorse or promote products derived from this software without
//     specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
// WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
// EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
// OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "kml/convenience/convenience.h"
#include <string>
#include "boost/scoped_ptr.hpp"
#include "kml/base/attributes.h"
#include "kml/base/date_time.h"
#include "kml/base/math_util.h"
#include "kml/base/vec3.h"
#include "kml/dom.h"

using kmlbase::Attributes;
using kmlbase::DateTime;
using kmlbase::Vec3;
using kmldom::CoordinatesPtr;
using kmldom::ExtendedDataPtr;
using kmldom::DataPtr;
using kmldom::FeaturePtr;
using kmldom::KmlFactory;
using kmldom::LatLonAltBoxPtr;
using kmldom::LodPtr;
using kmldom::OuterBoundaryIsPtr;
using kmldom::PlacemarkPtr;
using kmldom::PointPtr;
using kmldom::PolygonPtr;
using kmldom::RegionPtr;
using kmldom::TimeStampPtr;

namespace kmlconvenience {

void AddExtendedDataValue(const std::string& name, const std::string& value,
                          FeaturePtr feature) {
  if (!feature) {
    return;
  }
  if (!feature->has_extendeddata()) {
    feature->set_extendeddata(KmlFactory::GetFactory()->CreateExtendedData());
  }
  feature->get_extendeddata()->add_data(CreateDataNameValue(name, value));
}

PlacemarkPtr CreateBasicPolygonPlacemark(
    const kmldom::LinearRingPtr& lr) {
  KmlFactory* factory = KmlFactory::GetFactory();
  OuterBoundaryIsPtr obi = factory->CreateOuterBoundaryIs();
  obi->set_linearring(lr);
  PolygonPtr poly = factory->CreatePolygon();
  poly->set_outerboundaryis(obi);
  PlacemarkPtr placemark = factory->CreatePlacemark();
  placemark->set_geometry(poly);
  return placemark;
}

CoordinatesPtr CreateCoordinatesCircle(double lat, double lng,
                                       double radius, size_t segments) {
  CoordinatesPtr coords = KmlFactory::GetFactory()->CreateCoordinates();
  for (size_t i = 0; i < segments; ++i) {
    coords->add_vec3(kmlbase::LatLngOnRadialFromPoint(lat, lng, radius, i));
  }
  return coords;
}

DataPtr CreateDataNameValue(const std::string& name, const std::string& value) {
  DataPtr data = KmlFactory::GetFactory()->CreateData();
  data->set_name(name);
  data->set_value(value);
  return data;
}

PointPtr CreatePointFromLatLonAtts(const char** atts) {
  boost::scoped_ptr<Attributes> attributes(Attributes::Create(atts));
  if (attributes.get()) {
    double latitude;
    double longitude;
    if (attributes->GetValue("lat", &latitude) &&
        attributes->GetValue("lon", &longitude)) {
      return CreatePointLatLon(latitude, longitude);
    }
  }
  return NULL;
}

PointPtr CreatePointFromVec3(const Vec3& vec) {
  KmlFactory* factory = KmlFactory::GetFactory();
  CoordinatesPtr coordinates = factory->CreateCoordinates();
  if (vec.has_altitude()) {
    coordinates->add_latlngalt(vec.get_latitude(), vec.get_longitude(),
                               vec.get_altitude());
  } else {
    coordinates->add_latlng(vec.get_latitude(), vec.get_longitude());
  }
  PointPtr point = factory->CreatePoint();
  point->set_coordinates(coordinates);
  return point;
}

PointPtr CreatePointLatLon(double lat, double lon) {
  KmlFactory* factory = KmlFactory::GetFactory();
  CoordinatesPtr coordinates = factory->CreateCoordinates();
  coordinates->add_latlng(lat, lon);
  PointPtr point = factory->CreatePoint();
  point->set_coordinates(coordinates);
  return point;
}

// This is a convenience function to create a Point Placemark.
PlacemarkPtr CreatePointPlacemark(const std::string& name, double lat,
                                  double lon) {
  PlacemarkPtr placemark = KmlFactory::GetFactory()->CreatePlacemark();
  placemark->set_name(name);
  placemark->set_geometry(CreatePointLatLon(lat, lon));
  return placemark;
}

// This creates a Region at the given bounding box with the given Lod range.
RegionPtr CreateRegion2d(double north, double south, double east, double west,
                         double minlodpixels, double maxlodpixels) {
  KmlFactory* factory = KmlFactory::GetFactory();
  RegionPtr region = factory->CreateRegion();
  LatLonAltBoxPtr latlonaltbox = factory->CreateLatLonAltBox();
  latlonaltbox->set_north(north);
  latlonaltbox->set_south(south);
  latlonaltbox->set_east(east);
  latlonaltbox->set_west(west);
  LodPtr lod = factory->CreateLod();
  lod->set_minlodpixels(minlodpixels);
  lod->set_maxlodpixels(maxlodpixels);
  region->set_latlonaltbox(latlonaltbox);
  region->set_lod(lod);
  return region;
}

bool GetExtendedDataValue(const FeaturePtr& feature,
                          const std::string& name,
                          std::string* value) {
  if (value && feature->has_extendeddata()) {
    ExtendedDataPtr extendeddata = feature->get_extendeddata();
    for (size_t i = 0; i < extendeddata->get_data_array_size(); ++i) {
      DataPtr data = extendeddata->get_data_array_at(i);
      if (data->has_name() && name == data->get_name()) {
        *value = data->get_value();
        return true;
      }
    }
  }
  return false;
}

void SetExtendedDataValue(const std::string& name, const std::string& value,
                          FeaturePtr feature) {
  if (!feature) {
    return;
  }
  feature->set_extendeddata(KmlFactory::GetFactory()->CreateExtendedData());
  AddExtendedDataValue(name, value, feature);
}

PlacemarkPtr CreatePointPlacemarkWithTimeStamp(const PointPtr& point,
                                               const DateTime& date_time,
                                               const char* style_id) {
  KmlFactory* kml_factory = KmlFactory::GetFactory();
  PlacemarkPtr placemark = kml_factory->CreatePlacemark();
  // <name>
  placemark->set_name(date_time.GetXsdTime());
  // <styleUrl>
  placemark->set_styleurl(std::string("#") + style_id);
  // <TimeStamp>
  TimeStampPtr time_stamp = kml_factory->CreateTimeStamp();
  time_stamp->set_when(date_time.GetXsdDateTime());
  placemark->set_timeprimitive(time_stamp);
  // <ExtendedData>
  AddExtendedDataValue("date", date_time.GetXsdDate(), placemark);
  AddExtendedDataValue("time", date_time.GetXsdTime(), placemark);
  // <Point>
  placemark->set_geometry(point);
  return placemark;
}

void SimplifyCoordinates(const CoordinatesPtr& src,
                         CoordinatesPtr dest, double merge_tolerance) {
  if (!src || !dest) {
    return;
  }
  // Remember the last coordinate.
  Vec3 last_vec;
  for (size_t i = 0; i < src->get_coordinates_array_size(); ++i) {
    // If this is the first tuple, just append it to the result vec.
    if (i == 0) {
      dest->add_vec3(src->get_coordinates_array_at(i));
      last_vec = src->get_coordinates_array_at(i);
      continue;
    }
    // If the distance between the position of the last point and the current
    // point is greater than merge_tolerance, do not append it to the vector.
    if (merge_tolerance > 0.0) {
      Vec3 this_vec = src->get_coordinates_array_at(i);
      if (merge_tolerance >= kmlbase::DistanceBetweenPoints3d(
            last_vec.get_latitude(), last_vec.get_longitude(),
            last_vec.get_altitude(), this_vec.get_latitude(),
            this_vec.get_longitude(), this_vec.get_altitude())) {
        last_vec = src->get_coordinates_array_at(i);
        continue;
      }
    }
    last_vec = src->get_coordinates_array_at(i);
    dest->add_vec3(src->get_coordinates_array_at(i));
  }
}

}  // end namespace kmlconvenience
/*****************************************************************************
 *
 * This file is part of Mapnik (c++ mapping toolkit)
 *
 *  Copyright (C) 2013 Artem Pavlenko
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *****************************************************************************/
#ifndef FILE_DATASOURCE_HPP
#define FILE_DATASOURCE_HPP

#include "mongo_featureset.hpp"
#include "mongo_connection_manager.hpp"
// mapnik
#include <mapnik/datasource.hpp>
#include <mapnik/params.hpp>
#include <mapnik/query.hpp>
#include <mapnik/feature.hpp>
#include <mapnik/box2d.hpp>
#include <mapnik/coord.hpp>
#include <mapnik/feature_layer_desc.hpp>

// boost
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>

// stl
#include <string>

using mapnik::transcoder;
using mapnik::datasource;
using mapnik::box2d;
using mapnik::layer_descriptor;
using mapnik::featureset_ptr;
using mapnik::feature_ptr;
using mapnik::query;
using mapnik::parameters;
using mapnik::coord2d;


class mongo_datasource : public mapnik::datasource
{ 
public:
    // constructor
#if (MAPNIK_VERSION > 220)
    mongo_datasource(mapnik::parameters const& params);
#else
    mongo_datasource(mapnik::parameters const& params,bool bind);
#endif
    // destructor
    virtual ~mongo_datasource ();

    void bind() const;
#if (MAPNIK_VERSION > 211)
    static const char* name();
    mapnik::datasource::datasource_t type() const;
#else
    static std::string name();
    int type() const;
#endif
    mapnik::featureset_ptr features(const mapnik::query& q) const;
    mapnik::featureset_ptr features_at_point(mapnik::coord2d const& pt) const;
    mapnik::box2d<double> envelope() const;
#if (MAPNIK_VERSION > 211)
    boost::optional<mapnik::datasource::geometry_t> get_geometry_type() const;
#endif
    mapnik::layer_descriptor get_descriptor() const;


private:
    //type_, extent_, and desc_
    mongo_connector<mongo_connection> connector_;
    const std::string geometry_table_;
#if (MAPNIK_VERSION > 211)
    mutable mapnik::datasource::datasource_t type_;
#else
    int type_;
#endif
    mutable mapnik::layer_descriptor desc_;
    mutable bool extent_initialized_;
    mutable mapnik::box2d<double> max_extent_;
    
};


#endif // FILE_DATASOURCE_HPP

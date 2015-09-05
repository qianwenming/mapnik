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
#include "mongo_featureset.hpp"
#include "time_date.h"
// mapnik
#include <mapnik/geometry.hpp>
#include <mapnik/feature.hpp>
#include <mapnik/global.hpp>
#include <mapnik/wkb.hpp>
#include <mapnik/unicode.hpp>
#include <mapnik/feature_factory.hpp>
#include <mapnik/global.hpp> // for int2net

//std
#include <string>
#include <set>
#include <list>

// boost
#include <boost/make_shared.hpp>
#include <boost/scoped_array.hpp>
#include <boost/iostreams/operations.hpp>

#include <sys/time.h> 
#include <time.h>

using mapnik::feature_ptr;
using mapnik::geometry_type;
using mapnik::byte;
using mapnik::geometry_utils;
using mapnik::feature_factory;

using namespace mongo;

mongo_featureset::mongo_featureset(
	boost::shared_ptr<mongo::DBClientCursor> const &rs,
    std::string const& encoding,
	mapnik::box2d<double> const& box,
	boost::shared_ptr<mongo_connection> conn,
	boost::shared_ptr<Pool<mongo_connection,mongo_connector> > pool)
  : rs_(rs),
    tr_(new mapnik::transcoder(encoding)),
    feature_id_(0),
    feature_index_(0),
	m_conn_(conn),
	m_pool_(pool)
{
#if (MAPNIK_VERSION > 211)
	mapnik::context_ptr ctx = boost::make_shared<mapnik::context_type>(); 
#endif

#ifdef POINT_VACUATE
	init_grid_map(box);
#endif

}

mongo_featureset::~mongo_featureset() 
{
	m_pool_->returnObject(m_conn_);
}

void mongo_featureset::prepare_next_bacth()
{
    int 			index = 0;
    BSONObj 		bsonObj; 
    BSONElement 	element;
    std::string		name;

    BSONElement         eGeomType;
    std::string		strGeomType;
    BSONElement 	coordinates;
    BSONElement		coordArray;
    BSONElement 	coord;
 
    while(index < MAX_STFEATURE_COUNT)
    {    
	if(!rs_->more())
	{
	     feature_batch_[index] = mapnik::feature_ptr();
	     break;
	}       

	#if (MAPNIK_VERSION > 211)
            mapnik::feature_ptr feature = feature_factory::create(ctx_,feature_id_++);
        #else
            mapnik::feature_ptr feature = feature_factory::create(feature_id_++);
        #endif

	bsonObj = rs_->next();
	BSONObjIterator itor = bsonObj.begin();
        while(itor.more())
	{
	    element = itor.next();
	    if(element.isNull() == true)
		continue;

	    name = element.fieldName();
	    if(name == "_id")
		continue;

	    switch((int)element.type())
            {
            case NumberDouble:  //BSONType::NumberDouble
                {
                    double value = element.numberDouble();
                #if (MAPNIK_VERSION > 211)
                    feature->put_new(name,&value);
                #else
                    boost::put(*feature,name,value);
                #endif
                }
                break;
            case String:        //BSONType::String
                {
		    UnicodeString value = tr_->transcode(element.valuestr());
                 #if (MAPNIK_VERSION > 211)
                    feature->put_new(name,value);
                 #else
                    boost::put(*feature,name,value);
                 #endif
                }
                break;
            case Bool:          //BSONType::Bool
                {
                    bool value = element.Bool();
                 #if (MAPNIK_VERSION > 211)
                     feature->put_new(name,value);
                 #else
                     boost::put(*feature,name,value);
                 #endif
                }
                break;
            case Date:          //BSONType::Date
                { 
                    Date_t value = element.Date();
                 #if (MAPNIK_VERSION > 211)
                     feature->put_new(name,&value);
                 #else
                     boost::put(*feature,name,&value);
                 #endif
                }
                break;
	    case NumberInt:     //BSONType::NumberInt
                {
                    int value = element.numberInt();
                 #if (MAPNIK_VERSION > 211)
                     feature->put_new(name,&value);
                 #else
                     boost::put(*feature,name,&value);
                 #endif
                }
                break;
            case NumberLong:    //BSONType::NumberLong
                {
                    long long value = element.numberLong();
                 #if (MAPNIK_VERSION > 211)
                     feature->put_new(name,&value);
                 #else
                     boost::put(*feature,name,&value);
                 #endif
                }
                break;
	    case Object:
		{
		    eGeomType = element["type"];
		    if(eGeomType.isNull() == true)
		    	break;

		    coordinates = element["coordinates"];
		    if(coordinates.isNull() == true)
			break;
		    
		    strGeomType = eGeomType.String();
		    if (strGeomType == "Point")
		    {
				double x = coordinates["0"].numberDouble();
				double y = coordinates["1"].numberDouble();
		    #ifdef POINT_VACUATE
				if(vacuate_point(x,y) == false)
					break;
		    #endif
				mapnik::geometry_type *point = new mapnik::geometry_type(mapnik::Point);
		 		point->move_to(x,y); 
				feature->add_geometry(point);
		    }
		    else if (strGeomType == "LineString")
		    {
                	mapnik::geometry_type *line = new mapnik::geometry_type(mapnik::LineString);
                	BSONObjIterator iter(coordinates.embeddedObject());
			if(iter.more())
                        {
                            coord = iter.next();
			    line->move_to(coord["0"].numberDouble(),coord["1"].numberDouble());
			}

                	while(iter.more())
                	{
                    	    coord = iter.next();
			    line->line_to(coord["0"].numberDouble(),coord["1"].numberDouble());
               	        }

                	feature->add_geometry(line);
		    }
		    else if (strGeomType == "Polygon")
		    {
                        mapnik::geometry_type *polygon = new mapnik::geometry_type(mapnik::Polygon);
                        BSONObjIterator iter(coordinates.embeddedObject());
			while(iter.more())
			{
			    coordArray = iter.next();
			    BSONObjIterator it(coordArray.embeddedObject());
                            if(it.more())
                            {
                                coord = it.next();
                                polygon->move_to(coord["0"].numberDouble(),coord["1"].numberDouble());
                            }

                            while(it.more())
                            {
                                coord = it.next();
                                polygon->line_to(coord["0"].numberDouble(),coord["1"].numberDouble());
                            }
			}

			feature->add_geometry(polygon);
		    }
		}
	    }
	}

        feature_batch_[index] = feature;
	index++;
    }
    
    return;
}

mapnik::feature_ptr mongo_featureset::next()
{
	BSONObj 		bsonObj; 
	BSONElement 	element;
	std::string		name;

	BSONElement         eGeomType;
	std::string		strGeomType;
	BSONElement 	coordinates;
	BSONElement		coordArray;
	BSONElement 	coord;

#ifdef POINT_VACUATE
	init_grid_map(box);
#endif
	   
	if(!rs_->more())
	{
		return mapnik::feature_ptr();
	}       

#if (MAPNIK_VERSION > 211)
	mapnik::feature_ptr feature = feature_factory::create(ctx_,feature_id_++);
#else
	mapnik::feature_ptr feature = feature_factory::create(feature_id_++);
#endif
	//time_date get_time;
	bsonObj = rs_->next();
	BSONObjIterator itor = bsonObj.begin();
	while(itor.more())
	{
		element = itor.next();
		if(element.isNull() == true)
			continue;

		name = element.fieldName();
		if(name == "_id")
			continue;

		switch((int)element.type())
		{
		case NumberDouble:  //BSONType::NumberDouble
			{
				double value = element.numberDouble();
			#if (MAPNIK_VERSION > 211)
				feature->put_new(name,&value);
			#else
				boost::put(*feature,name,value);
			#endif
			}
			break;
		case String:        //BSONType::String
			{
				UnicodeString value = tr_->transcode(element.valuestr());
			#if (MAPNIK_VERSION > 211)
				feature->put_new(name,value);
			#else
				boost::put(*feature,name,value);
			#endif
			}
			break;
		case Bool:          //BSONType::Bool
			{
				bool value = element.Bool();
			#if (MAPNIK_VERSION > 211)
				feature->put_new(name,value);
			#else
				boost::put(*feature,name,value);
			#endif
			}
			break;
		case Date:          //BSONType::Date
			{ 
				Date_t value = element.Date();
			#if (MAPNIK_VERSION > 211)
				feature->put_new(name,&value);
			#else
				boost::put(*feature,name,&value);
			#endif
			}
			break;
		case NumberInt:     //BSONType::NumberInt
			{
				int value = element.numberInt();
			#if (MAPNIK_VERSION > 211)
				feature->put_new(name,&value);
			#else
				boost::put(*feature,name,&value);
			#endif
			}
			break;
		case NumberLong:    //BSONType::NumberLong
			{
				long long value = element.numberLong();
			#if (MAPNIK_VERSION > 211)
				feature->put_new(name,&value);
			#else
				boost::put(*feature,name,&value);
			#endif
			}
			break;
		case Object:
			{
				eGeomType = element["type"];
				if(eGeomType.isNull() == true)
					break;

				coordinates = element["coordinates"];
				if(coordinates.isNull() == true)
					break;

				strGeomType = eGeomType.String();
				if (strGeomType == "Point")
				{
					double x = coordinates["0"].numberDouble();
					double y = coordinates["1"].numberDouble();
				#ifdef POINT_VACUATE
					if(vacuate_point(x,y) == false)
						break;
				#endif
					mapnik::geometry_type *point = new mapnik::geometry_type(mapnik::Point);
					point->move_to(x,y); 
					feature->add_geometry(point);
				}
				else if (strGeomType == "LineString")
				{
					mapnik::geometry_type *line = new mapnik::geometry_type(mapnik::LineString);
					BSONObjIterator iter(coordinates.embeddedObject());
					if(iter.more())
					{
						coord = iter.next();
						line->move_to(coord["0"].numberDouble(),coord["1"].numberDouble());
					}
					while(iter.more())
					{
						coord = iter.next();
						line->line_to(coord["0"].numberDouble(),coord["1"].numberDouble());
					}
					feature->add_geometry(line);
				}
				else if (strGeomType == "Polygon")
				{
					mapnik::geometry_type *polygon = new mapnik::geometry_type(mapnik::Polygon);
					BSONObjIterator iter(coordinates.embeddedObject());
					while(iter.more())
					{
						coordArray = iter.next();
						BSONObjIterator it(coordArray.embeddedObject());
						if(it.more())
						{
							coord = it.next();
							polygon->move_to(coord["0"].numberDouble(),coord["1"].numberDouble());
						}

						while(it.more())
						{
							coord = it.next();
							polygon->line_to(coord["0"].numberDouble(),coord["1"].numberDouble());
						}	
					}

					feature->add_geometry(polygon);
				}
			}
		}
	}
	//get_time.elapsed("get data");

	return feature;
}

void mongo_featureset::init_grid_map(mapnik::box2d<double> const &box)
{

    x_ = box.minx();
    y_ = box.miny();
    hdis = (box.maxx() - box.minx())/GRID_SPLIT_H;
    vdis = (box.maxy() - box.miny())/GRID_SPLIT_V;
    for(int i=0; i<MAX_GRID_COUNT; i++)
    {
	grid_map_.insert(pair<int,int>(i,0));
    }
}

bool mongo_featureset::vacuate_point(double &x, double &y)
{
    double difx = x - x_;
    double dify = y - y_;
    if(difx < 0.0 || dify < 0.0)
	return false;

    int hidx = (int)(difx/hdis);
    int vidx = (int)(dify/vdis);
    int gidx = hidx*GRID_SPLIT_H + vidx;	
  
    std::map<int,int>::iterator iter = grid_map_.find(gidx);
    if(iter == grid_map_.end())
	return false;

    if(iter->second > GRID_POINT_NUM)
	return false;

    iter->second++;
    return true;  
}

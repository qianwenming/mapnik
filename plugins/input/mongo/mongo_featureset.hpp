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
#ifndef mongo_FEATURESET_HPP
#define mongo_FEATURESET_HPP

#include <semaphore.h>  
#include <pthread.h>  
#include <vector>
// mapnik
#include <mapnik/datasource.hpp>
#include <mapnik/geometry.hpp>
#include <mapnik/feature.hpp>
#include <mapnik/pool.hpp>
// boost
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp> // needed for wrapping the transcoder
// mongo
#include <mongo/client/dbclient.h>
#include <mongo/bson/bsonobjiterator.h>

#define MAPNIK_VERSION 211
//#define MAPNIK_VERSION 212
//#define POINT_VACUATE


#define BATCH_SIZE	        202
#define MAX_STFEATURE_COUNT   	3*BATCH_SIZE

#define GRID_SPLIT_H		32
#define GRID_SPLIT_V		32
#define GRID_POINT_NUM		1
#define MAX_GRID_COUNT 		GRID_SPLIT_H*GRID_SPLIT_V

#include "mongo_connection_manager.hpp"
#include "mongo_connection.hpp"

using mapnik::PoolGuard;

// extend the mapnik::Featureset defined in include/mapnik/datasource.hpp
class mongo_featureset : public mapnik::Featureset
{
public:
	// this constructor can have any arguments you need
	mongo_featureset(boost::shared_ptr<mongo::DBClientCursor> const &rs,
		std::string const& encoding, 
		mapnik::box2d<double> const& box,
		boost::shared_ptr<mongo_connection> conn,
		boost::shared_ptr< Pool<mongo_connection,mongo_connector> > pool);
	// desctructor
	virtual ~mongo_featureset();

private:
	mapnik::feature_ptr next();
	void prepare_next_bacth();
	void init_grid_map(mapnik::box2d<double> const &box);
	bool vacuate_point(double &x, double &y);

private:
#if (MAPNIK_VERSION > 211)
	mapnik::context_ptr ctx_;
#endif

	boost::shared_ptr<mongo::DBClientCursor> rs_;
	boost::scoped_ptr<mapnik::transcoder> tr_;

	int feature_id_;
	int feature_index_;
	mapnik::feature_ptr feature_batch_[MAX_STFEATURE_COUNT];

	std::map<int,int> 	grid_map_;
	mapnik::box2d<double> const     box;
	double 			x_;
	double			y_;
	double 			hdis;
	double			vdis;
	boost::shared_ptr<mongo_connection> m_conn_;
	boost::shared_ptr<Pool<mongo_connection,mongo_connector> > m_pool_;
	//PoolGuard<boost::shared_ptr<mongo_connection>,boost::shared_ptr<Pool<mongo_connection,mongo_connector> > > guard_;
};

#endif // mongo_FEATURESET_HPP

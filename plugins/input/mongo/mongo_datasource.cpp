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
#include "mongo_datasource.hpp"
#include "time_date.h"

// mapnik
#include <mapnik/global.hpp>
#include <mapnik/sql_utils.hpp>
#include <mapnik/timer.hpp>

// boost
#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>
#include <boost/format.hpp>
#include <boost/make_shared.hpp>


#include <sys/time.h> 
#include <time.h>

using boost::shared_ptr;
using mapnik::datasource;
using mapnik::parameters;
using mapnik::PoolGuard;
using mapnik::attribute_descriptor;
using namespace mongo;

DATASOURCE_PLUGIN(mongo_datasource)

#if (MAPNIK_VERSION > 220)
mongo_datasource::mongo_datasource(parameters const& params)
#else
mongo_datasource::mongo_datasource(parameters const& params, bool bind)
#endif
   : datasource(params),
     connector_(params.get<std::string>("host"),
             params.get<std::string>("dbname"),
             params.get<std::string>("user"),
             params.get<std::string>("password")),
     geometry_table_(*params.get<std::string>("geometry_table","")),
     type_(datasource::Vector),
     desc_(*params_.get<std::string>("type"),*params_.get<std::string>("encoding","utf-8")),
     extent_initialized_(false),
     max_extent_()
     
{
    if (geometry_table_.empty())
    {
	throw mapnik::datasource_exception("mongo plugin: missing <table> parameter");
    }
#if (MAPNIK_VERSION > 220)
    this->bind();	
#else
    if(bind == true)
    {
        this->bind();
    }
#endif
}

mongo_datasource::~mongo_datasource()
{
}

void mongo_datasource::bind() const
{
    if (is_bound_) 
	return;

    boost::optional<std::string> ext = params_.get<std::string>("max_extent");
    if (ext && !ext->empty())
    {
        extent_initialized_ = max_extent_.from_string(*ext);
    }
    else
    {
	max_extent_.init(-179.9,-89.9,179.9,89.9);
	extent_initialized_ = true;
    }

    boost::optional<int> initial_size = params_.get<int>("initial_size", 1);
    boost::optional<int> max_size = params_.get<int>("max_size", 10);

    mongo_connector_manager *mgr = mongo_connector_manager::instance();
    mgr->registerPool(connector_, *initial_size, *max_size);
    shared_ptr< Pool<mongo_connection,mongo_connector> > pool = mgr->getPool(connector_.id());
    if (pool)
    {
        shared_ptr<mongo_connection> conn = pool->borrowObject();
        if (conn && conn->isOK())
        {
	     is_bound_ = true;
        }

        PoolGuard<shared_ptr<mongo_connection>,shared_ptr<Pool<mongo_connection,mongo_connector> > > guard(conn,pool);
		if(conn)
		{
			//添加处理代码
		}
    }

    return;
}

#if (MAPNIK_VERSION > 211)
const char*  mongo_datasource::name()
#else
std::string  mongo_datasource::name()
#endif
{
    return "mongo";
}

#if (MAPNIK_VERSION > 211)
mapnik::datasource::datasource_t mongo_datasource::type() const
#else
int mongo_datasource::type() const
#endif
{
    return type_;
}

mapnik::box2d<double> mongo_datasource::envelope() const
{
    return max_extent_;
}

#if (MAPNIK_VERSION > 211)
boost::optional<mapnik::datasource::geometry_t> mongo_datasource::get_geometry_type() const
{
    boost::optional<mapnik::datasource::geometry_t> result;

    return result;
}
#endif

mapnik::layer_descriptor mongo_datasource::get_descriptor() const
{
   return desc_;
}


mapnik::featureset_ptr mongo_datasource::features(mapnik::query const& q) const
{
	//time_date qq_time;
    if (!is_bound_) 
	bind();
    //建立连接
    mongo_connector_manager *mgr = mongo_connector_manager::instance();
    shared_ptr< Pool<mongo_connection,mongo_connector> > pool = mgr->getPool(connector_.id());
    if (!pool)
    {
	return mapnik::featureset_ptr();
    }
        
    shared_ptr<mongo_connection> conn = pool->borrowObject();
    if (!conn || conn->isOK() == false)
    {
        return mapnik::featureset_ptr();
    }

    //PoolGuard<shared_ptr<mongo_connection>,shared_ptr<Pool<mongo_connection,mongo_connector> > > guard(conn,pool);

    //获取查询范围
     //获取查询范围
    double minx, maxx, miny, maxy;
    mapnik::box2d<double> const& box = q.get_bbox();
    double hdis = (box.maxx() - box.minx())/256;
    double vdis = (box.maxy() - box.miny())/256;
    minx = box.minx() + hdis*32;
    maxx = box.maxx() - hdis*32;
    miny = box.miny() + vdis*32;
    maxy = box.maxy() - vdis*32;
    
	//std::string xyz = box.get_xyz_pos();
	//std::cout << "xyz : " << xyz << std::endl;
    //查询条件
    std::ostringstream osstring;
    osstring << "{";             //start of osstring
    boost::optional<std::string> geometry = params_.get<std::string>("geometry_field","geom");
    if (geometry && !geometry->empty())
    {
        osstring << "'" << *geometry << "' : {$geoIntersects : {$geometry:{type : 'Polygon',coordinates : [[["
        << std::setprecision(9) << minx << "," << std::setprecision(8) << miny << "],["
        << std::setprecision(9) << minx << "," << std::setprecision(8) << maxy << "],["
        << std::setprecision(9) << maxx << "," << std::setprecision(8) << maxy << "],["
        << std::setprecision(9) << maxx << "," << std::setprecision(8) << miny << "],["
        << std::setprecision(9) << minx << "," << std::setprecision(8) << miny << "]]]}}}";
    }

    boost::optional<std::string> query = params_.get<std::string>("query","");
    if (query && !query->empty())
    {
	osstring << ",'$and':[" << *query << "]";
    }
    osstring << "}";    		//end of osstring
    Query query_(osstring.str());
    //返回字段
    BSONObjBuilder builder;
    std::set<std::string> const& props=q.property_names();
    std::set<std::string>::const_iterator pos=props.begin();
    std::set<std::string>::const_iterator end=props.end();
    while (pos != end)
    {
        builder << *pos << "";
        ++pos;
    }
    builder << *geometry << "";
    BSONObj fields_ = builder.obj();

    //std::cout<<osstring.str()<<std::endl;
	//qq_time.elapsed("query ready");
    //查询数据
    boost::shared_ptr<mongo::DBClientCursor> rs;
    if (max_extent_.intersects(box))
    {
		//time_date query_time;
        rs = conn->query(geometry_table_, query_, fields_);
		//query_time.elapsed("query");
    }

    //返回
    return  boost::make_shared<mongo_featureset>(rs, desc_.get_encoding(), box, conn, pool); 
}

mapnik::featureset_ptr mongo_datasource::features_at_point(mapnik::coord2d const& pt) const
{
    if (!is_bound_) 
	bind();
    //建立连接
    mongo_connector_manager *mgr = mongo_connector_manager::instance();
    shared_ptr< Pool<mongo_connection,mongo_connector> > pool = mgr->getPool(connector_.id());
    if (!pool)
    {
        return mapnik::featureset_ptr();
    }

    shared_ptr<mongo_connection> conn = pool->borrowObject();
    if (!conn || conn->isOK() == false)
    {
        return mapnik::featureset_ptr();
    }

    //PoolGuard<shared_ptr<mongo_connection>,shared_ptr<Pool<mongo_connection,mongo_connector> > > guard(conn,pool);

    //获取查询范围
    mapnik::box2d<double> box(pt.x,pt.y,pt.x,pt.y);
    //查询条件
    std::ostringstream osstring;
    osstring << "{";                    //start of osstring
    boost::optional<std::string> geometry = params_.get<std::string>("geometry_field","geom");
    if (geometry && !geometry->empty())
    {
	osstring << "'" << *geometry << "' : {$geoIntersects : {$geometry:{type : 'Polygon',coordinates : [[["
        << std::setprecision(9) << box.minx() << "," << std::setprecision(8) << box.miny() << "],["
        << std::setprecision(9) << box.minx() << "," << std::setprecision(8) << box.maxy() << "],["
        << std::setprecision(9) << box.maxx() << "," << std::setprecision(8) << box.maxy() << "],["
        << std::setprecision(9) << box.maxx() << "," << std::setprecision(8) << box.miny() << "],["
        << std::setprecision(9) << box.minx() << "," << std::setprecision(8) << box.miny() << "]]]}}}";
    }
    boost::optional<std::string> query = params_.get<std::string>("query","");
    if (query && !query->empty())
    {
        osstring << ",'$and':[" << *query << "]";
    }
    osstring << "}";                   //end of osstring
    Query query_(osstring.str());
    //返回字段
    BSONObjBuilder builder;
    std::vector<attribute_descriptor>::const_iterator itr = desc_.get_descriptors().begin();
    std::vector<attribute_descriptor>::const_iterator end = desc_.get_descriptors().end();
    while (itr != end)
    {
        builder << itr->get_name() << "";
	++itr;
    }
    builder << *geometry << "";   
    BSONObj fields_ = builder.obj();

    //查询数据
    boost::shared_ptr<mongo::DBClientCursor> rs;
    if (max_extent_.intersects(box))
    {
        rs = conn->query(geometry_table_, query_, fields_);
    }

    //返回
    return boost::make_shared<mongo_featureset>(rs, desc_.get_encoding(), box, conn, pool); 
}



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

#ifndef POSTGIS_CONNECTION_MANAGER_HPP
#define POSTGIS_CONNECTION_MANAGER_HPP

#include "mongo_connection.hpp"

// mapnik
#include <mapnik/pool.hpp>
#include <mapnik/utils.hpp>

// boost
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/optional.hpp>

// stl
#include <string>
#include <sstream>

using mapnik::Pool;
using mapnik::singleton;
using mapnik::CreateStatic;

template <typename T>
class mongo_connector
{

public:
    mongo_connector(boost::optional<std::string> const& host,
                      boost::optional<std::string> const& dbname,
                      boost::optional<std::string> const& user,
                      boost::optional<std::string> const& pwd)
        : host_(host),
          dbname_(dbname),
          user_(user),
          pwd_(pwd)
    {
    }

    T* operator()() const
    {
        return new T(*host_, *dbname_, *user_, *pwd_);
    }

    inline std::string id() const
    {
        return connection_string();
    }

    inline std::string connection_string() const
    {
         std::string connect_str;
        if (host_   && !host_->empty()) 
		connect_str += "host=" + *host_;
        if (dbname_ && !dbname_->empty()) 
		connect_str += " dbname=" + *dbname_;
        if (user_   && !user_->empty()) 
		connect_str += " user=" + *user_;
	if (pwd_   && !pwd_->empty()) 
		connect_str += " pwd=" + *pwd_;
        return connect_str;
    }

private:
    boost::optional<std::string> host_;
    boost::optional<std::string> dbname_;
    boost::optional<std::string> user_;
    boost::optional<std::string> pwd_;
};

class mongo_connector_manager : public singleton <mongo_connector_manager,CreateStatic>
{

    friend class CreateStatic<mongo_connector_manager>;
    typedef Pool<mongo_connection,mongo_connector> PoolType;
    typedef std::map<std::string,boost::shared_ptr<PoolType> > ContType;
    typedef boost::shared_ptr<mongo_connection> HolderType;
    ContType pools_;

public:

    bool registerPool(const mongo_connector<mongo_connection>& creator,unsigned int initialSize,unsigned int maxSize)
    {
        if (pools_.find(creator.id())==pools_.end())
        {
            return pools_.insert(std::make_pair(creator.id(),boost::make_shared<PoolType>(creator,initialSize,maxSize))).second;
        }

        return false;

    }

    boost::shared_ptr<PoolType> getPool(std::string const& key)
    {
        ContType::const_iterator itr=pools_.find(key);
        if (itr!=pools_.end())
        {
            return itr->second;
        }
        static const boost::shared_ptr<PoolType> emptyPool;
        return emptyPool;
    }

    HolderType get(std::string const& key)
    {
#ifdef MAPNIK_THREADSAFE
        //mutex::scoped_lock lock(mutex_);
#endif
        ContType::const_iterator itr=pools_.find(key);
        if (itr!=pools_.end())
        {
            boost::shared_ptr<PoolType> pool=itr->second;
            return pool->borrowObject();
        }
        return HolderType();
    }

    mongo_connector_manager() {}
private:
    mongo_connector_manager(const mongo_connector_manager&);
    mongo_connector_manager& operator=(const mongo_connector_manager);
};

#endif // MONGODB_CONNECTION_MANAGER_HPP

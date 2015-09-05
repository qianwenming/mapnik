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

#ifndef MONGO_CONNECTION_HPP
#define MONGO_CONNECTION_HPP

#include "mongo_featureset.hpp"
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
#include <sstream>
#include <iostream>
#include <vector>

//mongo
#include <mongo/client/dbclient.h>
using namespace mongo;


enum _ConnType
{
    MASTER = 0,
    SET,
    SYNC,
    CUSTOM,
    INVALID
};

#define SAFE_DELETE(p) {if(p != NULL) {delete p; p=NULL;}}

class mongo_connection
{
public:
    mongo_connection(std::string const& servers,
		     std::string const& dbname,
		     std::string const& user,
		     std::string const& pwd)
        : _conn_type(0), 
	_dbname(dbname),
	_user(user),
	_pwd(pwd)
    {

	std::string errmsg;
	fillServers(servers);
	_conn = this->connect(errmsg, 120.0);
	if(_conn == NULL)
	{
	    std::ostringstream oss;
            oss << "Mongodb Plugin(mongo_connection):" << _user << " ";
            oss << "connect to mongodb failed!";
            std::cout << oss.str() << std::endl;
            return;
	}

	return;
     }

    ~mongo_connection()
    {
	if(_conn != NULL)
	    delete _conn;
	_conn = NULL;
    }

    void fillServers(const string& servers)
    {
	std::size_t start = 0;
	std::size_t end = servers.find(';');
	while(end != std::string::npos)
	{
	    HostAndPort hp(servers.substr(start, end-start));
	    _servers.push_back(hp);
	    
	    start = end+1;
	    end = servers.find(';', start);    
	}

	HostAndPort hp(servers.substr(start, servers.length()-start));
        _servers.push_back(hp);
	return;
    }

    DBClientBase* connect(string& errmsg, double socketTimeout)
    {
	switch(_conn_type)
	{
	case MASTER:
	    {
		DBClientConnection *conn = new DBClientConnection(true,0,socketTimeout);
            	if(conn->connect(_servers[0],errmsg) == true)
                    return conn;

	   	SAFE_DELETE(conn);
	    }
	    break;
    	case SET:
	    {
	    	DBClientReplicaSet *conn = new DBClientReplicaSet("admin" , _servers , socketTimeout); 
	    	if(conn->connect() == true)
		    return conn;

	    	SAFE_DELETE(conn);
	    }
	    break;
    	case SYNC:
	    {
	    	std::list<HostAndPort> lst;  
            	for (unsigned i=0; i<_servers.size(); i++)  
                    lst.push_back(_servers[i]);  

            	SyncClusterConnection* conn = new SyncClusterConnection(lst, socketTimeout);
		if(conn->prepare(errmsg) && conn->fsync(errmsg) == true)
	    	    return conn;  

	        SAFE_DELETE(conn);
	    }
	    break;
    	case CUSTOM:
	    break;
    	case INVALID:
	    {
	    	throw string("trying to connect to invalid ConnectionString");
	    }
	    break;
	}

 	return NULL;
    }

    boost::shared_ptr<mongo::DBClientCursor> query(boost::optional<std::string> const& table, Query const& query, BSONObj const &fields)
    { 
        if(isOK() == false)
	{
            static const boost::shared_ptr<mongo::DBClientCursor> empty;
            return empty;
	}

	std::string strtable = _dbname + "." + *table; 
        boost::shared_ptr<mongo::DBClientCursor> rs(_conn->query(strtable, query,0,0,&fields,0,BATCH_SIZE));
        
	return rs;
    }

    bool isOK() const
    {
	return !_conn->isFailed();
    }

private:
    DBClientBase *_conn;
    vector<HostAndPort> _servers;

    int _conn_type;    
    std::string _dbname;
    std::string _user;
    std::string _pwd;
};

#endif //CONNECTION_HPP

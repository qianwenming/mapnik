
// boost
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>

// stl
#include <string>
#include <sstream>
#include <iostream>
#include <vector>
#include <mongo/client/dbclient.h>

int main(int argc, const char* argv[])
{
        mongo::DBClientConnection m_conn;
        
	std::string m_host = "191.168.1.183:10001";      
        std::string m_dbname = "gdsDB"; 
        std::string m_user = "admin";
	std::string m_pwd = "123456";
        std::string errmsg;
        if(m_conn.connect(m_host,errmsg) == false)
        {
                std::ostringstream oss;
                oss << "Mongodb Plugin(mongo_connection):" << m_user << " ";
                oss << "connect to mongodb failed!";
                std::cout << oss.str() << std::endl;
                return 0;
        }

        if(m_conn.auth(m_dbname,m_user,m_pwd,errmsg) == false)
        {
                std::ostringstream oss;
                oss << "Mongodb Plugin(mongo_connection):" << m_user << " ";
                oss << "mongodb connect auth failed ";
                std::cout << oss.str() << std::endl;
                return 0;
        }

        std::string strtable = "gdsDB.current_node_282";
        std::string lookup = "{'geom.coordinates' : {'$within' : {'$box' : [[121.3318030003747,31.09630238453041],[121.5099050003747,31.24868866426755]]}}}";

        boost::shared_ptr<mongo::DBClientCursor> rs(m_conn.query(strtable, lookup,3000,0,0,0,3000));     

	return 0;
}

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
#ifndef _TIME_DATE_H_
#define _TIME_DATE_H_

#include <boost/date_time/posix_time/posix_time.hpp>
#include <iostream>
#include <string>

class time_date
{
public:
	time_date() 
	{
		_start_time = boost::date_time::microsec_clock<boost::posix_time::ptime>::local_time();
	} 

	void   restart()
	{ 
		_start_time = boost::date_time::microsec_clock<boost::posix_time::ptime>::local_time();
	} 

	void elapsed(const std::string msg) const                  
	{
		std::cout << msg <<" use time: " <<boost::date_time::microsec_clock<boost::posix_time::ptime>::local_time() - _start_time<<std::endl;
	}

private:
	boost::posix_time::ptime  _start_time;
}; 

#endif
/*****************************************************************************
 *
 * This file is part of Mapnik (c++ mapping toolkit)
 *
 * Copyright (C) 2011 Artem Pavlenko
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

#ifndef MAPNIK_IMAGE_DATA_HPP
#define MAPNIK_IMAGE_DATA_HPP

// mapnik
#include <mapnik/global.hpp>

// stl
#include <cassert>
#include <cstring>
#include <stdexcept>


namespace mapnik
{
template <class T> class ImageData
{
public:
    typedef T pixel_type;

    ImageData(int width, int height)
        : width_(static_cast<unsigned>(width)),
          height_(static_cast<unsigned>(height)),
          external_data_(false)
    {
        if (width < 0)
        {
            throw std::runtime_error("negative width not allowed for image_data");
        }
        if (height < 0)
        {
            throw std::runtime_error("negative height not allowed for image_data");
        }
        pData_ = (width!=0 && height!=0)? static_cast<T*>(::operator new(sizeof(T)*width*height)):0;
        if (pData_) std::memset(pData_,0,sizeof(T)*width_*height_);
    }

    ImageData(int width, int height, T * data)
        : width_(static_cast<unsigned>(width)),
          height_(static_cast<unsigned>(height)),
          external_data_(true),
          pData_(data)
    {
        if (width < 0)
        {
            throw std::runtime_error("negative width not allowed for image_data");
        }
        if (height < 0)
        {
            throw std::runtime_error("negative height not allowed for image_data");
        }
    }

    ImageData(ImageData<T> const& rhs)
        :width_(rhs.width_),
         height_(rhs.height_),
         external_data_(false),
         pData_((rhs.width_!=0 && rhs.height_!=0)?
                static_cast<T*>(::operator new(sizeof(T)*rhs.width_*rhs.height_)) :0)
    {
        if (pData_) std::memcpy(pData_,rhs.pData_,sizeof(T)*rhs.width_* rhs.height_);
    }
    inline T& operator() (unsigned i,unsigned j)
    {
        assert(i<width_ && j<height_);
        return pData_[j*width_+i];
    }
    inline const T& operator() (unsigned i,unsigned j) const
    {
        assert(i<width_ && j<height_);
        return pData_[j*width_+i];
    }
    inline unsigned width() const
    {
        return width_;
    }
    inline unsigned height() const
    {
        return height_;
    }
    inline void set(const T& t)
    {
        for (unsigned y = 0; y < height_; ++y)
        {
            T * row = getRow(y);
            for (unsigned x = 0; x < width_; ++x)
            {
                row[x] = t;
            }
        }
    }

    inline const T* getData() const
    {
        return pData_;
    }

    inline T* getData()
    {
        return pData_;
    }

    inline const unsigned char* getBytes() const
    {
        return (unsigned char*)pData_;
    }

    inline unsigned char* getBytes()
    {
        return (unsigned char*)pData_;
    }

    inline const T* getRow(unsigned row) const
    {
        return pData_+row*width_;
    }

    inline T* getRow(unsigned row)
    {
        return pData_+row*width_;
    }

    inline void setRow(unsigned row,const T* buf,unsigned size)
    {
        assert(row<height_);
        assert(size<=width_);
        std::memcpy(pData_+row*width_,buf,size*sizeof(T));
    }
    inline void setRow(unsigned row,unsigned x0,unsigned x1,const T* buf)
    {
        std::memcpy(pData_+row*width_+x0,buf,(x1-x0)*sizeof(T));
    }

    inline ~ImageData()
    {
        if (!external_data_)
        {
            ::operator delete(pData_),pData_=0;
        }
    }

private:
    const unsigned width_;
    const unsigned height_;
    bool external_data_;
    T *pData_;
    ImageData& operator=(ImageData const&);
};

typedef ImageData<unsigned> image_data_32;
typedef ImageData<byte>  image_data_8;
}

#endif // MAPNIK_IMAGE_DATA_HPP

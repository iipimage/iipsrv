/*
    IIPImage Server - Member functions for Watermark.h

    Development supported by Moravian Library in Brno (Moravska zemska
    knihovna v Brne, http://www.mzk.cz/) R&D grant MK00009494301 & Old
    Maps Online (http://www.oldmapsonline.org/) from the Ministry of
    Culture of the Czech Republic.


    Copyright (C) 2010 Ruven Pillay.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#include "Watermark.h"
#include <cstring>
#include <cstdlib>
#include <tiff.h>
#include <tiffio.h>



// Load up and initialize our watermark image
void Watermark::init()
{
  if( _image.length() > 0 ){

    TIFF *tiff_watermark;
    if( ( tiff_watermark = TIFFOpen( _image.c_str(), "r" ) ) ){

      TIFFGetField( tiff_watermark, TIFFTAG_IMAGEWIDTH, &_width );
      TIFFGetField( tiff_watermark, TIFFTAG_IMAGELENGTH, &_height );
      TIFFGetField( tiff_watermark, TIFFTAG_BITSPERSAMPLE, &_bpc );
      uint32 buffer[_width*_height];
      uint32* b = &buffer[0];

      if( TIFFReadRGBAImageOriented( tiff_watermark, _width, _height, b, ORIENTATION_TOPLEFT ) == 0 ){
	TIFFClose( tiff_watermark );
	return;
      }

      // Set our number of channels to 3 as TIFFReadRGBAImage always outputs a colour image
      _channels = 3;

      // Set up the memory storage
      _watermark = new unsigned char[_width*_height*_channels];
      memset( _watermark, 0, _width*_height*_channels );

      // Load the data into our buffers
      for( uint32 i=0; i<_width*_height; i++ ){
	uint32 rgba = buffer[i];
	unsigned char r,g,b;
	// Extract the RGB values
	r = (unsigned char) TIFFGetR(rgba);
	g = (unsigned char) TIFFGetG(rgba);
	b = (unsigned char) TIFFGetB(rgba);
	_watermark[i*3] = r * _opacity;
	_watermark[i*3 + 1] = g * _opacity;
	_watermark[i*3 + 2] = b * _opacity;
      }
    }

    _isSet = true;
    TIFFClose( tiff_watermark );
  }

}



// Apply the watermark to a buffer of data
void Watermark::apply( void* data, unsigned int width, unsigned int height, unsigned int channels, unsigned int bpc )
{
  // Get random number as a float between 0 and 1
  float random = (float) rand() / RAND_MAX;
 
  // Only apply if our random number is less than our given probability
  if( random < _probability ){

    // Vary watermark position randomly within the tile depending on available space
    unsigned int xoffset = 0;
    if( width > _width ){
      random = (float) rand() / RAND_MAX;
      xoffset = random * (width - _width);
    }

    unsigned int yoffset = 0;
    if( height > _height ){
      random = (float) rand() / RAND_MAX;
      yoffset = random * (height - _height);
    }

    // Limit the area of the watermark to the size of the tile
    unsigned int xlimit = _width;
    unsigned int ylimit = _height;
    if( _width > width ) xlimit = width;
    if( _height > height ) ylimit = height;

    for( unsigned int j=0; j<ylimit; j++ ){
      for( unsigned int i=0; i<xlimit; i++ ){
	for( unsigned int k=0; k<channels; k++ ){

	  unsigned int id = (j+yoffset)*width*channels + (i+xoffset)*channels + k;

	  // For 16bit images
	  if( bpc == 16 ){
	    unsigned short* d = (unsigned short*) data;
	    // If our tile is 16 bit but our watermark is 8, multiply it up
	    if( _bpc == 8 ) d[id] += _watermark[j*_width*_channels + i*_channels + k] * 256;
	    else d[id] += _watermark[j*_width*_channels + i*_channels + k];	      
	  }
	  // For 8bit images
	  else{
	    unsigned char* d = (unsigned char*) data;
	    // If our tile is 8 bit but our watermark is 16, divide down
	    if( _bpc == 16 ) d[id] += _watermark[j*_width*_channels + i*_channels + k] / 256;
	    else d[id] += _watermark[j*_width*_channels + i*_channels + k];
	  }
	}
      }
    }
  }

}

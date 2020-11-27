/*
 *  pxa_camera.h
 *
 *  Bulverde Processor Camera Interface driver.
 *
 *  Copyright (C) 2003, Intel Corporation
 *  Copyright (C) 2003, Montavista Software Inc.
 *  Copyright (C) 2003-2006 Motorola Inc.
 *
 *  Author: Intel Corporation Inc.
 *          MontaVista Software, Inc.
 *           source@mvista.com
 *          Motorola Inc.
 * 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 
Revision History:
                   Modification     
Author                 Date        Description of Changes
----------------   ------------    -------------------------
Motorola            12/19/2003     Created
Motorola            02/05/2004     Set frame rate in video mode
Motorola            03/08/2004     Photo effects setting
Motorola            11/01/2004     Change to SDK style
Motorola            07/26/2005     Adjusting light, style or mode is noneffective. 
Motorola            08/04/2005     Add camera strobe flash to 1.3MP camera driver for Sumatra first phase
Motorola            11/09/2005     add color styles and cloudy white blance
Motorola            11/08/2005     Upmerge to EZXBASE36
Motorola            02/15/2006     Add support for Omnivision OV2640
Motorola            03/03/2006     EZXBASE 48 upmerge
Motorola            03/15/2006     Add support for Micron MI2020SOC
Motorola            05/06/2006     Force alignment of union struct to 4-byte boundary
Motorola            07/12/2006     Viewfinder causing very long response delay to user input
Motorola            08/15/2006     Implement common driver
*/

/*================================================================================
                                 INCLUDE FILES
================================================================================*/
#ifndef __PXA_CAMERA_H__ 
#define __PXA_CAMERA_H__ 

/*!
 *  General description of the Motorola A780/E680 video device driver:
 *
 *  The Motorola A780/E680 video device is based on V4L (video for linux) 1.0, however not 
 *  all V4L features are supported. 
 *  There are also some additional extensions included for specific requirements beyond V4L.
 *   
 *  The video device driver has a "character special" file named /dev/video0. Developers can 
 *  access the video device via the file operator interfaces.
 *  Six file operator interfaces are supported:
 *     open
 *     ioctl
 *     mmap
 *     poll/select
 *     read
 *     close
 *  For information on using these fuctions, please refer to the standard linux 
 *  development documents.
 *  
 *  These four ioctl interfaces are important for getting the video device to work properly:
 *     VIDIOCGCAP       Gets the video device capability
 *     VIDIOCCAPTURE    Starts/stops the video capture 
 *     VIDIOCGMBUF      Gets the image frame buffer map info
 *     VIDIOCSWIN       Sets the picture size    
 *  These interfaces are compatible with V4L 1.0. Please refer to V4L documents for more details.
 *  sample.c demonstrates their use.
 * 
 *  The following ioctl interfaces are Motorola-specific extensions. These are not compatible with V4L 1.0.   
 *   WCAM_VIDIOCSCAMREG   	
 *   WCAM_VIDIOCGCAMREG   	
 *   WCAM_VIDIOCSCIREG    	
 *   WCAM_VIDIOCGCIREG    	
 *   WCAM_VIDIOCSINFOR	    
 *   WCAM_VIDIOCGINFOR
 *   WCAM_VIDIOCSSSIZE
 *   WCAM_VIDIOCSOSIZE
 *   WCAM_VIDIOCGSSIZE
 *   WCAM_VIDIOCGOSIZE
 *   WCAM_VIDIOCSFPS
 *   WCAM_VIDIOCSNIGHTMODE
 *   WCAM_VIDIOCSSTYLE      
 *   WCAM_VIDIOCSLIGHT      
 *   WCAM_VIDIOCSBRIGHT     
 *   WCAM_VIDIOCSBUFCOUNT   
 *   WCAM_VIDIOCGCURFRMS    
 *   WCAM_VIDIOCGSTYPE      
 *   WCAM_VIDIOCSCONTRAST   
 *   WCAM_VIDIOCSFLICKER
 *   WCAM_VIDIOCSVFPARAM
 *
 *   Detailed information about these constants are described below.   
 * 
 *  sample.c demonstrates most features of the Motorola A780/E680 video device driver.
 *    - Opening/closing the video device
 *    - Initializing the video device driver
 *    - Displaying the video image on a A780/E680 LCD screen     
 *    - Changing the image size
 *    - Changing the style
 *    - Changing the light mode
 *    - Changing the brightness
 *    - Capturing and saving a still picture
 */

/*!
 * These are the registers for the read/write camera module and the CIF 
 * (Intel PXA27x processer quick capture interface)
 * The following 4 ioctl interfaces are used for debugging and are not open to developers
 */
#define WCAM_VIDIOCSCAMREG   	211
#define WCAM_VIDIOCGCAMREG   	212
#define WCAM_VIDIOCSCIREG    	213
#define WCAM_VIDIOCGCIREG    	214

/*!
 * WCAM_VIDIOCSINFOR Sets the image data format
 *  
 * The following code sets the image format to YCbCr422_planar
 *
 *   struct {int val1, val2;}format;
 *   format.val1 = CAMERA_IMAGE_FORMAT_YCBCR422_PLANAR;
 *   format.val2 = CAMERA_IMAGE_FORMAT_YCBCR422_PLANAR;
 *   //dev is the video device handle 
 *   ioctl(dev, WCAM_VIDIOCSINFOR, &format);
 *
 * Remarks:
 *   val1 is the output format of the camera module, val2 is the output format of the CIF (capture  
 *   interface). Image data from the camera module can be converted to other formats through
 *   the CIF. val2 specifies the final output format of the video device.
 *   
 *   For more description on CIF please refer to the Intel PXA27x processor family developer's manual.
 *     http://www.intel.com/design/pca/prodbref/253820.html 
 */
#define WCAM_VIDIOCSINFOR	    215

/*
 * WCAM_VIDIOCGINFOR Gets the image data format
 *
 *  struct {int val1, val2;}format;
 *  ioctl(dev, WCAM_VIDIOCGINFOR, &format);
 */
#define WCAM_VIDIOCGINFOR	    216
 
/*! 
 *  WCAM_VIDIOCSSSIZE Sets the sensor window size
 *
 *   The following code sets the sensor size to 640 X 480:
 *
 *   struct {unsigned short w, h;}sensor_size;
 *   sensor_size.w = 640;
 *   sensor_size.h = 480;
 *   //dev is the video device handle 
 *   ioctl(dev, WCAM_VIDIOCSSSIZE, &sensor_size);
 *
 *  Remarks:
 *    The sensor size is restricted by the video device capability. 
 *    VIDIOCGCAP can get the video device capability.
 *    The sensor size must be an even of multiple of 8. If not, the driver changes the sensor size to a multiple of 8.
 */
#define WCAM_VIDIOCSSSIZE        217

/*!
 * WCAM_VIDIOCSOSIZE Sets output size of the video device
 *
 *   The following code segment shows how to set the output size to 240 X 320:
 *
 *   struct {unsigned short w, h;}out_size;
 *   out_size.w = 240;
 *   out_size.h = 320;
 *   //dev is the video device handle 
 *   ioctl(dev, WCAM_VIDIOCSSSIZE, &out_size);
 *
 *  Remarks:
 *   In video mode, the output size must be less than 240X320. However, in still mode, the output  
 *   size is restricted by the video device capability and the sensor size.
 *   The output size must always be less than the sensor size, so if the developer changes the output size  
 *   to be greater than the sensor size, the video device driver may work abnormally.
 *   The width and height must also be a multiple of 8. If it is not, the driver changes the width and height size to a multiple of 8.
 *   The developer can modify the sensor size and the output size to create a digital zoom. 
 */
#define WCAM_VIDIOCSOSIZE        218 

/*!
 * WCAM_VIDIOCGSSIZE Gets the current sensor size.
 * 
 * The following code segment shows how to use this function:
 *
 *   struct {unsigned short w, h;}sensor_size;
 *   //dev is the video device handle 
 *   ioctl(dev, WCAM_VIDIOCGSSIZE, &sensor_size); 
 *   printf("sensor width is %d, sensor_height is %d\n", sensor_size.w, sensor_size.h);
 *
 */
#define WCAM_VIDIOCGSSIZE        219

/*!
 * WCAM_VIDIOCGOSIZE Gets the current output size.
 * 
 * The following code segment shows how to use this function:
 *
 *   struct {unsigned short w, h;}out_size;
 *   //dev is the video device handle 
 *   ioctl(dev, WCAM_VIDIOCGOSIZE, &out_size); 
 *   printf("output width is %d, output height is %d\n", out_size.w, out_size.h);
 *
 */
#define WCAM_VIDIOCGOSIZE        220 

/*!
 * WCAM_VIDIOCSFPS Sets the output frame rate (fps- frames per second) of the video device
 *
 * The following code segment shows how to use this function:
 *
 *   struct {int maxfps, minfps;}fps;
 *   fps.maxfps  = 15;
 *   fps.minfps  = 12;
 *   ioctl(dev, WCAM_VIDIOCSFPS, &fps);
 *
 * Remarks:
 *   The minimum value of maxfps is 1; the maximum value is 15.  minfps must not exceed maxfps. 
 *   The default value of fps is [15, 10].
 *   minfps and maxfps only suggest a fps range. The video device driver will select 
 *   an appropriate value automatically. The actual fps depends on environmental circumstances  
 *   such as brightness, illumination, etc. 
 *   sample.c illustrates how to calculate actual frame rate.
 *   
 */
#define WCAM_VIDIOCSFPS          221 

/*!
 * WCAM_VIDIOCSNIGHTMODE Sets the video device capture mode. 
 *
 * The capture mode can use the following values
 *
 *   V4l_NM_AUTO     Auto mode(default value)
 *   V4l_NM_NIGHT    Night mode
 *   V4l_NM_ACTION   Action mode
 *  
 * The following code segment shows how to set the video device to night mode:
 *
 *   ioctl(dev, WCAM_VIDIOCSNIGHTMODE, V4l_NM_NIGHT);
 *
 * Remarks:
 *   Different capture modes represent different sensor exposure times. Night mode represents a longer 
 *   exposure time. Setting the video device to night mode can capture high quality image data in low light environments.
 *   Action mode represents a shorter exposure time. This is used for capture moving objects. When working in auto mode, the 
 *   video device will select an appropriate exposure time automatically.
 *
 *   Not all camera modules support this interface. Developers can also use WCAM_VIDIOCSFPS to achieve similar results.
 *   Smaller minfps represent longer exposure times.
 *
 */
#define WCAM_VIDIOCSNIGHTMODE    222 

/*
 * WCAM_VIDIOCSVFPARAM Sets camera viewfinder offset and size
 *
 * The following code segment shows how to use this function:
 *
 * struct V4l_VF_PARAM vfparam;
 * vf.xoffset  = xoffset_to_fb_upleft_corner;
 * vf.yoffset  = yoffset_to_fb_upleft_corner;
 * vf.width    = viewfinder_width;
 * vf.height   = viewfinder_height;
 * vf.rotation = rotation_to_csi_data
 * ioctl(dev, WCAM_VIDIOCSVFPARAM, &vfparam);
 *
 * The viewfinder size must be smaller than the overlay frame buffer size and
 * vf.width + vf.xoffset must less than overlay frame buffer width
 * vf.height + vf.yoffset must less than overlay frame buffer height
 */
#define WCAM_VIDIOCSVFPARAM    223

/*!
 * WCAM_VIDIOCSSTYLE Sets the image style.
 *
 * The following styles are supported:
 *
 *   V4l_STYLE_NORMAL        Normal (default value)
 *   V4l_STYLE_BLACK_WHITE   Black and white 
 *   V4l_STYLE_SEPIA         Sepia
 *   V4l_STYLE_SOLARIZE      Solarized (not supported by all camera modules)
 *   V4l_STYLE_NEG_ART       Negative (not supported by all camera modules)
 *
 * The following code segment demonstrates how to set the image style to black and white:
 *
 *   ioctl(dev, WCAM_VIDIOCSSTYLE, V4l_STYLE_BLACK_WHITE);
 *
 */
#define WCAM_VIDIOCSSTYLE        250  

/*!
 * WCAM_VIDIOCSLIGHT Sets the image light mode
 * 
 * The following light modes are supported:
 *   V4l_WB_AUTO           Auto mode(default)
 *   V4l_WB_DIRECT_SUN     Direct sun
 *   V4l_WB_INCANDESCENT   Incandescent
 *   V4l_WB_FLUORESCENT    Fluorescent
 * 
 * The following code sets the image light mode to incandescent:
 *   ioctl(dev, WCAM_VIDIOCSLIGHT, V4l_WB_INCANDESCENT);
 */
#define WCAM_VIDIOCSLIGHT        251

/*!
 * WCAM_VIDIOCSBRIGHT Sets the brightness of the image (exposure compensation value)
 *  
 *  parameter value      exposure value
 *   -4                     -2.0 EV
 *   -3                     -1.5 EV
 *   -2                     -1.0 EV
 *   -1                     -0.5 EV
 *    0                      0.0 EV(default value)
 *    1                     +0.5 EV
 *    2                     +1.0 EV
 *    3                     +1.5 EV
 *    4                     +2.0 EV
 *
 * The following code segment sets the brightness to 2.0 EV
 *   ioctl(dev, WCAM_VIDIOCSBRIGHT, 4);
 */
#define WCAM_VIDIOCSBRIGHT       252

/*!
 * Sets the frame buffer count for video mode. The default value is 3.
 *
 * Remarks:
 * The video device driver maintains some memory for buffering image data in the kernel space. When working in video mode,
 * there are at least 3 frame buffers in the driver.  In still mode, there is only 1 frame buffer.
 * This interface is not open to SDK developers.
 * 
 */
#define WCAM_VIDIOCSBUFCOUNT     253  

/*!
 * Gets the current available frames
 *
 * The following code demonstrates getting the current available frames:
 *
 *   struct {int first, last;}cur_frms;
 *   ioctl(dev, WCAM_VIDIOCGCURFRMS, &cur_frms);
 *
 * Remarks:
 *   cur_frms.first represents the earliest frame in frame buffer  
 *   cur_frms.last  represents the latest or most recent frame in frame buffer.
 */
#define WCAM_VIDIOCGCURFRMS      254  

/*!
 * Gets the camera sensor type
 *
 *  unsigned int sensor_type
 *  ioctl(dev, WCAM_VIDIOCGSTYPE, &sensor_type);
 *  if(sensor_type == CAMERA_TYPE_ADCM_2700)
 *  {
 *     printf("Agilent ADCM2700");
 *  }
 *
 * Remarks:
 *   For all possible values of sensor_type please refer to the sensor definitions below.
 */
#define WCAM_VIDIOCGSTYPE        255 

/*!
 * Sets the image contrast
 * Not open to SDK developers
 */
#define WCAM_VIDIOCSCONTRAST     256

/*!
 * Sets the flicker frequency(50hz/60hz)
 * Not open to SDK developers
 */
#define WCAM_VIDIOCSFLICKER      257


/*
 * Gets the camera exposure parameters
*/
#define WCAM_VIDIOCGEXPOPARA      258

/*
 * Sets the camera still mode size
*/
#define WCAM_VIDIOCSCSIZE         259

/*
 * Gets the camera still mode size
*/
#define WCAM_VIDIOCGCSIZE         260

/*
 * Sets the camera still mode format
*/
#define WCAM_VIDIOCSCINFOR        261

/*
 * Gets the camera still mode format
*/
#define WCAM_VIDIOCGCINFOR        262

/*
 * Sets the camera capture digital zoom number
*/
#define WCAM_VIDIOCSZOOM          263

/*
 * Gets the camera capture digital zoom number
*/
#define WCAM_VIDIOCGZOOM          264

/*
 * Gets the current frame
*/
#define WCAM_VIDIOCGRABFRAME      265

/*
 * Move to the next frame
*/
#define WCAM_VIDIOCNEXTFRAME      266

/*!
 * Gets the frame buffer count for video mode.
 */

#define WCAM_VIDIOCGBUFCOUNT      267

/*
 * Sets the camera still mode digital zoom number
*/
#define WCAM_VIDIOCSSZOOM         268

/*
 * Gets the camera still mode digital zoom number
*/
#define WCAM_VIDIOCGSZOOM         269

/*
 * Sets the camera JPEG quality
*/
#define WCAM_VIDIOCSJPEGQUALITY   270

/*
 * Gets the camera JPEG quality
*/
#define WCAM_VIDIOCGJPEGQUALITY   271

/*
 * Sets the camera horizontally or vertically mirroring
*/
#define WCAM_VIDIOCSMIRROR        272

/*
 * Sets the camera strobe flash enable/disable
*/
#define WCAM_VIDIOCSSTROBEFLASH   273

/* 
 * Configure I2C slave addr, I2C register size, and I2C data size.
 * An argument of type 'struct camera_i2c_param *' should be passed
 * along with this command.
 */
#define WCAM_VIDIOCSI2CPARAM 274

/*
 * Configure the master clock frequency supplied to the sensor.
 * An argument of type 'unsigned long' should be passed with this cmd.
 */
#define WCAM_VIDIOCSMCLK 275

/*
 * Phase 1 setup for capture.  Uses same argument as VIDIOCCAPTURE cmd.
 */
#define WCAM_VIDIOCCAPTURESETUP1 276

/*
 * Configures a group of camera registers via I2C.  Requires a
 * pointer argument of type 'void *', pointing to a group of
 * addr-data pairs.
 */
#define WCAM_VIDIOCSCAMREGBLK 279


/*
   Defines for parameter
*/

#define CAMERA_MIRROR_VERTICALLY    0x0001
#define CAMERA_MIRROR_HORIZONTALLY  0x0002

/* 
 * Define the camera digital zoom level multiple 
 * the zoom value should equal
 *     zoom level x CAMERA_ZOOM_LEVEL_MULTIPLE
*/
#define CAMERA_ZOOM_LEVEL_MULTIPLE  256

typedef struct V4l_VF_PARAM_STRUCT
{
  unsigned int xoffset;
  unsigned int yoffset;
  unsigned int width;
  unsigned int height;
  unsigned int rotation;
} V4l_VF_PARAM;

struct V4l_EXPOSURE_PARA
{
    unsigned int luminance;
    int shutter;
    int ISOSpeed;
    char *reserved[16];
};

typedef enum V4l_NIGHT_MODE
{
   V4l_NM_AUTO,
   V4l_NM_NIGHT,
   V4l_NM_ACTION
}V4l_NM;

typedef enum V4l_PIC_STYLE
{
   V4l_STYLE_NORMAL,
   V4l_STYLE_BLACK_WHITE,
   V4l_STYLE_SEPIA,
   V4l_STYLE_SOLARIZE,
   V4l_STYLE_NEG_ART,
   V4l_STYLE_BLUISH,
   V4l_STYLE_REDDISH,
   V4l_STYLE_GREENISH
}V4l_PIC_STYLE;

typedef enum V4l_PIC_WB
{
   V4l_WB_AUTO,
   V4l_WB_DIRECT_SUN,
   V4l_WB_INCANDESCENT,
   V4l_WB_FLUORESCENT,
   V4l_WB_CLOUDY
}V4l_PIC_WB;


struct V4l_IMAGE_FRAME
{
    int first;
    int last;
    /*! The width of image frame */
    int             width;
    /*! The height of image frame */
    int             height;
    /*! The format of image frame */
    int             format;
    /*! The plane number of image frame */
    int             planeNum;
    /*! The buffer offset of image planes */
    unsigned        planeOffset[3];
    /*! The bytes size of image planes */
    int             planeBytes[3];
};

/*!
 *Image format definitions
 *Remarks:
 *  Although not all formats are supported by all camera modules, YCBCR422_PLANAR is widely supported. 
 *  For detailed information on each format please refer to the Intel PXA27x processor family developer's manual. 
 *     http://www.intel.com/design/pca/prodbref/253820.html
 * 
 */
#define CAMERA_IMAGE_FORMAT_RAW8                0
#define CAMERA_IMAGE_FORMAT_RAW9                1
#define CAMERA_IMAGE_FORMAT_RAW10               2
                                                                                                                             
#define CAMERA_IMAGE_FORMAT_RGB444              3
#define CAMERA_IMAGE_FORMAT_RGB555              4
#define CAMERA_IMAGE_FORMAT_RGB565              5
#define CAMERA_IMAGE_FORMAT_RGB666_PACKED       6
#define CAMERA_IMAGE_FORMAT_RGB666_PLANAR       7
#define CAMERA_IMAGE_FORMAT_RGB888_PACKED       8
#define CAMERA_IMAGE_FORMAT_RGB888_PLANAR       9
#define CAMERA_IMAGE_FORMAT_RGBT555_0          10  //RGB+Transparent bit 0
#define CAMERA_IMAGE_FORMAT_RGBT888_0          11
#define CAMERA_IMAGE_FORMAT_RGBT555_1          12  //RGB+Transparent bit 1
#define CAMERA_IMAGE_FORMAT_RGBT888_1          13
                                                                                                                             
#define CAMERA_IMAGE_FORMAT_YCBCR400           14
#define CAMERA_IMAGE_FORMAT_YCBCR422_PACKED    15
#define CAMERA_IMAGE_FORMAT_YCBCR422_PLANAR    16
#define CAMERA_IMAGE_FORMAT_YCBCR444_PACKED    17
#define CAMERA_IMAGE_FORMAT_YCBCR444_PLANAR    18
#define CAMERA_IMAGE_FORMAT_JPEG               19
#define CAMERA_IMAGE_FORMAT_JPEG_MICRON        20

#define CAMERA_IMAGE_FORMAT_MAX                20


/*!
 *VIDIOCCAPTURE arguments
 */
#define RETURN_VIDEO            2
#define STILL_IMAGE				1
#define VIDEO_START				0
#define VIDEO_STOP				-1

/*!
 *Sensor type definitions
 */
#define CAMERA_TYPE_ADCM_2650               1
#define CAMERA_TYPE_ADCM_2670               2
#define CAMERA_TYPE_ADCM_2700               3
#define CAMERA_TYPE_OMNIVISION_9640         4
#define CAMERA_TYPE_MT9M111                 5
#define CAMERA_TYPE_MT9V111                 6
#define CAMERA_TYPE_ADCM3800                7
#define CAMERA_TYPE_OV9650                  8
#define CAMERA_TYPE_MI2010SOC               9
#define CAMERA_TYPE_OV2640                  10
#define CAMERA_TYPE_MI2020SOC               11
#define CAMERA_TYPE_MAX                     CAMERA_TYPE_MI2020SOC


/*
 * Definitions of the camera's i2c device
 */
#define CAMERA_I2C_WRITEW    101
#define CAMERA_I2C_WRITEB    102
#define CAMERA_I2C_READW     103
#define CAMERA_I2C_READB     104
#define CAMERA_I2C_DETECTID  105

struct camera_i2c_register {
    unsigned short  addr;
    union {
        unsigned short w;
        unsigned char b;
    } value __attribute__ (( __aligned__ (4)));
};

struct camera_i2c_detectid {
    int buflen;
    char data[256];
};

/*
 * This structure contains properties of the camera I2C interface.
 */
struct camera_i2c_param {
  unsigned char slave_addr; /* I2C 8-bit slave address, divided by 2 */
  unsigned char reg_size; /* I2C register size, expressed in bytes */
  unsigned char data_size; /* I2C data size, expressed in bytes */
};

//End of the camera's i2c device

#endif // __PXA_CAMERA_H__ 


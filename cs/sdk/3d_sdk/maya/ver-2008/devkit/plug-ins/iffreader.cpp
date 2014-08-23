//-
// ==========================================================================
// Copyright (C) 1995 - 2006 Autodesk, Inc. and/or its licensors.  All 
// rights reserved.
//
// The coded instructions, statements, computer programs, and/or related 
// material (collectively the "Data") in these files contain unpublished 
// information proprietary to Autodesk, Inc. ("Autodesk") and/or its 
// licensors, which is protected by U.S. and Canadian federal copyright 
// law and by international treaties.
//
// The Data is provided for use exclusively by You. You have the right 
// to use, modify, and incorporate this Data into other products for 
// purposes authorized by the Autodesk software license agreement, 
// without fee.
//
// The copyright notices in the Software and this entire statement, 
// including the above license grant, this restriction and the 
// following disclaimer, must be included in all copies of the 
// Software, in whole or in part, and all derivative works of 
// the Software, unless such copies or derivative works are solely 
// in the form of machine-executable object code generated by a 
// source language processor.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND. 
// AUTODESK DOES NOT MAKE AND HEREBY DISCLAIMS ANY EXPRESS OR IMPLIED 
// WARRANTIES INCLUDING, BUT NOT LIMITED TO, THE WARRANTIES OF 
// NON-INFRINGEMENT, MERCHANTABILITY OR FITNESS FOR A PARTICULAR 
// PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE, OR 
// TRADE PRACTICE. IN NO EVENT WILL AUTODESK AND/OR ITS LICENSORS 
// BE LIABLE FOR ANY LOST REVENUES, DATA, OR PROFITS, OR SPECIAL, 
// DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES, EVEN IF AUTODESK 
// AND/OR ITS LICENSORS HAS BEEN ADVISED OF THE POSSIBILITY 
// OR PROBABILITY OF SUCH DAMAGES.
//
// ==========================================================================
//+

#include <maya/MString.h>
#include <maya/MStatus.h>
#include "iffreader.h"

IFFimageReader::IFFimageReader ()
{
	fImage = NULL;
	fBuffer = NULL;
	fZBuffer = NULL;
}

IFFimageReader::~IFFimageReader ()
{
	close ();
}

MStatus IFFimageReader::open (MString filename)
{
	close ();

   fImage = ILopen (filename.asChar (), "rb");

	if (NULL == fImage)
		return MS::kFailure;

	// Read top-to-bottom, not bottom-to-top
	ILctrl (fImage, ILF_Updown, 1);
	// Convert all data to RGBA, even if there's no alpha channel
	ILctrl (fImage, ILF_Pack, 0);
	// If the data is 16 bits, read in the full 16 bits. Otherwise,
	// truncate to 8 bits. Default behaviour is to truncate 16 bit
	// data to 8 bits.
	if (ILgetbpp (fImage) == 2)
		ILctrl (fImage, ILF_Full, 1);
	else
		ILctrl (fImage, ILF_Full, 0);
	// Generate a zero alpha mask if there's no alpha channel
	ILctrl (fImage, ILF_NoMask, 0);

	return MS::kSuccess;
}

MStatus IFFimageReader::close ()
{
	if (NULL == fImage)
		return MS::kFailure;

	if (ILclose (fImage))
	{
		fImage = NULL;
		return MS::kFailure;
	}

	fImage = NULL;

	if (NULL != fBuffer)
	{
		delete [] fBuffer;
		fBuffer = NULL;
	}
	if (NULL != fZBuffer)
	{
		delete [] fZBuffer;
		fZBuffer = NULL;
	}

	return MS::kSuccess;
}

MStatus IFFimageReader::getSize (int &x, int &y)
{
	if (NULL == fImage)
		return MS::kFailure;

	if (ILgetsize (fImage, &x, &y))
		return MS::kFailure;

	return MS::kSuccess;
}

int IFFimageReader::getBytesPerChannel ()
{
	return ILgetbpp (fImage);
}

bool IFFimageReader::isRGB ()
{
	if (NULL == fImage)
		return false;
	int type = ILgettype (fImage);
	if (type == -1)
		return false;
	return (type&ILH_RGB) ? true : false;
}

bool IFFimageReader::isGrayscale ()
{
	if (NULL == fImage)
		return false;
	int type = ILgettype (fImage);
	if (type == -1)
		return false;
	return (type&ILH_Black) ? true : false;
}

bool IFFimageReader::hasAlpha ()
{
	if (NULL == fImage)
		return false;
	int type = ILgettype (fImage);
	if (type == -1)
		return false;
	return (type&ILH_Alpha) ? true : false;
}

bool IFFimageReader::hasDepthMap ()
{
	if (NULL == fImage)
		return false;
	int type = ILgettype (fImage);
	if (type == -1)
		return false;
	return (type&ILH_Zbuffer) ? true : false;
}

MStatus IFFimageReader::readImage ()
{
	if (NULL == fImage)
		return MS::kFailure;

	if (NULL != fBuffer || NULL != fZBuffer)
		return MS::kFailure;

	int width,height;
	ILgetsize (fImage, &width, &height);

	int bpp; // bytes per pixel, not bits per pixel
	bpp = ILgetbpp (fImage);

	int type;
	type = ILgettype (fImage);

	if ((type & ILH_RGB) || (type & ILH_Black))
		fBuffer = new byte [width * height * bpp * 4];

	if (type & ILH_Zbuffer)
		fZBuffer = new float [width * height];

	if (ILload (fImage, fBuffer, fZBuffer))
		return MS::kFailure;

	return MS::kSuccess;
}

MStatus IFFimageReader::getPixel (int x, int y, int *r, int *g, int *b,
								  int *a)
{
	if (NULL == fBuffer)
		return MS::kFailure;
	int width,height;
	ILgetsize (fImage, &width, &height);
	if (x >= width || y >= height || x < 0 || y < 0)
		return MS::kFailure;
	if (ILgetbpp (fImage) == 2) {
		int *ptr = (int *)&fBuffer [(y * width + x) * 4];
		// On IRIX pixels are stored as ABGR and on NT as BGRA
#ifdef _WIN32
		if (NULL != r)
			*r = ptr [2];
		if (NULL != g)
			*g = ptr [1];
		if (NULL != b)
			*b = ptr [0];
		if (NULL != a)
			*a = ptr [3];
#else
		if (NULL != r)
			*r = ptr [3];
		if (NULL != g)
			*g = ptr [2];
		if (NULL != b)
			*b = ptr [1];
		if (NULL != a)
			*a = ptr [0];
#endif
	} else {
		byte *ptr = &fBuffer [(y * width + x) * 4];
#ifdef _WIN32
		if (NULL != r)
			*r = ptr [2];
		if (NULL != g)
			*g = ptr [1];
		if (NULL != b)
			*b = ptr [0];
		if (NULL != a)
			*a = ptr [3];
#else
		if (NULL != r)
			*r = ptr [3];
		if (NULL != g)
			*g = ptr [2];
		if (NULL != b)
			*b = ptr [1];
		if (NULL != a)
			*a = ptr [0];
#endif
	}
	return MS::kSuccess;
}

MStatus IFFimageReader::getDepth (int x, int y, float *d)
{
	if (NULL == fZBuffer || NULL == d)
		return MS::kFailure;
	int width,height;
	ILgetsize (fImage, &width, &height);
	if (x >= width || y >= height || x < 0 || y < 0)
		return MS::kFailure;
	float depth = fZBuffer [y * width + x];
	if (depth == 0.)
		*d = 0.;
	else
		*d = -1.0f/depth;
	return MS::kSuccess;
}

MString IFFimageReader::errorString ()
{
	return FLstrerror (FLerror ());
}

const byte *IFFimageReader::getPixelMap () const
{
	return fBuffer;
}

const float *IFFimageReader::getDepthMap () const
{
	return fZBuffer;
}
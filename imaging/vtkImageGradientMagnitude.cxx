/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageGradientMagnitude.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include <math.h>
#include "vtkImageGradientMagnitude.h"


//----------------------------------------------------------------------------
// Description:
// Construct an instance of vtkImageGradientMagnitude fitler.
vtkImageGradientMagnitude::vtkImageGradientMagnitude()
{
  this->SetAxes(VTK_IMAGE_X_AXIS,VTK_IMAGE_Y_AXIS);
  this->Dimensionality = 2;

  this->SetOutputScalarType(VTK_FLOAT);
  this->HandleBoundariesOn();
  
  this->ExecuteDimensionality = 4;  
}


//----------------------------------------------------------------------------
void vtkImageGradientMagnitude::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkImageFilter::PrintSelf(os, indent);
  os << indent << "HandleBoundaries: " << this->HandleBoundaries << "\n";
}



//----------------------------------------------------------------------------
// Description:
// This method is passed a region that holds the image extent of this filters
// input, and changes the region to hold the image extent of this filters
// output.
void vtkImageGradientMagnitude::ComputeOutputImageInformation(
		      vtkImageRegion *inRegion, vtkImageRegion *outRegion)
{
  int extent[VTK_IMAGE_EXTENT_DIMENSIONS];
  int idx;

  inRegion->GetImageExtent(VTK_IMAGE_DIMENSIONS, extent);
  if ( ! this->HandleBoundaries)
    {
    // shrink output image extent.
    for (idx = 0; idx < this->Dimensionality; ++idx)
      {
      extent[idx*2] += 1;
      extent[idx*2 + 1] -= 1;
      }
    }
  
  outRegion->SetImageExtent(VTK_IMAGE_DIMENSIONS, extent);
}


//----------------------------------------------------------------------------
// Description:
// This method computes the input extent necessary to generate the output.
void vtkImageGradientMagnitude::ComputeRequiredInputRegionExtent(
			vtkImageRegion *outRegion, vtkImageRegion *inRegion)
{
  int extent[VTK_IMAGE_EXTENT_DIMENSIONS];
  int *imageExtent;
  int idx;

  imageExtent = inRegion->GetImageExtent();
  outRegion->GetExtent(VTK_IMAGE_DIMENSIONS, extent);
  
  // grow input image extent.
  for (idx = 0; idx < this->Dimensionality; ++idx)
    {
    extent[idx*2] -= 1;
    extent[idx*2+1] += 1;
    if (this->HandleBoundaries)
      {
      // we must clip extent with image extent is we hanlde boundaries.
      if (extent[idx*2] < imageExtent[idx*2])
	{
	extent[idx*2] = imageExtent[idx*2];
	}
      if (extent[idx*2 + 1] > imageExtent[idx*2 + 1])
	{
	extent[idx*2 + 1] = imageExtent[idx*2 + 1];
	}
      }
    }
  
  inRegion->SetExtent(VTK_IMAGE_DIMENSIONS, extent);
}





//----------------------------------------------------------------------------
// Description:
// This execute method handles boundaries.
// it handles boundaries. Pixels are just replicated to get values 
// out of extent.
template <class T>
static void vtkImageGradientMagnitudeExecute(vtkImageGradientMagnitude *self,
				      vtkImageRegion *inRegion, T *inPtr, 
				      vtkImageRegion *outRegion, float *outPtr)
{
  int axisIdx, axesNum;
  float d, sum;
  float r[4];
  // For looping though output (and input) pixels.
  int min0, max0, min1, max1, min2, max2, min3, max3;
  int outIdx0, outIdx1, outIdx2, outIdx3;
  int outInc0, outInc1, outInc2, outInc3;
  float *outPtr0, *outPtr1, *outPtr2, *outPtr3;
  int inInc0, inInc1, inInc2, inInc3;
  T *inPtr0, *inPtr1, *inPtr2, *inPtr3;
  // For computation of gradient (everything has to be arrays for loop).
  int *incs, *imageExtent, *idxs, outIdxs[VTK_IMAGE_DIMENSIONS];

  
  // Get the dimensionality of the gradient.
  axesNum = self->GetDimensionality();
  
  // Get information to march through data (skip component)
  inRegion->GetIncrements(inInc0, inInc1, inInc2, inInc3); 
  outRegion->GetIncrements(outInc0, outInc1, outInc2, outInc3); 
  outRegion->GetExtent(min0,max0, min1,max1, min2,max2, min3,max3);
    
  // We want the input pixel to correspond to output
  inPtr = (T *)(inRegion->GetScalarPointer(min0,min1,min2,min3));

  // The data spacing is important for computing the gradient.
  inRegion->GetSpacing(4, r);
  r[0] = 1.0 / r[0];
  r[1] = 1.0 / r[1];
  r[2] = 1.0 / r[2];
  r[3] = 1.0 / r[3];
  
  // loop through pixels of output
  outPtr3 = outPtr;
  inPtr3 = inPtr;
  for (outIdx3 = min3; outIdx3 <= max3; ++outIdx3)
    {
    outIdxs[3] = outIdx3;
    outPtr2 = outPtr3;
    inPtr2 = inPtr3;
    for (outIdx2 = min2; outIdx2 <= max2; ++outIdx2)
      {
      outIdxs[2] = outIdx2;
      outPtr1 = outPtr2;
      inPtr1 = inPtr2;
      for (outIdx1 = min1; outIdx1 <= max1; ++outIdx1)
	{
	outIdxs[1] = outIdx1;
	outPtr0 = outPtr1;
	inPtr0 = inPtr1;
	for (outIdx0 = min0; outIdx0 <= max0; ++outIdx0)
	  {
	  *outIdxs = outIdx0;

	  // compute magnitude or vector.
	  sum = 0.0;
	  idxs = outIdxs;
	  incs = inRegion->GetIncrements(); 
	  imageExtent = inRegion->GetImageExtent(); 
	  for(axisIdx = 0; axisIdx < axesNum; ++axisIdx)
	    {
	    // Compute difference using central differences (if in extent).
	    d = ((*idxs - 1) < *imageExtent) ? *inPtr0 : inPtr0[-*incs];
	    ++imageExtent;  // now points to max.
	    d -= ((*idxs + 1) > *imageExtent) ? *inPtr0 : inPtr0[*incs];
	    d *= r[axisIdx]; // multiply by the data spacing
	    ++imageExtent;
	    ++idxs;
	    ++incs;
	    sum += d * d;
	    }
	  *outPtr0 = sqrt(sum);
	  
	  outPtr0 += outInc0;
	  inPtr0 += inInc0;
	  }
	outPtr1 += outInc1;
	inPtr1 += inInc1;
	}
      outPtr2 += outInc2;
      inPtr2 += inInc2;
      }
    outPtr3 += outInc3;
    inPtr3 += inInc3;
    }
}


//----------------------------------------------------------------------------
// Description:
// This method contains a switch statement that calls the correct
// templated function for the input region type.  The output region
// must be of type float.  This method does handle boundary conditions.
// The third axis is the component axis for the output.
void vtkImageGradientMagnitude::Execute(vtkImageRegion *inRegion, 
			       vtkImageRegion *outRegion)
{
  void *inPtr = inRegion->GetScalarPointer();
  void *outPtr = outRegion->GetScalarPointer();
  
  // this filter expects that output is type float.
  if (outRegion->GetScalarType() != VTK_FLOAT)
    {
    vtkErrorMacro(<< "Execute: output ScalarType, "
                  << vtkImageScalarTypeNameMacro(outRegion->GetScalarType())
                  << ", must be float");
    return;
    }
  
  switch (inRegion->GetScalarType())
    {
    case VTK_FLOAT:
      vtkImageGradientMagnitudeExecute(this, 
			  inRegion, (float *)(inPtr), 
			  outRegion, (float *)(outPtr));
      break;
    case VTK_INT:
      vtkImageGradientMagnitudeExecute(this, 
			  inRegion, (int *)(inPtr), 
			  outRegion, (float *)(outPtr));
      break;
    case VTK_SHORT:
      vtkImageGradientMagnitudeExecute(this, 
			  inRegion, (short *)(inPtr), 
			  outRegion, (float *)(outPtr));
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageGradientMagnitudeExecute(this, 
			  inRegion, (unsigned short *)(inPtr), 
			  outRegion, (float *)(outPtr));
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageGradientMagnitudeExecute(this, 
			  inRegion, (unsigned char *)(inPtr), 
			  outRegion, (float *)(outPtr));
      break;
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
}







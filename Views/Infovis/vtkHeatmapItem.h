/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHeatmapItem.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkHeatmapItem - A 2D graphics item for rendering a heatmap
//
// .SECTION Description
// This item draws a heatmap as a part of a vtkContextScene.
//
// .SEE ALSO
// vtkTable

#ifndef __vtkHeatmapItem_h
#define __vtkHeatmapItem_h

#include "vtkViewsInfovisModule.h" // For export macro
#include "vtkContextItem.h"

#include "vtkNew.h"                // For vtkNew ivars
#include "vtkSmartPointer.h"       // For vtkSmartPointer ivars
#include "vtkVector.h"             // For vtkVector2f ivar
#include <map>                     // For column ranges
#include <vector>                  // For row mapping

class vtkLookupTable;
class vtkStringArray;
class vtkTable;
class vtkTooltipItem;

class VTKVIEWSINFOVIS_EXPORT vtkHeatmapItem : public vtkContextItem
{
public:
  static vtkHeatmapItem *New();
  vtkTypeMacro(vtkHeatmapItem, vtkContextItem);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Set the table that this item draws.  The first column of the table
  // must contain the names of the rows.
  virtual void SetTable(vtkTable *table);

  // Description:
  // Get the table that this item draws.
  vtkTable * GetTable();

  // Description:
  // Set which way the table should face within the visualization.
  void SetOrientation(int orientation);

  // Description:
  // Get the current heatmap orientation.
  int GetOrientation();

  // Description:
  // Get the angle that row labels should be rotated for the correponding
  // heatmap orientation.  For the default orientation (LEFT_TO_RIGHT), this
  // is 0 degrees.
  double GetTextAngleForOrientation(int orientation);

  // Description:
  // Set the position of the heatmap.
  vtkSetVector2Macro(Position, float);
  void SetPosition(const vtkVector2f &pos);

  // Description:
  // Get position of the heatmap.
  vtkGetVector2Macro(Position, float);
  vtkVector2f GetPositionVector();

  // Description:
  // Get/Set the height of the cells in our heatmap.
  // Default is 18 pixels.
  vtkGetMacro(CellHeight, double);
  vtkSetMacro(CellHeight, double);

  // Description:
  // Get/Set the width of the cells in our heatmap.
  // Default is 36 pixels.
  vtkGetMacro(CellWidth, double);
  vtkSetMacro(CellWidth, double);

  // Description:
  // Get the bounds for this item as (Xmin,Xmax,Ymin,Ymax).
  virtual void GetBounds(double bounds[4]);

  //BTX

  // Description:
  // Enum for Orientation.
  enum
    {
    LEFT_TO_RIGHT,
    UP_TO_DOWN,
    RIGHT_TO_LEFT,
    DOWN_TO_UP
    };

  // Description:
  // Returns true if the transform is interactive, false otherwise.
  virtual bool Hit(const vtkContextMouseEvent &mouse);

  // Description:
  // Display a tooltip when the user mouses over a cell in the heatmap.
  virtual bool MouseMoveEvent(const vtkContextMouseEvent &event);

  //ETX

protected:
  vtkHeatmapItem();
  ~vtkHeatmapItem();

  vtkVector2f PositionVector;
  float* Position;

  // Description:
  // Generate some data needed for painting.  We cache this information as
  // it only needs to be generated when the input data changes.
  virtual void RebuildBuffers();

  // Description:
  // This function does the bulk of the actual work in rendering our heatmap.
  virtual void PaintBuffers(vtkContext2D *painter);

  // Description:
  // This function returns a bool indicating whether or not we need to rebuild
  // our cached data before painting.
  virtual bool IsDirty();

  // Description:
  // Generate a separate vtkLookupTable for each column in the table.
  void InitializeLookupTables();

  // Description:
  // Paints the table as a heatmap.
  virtual bool Paint(vtkContext2D *painter);

  // Description:
  // Helper function.  Find the prominent, distinct values in the specified
  // column of strings and add it to our "master list" of categorical values.
  // This list is then used to generate a vtkLookupTable for all categorical
  // data within the heatmap.
  void AccumulateProminentCategoricalDataValues(vtkIdType column);

  // Description:
  // Setup the default lookup table to use for continuous (not categorical)
  // data.
  void GenerateContinuousDataLookupTable();

  // Description:
  // Setup the default lookup table to use for categorical (not continuous)
  // data.
  void GenerateCategoricalDataLookupTable();

  // Description:
  // Get the value for the cell of the heatmap located at scene position (x, y)
  // This function assumes the caller has already determined that (x, y) falls
  // within the heatmap.
  std::string GetTooltipText(float x, float y);

  // Description:
  // Calculate the extent of the data that is visible within the window.
  // This information is used to ensure that we only draw details that
  // will be seen by the user.  This improves rendering speed, particularly
  // for larger data.
  void UpdateVisibleSceneExtent(vtkContext2D *painter);

  // Description:
  // Returns true if any part of the line segment defined by endpoints
  // (x0, y0), (x1, y1) falls within the extent of the currently
  // visible scene.  Returns false otherwise.
  bool LineIsVisible(double x0, double y0, double x1, double y1);

  vtkSmartPointer<vtkTable> Table;

private:
  vtkHeatmapItem(const vtkHeatmapItem&); // Not implemented
  void operator=(const vtkHeatmapItem&); // Not implemented

  unsigned long HeatmapBuildTime;
  vtkNew<vtkTooltipItem> Tooltip;
  vtkNew<vtkLookupTable> ContinuousDataLookupTable;
  vtkNew<vtkLookupTable> CategoricalDataLookupTable;
  vtkNew<vtkStringArray> CategoricalDataValues;
  double CellWidth;
  double CellHeight;

  std::map< vtkIdType, std::pair< double, double > > ColumnRanges;
  std::vector< vtkIdType > SceneRowToTableRowMap;

  double MinX;
  double MinY;
  double MaxX;
  double MaxY;
  double SceneBottomLeft[3];
  double SceneTopRight[3];
};

#endif

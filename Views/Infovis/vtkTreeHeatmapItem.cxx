/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTreeHeatmapItem.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTreeHeatmapItem.h"
#include "vtkDendrogramItem.h"
#include "vtkHeatmapItem.h"

#include "vtkDataSetAttributes.h"
#include "vtkBitArray.h"
#include "vtkObjectFactory.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkTree.h"

vtkStandardNewMacro(vtkTreeHeatmapItem);

//-----------------------------------------------------------------------------
vtkTreeHeatmapItem::vtkTreeHeatmapItem()
{
  this->Interactive = true;
  this->Orientation = vtkDendrogramItem::LEFT_TO_RIGHT;
  this->TreeHeatmapBuildTime = 0;

  this->Dendrogram = vtkSmartPointer<vtkDendrogramItem>::New();
  this->Dendrogram->ExtendLeafNodesOn();
  this->Dendrogram->SetVisible(false);
  this->AddItem(this->Dendrogram);

  this->Heatmap = vtkSmartPointer<vtkHeatmapItem>::New();
  this->Heatmap->SetVisible(false);
  this->AddItem(this->Heatmap);
}

//-----------------------------------------------------------------------------
vtkTreeHeatmapItem::~vtkTreeHeatmapItem()
{
}

//-----------------------------------------------------------------------------
void vtkTreeHeatmapItem::SetTree(vtkTree *tree)
{
  this->Dendrogram->SetTree(tree);

  if (this->GetTable() != NULL &&
      this->GetTable()->GetNumberOfRows() != 0)
    {
    this->Dendrogram->SetDrawLabels(false);
    }
  this->Dendrogram->SetVisible(true);

  // rearrange our table to match the order of the leaf nodes in this tree.
  if (this->GetTable() != NULL && this->GetTable()->GetNumberOfRows() != 0)
    {
    this->ReorderTable();
    }
}

//-----------------------------------------------------------------------------
void vtkTreeHeatmapItem::SetTable(vtkTable *table)
{
  this->Heatmap->SetTable(table);

  if (this->Dendrogram->GetTree() != NULL &&
      this->Dendrogram->GetTree()->GetNumberOfVertices() != 0)
    {
    this->Dendrogram->SetDrawLabels(false);
    }
  this->Heatmap->SetVisible(true);

  // rearrange our table to match the order of the leaf nodes in this tree.
  if (this->GetTree() != NULL && this->GetTree()->GetNumberOfVertices() != 0)
    {
    this->ReorderTable();
    }

  // add an array to this table's field data to keep track of collapsed rows
  // (unless it already has the array)
  vtkBitArray *existingArray = vtkBitArray::SafeDownCast(
    this->GetTable()->GetFieldData()->GetArray("collapsed rows"));
  if (existingArray)
    {
    for(vtkIdType row = 0; row < this->GetTable()->GetNumberOfRows(); ++row)
      {
      existingArray->SetValue(row, 0);
      }
    }
  else
    {
    vtkSmartPointer<vtkBitArray> collapsedRowsArray =
      vtkSmartPointer<vtkBitArray>::New();
    collapsedRowsArray->SetNumberOfComponents(1);
    collapsedRowsArray->SetName("collapsed rows");
    for(vtkIdType row = 0; row < this->GetTable()->GetNumberOfRows(); ++row)
      {
      collapsedRowsArray->InsertNextValue(0);
      }
    this->GetTable()->GetFieldData()->AddArray(collapsedRowsArray);
    }
}

//-----------------------------------------------------------------------------
vtkSmartPointer<vtkDendrogramItem> vtkTreeHeatmapItem::GetDendrogram()
{
  return this->Dendrogram;
}

//-----------------------------------------------------------------------------
void vtkTreeHeatmapItem::SetDendrogram(vtkSmartPointer<vtkDendrogramItem> item)
{
  this->Dendrogram = item;
}

//-----------------------------------------------------------------------------
vtkSmartPointer<vtkHeatmapItem> vtkTreeHeatmapItem::GetHeatmap()
{
  return this->Heatmap;
}

//-----------------------------------------------------------------------------
void vtkTreeHeatmapItem::SetHeatmap(vtkSmartPointer<vtkHeatmapItem> item)
{
  this->Heatmap = item;
}

//-----------------------------------------------------------------------------
vtkTree * vtkTreeHeatmapItem::GetTree()
{
  return this->Dendrogram->GetTree();
}

//-----------------------------------------------------------------------------
vtkTable * vtkTreeHeatmapItem::GetTable()
{
  return this->Heatmap->GetTable();
}

//-----------------------------------------------------------------------------
void vtkTreeHeatmapItem::ReorderTable()
{
  // make a copy of our table and then empty out the original.
  vtkNew<vtkTable> tableCopy;
  tableCopy->DeepCopy(this->GetTable());
  for (vtkIdType row = 0; this->GetTable()->GetNumberOfRows(); ++row)
    {
    this->GetTable()->RemoveRow(row);
    }

  // get the names of the vertices in our tree.
  vtkStringArray *vertexNames = vtkStringArray::SafeDownCast(
    this->GetTree()->GetVertexData()->GetAbstractArray("node name"));

  // get array of row names from the table.  We assume this is the first row.
  vtkStringArray *rowNames = vtkStringArray::SafeDownCast(
    tableCopy->GetColumn(0));

  for (vtkIdType vertex = 0; vertex < this->GetTree()->GetNumberOfVertices();
       ++vertex)
    {
    if (!this->GetTree()->IsLeaf(vertex))
      {
      continue;
      }

    // find the row in the table that corresponds to this vertex
    std::string vertexName = vertexNames->GetValue(vertex);
    vtkIdType tableRow = rowNames->LookupValue(vertexName);
    if (tableRow < 0)
      {
      continue;
      }

    // copy it back into our original table
    this->GetTable()->InsertNextRow(tableCopy->GetRow(tableRow));
    }

  if (this->Orientation == vtkDendrogramItem::RIGHT_TO_LEFT ||
      this->Orientation == vtkDendrogramItem::DOWN_TO_UP)
    {
    this->ReverseTable();
    }
}

//-----------------------------------------------------------------------------
void vtkTreeHeatmapItem::ReverseTable()
{
  // make a copy of our table and then empty out the original.
  vtkNew<vtkTable> tableCopy;
  tableCopy->DeepCopy(this->GetTable());
  for (vtkIdType row = 0; this->GetTable()->GetNumberOfRows(); ++row)
    {
    this->GetTable()->RemoveRow(row);
    }

  // re-insert the rows back into our original table in reverse order
  for (vtkIdType tableRow = tableCopy->GetNumberOfRows() - 1; tableRow >= 0;
       --tableRow)
    {
    this->GetTable()->InsertNextRow(tableCopy->GetRow(tableRow));
    }
}

//-----------------------------------------------------------------------------
bool vtkTreeHeatmapItem::Paint(vtkContext2D *painter)
{
  this->Dendrogram->PrepareToPaint();

  double treeBounds[4];
  this->Dendrogram->GetBounds(treeBounds);
  double leafSpacing = this->Dendrogram->GetLeafSpacing();

  double heatmapStartX, heatmapStartY;
  switch (this->Orientation)
    {
    case vtkDendrogramItem::UP_TO_DOWN:
      heatmapStartX = treeBounds[0] - leafSpacing / 2.0;
      heatmapStartY = treeBounds[2] - (this->GetTable()->GetNumberOfColumns() - 1) *
                      this->Heatmap->GetCellWidth() - leafSpacing / 2.0;
      break;
    case vtkDendrogramItem::DOWN_TO_UP:
      heatmapStartX = treeBounds[0] - leafSpacing / 2.0;
      heatmapStartY = treeBounds[3] + leafSpacing / 2.0;
      break;
    case vtkDendrogramItem::RIGHT_TO_LEFT:
      heatmapStartX = treeBounds[0] - (this->GetTable()->GetNumberOfColumns() - 1) *
                      this->Heatmap->GetCellWidth() - leafSpacing / 2.0;
      heatmapStartY = treeBounds[2] - leafSpacing / 2.0;
      break;
    case vtkDendrogramItem::LEFT_TO_RIGHT:
    default:
      heatmapStartX = treeBounds[1] + leafSpacing / 2.0;
      heatmapStartY = treeBounds[2] - leafSpacing / 2.0;
      break;
    }
  this->Heatmap->SetPosition(heatmapStartX, heatmapStartY);

  this->PaintChildren(painter);
  return true;
}


//-----------------------------------------------------------------------------
bool vtkTreeHeatmapItem::MouseDoubleClickEvent(
  const vtkContextMouseEvent &event)
{
  bool treeChanged = this->Dendrogram->MouseDoubleClickEvent(event);

  // update the heatmap if a subtree just collapsed or expanded.
  if (treeChanged)
    {
    this->CollapseHeatmapRows();
    }
  return treeChanged;
}

//-----------------------------------------------------------------------------
void vtkTreeHeatmapItem::CollapseHeatmapRows()
{
  vtkBitArray *collapsedRowsArray = vtkBitArray::SafeDownCast(
    this->GetTable()->GetFieldData()->GetArray("collapsed rows"));

  vtkStringArray *vertexNames = vtkStringArray::SafeDownCast(
    this->Dendrogram->GetPrunedTree()->GetVertexData()
    ->GetAbstractArray("node name"));

  vtkStringArray *rowNames = vtkStringArray::SafeDownCast(
    this->GetTable()->GetColumn(0));

  for (vtkIdType row = 0; row < this->GetTable()->GetNumberOfRows(); ++row)
    {
    std::string name = rowNames->GetValue(row);
    // if we can't find this name in the layout tree, then the corresponding
    // row in the heatmap should be marked as collapsed.
    if (vertexNames->LookupValue(name) == -1)
      {
      collapsedRowsArray->SetValue(row, 1);
      }
    else
      {
      collapsedRowsArray->SetValue(row, 0);
      }
    }
}

//-----------------------------------------------------------------------------
void vtkTreeHeatmapItem::SetOrientation(int orientation)
{
  int previousOrientation = this->Orientation;
  this->Orientation = orientation;
  this->Dendrogram->SetOrientation(this->Orientation);
  this->Heatmap->SetOrientation(this->Orientation);

  // reverse our table if we're changing from a "not backwards" orientation
  // to one that it backwards.
  if ( (this->Orientation == vtkDendrogramItem::RIGHT_TO_LEFT ||
        this->Orientation == vtkDendrogramItem::DOWN_TO_UP) &&
       (previousOrientation != vtkDendrogramItem::RIGHT_TO_LEFT &&
        previousOrientation != vtkDendrogramItem::DOWN_TO_UP) )
    {
    this->ReverseTable();
    }
}

//-----------------------------------------------------------------------------
int vtkTreeHeatmapItem::GetOrientation()
{
  return this->Orientation;
}

//-----------------------------------------------------------------------------
void vtkTreeHeatmapItem::GetBounds(double bounds[4])
{
  double treeBounds[4];
  this->Dendrogram->GetBounds(treeBounds);

  double tableBounds[4];
  this->Heatmap->GetBounds(tableBounds);

  double xMin, xMax, yMin, yMax;

  if (treeBounds[0] < tableBounds[0])
    {
    xMin = treeBounds[0];
    }
  else
    {
    xMin = tableBounds[0];
    }
  if (treeBounds[1] < tableBounds[1])
    {
    xMax = treeBounds[1];
    }
  else
    {
    xMax = tableBounds[1];
    }
  if (treeBounds[2] < tableBounds[2])
    {
    yMin = treeBounds[2];
    }
  else
    {
    yMin = tableBounds[2];
    }
  if (treeBounds[3] < tableBounds[3])
    {
    yMax = treeBounds[3];
    }
  else
    {
    yMax = tableBounds[3];
    }

  bounds[0] = xMin;
  bounds[1] = xMax;
  bounds[2] = yMin;
  bounds[3] = yMax;
}

//-----------------------------------------------------------------------------
void vtkTreeHeatmapItem::GetCenter(double *center)
{
  double bounds[4];
  this->GetBounds(bounds);

  center[0] = bounds[0] + (bounds[1] - bounds[0]) / 2.0;
  center[1] = bounds[2] + (bounds[3] - bounds[2]) / 2.0;
}

//-----------------------------------------------------------------------------
void vtkTreeHeatmapItem::GetSize(double *size)
{
  double bounds[4];
  this->GetBounds(bounds);

  size[0] = abs(bounds[1] - bounds[0]);
  size[1] = abs(bounds[3] - bounds[2]);
}

//-----------------------------------------------------------------------------
void vtkTreeHeatmapItem::SetTreeColorArray(const char *arrayName)
{
  this->Dendrogram->SetColorArray(arrayName);
}

//-----------------------------------------------------------------------------
void vtkTreeHeatmapItem::CollapseToNumberOfLeafNodes(unsigned int n)
{
  this->Dendrogram->CollapseToNumberOfLeafNodes(n);
  this->CollapseHeatmapRows();
}

//-----------------------------------------------------------------------------
vtkTree * vtkTreeHeatmapItem::GetPrunedTree()
{
  return this->Dendrogram->GetPrunedTree();
}

//-----------------------------------------------------------------------------
bool vtkTreeHeatmapItem::Hit(const vtkContextMouseEvent &vtkNotUsed(mouse))
{
  // If we are interactive, we want to catch anything that propagates to the
  // background, otherwise we do not want any mouse events.
  return this->Interactive;
}

//-----------------------------------------------------------------------------
void vtkTreeHeatmapItem::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  this->Dendrogram->PrintSelf(os, indent);
  this->Heatmap->PrintSelf(os, indent);
}

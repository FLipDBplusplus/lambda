#include "categorical_table_cell.h"

#include <inference/models/statistic/slope_t_interval.h>
#include <inference/models/statistic/slope_t_test.h>

#include "categorical_controller.h"
#include "inference/app.h"

using namespace Escher;

namespace Inference {

/* CategoricalTableCell */

CategoricalTableCell::CategoricalTableCell(
    Escher::Responder *parentResponder, Escher::TableViewDataSource *dataSource,
    Escher::ScrollViewDelegate *scrollViewDelegate)
    : Escher::Responder(parentResponder),
      m_selectableTableView(this, dataSource, this, this, scrollViewDelegate) {
  m_selectableTableView.setBackgroundColor(Escher::Palette::WallScreenDark);
  m_selectableTableView.hideScrollBars();
}

void CategoricalTableCell::didBecomeFirstResponder() {
  assert(0 <= selectedColumn() &&
         selectedColumn() < tableViewDataSource()->numberOfColumns());
  assert(selectedRow() < tableViewDataSource()->numberOfRows());
  if (selectedRow() < 0) {
    selectRow(1);
  }
  Escher::App::app()->setFirstResponder(&m_selectableTableView);
}

bool CategoricalTableCell::handleEvent(Ion::Events::Event e) {
  if (e == Ion::Events::Left) {
    // Catch left event to avoid popping controller from StackViewController
    return true;
  }
  if (e == Ion::Events::Down) {
    /* The categorical list controller will select cell below the
     * categorical table cell, so we need to scroll down the table cell first */
    assert(categoricalController()->indexOfTableCell() <
           categoricalController()->numberOfRows() - 1);
    m_selectableTableView.scrollToBottom();
    assert(m_selectableTableView.contentOffset().y() >= 0);
  }
  return false;
}

void CategoricalTableCell::tableViewDidChangeSelectionAndDidScroll(
    SelectableTableView *t, int previousSelectedCol, int previousSelectedRow,
    KDPoint previousOffset, bool withinTemporarySelection) {
  assert(t == &m_selectableTableView);
  if (!withinTemporarySelection && previousOffset != t->contentOffset()) {
    categoricalController()->didScroll();
    categoricalController()->selectableListView()->reloadData(false);
  }
}

void CategoricalTableCell::layoutSubviews(bool force) {
  /* We let an empty border as it will be drawn by the next cell (thanks to the
   * cell overlap) */
  setChildFrame(&m_selectableTableView,
                KDRect(0, 0, bounds().width(),
                       bounds().height() - Metric::CellSeparatorThickness),
                force);
}

/* InputCategoricalTableCell */

InputCategoricalTableCell::InputCategoricalTableCell(
    Escher::Responder *parentResponder, Escher::TableViewDataSource *dataSource,
    Statistic *statistic, Escher::ScrollViewDelegate *scrollViewDelegate)
    : CategoricalTableCell(parentResponder, dataSource, scrollViewDelegate),
      m_statistic(statistic),
      m_numberOfRows(0),
      m_numberOfColumns(0) {
  m_selectableTableView.margins()->setBottom(k_bottomMargin);
}

bool InputCategoricalTableCell::textFieldShouldFinishEditing(
    AbstractTextField *textField, Ion::Events::Event event) {
  return TextFieldDelegate::textFieldShouldFinishEditing(textField, event) ||
         (event == Ion::Events::Right &&
          m_selectableTableView.selectedColumn() <
              tableViewDataSource()->numberOfColumns() - 1) ||
         (event == Ion::Events::Left &&
          m_selectableTableView.selectedColumn() > 0) ||
         (event == Ion::Events::Down &&
          m_selectableTableView.selectedRow() <
              tableViewDataSource()->numberOfRows()) ||
         (event == Ion::Events::Up && m_selectableTableView.selectedRow() > 0);
}

bool InputCategoricalTableCell::textFieldDidFinishEditing(
    Escher::AbstractTextField *textField, Ion::Events::Event event) {
  double p = ParseInputFloatValue<double>(textField->draftText());
  if (HasUndefinedValue(p)) {
    return false;
  }
  int row = m_selectableTableView.selectedRow(),
      column = m_selectableTableView.selectedColumn();
  if (!tableModel()->authorizedParameterAtPosition(p, relativeRow(row),
                                                   relativeColumn(column))) {
    App::app()->displayWarning(I18n::Message::ForbiddenValue);
    return false;
  }
  tableModel()->setParameterAtPosition(p, relativeRow(row),
                                       relativeColumn(column));

  m_selectableTableView.deselectTable(true);
  // Add row or column
  if ((row == tableViewDataSource()->numberOfRows() - 1 &&
       relativeRow(tableViewDataSource()->numberOfRows()) <
           tableModel()->maxNumberOfRows()) ||
      (column == tableViewDataSource()->numberOfColumns() - 1 &&
       relativeColumn(tableViewDataSource()->numberOfColumns()) <
           tableModel()->maxNumberOfColumns())) {
    recomputeDimensionsAndReload();
  }
  m_selectableTableView.reloadCellAtLocation(column, row);
  m_selectableTableView.selectCellAtLocation(column, row);
  if (event == Ion::Events::OK || event == Ion::Events::EXE) {
    event = Ion::Events::Down;
  }
  m_selectableTableView.handleEvent(event);
  return true;
}

bool InputCategoricalTableCell::handleEvent(Ion::Events::Event event) {
  int column = m_selectableTableView.selectedColumn();
  int row = m_selectableTableView.selectedRow();
  int cellType = tableViewDataSource()->typeAtLocation(column, row);
  if (event == Ion::Events::Backspace &&
      cellType == CategoricalTableViewDataSource::k_typeOfInnerCells) {
    deleteSelectedValue();
    return true;
  } else if (event == Ion::Events::Backspace || event == Ion::Events::Clear) {
    presentClearSelectedColumnPopupIfClearable();
    return true;
  }
  return CategoricalTableCell::handleEvent(event);
}

void InputCategoricalTableCell::initCell(InferenceEvenOddEditableCell,
                                         void *cell, int index) {
  InferenceEvenOddEditableCell *c =
      static_cast<InferenceEvenOddEditableCell *>(cell);
  c->setParentResponder(&m_selectableTableView);
  c->editableTextCell()->textField()->setDelegate(this);
}

bool InputCategoricalTableCell::deleteSelectedValue() {
  int row = m_selectableTableView.selectedRow(),
      col = m_selectableTableView.selectedColumn();
  assert(relativeRow(row) >= 0 && relativeColumn(col) >= 0);
  // Remove value
  bool shouldDeleteRowOrCol = tableModel()->deleteParameterAtPosition(
      relativeRow(row), relativeColumn(col));
  if (!shouldDeleteRowOrCol) {
    // Only one cell needs to reload.
    assert(row < tableViewDataSource()->numberOfRows() &&
           col < tableViewDataSource()->numberOfColumns());
    m_selectableTableView.reloadCellAtLocation(col, row);
    return false;
  } else {
    // A row and/or column has been deleted, we should recompute data
    tableModel()->recomputeData();
    fullReload(true);
    return true;
  }
}

int InputCategoricalTableCell::numberOfElementsInColumn(int column) const {
  int n = constTableModel()->maxNumberOfRows();
  column = relativeColumn(column);
  int res = 0;
  for (int row = 0; row < n; row++) {
    res += std::isfinite(constTableModel()->parameterAtPosition(row, column));
  }
  return res;
}

void InputCategoricalTableCell::clearSelectedColumn() {
  int column = m_selectableTableView.selectedColumn();
  tableModel()->deleteParametersInColumn(relativeColumn(column));
  tableModel()->recomputeData();
  m_selectableTableView.deselectTable();
  m_selectableTableView.resetScroll();
  recomputeDimensionsAndReload(true, true);
  m_selectableTableView.selectCellAtLocation(column, 1, false);
}

bool InputCategoricalTableCell::recomputeDimensions() {
  Chi2Test::Index2D dimensions = tableModel()->computeDimensions();
  if (m_numberOfRows != dimensions.row || m_numberOfColumns != dimensions.col) {
    m_numberOfRows = dimensions.row;
    m_numberOfColumns = dimensions.col;
    return true;
  }
  return false;
}

bool InputCategoricalTableCell::recomputeDimensionsAndReload(
    bool forceReloadTableCell, bool forceReloadPage) {
  bool didChange = recomputeDimensions();
  /* Relayout when inner table changes size. We need to reload the table because
   * its width might change but it won't relayout as its frame isn't changed by
   * the InputCategoricalController */
  if (didChange || forceReloadTableCell) {
    m_selectableTableView.reloadData(false);
  }
  if (didChange || forceReloadPage) {
    categoricalController()->selectableListView()->reloadData(false);
  }
  return didChange;
}

void InputCategoricalTableCell::fullReload(bool forceReloadTableCell,
                                           bool forceReloadPage) {
  int row = m_selectableTableView.selectedRow();
  int col = m_selectableTableView.selectedColumn();
  if (row >= 0) {
    m_selectableTableView.deselectTable();
  }
  recomputeDimensionsAndReload(forceReloadTableCell, forceReloadPage);
  m_selectableTableView.selectCellAtLocation(col, row, true);
}

/* DoubleColumnTableCell */

DoubleColumnTableCell::DoubleColumnTableCell(
    Escher::Responder *parentResponder, Statistic *statistic,
    Escher::ScrollViewDelegate *scrollViewDelegate)
    : InputCategoricalTableCell(parentResponder, this, statistic,
                                scrollViewDelegate),
      DynamicCellsDataSource<InferenceEvenOddEditableCell,
                             k_doubleColumnTableNumberOfReusableCells>(this) {}

int DoubleColumnTableCell::reusableCellCount(int type) const {
  if (type == k_typeOfHeaderCells) {
    return k_maxNumberOfColumns;
  }
  return k_maxNumberOfReusableRows * k_maxNumberOfColumns;
}

HighlightCell *DoubleColumnTableCell::reusableCell(int i, int type) {
  assert(i < reusableCellCount(type));
  if (type == k_typeOfHeaderCells) {
    assert(i < k_maxNumberOfColumns);
    return headerCell(i);
  }
  return cell(i);
}

void DoubleColumnTableCell::fillCellForLocation(Escher::HighlightCell *cell,
                                                int column, int row) {
  if (typeAtLocation(column, row) == k_typeOfHeaderCells) {
    return;
  }
  InferenceEvenOddEditableCell *myCell =
      static_cast<InferenceEvenOddEditableCell *>(cell);
  fillValueCellForLocation(myCell->editableTextCell()->textField(), myCell,
                           column, row - 1, tableModel());
}

}  // namespace Inference

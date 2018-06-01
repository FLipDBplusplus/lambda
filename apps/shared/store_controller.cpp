#include "store_controller.h"
#include "../apps_container.h"
#include "../constant.h"
#include <escher/metric.h>
#include <assert.h>

using namespace Poincare;

namespace Shared {

StoreController::ContentView::ContentView(FloatPairStore * store, Responder * parentResponder, TableViewDataSource * dataSource, SelectableTableViewDataSource * selectionDataSource, TextFieldDelegate * textFieldDelegate) :
  View(),
  Responder(parentResponder),
  m_dataView(store, this, dataSource, selectionDataSource),
  m_formulaInputView(this, textFieldDelegate),
  m_displayFormulaInputView(false)
{
  m_dataView.setBackgroundColor(Palette::WallScreenDark);
  m_dataView.setVerticalCellOverlap(0);
  m_dataView.setMargins(k_margin, k_scrollBarMargin, k_scrollBarMargin, k_margin);
}

void StoreController::ContentView::displayFormulaInput(bool display) {
  if (m_displayFormulaInputView != display) {
    m_displayFormulaInputView = display;
    layoutSubviews();
    markRectAsDirty(bounds());
  }
}

void StoreController::ContentView::didBecomeFirstResponder() {
  app()->setFirstResponder(m_displayFormulaInputView ? static_cast<Responder *>(&m_formulaInputView) : static_cast<Responder *>(&m_dataView));
}

View * StoreController::ContentView::subviewAtIndex(int index) {
  assert(index >= 0 && index < numberOfSubviews());
  View * views[] = {&m_dataView, &m_formulaInputView};
  return views[index];
}

void StoreController::ContentView::layoutSubviews() {
  KDRect dataViewFrame(0, 0, bounds().width(), bounds().height() - (m_displayFormulaInputView ? BufferTextViewWithTextField::k_height : 0));
  m_dataView.setFrame(dataViewFrame);
  m_formulaInputView.setFrame(formulaFrame());
}

KDRect StoreController::ContentView::formulaFrame() const {
  return KDRect(0, bounds().height() - BufferTextViewWithTextField::k_height, bounds().width(), m_displayFormulaInputView ? BufferTextViewWithTextField::k_height : 0);
}

StoreController::StoreController(Responder * parentResponder, FloatPairStore * store, ButtonRowController * header) :
  EditableCellTableViewController(parentResponder),
  ButtonRowDelegate(header, nullptr),
  m_editableCells{},
  m_store(store),
  m_storeParameterController(this, store, this)
{
}

void StoreController::displayFormulaInput() {
  setFormulaLabel();
  contentView()->displayFormulaInput(true);
}

bool StoreController::textFieldDidFinishEditing(TextField * textField, const char * text, Ion::Events::Event event) {
  if (textField == contentView()->formulaInputView()->textField()) {
    // Handle formula input
    Expression * expression = Expression::parse(textField->text());
    if (expression == nullptr) {
      app()->displayWarning(I18n::Message::SyntaxError);
      return false;
    }
    contentView()->displayFormulaInput(false);
    fillColumnWithFormula(expression);
    delete expression;
    app()->setFirstResponder(contentView());
    return true;
  }
  AppsContainer * appsContainer = ((TextFieldDelegateApp *)app())->container();
  Context * globalContext = appsContainer->globalContext();
  double floatBody = Expression::approximateToScalar<double>(text, *globalContext);
  if (std::isnan(floatBody) || std::isinf(floatBody)) {
    app()->displayWarning(I18n::Message::UndefinedValue);
    return false;
  }
  if (!setDataAtLocation(floatBody, selectedColumn(), selectedRow())) {
    app()->displayWarning(I18n::Message::ForbiddenValue);
    return false;
  }
  // FIXME Find out if redrawing errors can be suppressed without always reloading all the data
  selectableTableView()->reloadData();
  if (event == Ion::Events::EXE || event == Ion::Events::OK) {
    selectableTableView()->selectCellAtLocation(selectedColumn(), selectedRow()+1);
  } else {
    selectableTableView()->handleEvent(event);
  }
  return true;
}

int StoreController::numberOfColumns() {
  return FloatPairStore::k_numberOfColumnsPerSeries * FloatPairStore::k_numberOfSeries;
}

KDCoordinate StoreController::columnWidth(int i) {
  return k_cellWidth;
}

KDCoordinate StoreController::cumulatedWidthFromIndex(int i) {
  return i*k_cellWidth;
}

int StoreController::indexFromCumulatedWidth(KDCoordinate offsetX) {
  return (offsetX-1) / k_cellWidth;
}

HighlightCell * StoreController::reusableCell(int index, int type) {
  assert(index >= 0);
  switch (type) {
    case k_titleCellType:
      assert(index < k_numberOfTitleCells);
      return titleCells(index);
    case k_editableCellType:
      assert(index < k_maxNumberOfEditableCells);
      return m_editableCells[index];
    default:
      assert(false);
      return nullptr;
  }
}

int StoreController::reusableCellCount(int type) {
  return type == k_titleCellType ? k_numberOfTitleCells : k_maxNumberOfEditableCells;
}

int StoreController::typeAtLocation(int i, int j) {
  return j == 0 ? k_titleCellType : k_editableCellType;
}

void StoreController::willDisplayCellAtLocation(HighlightCell * cell, int i, int j) {
  // Handle the separator
  if (cellAtLocationIsEditable(i, j)) {
    bool shouldHaveLeftSeparator = i % FloatPairStore::k_numberOfColumnsPerSeries == 0;
    static_cast<StoreCell *>(cell)->setSeparatorLeft(shouldHaveLeftSeparator);
  }
  // Handle empty cells
  if (j > 0 && j > m_store->numberOfPairsOfSeries(seriesAtColumn(i)) && j < numberOfRows()) {
    StoreCell * myCell = static_cast<StoreCell *>(cell);
    myCell->editableTextCell()->textField()->setText("");
    if (cellShouldBeTransparent(i,j)) {
      myCell->setHide(true);
    } else {
      myCell->setEven(j%2 == 0);
      myCell->setHide(false);
    }
    return;
  }
  if (cellAtLocationIsEditable(i, j)) {
    static_cast<StoreCell *>(cell)->setHide(false);
  }
  willDisplayCellAtLocationWithDisplayMode(cell, i, j, PrintFloat::Mode::Decimal);
}

const char * StoreController::title() {
  return I18n::translate(I18n::Message::DataTab);
}

bool StoreController::handleEvent(Ion::Events::Event event) {
  if (event == Ion::Events::Up) {
    selectableTableView()->deselectTable();
    assert(selectedRow() == -1);
    app()->setFirstResponder(tabController());
    return true;
  }
  assert(selectedColumn() >= 0 && selectedColumn() < numberOfColumns());
  int series = seriesAtColumn(selectedColumn());
  if ((event == Ion::Events::OK || event == Ion::Events::EXE) && selectedRow() == 0) {
    m_storeParameterController.selectXColumn(selectedColumn()%FloatPairStore::k_numberOfColumnsPerSeries == 0);
    m_storeParameterController.selectSeries(series);
    StackViewController * stack = ((StackViewController *)parentResponder()->parentResponder());
    stack->push(&m_storeParameterController);
    return true;
  }
  if (event == Ion::Events::Backspace) {
    if (selectedRow() == 0 || selectedRow() > m_store->numberOfPairsOfSeries(selectedColumn()/FloatPairStore::k_numberOfColumnsPerSeries)) {
      return false;
    }
    m_store->deletePairOfSeriesAtIndex(series, selectedRow()-1);
    selectableTableView()->reloadData();
    return true;
  }
  return false;
}

void StoreController::didBecomeFirstResponder() {
  if (selectedRow() < 0 || selectedColumn() < 0) {
    selectCellAtLocation(0, 0);
  }
  EditableCellTableViewController::didBecomeFirstResponder();
  app()->setFirstResponder(contentView());
}

Responder * StoreController::tabController() const {
  return (parentResponder()->parentResponder()->parentResponder());
}

SelectableTableView * StoreController::selectableTableView() {
  return contentView()->dataView();
}

bool StoreController::cellAtLocationIsEditable(int columnIndex, int rowIndex) {
  if (rowIndex > 0) {
    return true;
  }
  return false;
}

bool StoreController::setDataAtLocation(double floatBody, int columnIndex, int rowIndex) {
  m_store->set(floatBody, seriesAtColumn(columnIndex), columnIndex%FloatPairStore::k_numberOfColumnsPerSeries, rowIndex-1);
  return true;
}

double StoreController::dataAtLocation(int columnIndex, int rowIndex) {
  return m_store->get(seriesAtColumn(columnIndex), columnIndex%FloatPairStore::k_numberOfColumnsPerSeries, rowIndex-1);
}

int StoreController::numberOfElements() {
  int result = 0;
  for (int i = 0; i < FloatPairStore::k_numberOfSeries; i++) {
    result = max(result, m_store->numberOfPairsOfSeries(i));
  }
  return result;
}

int StoreController::maxNumberOfElements() const {
  return FloatPairStore::k_maxNumberOfPairs;
}

View * StoreController::loadView() {
  ContentView * contentView = new ContentView(m_store, this, this, this, this);
  for (int i = 0; i < k_maxNumberOfEditableCells; i++) {
    m_editableCells[i] = new StoreCell(contentView->dataView(), this, m_draftTextBuffer);
  }
  return contentView;
}

void StoreController::unloadView(View * view) {
  for (int i = 0; i < k_maxNumberOfEditableCells; i++) {
    delete m_editableCells[i];
    m_editableCells[i] = nullptr;
  }
  delete view;
}

bool StoreController::cellShouldBeTransparent(int i, int j) {
  int seriesIndex = i/FloatPairStore::k_numberOfColumnsPerSeries;
  return j > 1 + m_store->numberOfPairsOfSeries(seriesIndex);
}

}

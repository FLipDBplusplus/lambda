#include "expression_model_list_controller.h"
#include <assert.h>

namespace Shared {

ExpressionModelListController::ExpressionModelListController(Responder * parentResponder, I18n::Message text) :
  ViewController(parentResponder),
  m_addNewModel()
{
  m_addNewModel.setMessage(text);
}

/* Table Data Source */
int ExpressionModelListController::numberOfExpressionRows() {
  if (modelStore()->numberOfModels() == modelStore()->maxNumberOfModels()) {
    return modelStore()->numberOfModels();
  }
  return 1 + modelStore()->numberOfModels();
}

KDCoordinate ExpressionModelListController::expressionRowHeight(int j) {
  if (isAddEmptyRow(j)) {
    return Metric::StoreRowHeight;
  }
  ExpressionModel * m = modelStore()->modelAtIndex(j);
  if (m->layout().isUninitialized()) {
    return Metric::StoreRowHeight;
  }
  KDCoordinate modelSize = m->layout().layoutSize().height();
  return modelSize + Metric::StoreRowHeight - KDFont::LargeFont->glyphSize().height();
}

void ExpressionModelListController::willDisplayExpressionCellAtIndex(HighlightCell * cell, int j) {
  EvenOddExpressionCell * myCell = (EvenOddExpressionCell *)cell;
  ExpressionModel * m = modelStore()->modelAtIndex(j);
  myCell->setLayout(m->layout());
}

/* Responder */

bool ExpressionModelListController::handleEventOnExpression(Ion::Events::Event event) {
  if (selectedRow() < 0) {
    return false;
  }
  if (event == Ion::Events::OK || event == Ion::Events::EXE) {
    if (isAddEmptyRow(selectedRow())) {
      addEmptyModel();
      selectableTableView()->reloadCellAtLocation(selectedColumn(), selectedRow());
      return true;
    }
    ExpressionModel * model = modelStore()->modelAtIndex(modelIndexForRow(selectedRow()));
    editExpression(model, event);
    return true;
  }
  if (event == Ion::Events::Backspace && !isAddEmptyRow(selectedRow())) {
    ExpressionModel * model = modelStore()->modelAtIndex(modelIndexForRow(selectedRow()));
    if (model->shouldBeClearedBeforeRemove()) {
      reinitExpression(model);
      selectableTableView()->reloadCellAtLocation(selectedColumn(), selectedRow());
    } else {
      if (removeModelRow(model)) {
        int newSelectedRow = selectedRow() >= numberOfExpressionRows() ? numberOfExpressionRows()-1 : selectedRow();
        selectCellAtLocation(selectedColumn(), newSelectedRow);
        selectableTableView()->reloadCellAtLocation(selectedColumn(), selectedRow());
        selectableTableView()->reloadData();
      }
    }
    return true;
  }
  if ((event.hasText() || event == Ion::Events::XNT || event == Ion::Events::Paste || event == Ion::Events::Toolbox || event == Ion::Events::Var)
      && !isAddEmptyRow(selectedRow())) {
    ExpressionModel * model = modelStore()->modelAtIndex(modelIndexForRow(selectedRow()));
    editExpression(model, event);
    return true;
  }
  return false;
}

void ExpressionModelListController::addEmptyModel() {
  ExpressionModel * e = modelStore()->addEmptyModel();
  selectableTableView()->reloadData();
  editExpression(e, Ion::Events::OK);
}

void ExpressionModelListController::reinitExpression(ExpressionModel * model) {
  model->setContent("");
  selectableTableView()->reloadData();
}

void ExpressionModelListController::editExpression(ExpressionModel * model, Ion::Events::Event event) {
  char * initialText = nullptr;
  char initialTextContent[TextField::maxBufferSize()];
  if (event == Ion::Events::OK || event == Ion::Events::EXE) {
    strlcpy(initialTextContent, model->text(), sizeof(initialTextContent));
    initialText = initialTextContent;
  }
  inputController()->edit(this, event, model, initialText,
    [](void * context, void * sender){
    ExpressionModel * myModel = static_cast<ExpressionModel *>(context);
    InputViewController * myInputViewController = (InputViewController *)sender;
    const char * textBody = myInputViewController->textBody();
    myModel->setContent(textBody);
    return true; // TODO we should return a result from myModel->setContent, but we will remove ExpressionModelListController soon anyway
    },
    [](void * context, void * sender){
    return true;
    });
}

bool ExpressionModelListController::removeModelRow(ExpressionModel * model) {
  modelStore()->removeModel(model);
  return true;
}

int ExpressionModelListController::modelIndexForRow(int j) {
  return j;
}

bool ExpressionModelListController::isAddEmptyRow(int j) {
  return j == modelStore()->numberOfModels();
}

}

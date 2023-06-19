#include "goodness_table_cell.h"

#include <shared/column_parameter_controller.h>

#include "inference/statistic/chi_square_and_slope/input_goodness_controller.h"
#include "input_goodness_controller.h"

using namespace Escher;

namespace Inference {

GoodnessTableCell::GoodnessTableCell(
    Responder* parentResponder, GoodnessTest* test,
    InputGoodnessController* inputGoodnessController)
    : DoubleColumnTableCell(parentResponder, test),
      m_inputGoodnessController(inputGoodnessController) {
  for (int i = 0; i < GoodnessTest::k_maxNumberOfColumns; i++) {
    m_header[i].setMessage(k_columnHeaders[i]);
    m_header[i].setEven(true);
    m_header[i].setMessageFont(KDFont::Size::Small);
  }
}

bool GoodnessTableCell::textFieldDidFinishEditing(
    Escher::AbstractTextField* textField, Ion::Events::Event event) {
  int previousDegreeOfFreedom = statistic()->computeDegreesOfFreedom();
  if (InputCategoricalTableCell::textFieldDidFinishEditing(textField, event)) {
    int newDegreeOfFreedom = statistic()->computeDegreesOfFreedom();
    if (previousDegreeOfFreedom != newDegreeOfFreedom) {
      statistic()->setDegreeOfFreedom(newDegreeOfFreedom);
      m_inputGoodnessController->updateDegreeOfFreedomCell();
    }
    return true;
  }
  return false;
}

bool GoodnessTableCell::recomputeDimensions() {
  // Update degree of freedom if Number of non-empty rows changed
  if (InputCategoricalTableCell::recomputeDimensions()) {
    int newDegreeOfFreedom = statistic()->computeDegreesOfFreedom();
    statistic()->setDegreeOfFreedom(newDegreeOfFreedom);
    m_inputGoodnessController->updateDegreeOfFreedomCell();
    return true;
  }
  return false;
}

int GoodnessTableCell::fillColumnName(int column, char* buffer) {
  return strlcpy(buffer, I18n::translate(k_columnHeaders[column]),
                 Shared::ColumnParameterController::k_titleBufferSize);
}

CategoricalController* GoodnessTableCell::categoricalController() {
  return m_inputGoodnessController;
}

}  // namespace Inference

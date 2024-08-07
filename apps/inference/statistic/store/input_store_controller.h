#ifndef INFERENCE_STATISTIC_INPUT_STORE_CONTROLLER_H
#define INFERENCE_STATISTIC_INPUT_STORE_CONTROLLER_H

#include <escher/buffer_text_highlight_cell.h>

#include "../categorical_controller.h"
#include "../input_controller.h"
#include "store_column_parameter_controller.h"
#include "store_table_cell.h"

namespace Inference {

class InputStoreController : public InputCategoricalController,
                             Escher::DropdownCallback {
 public:
  InputStoreController(Escher::StackViewController* parent,
                       Escher::ViewController* resultsController,
                       Statistic* statistic, Poincare::Context* context);

  // Responder
  bool handleEvent(Ion::Events::Event event) override;

  // ViewController
  const char* title() override {
    InputController::InputTitle(this, m_statistic, m_titleBuffer,
                                InputController::k_titleBufferSize);
    return m_titleBuffer;
  }
  ViewController::TitlesDisplay titlesDisplay() override {
    return m_statistic->subApp() == Statistic::SubApp::Interval
               ? ViewController::TitlesDisplay::DisplayLastTitle
           : m_statistic->canChooseDataset()
               ? ViewController::TitlesDisplay::DisplayLastAndThirdToLast
               : ViewController::TitlesDisplay::DisplayLastTwoTitles;
  }
  void viewWillAppear() override;
  void initView() override;

  // DropdownCallback
  void onDropdownSelected(int selectedRow) override;

  void initSeriesSelection() {
    selectSeriesForDropdownRow(m_dropdownCell.dropdown()->selectedRow());
  }

 private:
  class DropdownDataSource : public Escher::ExplicitListViewDataSource {
   public:
    static int RowForSeriesPair(int s1, int s2) {
      return s1 == 1 ? 2 : s2 == 1 ? 0 : 1;
    }
    static int Series1ForRow(int row) { return row == 2 ? 1 : 0; }
    static int Series2ForRow(int row) { return row == 0 ? 1 : 2; }

    int numberOfRows() const override { return k_numberOfRows; }
    Escher::HighlightCell* cell(int row) override { return &m_cells[row]; }

   private:
    constexpr static int k_numberOfRows = 3;  // FIXME
    Escher::SmallBufferTextHighlightCell m_cells[k_numberOfRows];
  };

  class PrivateStackViewController
      : public Escher::StackViewController::Default {
   public:
    using Escher::StackViewController::Default::Default;
    TitlesDisplay titlesDisplay() override { return m_titlesDisplay; }
    void setTitlesDisplay(TitlesDisplay titlesDisplay) {
      m_titlesDisplay = titlesDisplay;
    }

   private:
    TitlesDisplay m_titlesDisplay;
  };

  constexpr static int k_dropdownCellIndex = 0;
  constexpr static int k_maxNumberOfExtraParameters = 2;

  Escher::HighlightCell* explicitCellAtRow(int row) override;
  InputCategoricalTableCell* categoricalTableCell() override {
    return &m_storeTableCell;
  }
  void createDynamicCells() override;
  int indexOfTableCell() const override { return k_dropdownCellIndex + 1; }
  int indexOfSignificanceCell() const override {
    return indexOfFirstExtraParameter() + numberOfExtraParameters();
  }
  int indexOfFirstExtraParameter() const { return indexOfTableCell() + 1; }
  Escher::StackViewController* stackController() const {
    return static_cast<Escher::StackViewController*>(parentResponder());
  }
  const Shared::DoublePairStore* store() const {
    return m_storeTableCell.store();
  }
  int numberOfExtraParameters() const {
    return m_statistic->distributionType() == DistributionType::Z
               ? m_statistic->significanceTestType() ==
                         SignificanceTestType::TwoMeans
                     ? 2
                     : 1
               : 0;
  }
  int indexOfEditedParameterAtIndex(int index) const override;
  void selectSeriesForDropdownRow(int row);

  DropdownDataSource m_dropdownDataSource;
  DropdownCategoricalCell m_dropdownCell;
  InputCategoricalCell<Escher::LayoutView>
      m_extraParameters[k_maxNumberOfExtraParameters];
  StoreTableCell m_storeTableCell;
  /* This second stack view controller is used to make the banner of the store
   * parameter controller white, which deviates from the style of the main
   * stack view controller (gray scales). */
  PrivateStackViewController m_secondStackController;
  StoreColumnParameterController m_storeParameterController;
  char m_titleBuffer[InputController::k_titleBufferSize];
  Statistic::SubApp m_loadedSubApp;
  DistributionType m_loadedDistribution;
  SignificanceTestType m_loadedTest;
};

}  // namespace Inference

#endif

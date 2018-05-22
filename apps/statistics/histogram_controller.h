#ifndef STATISTICS_HISTOGRAM_CONTROLLER_H
#define STATISTICS_HISTOGRAM_CONTROLLER_H

#include <escher.h>
#include "store.h"
#include "histogram_view.h"
#include "histogram_banner_view.h"
#include "histogram_parameter_controller.h"
#include "../shared/curve_view.h"

namespace Statistics {

class HistogramController : public ViewController, public ButtonRowDelegate, public AlternateEmptyViewDelegate {

public:
  HistogramController(Responder * parentResponder, ButtonRowController * header, Store * store, int series, uint32_t * m_storeVersion, uint32_t * m_barVersion, uint32_t * m_rangeVersion, int * m_selectedBarIndex);
  StackViewController * stackController();
  HistogramParameterController * histogramParameterController() { return &m_histogramParameterController; }
  void setCurrentDrawnSeries(int series);

  // ButtonRowDelegate
  int numberOfButtons(ButtonRowController::Position) const override;
  Button * buttonAtIndex(int index, ButtonRowController::Position position) const override;

  // AlternateEmptyViewDelegate
  bool isEmpty() const override;
  I18n::Message emptyMessage() override;
  Responder * defaultController() override;

  // ViewController
  const char * title() override;
  View * view() override { return &m_view; }
  void viewWillAppear() override;

  // Responder
  bool handleEvent(Ion::Events::Event event) override;
  void didBecomeFirstResponder() override;
  void willExitResponderChain(Responder * nextFirstResponder) override;
private:
  constexpr static int k_maxNumberOfBarsPerWindow = 100;
  constexpr static int k_maxIntervalLegendLength = 33;
  constexpr static int k_maxLegendLength = 13;
  constexpr static int k_maxNumberOfCharacters = 30;
  class ContentView : public View {
  public:
    ContentView(HistogramController * controller, Store * store);
    void reload() {}
    HistogramView * histogramViewAtIndex(int index);
    int seriesOfSubviewAtIndex(int index);
    HistogramBannerView * bannerView() { return &m_bannerView; }
    void layoutSubviews() override;
  private:
    int numberOfSubviews() const override;
    View * subviewAtIndex(int index) override;
    HistogramView m_histogramView1;
    HistogramView m_histogramView2;
    HistogramView m_histogramView3;
    HistogramBannerView m_bannerView;
    Store * m_store; // TODO Do not duplicate this pointer
  };
  Responder * tabController() const;
  void reloadBannerView();
  void initRangeParameters();
  void initYRangeParameters(int series);
  void initBarParameters();
  void initBarSelection();
  // return true if the window has scrolled
  bool moveSelection(int deltaIndex);
  Button m_settingButton;
  Store * m_store;
  ContentView m_view;
  uint32_t * m_storeVersion;
  uint32_t * m_barVersion;
  uint32_t * m_rangeVersion;
  int m_selectedSeries;
  int * m_selectedBarIndex;
  HistogramParameterController m_histogramParameterController;
};

}


#endif

#include "histogram_controller.h"
#include "../apps_container.h"
#include "app.h"
#include <cmath>
#include <assert.h>
#include <float.h>

using namespace Poincare;
using namespace Shared;

namespace Statistics {

HistogramController::ContentView::ContentView(HistogramController * controller, Store * store) :
  m_histogramView1(controller, store, 0, &m_bannerView),
  m_histogramView2(controller, store, 1, &m_bannerView),
  m_histogramView3(controller, store, 2, &m_bannerView),
  m_bannerView(),
  m_store(store)
{
}

HistogramView *  HistogramController::ContentView::histogramViewAtIndex(int index) {
  assert(index >= 0 && index < 3);
  HistogramView * views[] = {&m_histogramView1, &m_histogramView2, &m_histogramView3};
  return views[index];
}

int HistogramController::ContentView::seriesOfSubviewAtIndex(int index) {
  assert(index >= 0 && index < numberOfSubviews());
  return static_cast<HistogramView *>(subviewAtIndex(index))->series();
}

void HistogramController::ContentView::layoutSubviews() {
  int numberSubviews = numberOfSubviews();
  KDCoordinate subviewHeight = bounds().height()/numberSubviews;
  int displayedSubviewIndex = 0;
  for (int i = 0; i < 3; i++) {
    if (!m_store->seriesIsEmpty(i)) {
      KDRect frame = KDRect(0, displayedSubviewIndex*subviewHeight, bounds().width(), subviewHeight);
      subviewAtIndex(displayedSubviewIndex)->setFrame(frame);
      displayedSubviewIndex++;
    }
  }
}

int HistogramController::ContentView::numberOfSubviews() const {
  int result = m_store->numberOfNonEmptySeries();
  assert(result <= Store::k_numberOfSeries);
  return result;
}

View * HistogramController::ContentView::subviewAtIndex(int index) {
  int seriesIndex = 0;
  int nonEmptySeriesIndex = 0;
  while (nonEmptySeriesIndex < index && seriesIndex < Store::k_numberOfSeries) {
    if (!m_store->seriesIsEmpty(seriesIndex)) {
      nonEmptySeriesIndex++;
    }
    seriesIndex++;
  }
  if (nonEmptySeriesIndex == index) {
    assert(seriesIndex >=0 && seriesIndex < 3);
    View * views[] = {&m_histogramView1, &m_histogramView2, &m_histogramView3};
    return views[seriesIndex];
  }
  assert(false);
  return nullptr;
}

HistogramController::HistogramController(Responder * parentResponder, ButtonRowController * header, Store * store, int series, uint32_t * storeVersion, uint32_t * barVersion, uint32_t * rangeVersion, int * selectedBarIndex) :
  ViewController(parentResponder),
  ButtonRowDelegate(header, nullptr),
  m_settingButton(this, I18n::Message::HistogramSet, Invocation([](void * context, void * sender) {
    HistogramController * histogramController = (HistogramController *) context;
    StackViewController * stack = ((StackViewController *)histogramController->stackController());
    stack->push(histogramController->histogramParameterController());
  }, this)),
  m_store(store),
  m_view(this, store),
  m_storeVersion(storeVersion),
  m_barVersion(barVersion),
  m_rangeVersion(rangeVersion),
  m_selectedSeries(-1),
  m_selectedBarIndex(selectedBarIndex),
  m_histogramParameterController(nullptr, store)
{
}

StackViewController * HistogramController::stackController() {
  StackViewController * stack = (StackViewController *)(parentResponder()->parentResponder()->parentResponder());
  return stack;
}

void HistogramController::setCurrentDrawnSeries(int series) {
  initYRangeParameters(series);
}

int HistogramController::numberOfButtons(ButtonRowController::Position) const {
  return isEmpty() ? 0 : 1;
}
Button * HistogramController::buttonAtIndex(int index, ButtonRowController::Position) const {
  return (Button *)&m_settingButton;
}

bool HistogramController::isEmpty() const {
  return m_store->isEmpty();
}

I18n::Message HistogramController::emptyMessage() {
  return I18n::Message::NoDataToPlot;
}

Responder * HistogramController::defaultController() {
  return tabController();
}

const char * HistogramController::title() {
  return I18n::translate(I18n::Message::HistogramTab);
}

void HistogramController::viewWillAppear() {
  if (m_selectedSeries < 0) {
    m_selectedSeries = m_view.seriesOfSubviewAtIndex(0);
    m_view.histogramViewAtIndex(m_selectedSeries)->selectMainView(true);
    header()->setSelectedButton(-1);
  }
  reloadBannerView();
  m_view.reload();
}

bool HistogramController::handleEvent(Ion::Events::Event event) {
  if (event == Ion::Events::Down) {
    if (!m_view.histogramViewAtIndex(m_selectedSeries)->isMainViewSelected()) {
      header()->setSelectedButton(-1);
      m_view.histogramViewAtIndex(m_selectedSeries)->selectMainView(true);
      reloadBannerView();
      m_view.reload();
      app()->setFirstResponder(this);
      return true;
    }
    return false;
  }
  if (event == Ion::Events::Up) {
    if (!m_view.histogramViewAtIndex(m_selectedSeries)->isMainViewSelected()) {
      header()->setSelectedButton(-1);
      app()->setFirstResponder(tabController());
      return true;
    }
    m_view.histogramViewAtIndex(m_selectedSeries)->selectMainView(false);
    header()->setSelectedButton(0);
    return true;
  }
  if (m_view.histogramViewAtIndex(m_selectedSeries)->isMainViewSelected() && (event == Ion::Events::Left || event == Ion::Events::Right)) {
    int direction = event == Ion::Events::Left ? -1 : 1;
    if (moveSelection(direction)) {
      reloadBannerView();
      m_view.reload();
    }
    return true;
  }
  return false;
}

void HistogramController::didBecomeFirstResponder() {
  uint32_t storeChecksum = m_store->storeChecksum();
  if (*m_storeVersion != storeChecksum) {
    *m_storeVersion = storeChecksum;
    initBarParameters();
    initRangeParameters();
  }
  uint32_t barChecksum = m_store->barChecksum();
  if (*m_barVersion != barChecksum) {
    *m_barVersion = barChecksum;
    initRangeParameters();
  }
  uint32_t rangeChecksum = m_store->rangeChecksum();
  if (*m_rangeVersion != rangeChecksum) {
    *m_rangeVersion = rangeChecksum;
    initBarSelection();
    reloadBannerView();
  }
  if (!m_view.histogramViewAtIndex(m_selectedSeries)->isMainViewSelected()) {
    header()->setSelectedButton(0);
  } else {
    m_view.histogramViewAtIndex(m_selectedSeries)->setHighlight(m_store->startOfBarAtIndex(m_selectedSeries, *m_selectedBarIndex), m_store->endOfBarAtIndex(0, *m_selectedBarIndex));
  }
}

void HistogramController::willExitResponderChain(Responder * nextFirstResponder) {
  if (nextFirstResponder == nullptr || nextFirstResponder == tabController()) {
    m_view.histogramViewAtIndex(m_selectedSeries)->selectMainView(false);
    header()->setSelectedButton(-1);
    m_view.reload();
  }
}

Responder * HistogramController::tabController() const {
  return (parentResponder()->parentResponder()->parentResponder()->parentResponder());
}

void HistogramController::reloadBannerView() {
  char buffer[k_maxNumberOfCharacters+ PrintFloat::bufferSizeForFloatsWithPrecision(Constant::LargeNumberOfSignificantDigits)*2];
  int numberOfChar = 0;

  // Add Interval Data
  const char * legend = " [";
  int legendLength = strlen(legend);
  strlcpy(buffer, legend, legendLength+1);
  numberOfChar += legendLength;

  // Add lower bound
  double lowerBound = m_store->startOfBarAtIndex(m_selectedSeries, *m_selectedBarIndex);
  numberOfChar += PrintFloat::convertFloatToText<double>(lowerBound, buffer+numberOfChar, PrintFloat::bufferSizeForFloatsWithPrecision(Constant::LargeNumberOfSignificantDigits), Constant::LargeNumberOfSignificantDigits);

  buffer[numberOfChar++] = ';';

  // Add upper bound
  double upperBound = m_store->endOfBarAtIndex(m_selectedSeries, *m_selectedBarIndex);
  numberOfChar += PrintFloat::convertFloatToText<double>(upperBound, buffer+numberOfChar, PrintFloat::bufferSizeForFloatsWithPrecision(Constant::LargeNumberOfSignificantDigits), Constant::LargeNumberOfSignificantDigits);

  buffer[numberOfChar++] = '[';

  // Padding
  for (int i = numberOfChar; i < k_maxIntervalLegendLength; i++) {
    buffer[numberOfChar++] = ' ';
  }
  buffer[k_maxIntervalLegendLength] = 0;
  m_view.bannerView()->setLegendAtIndex(buffer, 1);

  // Add Size Data
  numberOfChar = 0;
  legend = ": ";
  legendLength = strlen(legend);
  strlcpy(buffer, legend, legendLength+1);
  numberOfChar += legendLength;
  double size = m_store->heightOfBarAtIndex(m_selectedSeries, *m_selectedBarIndex);
  numberOfChar += PrintFloat::convertFloatToText<double>(size, buffer+numberOfChar, PrintFloat::bufferSizeForFloatsWithPrecision(Constant::LargeNumberOfSignificantDigits), Constant::LargeNumberOfSignificantDigits);
  // Padding
  for (int i = numberOfChar; i < k_maxLegendLength; i++) {
    buffer[numberOfChar++] = ' ';
  }
  buffer[k_maxLegendLength] = 0;
  m_view.bannerView()->setLegendAtIndex(buffer, 3);

  // Add Frequency Data
  numberOfChar = 0;
  legend = ": ";
  legendLength = strlen(legend);
  strlcpy(buffer, legend, legendLength+1);
  numberOfChar += legendLength;
  double frequency = size/m_store->sumOfOccurrences(m_selectedSeries);
  numberOfChar += PrintFloat::convertFloatToText<double>(frequency, buffer+numberOfChar, PrintFloat::bufferSizeForFloatsWithPrecision(Constant::LargeNumberOfSignificantDigits), Constant::LargeNumberOfSignificantDigits);
  // Padding
  for (int i = numberOfChar; i < k_maxLegendLength; i++) {
    buffer[numberOfChar++] = ' ';
  }
  buffer[k_maxLegendLength] = 0;
  m_view.bannerView()->setLegendAtIndex(buffer, 5);
}

bool HistogramController::moveSelection(int deltaIndex) {
  int newSelectedBarIndex = *m_selectedBarIndex;
  if (deltaIndex > 0) {
    do {
      newSelectedBarIndex++;
    } while (m_store->heightOfBarAtIndex(m_selectedSeries, newSelectedBarIndex) == 0 && newSelectedBarIndex < m_store->numberOfBars(m_selectedSeries));  } else {
    do {
      newSelectedBarIndex--;
    } while (m_store->heightOfBarAtIndex(m_selectedSeries, newSelectedBarIndex) == 0 && newSelectedBarIndex >= 0);
  }
  if (newSelectedBarIndex >= 0 && newSelectedBarIndex < m_store->numberOfBars(m_selectedSeries) && *m_selectedBarIndex != newSelectedBarIndex) {
    *m_selectedBarIndex = newSelectedBarIndex;
    m_view.histogramViewAtIndex(m_selectedSeries)->setHighlight(m_store->startOfBarAtIndex(m_selectedSeries, *m_selectedBarIndex), m_store->endOfBarAtIndex(m_selectedSeries, *m_selectedBarIndex));
    m_store->scrollToSelectedBarIndex(m_selectedSeries, *m_selectedBarIndex);
    return true;
  }
  return false;
}

void HistogramController::initRangeParameters() {
  float min = m_store->firstDrawnBarAbscissa();
  float max = m_store->maxValue(m_selectedSeries);
  float barWidth = m_store->barWidth();
  float xMin = min;
  float xMax = max + barWidth;
  /* if a bar is represented by less than one pixel, we cap xMax */
  if ((xMax - xMin)/barWidth > k_maxNumberOfBarsPerWindow) {
    xMax = xMin + k_maxNumberOfBarsPerWindow*barWidth;
  }
  /* Edge case */
  if (xMin >= xMax) {
    xMax = xMin + 10.0f*barWidth;
  }
  m_store->setXMin(xMin - Store::k_displayLeftMarginRatio*(xMax-xMin));
  m_store->setXMax(xMax + Store::k_displayRightMarginRatio*(xMax-xMin));

  initYRangeParameters(m_selectedSeries);
}

void HistogramController::initYRangeParameters(int series) {
  float yMax = -FLT_MAX;
  for (int index = 0; index < m_store->numberOfBars(series); index++) {
    float size = m_store->heightOfBarAtIndex(series, index);
    if (size > yMax) {
      yMax = size;
    }
  }
  yMax = yMax/m_store->sumOfOccurrences(series);
  yMax = yMax < 0 ? 1 : yMax;
  m_store->setYMin(-Store::k_displayBottomMarginRatio*yMax);
  m_store->setYMax(yMax*(1.0f+Store::k_displayTopMarginRatio));
}

void HistogramController::initBarParameters() {
  float min = m_store->minValue(m_selectedSeries);
  float max = m_store->maxValue(m_selectedSeries);
  max = min >= max ? min + std::pow(10.0f, std::floor(std::log10(std::fabs(min)))-1.0f) : max;
  m_store->setFirstDrawnBarAbscissa(min);
  float barWidth = m_store->computeGridUnit(CurveViewRange::Axis::X, min, max);
  if (barWidth <= 0.0f) {
    barWidth = 1.0f;
  }
  m_store->setBarWidth(barWidth);
}

void HistogramController::initBarSelection() {
  *m_selectedBarIndex = 0;
  while ((m_store->heightOfBarAtIndex(m_selectedSeries, *m_selectedBarIndex) == 0 ||
      m_store->startOfBarAtIndex(m_selectedSeries, *m_selectedBarIndex) < m_store->firstDrawnBarAbscissa()) && *m_selectedBarIndex < m_store->numberOfBars(m_selectedSeries)) {
    *m_selectedBarIndex = *m_selectedBarIndex+1;
  }
  if (*m_selectedBarIndex >= m_store->numberOfBars(m_selectedSeries)) {
    /* No bar is after m_firstDrawnBarAbscissa, so we select the first bar */
    *m_selectedBarIndex = 0;
    while (m_store->heightOfBarAtIndex(m_selectedSeries, *m_selectedBarIndex) == 0 && *m_selectedBarIndex < m_store->numberOfBars(m_selectedSeries)) {
      *m_selectedBarIndex = *m_selectedBarIndex+1;
    }
  }
  m_store->scrollToSelectedBarIndex(m_selectedSeries, *m_selectedBarIndex);
}

}

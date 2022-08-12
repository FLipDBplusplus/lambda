#include "function_graph_cell.h"
#include "../app.h"
#include "escher/palette.h"
#include "kandinsky/color.h"
#include "poincare/context.h"
#include "poincare/print.h"
#include <cmath>

using namespace Shared;
using namespace Poincare;
using namespace Escher;

namespace Calculation {

template<size_t N>
void FunctionAxis<N>::reloadAxis(AbstractPlotView * plotView, AbstractPlotView::Axis axis) {
  PlotPolicy::LabeledAxis<N>::reloadAxis(plotView, axis);
  const FunctionModel * model = static_cast<const FunctionModel *>(plotView->range());
  float t = axis == AbstractPlotView::Axis::Horizontal ? model->abscissa() : model->ordinate();
  // Compute special labels content and position
  Print::CustomPrintf(m_specialLabel, PlotPolicy::AbstractLabeledAxis::k_labelBufferMaxSize, "%*.*ef", t, Preferences::PrintFloatMode::Decimal, k_labelsPrecision);
  Coordinate2D<float> position = axis == AbstractPlotView::Axis::Horizontal ? Coordinate2D(t, 0.0f) : Coordinate2D(0.0f, t);
  m_specialLabelRect = plotView->labelRect(m_specialLabel, position, AbstractPlotView::RelativePosition::There, AbstractPlotView::RelativePosition::After);
}

template<size_t N>
void FunctionAxis<N>::drawAxis(const AbstractPlotView * plotView, KDContext * ctx, KDRect rect, AbstractPlotView::Axis axis) const {
  const FunctionModel * model = static_cast<const FunctionModel *>(plotView->range());
  float t = axis == AbstractPlotView::Axis::Horizontal ? model->abscissa() : model->ordinate();
  float other = axis == AbstractPlotView::Axis::Horizontal ? model->ordinate() : model->abscissa();
  // Draw the usual graduations
  PlotPolicy::SimpleAxis::drawAxis(plotView, ctx, rect, axis);
  // Draw the dashed lines since they are the ticks of the special labels
  plotView->drawStraightSegment(ctx, rect, axis, other, 0.0f, t, Palette::Red, 1, 3);
  // Draw the special label
  PlotPolicy::AbstractLabeledAxis::drawLabel(N, t, plotView, ctx, rect, axis, Palette::Red);
}

template<size_t N>
bool FunctionAxis<N>::labelWillBeDisplayed(KDRect labelRect) const {
  KDRect rect = labelRect.paddedWith(k_labelAvoidanceMargin);
  return !rect.intersects(m_specialLabelRect);
}

void FunctionGraphPolicy::drawPlot(const Shared::AbstractPlotView * plotView, KDContext * ctx, KDRect rect) const {
  // Since exp(-2.5) is generalized as exp(-x), x cannot be negative
  float x = m_model->abscissa();
  assert(x >= 0);
  float y = m_model->ordinate();

  Expression function = m_model->function();
  Curve2DEvaluation<float> evaluateFunction =
    [](float t, void * model, void * context) {
      Expression * e = (Expression *)model;
      Context * c = (Context *)context;
      constexpr static char k_unknownName[2] = {UCodePointUnknown, 0};
      return Coordinate2D<float>(t, PoincareHelpers::ApproximateWithValueForSymbol<float>(*e, k_unknownName, t, c));
    };

  plotView->drawDot(ctx, rect, Dots::Size::Large, {x, y}, Palette::Red);

  // Draw the curve
  Context * context = App::app()->localContext();
  CurveDrawing plot(Curve2D(evaluateFunction, &function), context, m_model->xMin(), m_model->xMax(), plotView->pixelWidth(), Palette::Red);
  plot.draw(plotView, ctx, rect);
}

template bool FunctionAxis<PlotPolicy::AbstractLabeledAxis::k_maxNumberOfYLabels>::labelWillBeDisplayed(KDRect labelRect) const;
template bool FunctionAxis<PlotPolicy::AbstractLabeledAxis::k_maxNumberOfXLabels>::labelWillBeDisplayed(KDRect labelRect) const;
template void FunctionAxis<PlotPolicy::AbstractLabeledAxis::k_maxNumberOfXLabels>::reloadAxis(AbstractPlotView * plotView, AbstractPlotView::Axis axis);
template void FunctionAxis<PlotPolicy::AbstractLabeledAxis::k_maxNumberOfYLabels>::reloadAxis(AbstractPlotView * plotView, AbstractPlotView::Axis axis);
template void FunctionAxis<PlotPolicy::AbstractLabeledAxis::k_maxNumberOfXLabels>::drawAxis(const AbstractPlotView * plotView, KDContext * ctx, KDRect rect, AbstractPlotView::Axis axis) const;
template void FunctionAxis<PlotPolicy::AbstractLabeledAxis::k_maxNumberOfYLabels>::drawAxis(const AbstractPlotView * plotView, KDContext * ctx, KDRect rect, AbstractPlotView::Axis axis) const;

}

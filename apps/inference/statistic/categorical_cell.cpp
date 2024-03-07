#include "categorical_cell.h"

#include <escher/layout_view.h>

using namespace Escher;

namespace Inference {

// AbstractCategoricalCell

KDSize AbstractCategoricalCell::minimalSizeForOptimalDisplay() const {
  KDSize innerSize = innerCell()->minimalSizeForOptimalDisplay();
  return KDSize(innerSize.width() + Metric::CommonMargins.width(),
                innerSize.height());
}

void AbstractCategoricalCell::drawRect(KDContext* ctx, KDRect rect) const {
  KDCoordinate width = bounds().width();
  KDCoordinate height = bounds().height();

  // Draw horizontal margins
  ctx->fillRect(KDRect(0, 0, Metric::CommonMargins.left(), height),
                Palette::WallScreenDark);
  ctx->fillRect(KDRect(width - Metric::CommonMargins.right(), 0,
                       Metric::CommonMargins.right(), height),
                Escher::Palette::WallScreenDark);
}

void AbstractCategoricalCell::setHighlighted(bool highlight) {
  HighlightCell::setHighlighted(highlight);
  innerCell()->setHighlighted(highlight);
}

void AbstractCategoricalCell::layoutSubviews(bool force) {
  KDCoordinate width = bounds().width();
  KDCoordinate height = bounds().height();
  setChildFrame(innerCell(),
                KDRect(Metric::CommonMargins.left(), 0,
                       width - Metric::CommonMargins.width(), height),
                force);
}

// InputCategoricalCell

template <>
void InputCategoricalCell<MessageTextView>::setMessages(
    I18n::Message labelMessage, I18n::Message subLabelMessage) {
  m_innerCell.label()->setMessage(labelMessage);
  m_innerCell.subLabel()->setMessage(subLabelMessage);
}

template <>
void InputCategoricalCell<LayoutView>::setMessages(
    Poincare::Layout labelLayout, I18n::Message subLabelMessage) {
  m_innerCell.label()->setLayout(labelLayout);
  m_innerCell.subLabel()->setMessage(subLabelMessage);
}

// DropdownCategoricalCell

KDSize DropdownCategoricalCell::minimalSizeForOptimalDisplay() const {
  KDSize res = AbstractCategoricalCell::minimalSizeForOptimalDisplay();
  return KDSize(res.width(), res.height() + k_topMargin);
}

void DropdownCategoricalCell::drawRect(KDContext* ctx, KDRect rect) const {
  // Fill top margin
  KDCoordinate width = bounds().width();
  ctx->fillRect(KDRect(0, 0, width, k_topMargin), Palette::WallScreenDark);

  AbstractCategoricalCell::drawRect(ctx, rect);
}

void DropdownCategoricalCell::layoutSubviews(bool force) {
  KDCoordinate width = bounds().width();
  KDCoordinate height = bounds().height();
  setChildFrame(
      &m_innerCell,
      KDRect(Metric::CommonMargins.left(), k_topMargin,
             width - Metric::CommonMargins.width(), height - k_topMargin),
      force);
}

}  // namespace Inference

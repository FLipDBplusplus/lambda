#include <escher/expression_view.h>
using namespace Poincare;

ExpressionView::ExpressionView(float horizontalAlignment, float verticalAlignment,
    KDColor textColor, KDColor backgroundColor) :
  m_layoutRef(),
  m_horizontalAlignment(horizontalAlignment),
  m_verticalAlignment(verticalAlignment),
  m_textColor(textColor),
  m_backgroundColor(backgroundColor),
  m_horizontalMargin(0)
{
}

void ExpressionView::setLayoutRef(LayoutRef layoutR) {
  m_layoutRef = layoutR;
  markRectAsDirty(bounds());
}

void ExpressionView::setBackgroundColor(KDColor backgroundColor) {
  if (m_backgroundColor != backgroundColor) {
    m_backgroundColor = backgroundColor;
    markRectAsDirty(bounds());
  }
}

void ExpressionView::setTextColor(KDColor textColor) {
  if (textColor != m_textColor) {
    m_textColor = textColor;
    markRectAsDirty(bounds());
  }
}

void ExpressionView::setAlignment(float horizontalAlignment, float verticalAlignment) {
  m_horizontalAlignment = horizontalAlignment;
  m_verticalAlignment = verticalAlignment;
  markRectAsDirty(bounds());
}

int ExpressionView::numberOfLayouts() const {
  return m_layoutRef.numberOfDescendants(true);
}

KDSize ExpressionView::minimalSizeForOptimalDisplay() const {
  if (!m_layoutRef.isDefined()) {
    return KDSizeZero;
  }
  KDSize expressionSize = m_layoutRef.layoutSize();
  return KDSize(expressionSize.width() + 2*m_horizontalMargin, expressionSize.height());
}

KDPoint ExpressionView::drawingOrigin() const {
  KDSize expressionSize = m_layoutRef.layoutSize();
  return KDPoint(m_horizontalMargin + m_horizontalAlignment*(m_frame.width() - 2*m_horizontalMargin - expressionSize.width()), max(0, (m_frame.height() - expressionSize.height())/2));
}

KDPoint ExpressionView::absoluteDrawingOrigin() const {
  return drawingOrigin().translatedBy(m_frame.topLeft());
}

void ExpressionView::drawRect(KDContext * ctx, KDRect rect) const {
  ctx->fillRect(rect, m_backgroundColor);
  if (m_layoutRef.isDefined()) {
    m_layoutRef.draw(ctx, drawingOrigin(), m_textColor, m_backgroundColor);
  }
}

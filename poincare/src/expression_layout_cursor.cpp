#include <poincare/expression_layout_cursor.h>

namespace Poincare {

bool ExpressionLayoutCursor::moveLeft() {
  return m_pointedExpressionLayout->moveLeft(this);
}

bool ExpressionLayoutCursor::moveRight() {
  return false; //TODO
}

bool ExpressionLayoutCursor::moveUp() {
  return false; //TODO
}

bool ExpressionLayoutCursor::moveDown() {
  return false; //TODO
}

}


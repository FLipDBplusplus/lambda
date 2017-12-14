#include "horizontal_layout.h"
#include "string_layout.h"
#include <poincare/expression_layout_cursor.h>

extern "C" {
#include <assert.h>
#include <kandinsky.h>
#include <stdlib.h>
}

namespace Poincare {

HorizontalLayout::HorizontalLayout(ExpressionLayout ** children_layouts, int number_of_children) :
  ExpressionLayout(), m_number_of_children(number_of_children) {
  assert(number_of_children > 0);
  m_children_layouts = new ExpressionLayout *[number_of_children];
  for (int i=0; i<m_number_of_children; i++) {
    m_children_layouts[i] = children_layouts[i];
    m_children_layouts[i]->setParent(this);
    if (m_children_layouts[i]->baseline() > m_baseline) {
      m_baseline = m_children_layouts[i]->baseline();
    }
  }
}

HorizontalLayout::~HorizontalLayout() {
  for (int i=0; i<m_number_of_children; i++) {
    delete m_children_layouts[i];
  }
  delete[] m_children_layouts;
}

bool HorizontalLayout::moveLeft(ExpressionLayoutCursor * cursor) {
  // Case: Left.
  // Ask the parent.
  if (cursor->pointedExpressionLayout() == this) {
    if (cursor->position() == ExpressionLayoutCursor::Position::Left) {
      if (m_parent) {
        return m_parent->moveLeft(cursor);
      }
      return false;
    }
    assert(cursor->position() == ExpressionLayoutCursor::Position::Right);
    // Case: Right.
    // Go to the last child if there is one, and move Left.
    // Else go Left and ask the parent.
    if (m_number_of_children < 1) {
      cursor->setPosition(ExpressionLayoutCursor::Position::Left);
      if (m_parent) {
        return m_parent->moveLeft(cursor);
      }
      return false;
    }
    ExpressionLayout * lastChild = m_children_layouts[m_number_of_children-1];
    assert(lastChild != nullptr);
    cursor->setPointedExpressionLayout(lastChild);
    return lastChild->moveLeft(cursor);
  }

  // Case: The cursor is Left of a child.
  assert(cursor->position() == ExpressionLayoutCursor::Position::Left);
  int childIndex = indexOfChild(cursor->pointedExpressionLayout());
  assert(childIndex >= 0);
  if (childIndex == 0) {
    // Case: the child is the leftmost.
    // Ask the parent.
    cursor->setPointedExpressionLayout(this);
    if (m_parent) {
      return m_parent->moveLeft(cursor);
    }
    return false;
  }
  // Case: the child is not the leftmost.
  // Go to its left brother and move Left.
  cursor->setPointedExpressionLayout(m_children_layouts[childIndex-1]);
  cursor->setPosition(ExpressionLayoutCursor::Position::Right);
  return m_children_layouts[childIndex-1]->moveLeft(cursor);
}

void HorizontalLayout::render(KDContext * ctx, KDPoint p, KDColor expressionColor, KDColor backgroundColor) {
}

KDSize HorizontalLayout::computeSize() {
  KDCoordinate totalWidth = 0;
  int i = 0;
  KDCoordinate max_under_baseline = 0;
  KDCoordinate max_above_baseline = 0;
  while (ExpressionLayout * c = child(i++)) {
    KDSize childSize = c->size();
    totalWidth += childSize.width();
    if (childSize.height() - c->baseline() > max_under_baseline) {
      max_under_baseline = childSize.height() - c->baseline() ;
    }
    if (c->baseline() > max_above_baseline) {
      max_above_baseline = c->baseline();
    }
  }
  return KDSize(totalWidth, max_under_baseline + max_above_baseline);
}

ExpressionLayout * HorizontalLayout::child(uint16_t index) {
  assert(index <= (unsigned int) m_number_of_children);
  if (index < (unsigned int) m_number_of_children) {
    return m_children_layouts[index];
  } else {
    return nullptr;
  }
}

KDPoint HorizontalLayout::positionOfChild(ExpressionLayout * child) {
  KDCoordinate x = 0;
  KDCoordinate y = 0;
  int index = indexOfChild(child);
  if (index > 0) {
    ExpressionLayout * previousChild = m_children_layouts[index-1];
    assert(previousChild != nullptr);
    x = previousChild->origin().x() + previousChild->size().width();
  }
  y = m_baseline - child->baseline();
  return KDPoint(x, y);
}

int HorizontalLayout::indexOfChild(ExpressionLayout * eL) const {
  for (int i = 0; i < m_number_of_children; i++) {
    if (m_children_layouts[i] == eL) {
      return i;
    }
  }
  return -1;
}

}

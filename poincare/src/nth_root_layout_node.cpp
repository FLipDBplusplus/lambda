#include <poincare/nth_root_layout_node.h>
#include <poincare/allocation_failure_layout_node.h>
#include <poincare/layout_helper.h>
#include <poincare/serialization_helper.h>
#include <ion/charset.h>
#include <assert.h>

namespace Poincare {

static inline KDCoordinate max(KDCoordinate x, KDCoordinate y) { return x > y ? x : y; }

const uint8_t radixPixel[NthRootLayoutNode::k_leftRadixHeight][NthRootLayoutNode::k_leftRadixWidth] = {
  {0x00, 0xFF, 0xFF, 0xFF, 0xFF},
  {0xFF, 0x00, 0xFF, 0xFF, 0xFF},
  {0xFF, 0x00, 0xFF, 0xFF, 0xFF},
  {0xFF, 0xFF, 0x00, 0xFF, 0xFF},
  {0xFF, 0xFF, 0x00, 0xFF, 0xFF},
  {0xFF, 0xFF, 0xFF, 0x00, 0xFF},
  {0xFF, 0xFF, 0xFF, 0x00, 0xFF},
  {0xFF, 0xFF, 0xFF, 0xFF, 0x00},
};

NthRootLayoutNode * NthRootLayoutNode::FailedAllocationStaticNode() {
  static AllocationFailureLayoutNode<NthRootLayoutNode> failure;
  TreePool::sharedPool()->registerStaticNodeIfRequired(&failure);
  return &failure;
}

void NthRootLayoutNode::moveCursorLeft(LayoutCursor * cursor, bool * shouldRecomputeLayout) {
  if (radicandLayout()
    && cursor->layoutNode() == radicandLayout()
    && cursor->position() == LayoutCursor::Position::Left)
  {
    // Case: Left of the radicand. Go the index if any, ir go Left of the root.
    if (indexLayout()) {
      cursor->setLayoutNode(indexLayout());
      cursor->setPosition(LayoutCursor::Position::Right);
    } else {
      cursor->setLayoutNode(this);
    }
    return;
  }
  if (indexLayout()
    && cursor->layoutNode() == indexLayout()
    && cursor->position() == LayoutCursor::Position::Left)
  {
    // Case: Left of the index. Go Left of the root.
    cursor->setLayoutNode(this);
    return;
  }
  assert(cursor->layoutNode() == this);
  if (cursor->position() == LayoutCursor::Position::Right) {
    // Case: Right. Go Right of the radicand.
    assert(radicandLayout() != nullptr);
    cursor->setLayoutNode(radicandLayout());
    return;
  }
  assert(cursor->position() == LayoutCursor::Position::Left);
  // Case: Left. Ask the parent.
  LayoutNode * parentNode = parent();
  if (parentNode != nullptr) {
    parentNode->moveCursorLeft(cursor, shouldRecomputeLayout);
  }
}

void NthRootLayoutNode::moveCursorRight(LayoutCursor * cursor, bool * shouldRecomputeLayout) {
  if (radicandLayout()
    && cursor->layoutNode() == radicandLayout()
    && cursor->position() == LayoutCursor::Position::Right)
  {
    // Case: Right of the radicand. Go the Right of the root.
    cursor->setLayoutNode(this);
    return;
  }
  if (indexLayout()
    && cursor->layoutNode() == indexLayout()
    && cursor->position() == LayoutCursor::Position::Right)
  {
    // Case: Right of the index. Go Left of the integrand.
    assert(radicandLayout() != nullptr);
    cursor->setLayoutNode(radicandLayout());
    cursor->setPosition(LayoutCursor::Position::Left);
    return;
  }
  assert(cursor->layoutNode() == this);
  if (cursor->position() == LayoutCursor::Position::Left) {
    // Case: Left. Go to the index if there is one, else go to the radicand.
    if (indexLayout()) {
      cursor->setLayoutNode(indexLayout());
    } else {
      assert(radicandLayout() != nullptr);
      cursor->setLayoutNode(radicandLayout());
    }
    return;
  }
  assert(cursor->position() == LayoutCursor::Position::Right);
  // Case: Right. Ask the parent.
  LayoutNode * parentNode = parent();
  if (parentNode) {
    parentNode->moveCursorRight(cursor, shouldRecomputeLayout);
  }
}

void NthRootLayoutNode::moveCursorUp(LayoutCursor * cursor, bool * shouldRecomputeLayout, bool equivalentPositionVisited) {
  if (indexLayout()
      && radicandLayout()
      && cursor->isEquivalentTo(LayoutCursor(radicandLayout(), LayoutCursor::Position::Left)))
  {
    // If the cursor is Left of the radicand, move it to the index.
    cursor->setLayoutNode(indexLayout());
    cursor->setPosition(LayoutCursor::Position::Right);
    return;
  }
  if (indexLayout()
      && cursor->layoutNode() == this
      && cursor->position() == LayoutCursor::Position::Left)
  {
    // If the cursor is Left, move it to the index.
    cursor->setLayoutNode(indexLayout());
    cursor->setPosition(LayoutCursor::Position::Left);
    return;
  }
  LayoutNode::moveCursorUp(cursor, shouldRecomputeLayout, equivalentPositionVisited);
}

void NthRootLayoutNode::moveCursorDown(LayoutCursor * cursor, bool * shouldRecomputeLayout, bool equivalentPositionVisited) {
  if (indexLayout() && cursor->layoutNode()->hasAncestor(indexLayout(), true)) {
    if (cursor->isEquivalentTo(LayoutCursor(indexLayout(), LayoutCursor::Position::Right))) {
      // If the cursor is Right of the index, move it to the radicand.
      assert(radicandLayout() != nullptr);
      cursor->setLayoutNode(radicandLayout());
      cursor->setPosition(LayoutCursor::Position::Left);
      return;
    }
    // If the cursor is Left of the index, move it Left .
    if (cursor->isEquivalentTo(LayoutCursor(indexLayout(), LayoutCursor::Position::Left))) {
      cursor->setLayoutNode(this);
      cursor->setPosition(LayoutCursor::Position::Left);
      return;
    }
  }
  LayoutNode::moveCursorDown(cursor, shouldRecomputeLayout, equivalentPositionVisited);
}

void NthRootLayoutNode::deleteBeforeCursor(LayoutCursor * cursor) {
  if (cursor->layoutNode() == radicandLayout()
      && cursor->position() == LayoutCursor::Position::Left)
  {
    // Case: Left of the radicand. Delete the layout, keep the radicand.
    LayoutRef radicand = LayoutRef(radicandLayout());
    LayoutRef thisRef = LayoutRef(this);
    LayoutRef rootRef = LayoutRef(root());
    thisRef.replaceChildWithGhostInPlace(radicand);
    // WARNING: Do not call "this" afterwards
    if (rootRef.isAllocationFailure()) {
      cursor->setLayoutReference(rootRef);
      return;
    }
    cursor->setLayoutReference(thisRef.childAtIndex(0));
    thisRef.replaceWith(radicand, cursor);
    return;
  }
  LayoutNode::deleteBeforeCursor(cursor);
}

static_assert('\x91' == Ion::Charset::Root, "Unicode error");
int NthRootLayoutNode::serialize(char * buffer, int bufferSize, Preferences::PrintFloatMode floatDisplayMode, int numberOfSignificantDigits) const {
  // Case: root(x,n)
  if (m_hasIndex
      && (const_cast<NthRootLayoutNode *>(this))->indexLayout()
      && !(const_cast<NthRootLayoutNode *>(this))->indexLayout()->isEmpty())
  {
    return SerializationHelper::Prefix(this, buffer, bufferSize, floatDisplayMode, numberOfSignificantDigits, "root");
  }
  // Case: squareRoot(x)
  if (!m_hasIndex) {
    return SerializationHelper::Prefix(this, buffer, bufferSize, floatDisplayMode, numberOfSignificantDigits, "\x91");
  }
  // Case: root(x,empty)
  // Write "'SquareRootSymbol'('radicandLayout')".
  assert((const_cast<NthRootLayoutNode *>(this))->indexLayout() && (const_cast<NthRootLayoutNode *>(this))->indexLayout()->isEmpty());
  if (bufferSize == 0) {
    return -1;
  }
  buffer[bufferSize-1] = 0;
  int numberOfChar = 0;

  buffer[numberOfChar++] = '\x91';
  if (numberOfChar >= bufferSize-1) {
    return bufferSize-1;
  }

  buffer[numberOfChar++] = '(';
  if (numberOfChar >= bufferSize-1) {
    return bufferSize-1;
  }

  numberOfChar += (const_cast<NthRootLayoutNode *>(this))->radicandLayout()->serialize(buffer+numberOfChar, bufferSize-numberOfChar, floatDisplayMode, numberOfSignificantDigits);
  if (numberOfChar >= bufferSize-1) { return bufferSize-1; }

  buffer[numberOfChar++] = ')';
  buffer[numberOfChar] = 0;
  return numberOfChar;
}

KDSize NthRootLayoutNode::computeSize() {
  KDSize radicandSize = radicandLayout()->layoutSize();
  KDSize indexSize = adjustedIndexSize();
  KDSize newSize = KDSize(
      indexSize.width() + 3*k_widthMargin + 2*k_radixLineThickness + radicandSize.width() + k_radixHorizontalOverflow,
      baseline() + radicandSize.height() - radicandLayout()->baseline() + k_heightMargin
      );
  return newSize;
}

KDCoordinate NthRootLayoutNode::computeBaseline() {
  if (indexLayout() != nullptr) {
    return max(
        radicandLayout()->baseline() + k_radixLineThickness + k_heightMargin,
        indexLayout()->layoutSize().height() + k_indexHeight);
  } else {
    return radicandLayout()->baseline() + k_radixLineThickness + k_heightMargin;
  }
}

KDPoint NthRootLayoutNode::positionOfChild(LayoutNode * child) {
  KDCoordinate x = 0;
  KDCoordinate y = 0;
  KDSize indexSize = adjustedIndexSize();
  if (child == radicandLayout()) {
    x = indexSize.width() + 2*k_widthMargin + k_radixLineThickness;
    y = baseline() - radicandLayout()->baseline();
  } else if (indexLayout() && child == indexLayout()) {
    x = 0;
    y = baseline() - indexSize.height() -  k_indexHeight;
  } else {
    assert(false);
  }
  return KDPoint(x,y);
}

KDSize NthRootLayoutNode::adjustedIndexSize() {
  return indexLayout() != nullptr ?
    KDSize(max(k_leftRadixWidth, indexLayout()->layoutSize().width()), indexLayout()->layoutSize().height()) :
    KDSize(k_leftRadixWidth, 0);
}

void NthRootLayoutNode::render(KDContext * ctx, KDPoint p, KDColor expressionColor, KDColor backgroundColor) {
  KDSize radicandSize = radicandLayout()->layoutSize();
  KDSize indexSize = adjustedIndexSize();
  KDColor workingBuffer[k_leftRadixWidth*k_leftRadixHeight];
  KDRect leftRadixFrame(p.x() + indexSize.width() + k_widthMargin - k_leftRadixWidth,
    p.y() + baseline() + radicandSize.height() - radicandLayout()->baseline() + k_heightMargin - k_leftRadixHeight,
    k_leftRadixWidth, k_leftRadixHeight);
  ctx->blendRectWithMask(leftRadixFrame, expressionColor, (const uint8_t *)radixPixel, (KDColor *)workingBuffer);
  // If the indice is higher than the root.
  if (indexSize.height() + k_indexHeight > radicandLayout()->baseline() + k_radixLineThickness + k_heightMargin) {
    // Vertical radix bar
    ctx->fillRect(KDRect(p.x() + indexSize.width() + k_widthMargin,
                         p.y() + indexSize.height() + k_indexHeight - radicandLayout()->baseline() - k_radixLineThickness - k_heightMargin,
                         k_radixLineThickness,
                         radicandSize.height() + 2*k_heightMargin + k_radixLineThickness), expressionColor);
    // Horizontal radix bar
    ctx->fillRect(KDRect(p.x() + indexSize.width() + k_widthMargin,
                         p.y() + indexSize.height() + k_indexHeight - radicandLayout()->baseline() - k_radixLineThickness - k_heightMargin,
                         radicandSize.width() + 2*k_widthMargin + k_radixHorizontalOverflow,
                         k_radixLineThickness), expressionColor);
    // Right radix bar
    ctx->fillRect(KDRect(p.x() + indexSize.width() + k_widthMargin + radicandSize.width() + 2*k_widthMargin + k_radixHorizontalOverflow,
                         p.y() + indexSize.height() + k_indexHeight - radicandLayout()->baseline() - k_radixLineThickness - k_heightMargin,
                         k_radixLineThickness,
                         k_rightRadixHeight + k_radixLineThickness), expressionColor);
  } else {
    ctx->fillRect(KDRect(p.x() + indexSize.width() + k_widthMargin,
                         p.y(),
                         k_radixLineThickness,
                         radicandSize.height() + 2*k_heightMargin + k_radixLineThickness), expressionColor);
    ctx->fillRect(KDRect(p.x() + indexSize.width() + k_widthMargin,
                         p.y(),
                         radicandSize.width() + 2*k_widthMargin + k_radixHorizontalOverflow,
                         k_radixLineThickness), expressionColor);
    ctx->fillRect(KDRect(p.x() + indexSize.width() + k_widthMargin + radicandSize.width() + 2*k_widthMargin + k_radixHorizontalOverflow,
                         p.y(),
                         k_radixLineThickness,
                         k_rightRadixHeight + k_radixLineThickness), expressionColor);
  }
}

}

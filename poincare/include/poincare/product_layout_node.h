#ifndef POINCARE_PRODUCT_LAYOUT_NODE_H
#define POINCARE_PRODUCT_LAYOUT_NODE_H

#include <poincare/layout_helper.h>
#include <poincare/sequence_layout_node.h>

namespace Poincare {

class ProductLayoutNode : public SequenceLayoutNode {
public:
  using SequenceLayoutNode::SequenceLayoutNode;
  int serialize(char * buffer, int bufferSize, Preferences::PrintFloatMode floatDisplayMode, int numberOfSignificantDigits) const override;
  static ProductLayoutNode * FailedAllocationStaticNode();
  ProductLayoutNode * failedAllocationStaticNode() override { return FailedAllocationStaticNode(); }
  size_t size() const override { return sizeof(ProductLayoutNode); }
#if TREE_LOG
  const char * description() const override { return "ProductLayout"; }
#endif

protected:
  void render(KDContext * ctx, KDPoint p, KDColor expressionColor, KDColor backgroundColor) override;
private:
  constexpr static KDCoordinate k_lineThickness = 1;
};

class ProductLayoutRef : public LayoutReference {
public:
  ProductLayoutRef(LayoutRef argument, LayoutRef lowerB, LayoutRef upperB) :
    LayoutReference(TreePool::sharedPool()->createTreeNode<ProductLayoutNode>())
  {
    replaceChildAtIndexInPlace(0, argument);
    replaceChildAtIndexInPlace(1, lowerB);
    replaceChildAtIndexInPlace(2, upperB);
  }
};

}

#endif

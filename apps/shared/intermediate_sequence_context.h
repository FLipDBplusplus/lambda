#ifndef SHARED_INTERMEDIATE_SEQUENCE_CONTEXT_H
#define SHARED_INTERMEDIATE_SEQUENCE_CONTEXT_H

#include <poincare/context.h>
#include <poincare/expression.h>
#include <poincare/symbol.h>

#include "sequence_context.h"

namespace Shared {

template <typename T>
class IntermediateSequenceContext : public Poincare::ContextWithParent {
 public:
  IntermediateSequenceContext(SequenceContext* sequenceContext);

 private:
  const Poincare::Expression protectedExpressionForSymbolAbstract(
      const Poincare::SymbolAbstract& symbol, bool clone,
      ContextWithParent* lastDescendantContext) override;
  SequenceContext* m_sequenceContext;
};

}  // namespace Shared

#endif

#ifndef SEQUENCE_SEQUENCE_H
#define SEQUENCE_SEQUENCE_H

#include "../shared/function.h"
#include "sequence_context.h"
#include <assert.h>

namespace Sequence {

class Sequence : public Shared::Function {
public:
  enum class Type {
    Explicit = 0,
    SingleRecurrence = 1,
    DoubleRecurrence = 2
  };
  Sequence(const char * text = nullptr, KDColor color = KDColorBlack);
  uint32_t checksum() override;
  Type type();
  int initialRank() const {
    return m_initialRank;
  }
  const char * firstInitialConditionText();
  const char * secondInitialConditionText();
  Poincare::Expression firstInitialConditionExpression(Poincare::Context * context) const;
  Poincare::Expression secondInitialConditionExpression(Poincare::Context * context) const;
  Poincare::Layout firstInitialConditionLayout();
  Poincare::Layout secondInitialConditionLayout();
  /* WARNING: after calling setType, setContent, setFirstInitialConditionContent
   * or setSecondInitialConditionContent, the sequence context needs to
   * invalidate the cache because the sequences evaluations might have changed. */
  void setType(Type type);
  void setInitialRank(int rank);
  void setFirstInitialConditionContent(const char * c);
  void setSecondInitialConditionContent(const char * c);
  int numberOfElements();
  Poincare::Layout nameLayout();
  Poincare::Layout definitionName();
  Poincare::Layout definitionNameWithEqual();
  Poincare::Layout firstInitialConditionName();
  Poincare::Layout secondInitialConditionName();
  bool isDefined() override;
  bool isEmpty() override;
  float evaluateAtAbscissa(float x, Poincare::Context * context) const override {
    return templatedApproximateAtAbscissa(x, static_cast<SequenceContext *>(context));
  }
  double evaluateAtAbscissa(double x, Poincare::Context * context) const override {
    return templatedApproximateAtAbscissa(x, static_cast<SequenceContext *>(context));
  }
  template<typename T> T approximateToNextRank(int n, SequenceContext * sqctx) const;
  double sumBetweenBounds(double start, double end, Poincare::Context * context) const override;
  void tidy() override;
  constexpr static int k_initialRankNumberOfDigits = 3; // m_initialRank is capped by 999
private:
  constexpr static const KDFont * k_layoutFont = KDFont::LargeFont;
  constexpr static double k_maxNumberOfTermsInSum = 100000.0;
  constexpr static size_t k_dataLengthInBytes = (3*TextField::maxBufferSize()+3)*sizeof(char)+sizeof(int)+1;
  static_assert((k_dataLengthInBytes & 0x3) == 0, "The sequence data size is not a multiple of 4 bytes (cannot compute crc)"); // Assert that dataLengthInBytes is a multiple of 4
  const char * symbol() const override { return "n"; }
  template<typename T> T templatedApproximateAtAbscissa(T x, SequenceContext * sqctx) const;
  Type m_type;
  char m_firstInitialConditionText[TextField::maxBufferSize()];
  char m_secondInitialConditionText[TextField::maxBufferSize()];
  mutable Poincare::Expression m_firstInitialConditionExpression;
  mutable Poincare::Expression m_secondInitialConditionExpression;
  Poincare::Layout m_firstInitialConditionLayout;
  Poincare::Layout m_secondInitialConditionLayout;
  Poincare::Layout m_nameLayout;
  Poincare::Layout m_definitionNameWithEqual;
  Poincare::Layout m_firstInitialConditionName;
  Poincare::Layout m_secondInitialConditionName;
  int m_initialRank;
};

}

#endif

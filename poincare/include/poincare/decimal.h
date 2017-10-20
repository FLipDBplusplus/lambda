#ifndef POINCARE_DECIMAL_H
#define POINCARE_DECIMAL_H

#include <poincare/static_hierarchy.h>
#include <poincare/integer.h>

namespace Poincare {

/* A decimal as 0.01234 is stored that way:
 *  - m_mantissa = 1234
 *  - m_exponent = -2
 */

class Decimal : public StaticHierarchy<0> {
public:
  static int exponent(const char * integralPart, int integralPartLength, const char * fractionalPart, int fractionalPartLength, const char * exponent, int exponentLength, bool exponentNegative);
  static Integer mantissa(const char * integralPart, int integralPartLength, const char * fractionalPart, int fractionalPartLength, bool negative);
  Decimal(Integer mantissa, int exponent);
  // Expression subclassing
  Type type() const override;
  Expression * clone() const override;
  int writeTextInBuffer(char * buffer, int bufferSize) const override;
  Expression * immediateSimplify() override;
private:
  Evaluation<float> * privateEvaluate(SinglePrecision p, Context& context, AngleUnit angleUnit) const override { return templatedEvaluate<float>(context, angleUnit); }
  Evaluation<double> * privateEvaluate(DoublePrecision p, Context& context, AngleUnit angleUnit) const override { return templatedEvaluate<double>(context, angleUnit); }
 template<typename T> Evaluation<T> * templatedEvaluate(Context& context, Expression::AngleUnit angleUnit) const;
  ExpressionLayout * privateCreateLayout(FloatDisplayMode floatDisplayMode, ComplexFormat complexFormat) const override;

  int numberOfDigitsInMantissa() const;
  /* Sorting */
  int compareToSameTypeExpression(const Expression * e) const override;

  constexpr static int k_maxLength = 10;
  Integer m_mantissa;
  int m_exponent;
};

}

#endif

#include <poincare/expression_reference.h>
#include <poincare/expression_node.h>
//#include <poincare/list_data.h>
//#include <poincare/matrix_data.h>
#include <poincare/rational.h>
#include <poincare/opposite.h>
#include <poincare/undefined.h>
#include <poincare/symbol.h>
//#include <poincare/variable_context.h>
#include <ion.h>
#include <cmath>
#include <float.h>
//#include "expression_parser.hpp"
//#include "expression_lexer.hpp"

int poincare_expression_yyparse(Poincare::ExpressionReference * expressionOutput);

namespace Poincare {

#include <stdio.h>

/* Constructor & Destructor */

ExpressionReference ExpressionReference::parse(char const * string) {
  if (string[0] == 0) {
    return nullptr;
  }
  YY_BUFFER_STATE buf = poincare_expression_yy_scan_string(string);
  ExpressionReference expression(nullptr);
  if (poincare_expression_yyparse(&expression) != 0) {
    // Parsing failed because of invalid input or memory exhaustion
    expression = ExpressionReference(nullptr);
  }
  poincare_expression_yy_delete_buffer(buf);

  return expression;
}

void ExpressionReference::ReplaceSymbolWithExpression(ExpressionReference * expressionAddress, char symbol, ExpressionReference expression) {
  *expressionAddress = expressionAddress->node()->replaceSymbolWithExpression(symbol, expression);
}

ExpressionReference ExpressionReference::clone() const {
  TreeReference c = this->treeClone();
  ExpressionReference * cast = static_cast<ExpressionReference *>(&c);
  return *cast;
}

/* Circuit breaker */

static ExpressionReference::CircuitBreaker sCircuitBreaker = nullptr;
static bool sSimplificationHasBeenInterrupted = false;

void ExpressionReference::setCircuitBreaker(CircuitBreaker cb) {
  sCircuitBreaker = cb;
}

bool ExpressionReference::shouldStopProcessing() {
  if (sCircuitBreaker == nullptr) {
    return false;
  }
  if (sCircuitBreaker()) {
    sSimplificationHasBeenInterrupted = true;
    return true;
  }
  return false;
}

/* Properties */

bool ExpressionReference::isRationalZero() const {
  return this->node()->type() == ExpressionNode::Type::Rational && static_cast<const RationalNode *>(this->node())->isZero();
}

bool ExpressionReference::recursivelyMatches(ExpressionTest test, Context & context) const {
  if (test(*this, context)) {
    return true;
  }
  for (int i = 0; i < this->numberOfChildren(); i++) {
    if (childAtIndex(i).recursivelyMatches(test, context)) {
      return true;
    }
  }
  return false;
}

bool ExpressionReference::isApproximate(Context & context) const {
  return recursivelyMatches([](const ExpressionReference e, Context & context) {
        return e.node()->type() == ExpressionNode::Type::Decimal || e.node()->type() == ExpressionNode::Type::Double || ExpressionReference::IsMatrix(e, context) || (e.node()->type() == ExpressionNode::Type::Symbol && SymbolReference::isApproximate(static_cast<SymbolNode *>(e.node())->name(), context));
    }, context);
}

bool ExpressionReference::IsMatrix(const ExpressionReference e, Context & context) {
  return e.node()->type() == ExpressionNode::Type::Matrix || e.node()->type() == ExpressionNode::Type::ConfidenceInterval || e.node()->type() == ExpressionNode::Type::MatrixDimension || e.node()->type() == ExpressionNode::Type::PredictionInterval || e.node()->type() == ExpressionNode::Type::MatrixInverse || e.node()->type() == ExpressionNode::Type::MatrixTranspose || (e.node()->type() == ExpressionNode::Type::Symbol && SymbolReference::isMatrixSymbol(static_cast<SymbolNode *>(e.node())->name()));
}

bool dependsOnVariables(const ExpressionReference e, Context & context) {
  return e.node()->type() == ExpressionNode::Type::Symbol && SymbolReference::isVariableSymbol(static_cast<SymbolNode *>(e.node())->name());
}

bool ExpressionReference::getLinearCoefficients(char * variables, ExpressionReference coefficients[], ExpressionReference constant[], Context & context, Preferences::AngleUnit angleUnit) const {
  char * x = variables;
  while (*x != 0) {
    int degree = polynomialDegree(*x);
    if (degree > 1 || degree < 0) {
      return false;
    }
    x++;
  }
  ExpressionReference equation = clone();
  x = variables;
  int index = 0;
  ExpressionReference polynomialCoefficients[k_maxNumberOfPolynomialCoefficients];
  while (*x != 0) {
    int degree = equation.getPolynomialCoefficients(*x, polynomialCoefficients, context, angleUnit);
    if (degree == 1) {
      coefficients[index] = polynomialCoefficients[1];
    } else {
      assert(degree == 0);
      coefficients[index] = RationalReference(0);
    }
    equation = polynomialCoefficients[0];
    x++;
    index++;
  }
  constant[0] = OppositeReference(equation).deepReduce(context, angleUnit);
  /* The expression can be linear on all coefficients taken one by one but
   * non-linear (ex: xy = 2). We delete the results and return false if one of
   * the coefficients contains a variable. */
  bool isMultivariablePolynomial = (constant[0]).recursivelyMatches(dependsOnVariables, context);
  for (int i = 0; i < index; i++) {
    if (isMultivariablePolynomial) {
      break;
    }
    isMultivariablePolynomial |= coefficients[i].recursivelyMatches(dependsOnVariables, context);
  }
  if (isMultivariablePolynomial) {
    for (int i = 0; i < index; i++) {
      coefficients[i] = ExpressionReference(nullptr);
    }
    constant[0] = ExpressionReference(nullptr);
    return false;
  }
  return true;
}

int ExpressionReference::getPolynomialCoefficients(char symbolName, ExpressionReference coefficients[], Context & context, Preferences::AngleUnit angleUnit) const {
  int degree = this->node()->getPolynomialCoefficients(symbolName, coefficients);
  for (int i = 0; i <= degree; i++) {
    coefficients[i] = coefficients[i].deepReduce(context, angleUnit);
  }
  return degree;
}

/* Comparison */

bool ExpressionReference::isEqualToItsApproximationLayout(ExpressionReference approximation, int bufferSize, Preferences::AngleUnit angleUnit, Preferences::PrintFloatMode floatDisplayMode, int numberOfSignificantDigits, Context & context) {
  char buffer[bufferSize];
  approximation.writeTextInBuffer(buffer, bufferSize, floatDisplayMode, numberOfSignificantDigits);
  /* Warning: we cannot use directly the the approximate expression but we have
   * to re-serialize it because the number of stored significative
   * numbers and the number of displayed significative numbers might not be
   * identical. (For example, 0.000025 might be displayed "0.00003" and stored
   * as Decimal(0.000025) and isEqualToItsApproximationLayout should return
   * false) */
  ExpressionReference approximateOutput = ExpressionReference::ParseAndSimplify(buffer, context, angleUnit);
  bool equal = isIdenticalTo(approximateOutput);
  return equal;
}

/* Simplification */

ExpressionReference ExpressionReference::ParseAndSimplify(const char * text, Context & context, Preferences::AngleUnit angleUnit) {
  ExpressionReference exp = parse(text);
  if (!exp.isDefined()) {
    return UndefinedReference();
  }
  Simplify(&exp, context, angleUnit);
  if (!exp.isDefined()) {
    return parse(text);
  }
  return exp;
}

void ExpressionReference::Simplify(ExpressionReference * expressionAddress, Context & context, Preferences::AngleUnit angleUnit) {
  sSimplificationHasBeenInterrupted = false;
#if MATRIX_EXACT_REDUCING
#else
  if (expressionAddress->recursivelyMatches(IsMatrix, context)) {
    return;
  }
#endif
  *expressionAddress = expressionAddress->deepReduce(context, angleUnit);
  *expressionAddress = expressionAddress->deepBeautify(context, angleUnit);
  if (sSimplificationHasBeenInterrupted) {
    *expressionAddress = ExpressionReference(nullptr);
  }
}

ExpressionReference ExpressionReference::deepReduce(Context & context, Preferences::AngleUnit angleUnit) {
  int nbChildren = this->numberOfChildren();
  for (int i = 0; i < nbChildren; i++) {
    ExpressionReference reducedChild = childAtIndex(i).deepReduce(context, angleUnit);
    if (numberOfChildren() < nbChildren) {
      addChildTreeAtIndex(reducedChild, i, nbChildren-1);
    } else {
      replaceTreeChildAtIndex(i, reducedChild);
    }
  }
  return shallowReduce(context, angleUnit);
}

ExpressionReference ExpressionReference::deepBeautify(Context & context, Preferences::AngleUnit angleUnit) {
  ExpressionReference beautifiedExpression = shallowBeautify(context, angleUnit);
  int nbChildren = beautifiedExpression.numberOfChildren();
  for (int i = 0; i < nbChildren; i++) {
    ExpressionReference beautifiedChild = beautifiedExpression.childAtIndex(i).deepBeautify(context, angleUnit);
    if (beautifiedExpression.numberOfChildren() < nbChildren) {
      beautifiedExpression.addChildTreeAtIndex(beautifiedChild, i, nbChildren-1);
    } else {
      beautifiedExpression.replaceTreeChildAtIndex(i, beautifiedChild);
    }
  }
  return beautifiedExpression;
}

/* Evaluation */

template<typename U>
ExpressionReference ExpressionReference::approximate(Context& context, Preferences::AngleUnit angleUnit, Preferences::Preferences::ComplexFormat complexFormat) const {
  EvaluationReference<U> e = this->node()->approximate(U(), context, angleUnit);
  return e->complexToExpression(complexFormat);
}

template<typename U>
U ExpressionReference::approximateToScalar(Context& context, Preferences::AngleUnit angleUnit) const {
  EvaluationReference<U> evaluation = this->node()->approximate(U(), context, angleUnit);
  return evaluation->toScalar();
}

template<typename U>
U ExpressionReference::approximateToScalar(const char * text, Context& context, Preferences::AngleUnit angleUnit) {
  ExpressionReference exp = ParseAndSimplify(text, context, angleUnit);
  return exp.approximateToScalar<U>(context, angleUnit);
}

template<typename U>
U ExpressionReference::approximateWithValueForSymbol(char symbol, U x, Context & context, Preferences::AngleUnit angleUnit) const {
  VariableContext<U> variableContext = VariableContext<U>(symbol, &context);
  variableContext.setApproximationForVariable(x);
  return approximateToScalar<U>(variableContext, angleUnit);
}

template<typename U>
U ExpressionReference::epsilon() {
  static U epsilon = sizeof(U) == sizeof(double) ? 1E-15 : 1E-7f;
  return epsilon;
}

/* Expression roots/extrema solver*/

typename ExpressionReference::Coordinate2D ExpressionReference::nextMinimum(char symbol, double start, double step, double max, Context & context, Preferences::AngleUnit angleUnit) const {
  return nextMinimumOfExpression(symbol, start, step, max, [](char symbol, double x, Context & context, Preferences::AngleUnit angleUnit, const ExpressionReference expression0, const ExpressionReference expression1 = nullptr) {
        return expression0.approximateWithValueForSymbol(symbol, x, context, angleUnit);
      }, context, angleUnit);
}

typename ExpressionReference::Coordinate2D ExpressionReference::nextMaximum(char symbol, double start, double step, double max, Context & context, Preferences::AngleUnit angleUnit) const {
  Coordinate2D minimumOfOpposite = nextMinimumOfExpression(symbol, start, step, max, [](char symbol, double x, Context & context, Preferences::AngleUnit angleUnit, const ExpressionReference expression0, const ExpressionReference expression1 = nullptr) {
        return -expression0.approximateWithValueForSymbol(symbol, x, context, angleUnit);
      }, context, angleUnit);
  return {.abscissa = minimumOfOpposite.abscissa, .value = -minimumOfOpposite.value};
}

double ExpressionReference::nextRoot(char symbol, double start, double step, double max, Context & context, Preferences::AngleUnit angleUnit) const {
  return nextIntersectionWithExpression(symbol, start, step, max, [](char symbol, double x, Context & context, Preferences::AngleUnit angleUnit, const ExpressionReference expression0, const ExpressionReference expression1 = nullptr) {
        return expression0.approximateWithValueForSymbol(symbol, x, context, angleUnit);
      }, context, angleUnit, nullptr);
}

typename ExpressionReference::Coordinate2D ExpressionReference::nextIntersection(char symbol, double start, double step, double max, Poincare::Context & context, Preferences::AngleUnit angleUnit, const ExpressionReference expression) const {
  double resultAbscissa = nextIntersectionWithExpression(symbol, start, step, max, [](char symbol, double x, Context & context, Preferences::AngleUnit angleUnit, const ExpressionReference expression0, const ExpressionReference expression1) {
        return expression0.approximateWithValueForSymbol(symbol, x, context, angleUnit)-expression1.approximateWithValueForSymbol(symbol, x, context, angleUnit);
      }, context, angleUnit, expression);
  typename ExpressionReference::Coordinate2D result = {.abscissa = resultAbscissa, .value = approximateWithValueForSymbol(symbol, resultAbscissa, context, angleUnit)};
  if (std::fabs(result.value) < step*k_solverPrecision) {
    result.value = 0.0;
  }
  return result;
}

typename ExpressionReference::Coordinate2D ExpressionReference::nextMinimumOfExpression(char symbol, double start, double step, double max, EvaluationAtAbscissa evaluate, Context & context, Preferences::AngleUnit angleUnit, const ExpressionReference expression, bool lookForRootMinimum) const {
  Coordinate2D result = {.abscissa = NAN, .value = NAN};
  if (start == max || step == 0.0) {
    return result;
  }
  double bracket[3];
  double x = start;
  bool endCondition = false;
  do {
    bracketMinimum(symbol, x, step, max, bracket, evaluate, context, angleUnit, expression);
    result = brentMinimum(symbol, bracket[0], bracket[2], evaluate, context, angleUnit, expression);
    x = bracket[1];
    // Because of float approximation, exact zero is never reached
    if (std::fabs(result.abscissa) < std::fabs(step)*k_solverPrecision) {
      result.abscissa = 0;
      result.value = evaluate(symbol, 0, context, angleUnit, *this, expression);
    }
    /* Ignore extremum whose value is undefined or too big because they are
     * really unlikely to be local extremum. */
    if (std::isnan(result.value) || std::fabs(result.value) > k_maxFloat) {
      result.abscissa = NAN;
    }
    // Idem, exact 0 never reached
    if (std::fabs(result.value) < std::fabs(step)*k_solverPrecision) {
      result.value = 0;
    }
    endCondition = std::isnan(result.abscissa) && (step > 0.0 ? x <= max : x >= max);
    if (lookForRootMinimum) {
      endCondition |= std::fabs(result.value) > 0 && (step > 0.0 ? x <= max : x >= max);
    }
  } while (endCondition);
  if (lookForRootMinimum) {
    result.abscissa = std::fabs(result.value) > 0 ? NAN : result.abscissa;
  }
  return result;
}

void ExpressionReference::bracketMinimum(char symbol, double start, double step, double max, double result[3], EvaluationAtAbscissa evaluate, Context & context, Preferences::AngleUnit angleUnit, const ExpressionReference expression) const {
  Coordinate2D p[3];
  p[0] = {.abscissa = start, .value = evaluate(symbol, start, context, angleUnit, *this, expression)};
  p[1] = {.abscissa = start+step, .value = evaluate(symbol, start+step, context, angleUnit, *this, expression)};
  double x = start+2.0*step;
  while (step > 0.0 ? x <= max : x >= max) {
    p[2] = {.abscissa = x, .value = evaluate(symbol, x, context, angleUnit, *this, expression)};
    if ((p[0].value > p[1].value || std::isnan(p[0].value)) && (p[2].value > p[1].value || std::isnan(p[2].value)) && (!std::isnan(p[0].value) || !std::isnan(p[2].value))) {
      result[0] = p[0].abscissa;
      result[1] = p[1].abscissa;
      result[2] = p[2].abscissa;
      return;
    }
    if (p[0].value > p[1].value && p[1].value == p[2].value) {
    } else {
      p[0] = p[1];
      p[1] = p[2];
    }
    x += step;
  }
  result[0] = NAN;
  result[1] = NAN;
  result[2] = NAN;
}

typename ExpressionReference::Coordinate2D ExpressionReference::brentMinimum(char symbol, double ax, double bx, EvaluationAtAbscissa evaluate, Context & context, Preferences::AngleUnit angleUnit, const ExpressionReference expression) const {
  /* Bibliography: R. P. Brent, Algorithms for finding zeros and extrema of
   * functions without calculating derivatives */
  if (ax > bx) {
    return brentMinimum(symbol, bx, ax, evaluate, context, angleUnit, expression);
  }
  double e = 0.0;
  double a = ax;
  double b = bx;
  double x = a+k_goldenRatio*(b-a);
  double v = x;
  double w = x;
  double fx = evaluate(symbol, x, context, angleUnit, *this, expression);
  double fw = fx;
  double fv = fw;

  double d = NAN;
  double u, fu;

  for (int i = 0; i < 100; i++) {
    double m = 0.5*(a+b);
    double tol1 = k_sqrtEps*std::fabs(x)+1E-10;
    double tol2 = 2.0*tol1;
    if (std::fabs(x-m) <= tol2-0.5*(b-a))  {
      double middleFax = evaluate(symbol, (x+a)/2.0, context, angleUnit, *this, expression);
      double middleFbx = evaluate(symbol, (x+b)/2.0, context, angleUnit, *this, expression);
      double fa = evaluate(symbol, a, context, angleUnit, *this, expression);
      double fb = evaluate(symbol, b, context, angleUnit, *this, expression);
      if (middleFax-fa <= k_sqrtEps && fx-middleFax <= k_sqrtEps && fx-middleFbx <= k_sqrtEps && middleFbx-fb <= k_sqrtEps) {
        Coordinate2D result = {.abscissa = x, .value = fx};
        return result;
      }
    }
    double p = 0;
    double q = 0;
    double r = 0;
    if (std::fabs(e) > tol1) {
      r = (x-w)*(fx-fv);
      q = (x-v)*(fx-fw);
      p = (x-v)*q -(x-w)*r;
      q = 2.0*(q-r);
      if (q>0.0) {
        p = -p;
      } else {
        q = -q;
      }
      r = e;
      e = d;
    }
    if (std::fabs(p) < std::fabs(0.5*q*r) && p<q*(a-x) && p<q*(b-x)) {
      d = p/q;
      u= x+d;
      if (u-a < tol2 || b-u < tol2) {
        d = x < m ? tol1 : -tol1;
      }
    } else {
      e = x<m ? b-x : a-x;
      d = k_goldenRatio*e;
    }
    u = x + (std::fabs(d) >= tol1 ? d : (d>0 ? tol1 : -tol1));
    fu = evaluate(symbol, u, context, angleUnit, *this, expression);
    if (fu <= fx) {
      if (u<x) {
        b = x;
      } else {
        a = x;
      }
      v = w;
      fv = fw;
      w = x;
      fw = fx;
      x = u;
      fx = fu;
    } else {
      if (u<x) {
        a = u;
      } else {
        b = u;
      }
      if (fu <= fw || w == x) {
        v = w;
        fv = fw;
        w = u;
        fw = fu;
      } else if (fu <= fv || v == x || v == w) {
        v = u;
        fv = fu;
      }
    }
  }
  Coordinate2D result = {.abscissa = x, .value = fx};
  return result;
}

double ExpressionReference::nextIntersectionWithExpression(char symbol, double start, double step, double max, EvaluationAtAbscissa evaluation, Context & context, Preferences::AngleUnit angleUnit, const ExpressionReference expression) const {
  if (start == max || step == 0.0) {
    return NAN;
  }
  double bracket[2];
  double result = NAN;
  static double precisionByGradUnit = 1E6;
  double x = start+step;
  do {
    bracketRoot(symbol, x, step, max, bracket, evaluation, context, angleUnit, expression);
    result = brentRoot(symbol, bracket[0], bracket[1], std::fabs(step/precisionByGradUnit), evaluation, context, angleUnit, expression);
    x = bracket[1];
  } while (std::isnan(result) && (step > 0.0 ? x <= max : x >= max));

  double extremumMax = std::isnan(result) ? max : result;
  Coordinate2D resultExtremum[2] = {
    nextMinimumOfExpression(symbol, start, step, extremumMax, [](char symbol, double x, Context & context, Preferences::AngleUnit angleUnit, const ExpressionReference expression0, const ExpressionReference expression1) {
        if (expression1.isDefined()) {
          return expression0.approximateWithValueForSymbol(symbol, x, context, angleUnit)-expression1.approximateWithValueForSymbol(symbol, x, context, angleUnit);
        } else {
          return expression0.approximateWithValueForSymbol(symbol, x, context, angleUnit);
        }
      }, context, angleUnit, expression, true),
    nextMinimumOfExpression(symbol, start, step, extremumMax, [](char symbol, double x, Context & context, Preferences::AngleUnit angleUnit, const ExpressionReference expression0, const ExpressionReference expression1) {
        if (expression1.isDefined()) {
          return expression1.approximateWithValueForSymbol(symbol, x, context, angleUnit)-expression0.approximateWithValueForSymbol(symbol, x, context, angleUnit);
        } else {
          return -expression0.approximateWithValueForSymbol(symbol, x, context, angleUnit);
        }
      }, context, angleUnit, expression, true)};
  for (int i = 0; i < 2; i++) {
    if (!std::isnan(resultExtremum[i].abscissa) && (std::isnan(result) || std::fabs(result - start) > std::fabs(resultExtremum[i].abscissa - start))) {
      result = resultExtremum[i].abscissa;
    }
  }
  if (std::fabs(result) < std::fabs(step)*k_solverPrecision) {
    result = 0;
  }
  return result;
}

void ExpressionReference::bracketRoot(char symbol, double start, double step, double max, double result[2], EvaluationAtAbscissa evaluation, Context & context, Preferences::AngleUnit angleUnit, const ExpressionReference expression) const {
  double a = start;
  double b = start+step;
  while (step > 0.0 ? b <= max : b >= max) {
    double fa = evaluation(symbol, a, context, angleUnit, *this, expression);
    double fb = evaluation(symbol, b, context, angleUnit,* this, expression);
    if (fa*fb <= 0) {
      result[0] = a;
      result[1] = b;
      return;
    }
    a = b;
    b = b+step;
  }
  result[0] = NAN;
  result[1] = NAN;
}

double ExpressionReference::brentRoot(char symbol, double ax, double bx, double precision, EvaluationAtAbscissa evaluation, Context & context, Preferences::AngleUnit angleUnit, const ExpressionReference expression) const {
  if (ax > bx) {
    return brentRoot(symbol, bx, ax, precision, evaluation, context, angleUnit, expression);
  }
  double a = ax;
  double b = bx;
  double c = bx;
  double d = b-a;
  double e = b-a;
  double fa = evaluation(symbol, a, context, angleUnit, *this, expression);
  double fb = evaluation(symbol, b, context, angleUnit, *this, expression);
  double fc = fb;
  for (int i = 0; i < 100; i++) {
    if ((fb > 0.0 && fc > 0.0) || (fb < 0.0 && fc < 0.0)) {
      c = a;
      fc = fa;
      e = b-a;
      d = b-a;
    }
    if (std::fabs(fc) < std::fabs(fb)) {
      a = b;
      b = c;
      c = a;
      fa = fb;
      fb = fc;
      fc = fa;
    }
    double tol1 = 2.0*DBL_EPSILON*std::fabs(b)+0.5*precision;
    double xm = 0.5*(c-b);
    if (std::fabs(xm) <= tol1 || fb == 0.0) {
      double fbcMiddle = evaluation(symbol, 0.5*(b+c), context, angleUnit, *this, expression);
      double isContinuous = (fb <= fbcMiddle && fbcMiddle <= fc) || (fc <= fbcMiddle && fbcMiddle <= fb);
      if (isContinuous) {
        return b;
      }
    }
    if (std::fabs(e) >= tol1 && std::fabs(fa) > std::fabs(b)) {
      double s = fb/fa;
      double p = 2.0*xm*s;
      double q = 1.0-s;
      if (a != c) {
        q = fa/fc;
        double r = fb/fc;
        p = s*(2.0*xm*q*(q-r)-(b-a)*(r-1.0));
        q = (q-1.0)*(r-1.0)*(s-1.0);
      }
      q = p > 0.0 ? -q : q;
      p = std::fabs(p);
      double min1 = 3.0*xm*q-std::fabs(tol1*q);
      double min2 = std::fabs(e*q);
      if (2.0*p < (min1 < min2 ? min1 : min2)) {
        e = d;
        d = p/q;
      } else {
        d = xm;
        e =d;
      }
    } else {
      d = xm;
      e = d;
    }
    a = b;
    fa = fb;
    if (std::fabs(d) > tol1) {
      b += d;
    } else {
      b += xm > 0.0 ? tol1 : tol1;
    }
    fb = evaluation(symbol, b, context, angleUnit, *this, expression);
  }
  return NAN;
}

}

#include "calculation.h"
#include <string.h>
using namespace Poincare;

namespace Calculation {

Calculation::Calculation() :
  m_input(nullptr),
  m_inputLayout(nullptr),
  m_output(nullptr),
  m_outputLayout(nullptr)
{
}

Calculation::~Calculation() {
  if (m_inputLayout != nullptr) {
    delete m_inputLayout;
    m_inputLayout = nullptr;
  }
  if (m_input != nullptr) {
    delete m_input;
    m_input = nullptr;
  }
  if (m_output != nullptr) {
    delete m_output;
    m_output = nullptr;
  }
  if (m_outputLayout != nullptr) {
    delete m_outputLayout;
    m_outputLayout = nullptr;
  }
}

void Calculation::reset() {
  m_inputText[0] = 0;
  m_outputText[0] = 0;
  tidy();
}

void Calculation::setContent(const char * c, Context * context) {
  reset();
  strlcpy(m_inputText, c, sizeof(m_inputText));
  output(context)->writeTextInBuffer(m_outputText, sizeof(m_outputText));
}

const char * Calculation::inputText() {
  return m_inputText;
}

const char * Calculation::outputText() {
  return m_outputText;
}

Expression * Calculation::input() {
  if (m_input == nullptr) {
    m_input = Expression::parse(m_inputText);
  }
  return m_input;
}

ExpressionLayout * Calculation::inputLayout() {
  if (m_inputLayout == nullptr && input() != nullptr) {
    m_inputLayout = input()->createLayout(Expression::FloatDisplayMode::Decimal, Expression::ComplexFormat::Algebric);
  }
  return m_inputLayout;
}

Expression * Calculation::output(Context * context) {
  if (m_output == nullptr && input() != nullptr) {
    m_output = input()->evaluate(*context);
  }
  return m_output;
}

ExpressionLayout * Calculation::outputLayout(Context * context) {
  if (m_outputLayout == nullptr && output(context) != nullptr) {
    m_outputLayout = output(context)->createLayout();
  }
  return m_outputLayout;
}

bool Calculation::isEmpty() {
  if (strlen(m_inputText) == 0) {
    return true;
  }
  return false;
}

void Calculation::tidy() {
  if (m_input != nullptr) {
    delete m_input;
  }
  m_input = nullptr;
  if (m_inputLayout != nullptr) {
    delete m_inputLayout;
  }
  m_inputLayout = nullptr;
  if (m_output != nullptr) {
    delete m_output;
  }
  m_output = nullptr;
  if (m_outputLayout != nullptr) {
    delete m_outputLayout;
  }
  m_outputLayout = nullptr;
}

}

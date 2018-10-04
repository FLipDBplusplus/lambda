#ifndef SHARED_STORAGE_FUNCTION_H
#define SHARED_STORAGE_FUNCTION_H

#include <escher/text_field.h>
#include "storage_expression_model.h"

namespace Shared {

class StorageFunction : public StorageExpressionModel {
public:
  StorageFunction(const char * name = nullptr, KDColor color = KDColorBlack) :
    StorageExpressionModel(name),
    m_color(color),
    m_active(true)
  {}
  StorageFunction(Ion::Storage::Record record, KDColor color = KDColorBlack) :
    StorageExpressionModel(record),
    m_color(color),
    m_active(true)
  {
  }
  virtual uint32_t checksum();
  KDColor color() const { return m_color; }
  bool isActive() const { return m_active; }
  void setActive(bool active) { m_active = active; }
  void setColor(KDColor color) { m_color = color; }
  virtual float evaluateAtAbscissa(float x, Poincare::Context * context) const {
    return templatedApproximateAtAbscissa(x, context);
  }
  virtual double evaluateAtAbscissa(double x, Poincare::Context * context) const {
    return templatedApproximateAtAbscissa(x, context);
  }
  virtual double sumBetweenBounds(double start, double end, Poincare::Context * context) const = 0;
private:
  constexpr static size_t k_dataLengthInBytes = (TextField::maxBufferSize()+2)*sizeof(char)+2;
  static_assert((k_dataLengthInBytes & 0x3) == 0, "The function data size is not a multiple of 4 bytes (cannot compute crc)"); // Assert that dataLengthInBytes is a multiple of 4
  template<typename T> T templatedApproximateAtAbscissa(T x, Poincare::Context * context) const;
  virtual const char * symbol() const = 0;
  KDColor m_color;
  bool m_active;
};

}

#endif

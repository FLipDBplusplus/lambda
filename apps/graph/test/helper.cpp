#include "helper.h"

namespace Graph {

NewFunction * addFunction(const char * definition, NewFunction::PlotType type, ContinuousFunctionStore * store, Context * context) {
  Ion::Storage::Record::ErrorStatus err = store->addEmptyModel();
  assert(err == Ion::Storage::Record::ErrorStatus::None);
  (void) err; // Silence compilation warning about unused variable.
  Ion::Storage::Record record = store->recordAtIndex(store->numberOfModels() - 1);
  NewFunction * f = static_cast<NewFunction *>(store->modelForRecord(record).operator->());
  f->setPlotType(type, Poincare::Preferences::AngleUnit::Degree, context);
  f->setContent(definition, context);
  return f;
}

}

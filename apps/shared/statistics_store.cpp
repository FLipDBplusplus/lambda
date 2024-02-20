#include "statistics_store.h"

namespace Shared {

StatisticsStore::StatisticsStore(GlobalContext* context,
                                 DoublePairStorePreferences* preferences)
    : DoublePairStore(context, preferences) {
  /* Update series after having set the datasets, which are needed in
   * updateSeries */
  initListsFromStorage(true);
  for (int s = 0; s < k_numberOfSeries; s++) {
    m_datasets[s] = Poincare::StatisticsDataset<double>(&m_dataLists[s][0],
                                                        &m_dataLists[s][1]);
    updateSeries(s);
  }
}

void StatisticsStore::invalidateSortedIndexes() {
  for (int i = 0; i < k_numberOfSeries; i++) {
    m_datasets[i].setHasBeenModified();
  }
}

double StatisticsStore::sumOfOccurrences(int series) const {
  return m_datasets[series].totalWeight();
}

double StatisticsStore::mean(int series) const {
  return m_datasets[series].mean();
}

double StatisticsStore::standardDeviation(int series) const {
  return m_datasets[series].standardDeviation();
}

double StatisticsStore::sampleStandardDeviation(int series) const {
  return m_datasets[series].sampleStandardDeviation();
}

bool StatisticsStore::updateSeries(int series, bool delayUpdate) {
  m_datasets[series].setHasBeenModified();
  return DoublePairStore::updateSeries(series, delayUpdate);
}

}  // namespace Shared

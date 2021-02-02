#ifndef ESCHER_METRIC_H
#define ESCHER_METRIC_H

#include <kandinsky/coordinate.h>

namespace Escher {

class Metric {
public:
  constexpr static KDCoordinate SmallCellMargin = 2;
  constexpr static KDCoordinate BigCellMargin = 8;
  constexpr static KDCoordinate CellTopMargin = BigCellMargin;
  constexpr static KDCoordinate CellRightMargin = BigCellMargin;
  constexpr static KDCoordinate CellLeftMargin = BigCellMargin;
  constexpr static KDCoordinate CellBottomMargin = BigCellMargin;
  constexpr static KDCoordinate CellHorizontalElementMargin = BigCellMargin;
  constexpr static KDCoordinate CellVerticalElementMargin = 4;
  constexpr static KDCoordinate CommonLeftMargin = 14;
  constexpr static KDCoordinate CommonRightMargin = 14;
  constexpr static KDCoordinate CommonTopMargin = 9;
  constexpr static KDCoordinate CommonBottomMargin = 9;
  constexpr static KDCoordinate CommonLargeMargin = 10;
  constexpr static KDCoordinate CommonSmallMargin = 5;
  constexpr static KDCoordinate CommonMenuMargin = 5;
  constexpr static KDCoordinate TitleBarExternHorizontalMargin = 5;
  constexpr static KDCoordinate TitleBarHeight = 18;
  constexpr static KDCoordinate TabHeight = 27;
  constexpr static KDCoordinate ScrollStep = 10;
  constexpr static KDCoordinate PopUpMargin = 27;
  constexpr static KDCoordinate PopUpLeftMargin = PopUpMargin;
  constexpr static KDCoordinate PopUpRightMargin = PopUpMargin;
  constexpr static KDCoordinate PopUpTopMargin = PopUpMargin;
  constexpr static KDCoordinate PopUpBottomMargin = 55;
  constexpr static KDCoordinate StoreRowHeight = 50;
  constexpr static KDCoordinate StackTitleHeight = 20;
  constexpr static KDCoordinate FractionAndConjugateHorizontalOverflow = 2;
  constexpr static KDCoordinate FractionAndConjugateHorizontalMargin = 2;
  constexpr static KDCoordinate MinimalBracketAndParenthesisHeight = 18;
  constexpr static KDCoordinate CellSeparatorThickness = 1;
  constexpr static KDCoordinate TableSeparatorThickness = 5;
  constexpr static KDCoordinate ExpressionViewHorizontalMargin = 5;
  constexpr static KDCoordinate EllipsisCellWidth = 37;
};

}
#endif

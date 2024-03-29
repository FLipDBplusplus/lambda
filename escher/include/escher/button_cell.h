#ifndef SHARED_BUTTON_CELL_H
#define SHARED_BUTTON_CELL_H

#include <escher/abstract_button_cell.h>
#include <escher/metric.h>

namespace Escher {

class ButtonCell : public Escher::AbstractButtonCell {
 public:
  enum class Style { EmbossedLight, EmbossedGray };

  ButtonCell(Responder* parentResponder, I18n::Message textBody,
             Escher::Invocation invocation, Style style,
             KDColor backgroundColor = Escher::Palette::WallScreen,
             KDCoordinate horizontalMargins = 0,
             KDFont::Size fontSize = KDFont::Size::Large,
             KDColor textColor = KDColorBlack);
  void drawRect(KDContext* ctx, KDRect rect) const override;
  KDSize minimalSizeForOptimalDisplay() const override;

  KDColor backgroundColor() const { return m_backgroundColor; }
  void setBackgroundColor(const KDColor backgroundColor) {
    m_backgroundColor = backgroundColor;
  }

 protected:
  constexpr static KDCoordinate k_lineThickness =
      Escher::Metric::CellSeparatorThickness;

 private:
  void layoutSubviews(bool force = false) override;
  void drawBorder(KDContext* ctx, OMG::Direction direction, int index,
                  KDColor color) const;
  int numberOfBordersInDirection(OMG::Direction direction) const;

  KDColor m_backgroundColor;
  KDCoordinate m_horizontalMargins;
  Style m_style;
};

}  // namespace Escher
#endif

#ifndef POINCARE_EXPRESSION_LAYOUT_H
#define POINCARE_EXPRESSION_LAYOUT_H

#include <kandinsky.h>

namespace Poincare {

class ExpressionLayoutCursor;

class ExpressionLayout {
public:
  enum class VerticalDirection {
    Up,
    Down
  };
  enum class HorizontalDirection {
    Left,
    Right
  };

  /* Constructor & Destructor */
  ExpressionLayout();
  virtual  ~ExpressionLayout() = default;
  virtual ExpressionLayout * clone() const = 0;

  /* Rendering */
  void draw(KDContext * ctx, KDPoint p, KDColor expressionColor = KDColorBlack, KDColor backgroundColor = KDColorWhite);
  KDPoint origin();
  KDPoint absoluteOrigin();
  KDSize size();
  KDCoordinate baseline();
  virtual void invalidAllSizesPositionsAndBaselines();

  /* Hierarchy */
  virtual const ExpressionLayout * const * children() const = 0;
  const ExpressionLayout * child(int i) const;
  ExpressionLayout * editableChild(int i) { return const_cast<ExpressionLayout *>(child(i)); }
  virtual int numberOfChildren() const = 0;
  int indexOfChild(ExpressionLayout * child) const;

  void setParent(ExpressionLayout * parent);
  const ExpressionLayout * parent() const { return m_parent; }
  ExpressionLayout * editableParent() { return m_parent; }
  bool hasAncestor(const ExpressionLayout * e) const;

  bool insertLayoutAtCursor(ExpressionLayout * newChild, ExpressionLayoutCursor * cursor);

  virtual void addBrother(ExpressionLayoutCursor * cursor, ExpressionLayout * brother);
  ExpressionLayout * replaceWith(ExpressionLayout * newChild, bool deleteAfterReplace = true);
  ExpressionLayout * replaceWithJuxtapositionOf(ExpressionLayout * leftChild, ExpressionLayout * rightChild, bool deleteAfterReplace);
  virtual void replaceChild(const ExpressionLayout * oldChild, ExpressionLayout * newChild, bool deleteOldChild = true);
  void detachChild(const ExpressionLayout * e); // Removes a child WITHOUT deleting it

  /* Dynamic Layout*/
  virtual bool addChildAtIndex(ExpressionLayout * child, int index) { return false; }
  virtual void removeChildAtIndex(int index, bool deleteAfterRemoval);
  virtual void backspaceAtCursor(ExpressionLayoutCursor * cursor);

  /* Tree navigation */
  virtual bool moveLeft(ExpressionLayoutCursor * cursor) { return false; } //TODO should be virtual pure?
  virtual bool moveRight(ExpressionLayoutCursor * cursor) { return false; } //TODO should be virtual pure?
  virtual bool moveUp(ExpressionLayoutCursor * cursor, ExpressionLayout * previousLayout = nullptr, ExpressionLayout * previousPreviousLayout = nullptr);
  bool moveUpInside(ExpressionLayoutCursor * cursor);
  virtual bool moveDown(ExpressionLayoutCursor * cursor, ExpressionLayout * previousLayout = nullptr, ExpressionLayout * previousPreviousLayout = nullptr);
  bool moveDownInside(ExpressionLayoutCursor * cursor);

  /* Expression Engine */
  virtual int writeTextInBuffer(char * buffer, int bufferSize) const = 0;

  /* Other */
  virtual bool isCollapsable(int * numberOfOpenParenthesis, bool goingLeft) const { return true; }
  // isCollapsable is used when adding a brother fraction: should the layout be inserted in the numerator (or denominator)?
  virtual bool isHorizontal() const { return false; }
  virtual bool isLeftParenthesis() const { return false; }
  virtual bool isRightParenthesis() const { return false; }
  virtual bool isLeftBracket() const { return false; }
  virtual bool isRightBracket() const { return false; }
  virtual bool isEmpty() const { return false; }
  virtual char XNTChar() const;

protected:
  virtual void render(KDContext * ctx, KDPoint p, KDColor expressionColor, KDColor backgroundColor) = 0;
  virtual KDSize computeSize() = 0;
  virtual void computeBaseline() = 0;
  virtual KDPoint positionOfChild(ExpressionLayout * child) = 0;
  void detachChildAtIndex(int i);
  virtual void moveCursorInsideAtDirection (
    VerticalDirection direction,
    ExpressionLayoutCursor * cursor,
    ExpressionLayout ** childResult,
    void * resultPosition,
    int * resultPositionInside,
    int * resultScore);
  KDCoordinate m_baseline;
  ExpressionLayout * m_parent;
  bool m_sized;
  bool m_baselined;
  bool m_positioned;
private:
  bool moveInside(VerticalDirection direction, ExpressionLayoutCursor * cursor);
  void replaceWithJuxtapositionOf(ExpressionLayout * firstLayout, ExpressionLayout * secondLayout);
  KDRect m_frame;
};

}

#endif

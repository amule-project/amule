//
// DimensionSafety.h - Centralized defensive dimension validation utilities
// Part of the aMule Project
//
// Provides safe dimension calculations to prevent negative values and
// ensure UI consistency across the application.
//

#ifndef DIMENSION_SAFETY_H
#define DIMENSION_SAFETY_H

#include <algorithm>
#include <wx/gdicmn.h>

namespace DimensionSafety {

/**
 * Safely calculate a dimension with offset, ensuring non-negative result
 * @param dimension Original dimension value
 * @param offset Offset to subtract (typically padding/margin)
 * @param minValue Minimum allowed value (default 1 for UI elements)
 * @return Dimension clamped to minimum value
 */
inline int SafeDimension(int dimension, int offset = 0, int minValue = 1)
{
    return std::max(dimension - offset, minValue);
}

/**
 * Safely calculate an unsigned dimension with offset
 * @param dimension Original dimension value
 * @param offset Offset to subtract
 * @param minValue Minimum allowed value (default 1)
 * @return Dimension clamped to minimum value
 */
inline unsigned int SafeDimension(unsigned int dimension, unsigned int offset = 0, 
                                  unsigned int minValue = 1)
{
    return (dimension > offset) ? (dimension - offset) : minValue;
}

/**
 * Safe centering calculation to prevent negative positioning
 * @param containerSize Size of the containing element
 * @param elementSize Size of the element to center
 * @return Position coordinate clamped to minimum 0
 */
inline int SafeCenterPosition(int containerSize, int elementSize)
{
    return std::max((containerSize - elementSize) / 2, 0);
}

/**
 * Safe wxSize calculation with minimum dimensions
 * @param size Original size
 * @param minWidth Minimum width (default 1)
 * @param minHeight Minimum height (default 1)
 * @return Size clamped to minimum values
 */
inline wxSize SafeSize(const wxSize& size, int minWidth = 1, int minHeight = 1)
{
    return wxSize(std::max(size.GetWidth(), minWidth),
                  std::max(size.GetHeight(), minHeight));
}

/**
 * Safe rectangle dimensions to prevent invalid drawing areas
 * @param rect Original rectangle
 * @param minSize Minimum size for both dimensions (default 1)
 * @return Rectangle with safe dimensions
 */
inline wxRect SafeRect(const wxRect& rect, int minSize = 1)
{
    return wxRect(rect.GetPosition(), 
                 wxSize(std::max(rect.GetWidth(), minSize),
                        std::max(rect.GetHeight(), minSize)));
}

} // namespace DimensionSafety

#endif // DIMENSION_SAFETY_H
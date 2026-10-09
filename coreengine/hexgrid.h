#ifndef HEXGRID_H
#define HEXGRID_H

#include <QtGlobal>
#include <QtMath>
#include <QPoint>
#include <QPointF>

#include <array>

/**
 * @brief central primitives of the experimental hex grid prototype.
 *
 * The map keeps its (x, y) storage, but the coordinates are interpreted as axial hex coordinates (q = x, r = y)
 * of a pointy top hex grid. A rectangular map storage is therefore shown as a parallelogram.
 * Axial coordinates are used instead of an offset layout, cause the six neighbour offsets don't depend
 * on the field. This keeps all relative offset api's (e.g. globals.getCircle) valid for scripts and the ai.
 *
 * A hex is drawn into the usual imageSize x imageSize box. Rows are moved 3/4 of the image size down and
 * half an image size to the right.
 */
namespace HexGrid
{
    inline bool g_enabled = !qEnvironmentVariableIsSet("COW_SQUARE_GRID");

    inline bool enabled()
    {
        return g_enabled;
    }
    /**
     * @brief neighbourCount amount of neighbours of a field in the current grid mode
     */
    inline qint32 neighbourCount()
    {
        return g_enabled ? 6 : 4;
    }
    /**
     * @brief neighbours the first four entries match the square grid neighbours (east, west, south, north)
     */
    inline const std::array<QPoint, 6> neighbours = {QPoint(1, 0), QPoint(-1, 0), QPoint(0, 1), QPoint(0, -1), QPoint(1, -1), QPoint(-1, 1)};
    /**
     * @brief g_wrapWidth width of the shown map if it wraps around in east west direction, else 0.
     * On a wrapping map the field left of column 0 is the last column of the same row and the other way round.
     * A wrapping map stores row y turned by y / 2 columns, so the six neighbour offsets stay the same for all fields:
     * the field drawn in column c of row y is stored at column wrapX(c - y / 2).
     */
    inline qint32 g_wrapWidth = 0;
    /**
     * @brief g_wrapScroll horizontal camera position of a wrapping map in pixel for an image size of defaultScrollSize
     */
    inline qint32 g_wrapScrollHalf = 0;
    inline bool wraps()
    {
        return g_enabled && g_wrapWidth > 0;
    }
    /**
     * @brief wrapX column of the map storage for any column. Unchanged for maps which don't wrap.
     */
    inline qint32 wrapX(qint32 x)
    {
        if (g_wrapWidth > 0 && g_enabled)
        {
            x %= g_wrapWidth;
            if (x < 0)
            {
                x += g_wrapWidth;
            }
        }
        return x;
    }
    /**
     * @brief wrapDelta shortest column difference on a wrapping map
     */
    inline qint32 wrapDelta(qint32 dx)
    {
        if (g_wrapWidth > 0 && g_enabled)
        {
            dx = wrapX(dx);
            if (dx > g_wrapWidth / 2)
            {
                dx -= g_wrapWidth;
            }
        }
        return dx;
    }
    /**
     * @brief neighbourIndex index of the given offset in the neighbours array or -1 if it's no neighbour offset
     */
    inline qint32 neighbourIndex(qint32 dx, qint32 dy)
    {
        dx = wrapDelta(dx);
        for (qint32 i = 0; i < neighbourCount(); ++i)
        {
            if (neighbours[i].x() == dx && neighbours[i].y() == dy)
            {
                return i;
            }
        }
        return -1;
    }
    /**
     * @brief distance of the given offset in fields
     */
    inline qint32 distance(qint32 dx, qint32 dy)
    {
        if (g_enabled)
        {
            qint32 result = (qAbs(dx) + qAbs(dy) + qAbs(dx + dy)) / 2;
            if (g_wrapWidth > 0)
            {
                // the other way around the map may be shorter
                for (qint32 turn = -1; turn <= 1; turn += 2)
                {
                    const qint32 wrappedDx = dx + turn * g_wrapWidth;
                    result = qMin(result, (qAbs(wrappedDx) + qAbs(dy) + qAbs(wrappedDx + dy)) / 2);
                }
            }
            return result;
        }
        return qAbs(dx) + qAbs(dy);
    }
    inline qint32 distance(qint32 x1, qint32 y1, qint32 x2, qint32 y2)
    {
        return distance(x1 - x2, y1 - y2);
    }
    /**
     * @brief rowHeight vertical distance between two rows in pixel
     */
    inline qint32 rowHeight(qint32 imageSize)
    {
        return g_enabled ? imageSize * 3 / 4 : imageSize;
    }
    /**
     * @brief play area of the shown map. Maps with fields without a tile (terrain VOID), e.g. hexagon shaped maps,
     * are only drawn inside of the bounding box of their existing fields.
     * Horizontal positions are counted in half fields (2 * x + y), vertical positions in rows.
     * A span below zero means the whole map storage is shown.
     */
    inline qint32 g_areaMinHalf = 0;
    inline qint32 g_areaSpanHalf = -1;
    inline qint32 g_areaMinRow = 0;
    inline qint32 g_areaSpanRows = -1;
    inline void setPlayArea(qint32 minHalf, qint32 maxHalf, qint32 minRow, qint32 maxRow)
    {
        g_areaMinHalf = minHalf;
        g_areaSpanHalf = maxHalf - minHalf;
        g_areaMinRow = minRow;
        g_areaSpanRows = maxRow - minRow;
    }
    inline void resetPlayArea()
    {
        g_areaMinHalf = 0;
        g_areaSpanHalf = -1;
        g_areaMinRow = 0;
        g_areaSpanRows = -1;
    }
    inline bool hasPlayArea()
    {
        return g_enabled && g_areaSpanHalf >= 0;
    }
    /**
     * @brief fieldToScreen top left corner of the image box of the given field in map pixel
     */
    inline QPoint fieldToScreen(qint32 x, qint32 y, qint32 imageSize)
    {
        if (wraps())
        {
            // column on the screen in half fields: odd rows are moved half a field to the right
            const qint32 halfWidth = 2 * g_wrapWidth;
            qint32 half = (2 * wrapX(x + (y >> 1)) + (y & 1) - g_wrapScrollHalf) % halfWidth;
            if (half < 0)
            {
                half += halfWidth;
            }
            return QPoint(half * imageSize / 2, y * rowHeight(imageSize));
        }
        if (hasPlayArea())
        {
            return QPoint((2 * x + y - g_areaMinHalf) * imageSize / 2, (y - g_areaMinRow) * rowHeight(imageSize));
        }
        if (g_enabled)
        {
            return QPoint(x * imageSize + y * imageSize / 2, y * rowHeight(imageSize));
        }
        return QPoint(x * imageSize, y * imageSize);
    }
    /**
     * @brief offsetToScreen pixel offset between two fields which are dx and dy fields apart.
     * Use this instead of fieldToScreen for positions relative to another field e.g. a range outline around a unit:
     * fieldToScreen includes the origin of the shown play area.
     */
    inline QPoint offsetToScreen(qint32 dx, qint32 dy, qint32 imageSize)
    {
        if (g_enabled)
        {
            return QPoint(dx * imageSize + dy * imageSize / 2, dy * rowHeight(imageSize));
        }
        return QPoint(dx * imageSize, dy * imageSize);
    }
    /**
     * @brief mapPixelWidth width of the whole map in pixel
     */
    inline qint32 mapPixelWidth(qint32 width, qint32 height, qint32 imageSize)
    {
        if (wraps())
        {
            return g_wrapWidth * imageSize + imageSize / 2;
        }
        if (hasPlayArea())
        {
            return (g_areaSpanHalf + 2) * imageSize / 2;
        }
        if (g_enabled)
        {
            return width * imageSize + (height > 0 ? (height - 1) * imageSize / 2 : 0);
        }
        return width * imageSize;
    }
    /**
     * @brief wrapPeriod horizontal distance in pixel after which a wrapping map repeats, 0 for other maps
     */
    inline qint32 wrapPeriod(qint32 imageSize)
    {
        return wraps() ? g_wrapWidth * imageSize : 0;
    }
    /**
     * @brief wrapCopies a wrapping map is drawn several times next to each other, so the camera can scroll endless
     */
    inline constexpr qint32 wrapCopies = 3;
    /**
     * @brief mapActorWidth width of the map actor in pixel. Includes all drawn copies of a wrapping map
     */
    inline qint32 mapActorWidth(qint32 width, qint32 height, qint32 imageSize)
    {
        return mapPixelWidth(width, height, imageSize) + (wrapCopies - 1) * wrapPeriod(imageSize);
    }
    /**
     * @brief mapPixelHeight height of the whole map in pixel
     */
    inline qint32 mapPixelHeight(qint32 height, qint32 imageSize)
    {
        if (wraps())
        {
            return height > 0 ? (height - 1) * rowHeight(imageSize) + imageSize : 0;
        }
        if (hasPlayArea())
        {
            return g_areaSpanRows * rowHeight(imageSize) + imageSize;
        }
        if (g_enabled)
        {
            return height > 0 ? (height - 1) * rowHeight(imageSize) + imageSize : 0;
        }
        return height * imageSize;
    }
    /**
     * @brief screenToField field at the given map pixel. The result may be outside of the map.
     */
    inline QPoint screenToField(qreal pixelX, qreal pixelY, qint32 imageSize)
    {
        if (imageSize <= 0)
        {
            return QPoint(0, 0);
        }
        if (!g_enabled)
        {
            return QPoint(qFloor(pixelX / imageSize), qFloor(pixelY / imageSize));
        }
        if (wraps())
        {
            // find the closest field of the rectangle on the screen and turn its column back into the storage column
            const qreal rowSize = rowHeight(imageSize);
            const qint32 firstRow = qFloor((pixelY - imageSize / 2.0) / rowSize);
            QPoint best(0, 0);
            qreal bestDistance = -1;
            for (qint32 y = firstRow; y <= firstRow + 1; ++y)
            {
                const qreal rowShift = (y & 1) * imageSize / 2.0 - g_wrapScrollHalf * imageSize / 2.0;
                const qint32 firstColumn = qFloor((pixelX - rowShift - imageSize / 2.0) / imageSize);
                for (qint32 column = firstColumn; column <= firstColumn + 1; ++column)
                {
                    const qreal dx = column * imageSize + rowShift + imageSize / 2.0 - pixelX;
                    const qreal dy = (y * rowSize + imageSize / 2.0 - pixelY) * 1.1547;
                    const qreal dist = dx * dx + dy * dy;
                    if (bestDistance < 0 || dist < bestDistance)
                    {
                        bestDistance = dist;
                        best = QPoint(wrapX(column - (y >> 1)), y);
                    }
                }
            }
            return best;
        }
        if (hasPlayArea())
        {
            // back to the positions of the whole map storage
            pixelX += g_areaMinHalf * imageSize / 2.0;
            pixelY += g_areaMinRow * rowHeight(imageSize);
        }
        // take the candidate rows and use the field with the closest center
        const qreal rowSize = rowHeight(imageSize);
        const qint32 row = qFloor((pixelY - imageSize / 2.0) / rowSize);
        QPoint best(0, 0);
        qreal bestDistance = -1;
        for (qint32 y = row; y <= row + 1; ++y)
        {
            const qint32 column = qFloor((pixelX - y * imageSize / 2.0 - imageSize / 2.0) / imageSize);
            for (qint32 x = column; x <= column + 1; ++x)
            {
                const qreal dx = x * imageSize + y * imageSize / 2.0 + imageSize / 2.0 - pixelX;
                // scale y so the row distance matches the one of a regular hex
                const qreal dy = (y * rowSize + imageSize / 2.0 - pixelY) * 1.1547;
                const qreal dist = dx * dx + dy * dy;
                if (bestDistance < 0 || dist < bestDistance)
                {
                    bestDistance = dist;
                    best = QPoint(x, y);
                }
            }
        }
        return best;
    }
}

#endif // HEXGRID_H

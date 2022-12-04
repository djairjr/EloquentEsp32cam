//
// Created by Simone on 29/11/22.
//

#ifndef ELOQUENTESP32CAM_LINECROSSINGCOUNTER_H
#define ELOQUENTESP32CAM_LINECROSSINGCOUNTER_H


#include "../motion/Detector.h"
#include "../../traits/HasErrorMessage.h"
#include "../../traits/Debounces.h"

namespace Eloquent {
    namespace Esp32cam {
        namespace Applications {
            /**
             * Detect objects that cross a vertical line
             */
            class LineCrossingCounter : public HasErrorMessage, public Debounces {
            public:

                /**
                 *
                 * @param detector
                 */
                LineCrossingCounter(Motion::Detector& detector) :
                    _detector(&detector),
                    _x(0),
                    _miny(0),
                    _maxy(0.999),
                    _t(0),
                    _lag(3),
                    _sparsity(4),
                    _ltrCount(0),
                    _rtlCount(0) {

                }

                /**
                 * Set x-coordinate of vertical line
                 *
                 * @param x
                 */
                inline void lineAt(float x) {
                    _x = x;
                }

                /**
                 * Limit analysis to region above given coordinate
                 * @param y
                 */
                inline void above(float y) {
                    _miny = y;
                }

                /**
                 * Limit analysis to region below given coordinate
                 * @param y
                 */
                inline void below(float y) {
                    _maxy = y;
                }

                /**
                 * How many frames is object allowed to take
                 * to cross the line?
                 * Increase this value to accomodate slower transitions
                 *
                 * @param lag
                 */
                inline void lag(uint8_t lag) {
                    _lag = lag;
                }

                /**
                 * How large each band to the left and to
                 * the right of the vertical line is?
                 * The lower, the smaller objects will be detected
                 * @param sparsity
                 */
                inline void sparsity(uint8_t sparsity)
                {
                    _sparsity = sparsity;
                }

                /**
                 *
                 * @return
                 */
                bool update() {
                    const uint16_t width = _detector->getWidth();
                    const uint16_t height = _detector->getHeight();
                    const uint16_t x = _x >= 1 ? _x / 8 : _x * width;
                    const uint16_t above = height - (_miny >= 1 ? _miny / 8 : _miny * height);
                    const uint16_t below = height - (_maxy >= 1 ? _maxy / 8 : _maxy * height);

                    if (x < 1)
                        return setErrorMessage("x-coordinate must be >= 8");

                    if (below >= above)
                        return setErrorMessage("above/below limits mismatch");

                    _t += 1;

                    if (_t > 65000)
                        _t = 1;

                    // detect foreground 3 cells to the left and 3 to the right
                    // of x, within the y limits (if any)
                    for (int16_t i = -3; i <= 3; i++) {
                        if (i == 0)
                            continue;

                        if (_motion[i + 3] >= _t - _lag)
                            continue;

                        for (uint8_t j = 0; j < _sparsity; j++) {
                            const int16_t dx = i * _sparsity + j;

                            if (dx + x < 0 || dx + x > width)
                                continue;

                            for (uint16_t y = below; y < above; y++) {
                                if (_detector->isForeground(dx + x, y)) {
                                    _motion[i + 3] = _t;
                                    break;
                                }
                            }

                            if (_motion[i + 3])
                                break;
                        }
                    }

                    return setErrorMessage("");
                }

                /**
                 * Test if object crossed the line from left to right
                 *
                 * @return
                 */
                bool crossedFromLeftToRight() {
                    if (!debounced())
                        return false;

                    const uint8_t crossed = gt(-2, -3) + gt(-1, -2) + gt(1, -1) + gt(2, 1);

                    if (crossed > 20 && (crossed % 10) > 0) {
                        _ltrCount += 1;
                        touch();
                        return true;
                    }

                    return false;
                }

                /**
                 * Test if object crossed the line from right to left
                 *
                 * @return
                 */
                bool crossedFromRightToLeft() {
                    if (!debounced())
                        return false;

                    const uint8_t crossed = gt(-2, -1) + gt(-1, 1) + gt(1, 2) + gt(2, 3);

                    if (crossed > 20 && (crossed % 10) > 0) {
                        _rtlCount += 1;
                        touch();
                        return true;
                    }

                    return false;
                }

                /**
                 * How many objects crossed the line from left to right
                 * so far?
                 *
                 * @return
                 */
                inline uint16_t getLeftToRightCount() {
                    return _ltrCount;
                }

                /**
                 * How many objects crossed the line from right to left
                 * so far?
                 *
                 * @return
                 */
                inline uint16_t getRightToLeftCount() {
                    return _rtlCount;
                }

                String debug() {
                    return String("motion = {")
                        + _motion[0]
                        + ", "
                        + _motion[1]
                        + ", "
                        + _motion[2]
                        + ", "
                        + _motion[3]
                        + ", "
                        + _motion[4]
                        + ", "
                        + _motion[5]
                        + ", "
                        + _motion[6]
                        + "}";
                }

            protected:
                float _x;
                float _miny;
                float _maxy;
                uint8_t _lag;
                uint8_t _sparsity;
                uint16_t _t;
                uint16_t _ltrCount;
                uint16_t _rtlCount;
                uint16_t _motion[7];
                Motion::Detector *_detector;

                /**
                 * Test if motion at index a happened after
                 * motion at index b within a time lag
                 *
                 * @param a
                 * @param b
                 * @return
                 */
                uint8_t gt(int a, int b) {
                    a += 3;
                    b += 3;

                    if (_motion[a] == 0 || _motion[b] == 0 || _motion[a] < _t - 2 * _lag)
                        return 0;

                    // strict gt
                    if (_motion[a] > _motion[b] && _motion[a] - _motion[b] <= _lag)
                        return 10;

                    return _motion[a] >= _motion[b] && _motion[a] - _motion[b] <= _lag;
                }
            };
        }
    }
}

#endif //ELOQUENTESP32CAM_LINECROSSINGCOUNTER_H

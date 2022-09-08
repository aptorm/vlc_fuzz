/*****************************************************************************
 * Copyright (C) 2022 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * ( at your option ) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/
import QtQuick 2.11

import org.videolan.compat 0.1

import "qrc:///style/"
import "qrc:///util/Helpers.js" as Helpers

ListView {
    id: root

    property bool disableBeginningFade: false
    property bool disableEndFade: false

    property real fadeSize: root.delegateItem
                           ? (orientation === Qt.Vertical ? root.delegateItem.height
                                                          : root.delegateItem.width) / 2
                           : (VLCStyle.margin_large * 2)

    readonly property bool transitionsRunning: ((add ? add.running : false) ||
                                                (addDisplaced ? addDisplaced.running : false) ||
                                                (populate ? populate.running : false) ||
                                                (remove ? remove.running : false) ||
                                                (removeDisplaced ? removeDisplaced.running : false))

    // TODO: Use itemAtIndex(0) Qt >= 5.13
    // FIXME: Delegate with variable size
    readonly property Item delegateItem: root.contentItem.children.length > 0
                                         ? root.contentItem.children[root.contentItem.children.length - 1]
                                         : null

    readonly property Item firstVisibleItem: {
        if (transitionsRunning || !delegateItem)
            return null

        var margin = 0 // -root.displayMarginBeginning
        if (orientation === Qt.Vertical) {
            if (headerItem && headerItem.visible && headerPositioning === ListView.OverlayHeader)
                margin += headerItem.height

            return itemAt(contentX + (delegateItem.x + delegateItem.width / 2), contentY + margin)
        } else {
            if (headerItem && headerItem.visible && headerPositioning === ListView.OverlayHeader)
                margin += headerItem.width

            return itemAt(contentX + margin, contentY + (delegateItem.y + delegateItem.height / 2))
        }
    }

    readonly property Item lastVisibleItem: {
        if (transitionsRunning || !delegateItem)
            return null

        var margin = 0 // -root.displayMarginEnd
        if (orientation === Qt.Vertical) {
            if (footerItem && footerItem.visible && footerPositioning === ListView.OverlayFooter)
                margin += footerItem.height

            return itemAt(contentX + (delegateItem.x + delegateItem.width / 2), contentY + height - margin - 1)
        } else {
            if (footerItem && footerItem.visible && footerPositioning === ListView.OverlayFooter)
                margin += footerItem.width

            return itemAt(contentX + width - margin - 1, contentY + (delegateItem.y + delegateItem.height / 2))
        }
    }

    readonly property bool _fadeRectEnoughSize: (root.orientation === Qt.Vertical
                                                 ? root.height
                                                 : root.width) > (fadeSize * 2 + VLCStyle.dp(25))

    readonly property bool _beginningFade: !disableBeginningFade &&
                                           _fadeRectEnoughSize &&
                                           (orientation === ListView.Vertical ? !atYBeginning
                                                                              : !atXBeginning) &&
                                           (!firstVisibleItem ||
                                           (!firstVisibleItem.activeFocus &&
                                            !Helpers.get(firstVisibleItem, "hovered", false)))

    readonly property bool _endFade:       !disableEndFade &&
                                           _fadeRectEnoughSize &&
                                           (orientation === ListView.Vertical ? !atYEnd
                                                                              : !atXEnd) &&
                                           (!lastVisibleItem ||
                                           (!lastVisibleItem.activeFocus &&
                                            !Helpers.get(lastVisibleItem, "hovered", false)))

    // If Qt < 5.14, have to force clip enable,
    // because we can not dictate the reused
    // one to always have a binding assigned
    // to clip property.
    Component.onCompleted: {
        if (!(clipBinding.restoreMode === 3 /* RestoreBindingOrValue */)) {
            clip = Qt.binding(function() { return true })
        }
    }

    // When the effect is active,
    // it will clip inherently.
    // So the parent does not need
    // clipping enabled.
    BindingCompat on clip {
        id: clipBinding
        when: proxyContentItem.visible
        value: false
    }

    ShaderEffectSource {
        id: proxyContentItem

        visible: (root._beginningFade || root._endFade) ||
                 (layer.effect.beginningFadeSize > 0 || layer.effect.endFadeSize > 0)

        BindingCompat on visible {
            // Let's see if the effect is compatible...
            value: false
            when: proxyContentItem.effectCompatible
        }

        readonly property bool effectCompatible: !(((GraphicsInfo.shaderType === GraphicsInfo.GLSL)) &&
                                                  ((GraphicsInfo.shaderSourceType & GraphicsInfo.ShaderSourceString)))

        anchors.fill: parent

        z: root.contentItem.z

        sourceItem: root.contentItem

        sourceRect: Qt.rect(root.contentX,
                            root.contentY,
                            root.width,
                            root.height)

        // Make sure contentItem is not rendered twice:
        hideSource: visible

        layer.enabled: true
        layer.effect: ShaderEffect {
            // It makes sense to use the effect for only in the fading part.
            // However, it would complicate things in the QML side. As it
            // would require two additional items, as well as two more texture
            // allocations.
            // Given the shading done here is not complex, this is not done.
            // Applying the texture and the effect is done in one step.

            id: effect

            readonly property bool vertical: (root.orientation === Qt.Vertical)
            readonly property real normalFadeSize: root.fadeSize / (vertical ? height : width)

            property real beginningFadeSize: root._beginningFade ? normalFadeSize : 0
            property real endFadeSize: root._endFade ? normalFadeSize : 0
            readonly property real endFadePos: 1.0 - endFadeSize

            // TODO: Qt >= 5.15 use inline component
            Behavior on beginningFadeSize {
                // Qt Bug: UniformAnimator does not work...
                NumberAnimation {
                    duration: VLCStyle.duration_veryShort
                    easing.type: Easing.InOutSine
                }
            }

            Behavior on endFadeSize {
                // Qt Bug: UniformAnimator does not work...
                NumberAnimation {
                    duration: VLCStyle.duration_veryShort
                    easing.type: Easing.InOutSine
                }
            }

            // Atlas textures can be supported
            // but in this use case it is not
            // necessary as the layer texture
            // can not be placed in the atlas.
            supportsAtlasTextures: false

            vertexShader: " uniform highp mat4 qt_Matrix;

                            attribute highp vec4 qt_Vertex;
                            attribute highp vec2 qt_MultiTexCoord0;

                            varying highp vec2 coord;
                            varying highp float pos; // x or y component of coord depending on orientation

                            void main() {
                                coord = qt_MultiTexCoord0;

                                pos = qt_MultiTexCoord0.%1;

                                gl_Position = qt_Matrix * qt_Vertex;
                            } ".arg(vertical ? "y" : "x")

            fragmentShader: " uniform lowp sampler2D source;
                              uniform lowp float qt_Opacity;

                              uniform highp float beginningFadeSize;
                              uniform highp float endFadePos;

                              varying highp vec2 coord;
                              varying highp float pos;

                              void main() {
                                  lowp vec4 texel = texture2D(source, coord);

                                  // Note that the whole texel is multiplied instead
                                  // of only the alpha component because it must be
                                  // in premultiplied alpha format.
                                  texel *= (1.0 - smoothstep(endFadePos, 1.0, pos));
                                  texel *= (smoothstep(0.0, beginningFadeSize, pos));

                                  // We still need to respect the accumulated scene graph opacity:
                                  gl_FragColor = texel * qt_Opacity;
                              } "
        }
    }
}

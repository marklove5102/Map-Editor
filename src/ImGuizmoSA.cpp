#include "pch.h"
#include "utils/utils.h"
#include "ImGuizmoSA.h"
#include "CMatrix.h"
#include "CCamera.h"
#include "CWorld.h"
#include "CGame.h"
#include <cmath>

// In ImGuizmoSA.h - change these values to adjust behavior:

static constexpr float BASE_WORLD_RADIUS = 0.3f;      // Smaller base size
static constexpr float DISTANCE_SCALE_FACTOR = 0.08f; // Less aggressive scaling
static constexpr float TRANSLATE_SPEED = 0.02f;       // Faster translation
static constexpr float ROTATE_SPEED = 0.01f;          // Rotation sensitivity
static constexpr float MAX_WORLD_RADIUS = 3.0f;       // Reasonable maximum size

namespace ImGuizmoSA
{
    Gizmo g_Gizmo;

    Gizmo::Gizmo()
    {
        mCurrentMode = SPACE_MODE::LOCAL;
        mIsUsing = false;
        mIsOver = false;
        mHoveredAxis = -1;
        mActiveAxis = -1;
        mIsTranslating = false;
        mIsRotating = false;

        // Classic axis colors
        mAxisColors[0] = IM_COL32(255, 60, 60, 255);  // Red
        mAxisColors[1] = IM_COL32(60, 255, 60, 255);  // Green
        mAxisColors[2] = IM_COL32(60, 120, 255, 255); // Blue
        mHoverColor = IM_COL32(255, 255, 80, 255);    // Yellow
        mActiveColor = IM_COL32(255, 255, 0, 255);    // Bright Yellow
    }

    bool Gizmo::Manipulate(CObject *object)
    {
        if (!object)
            return false;

        CVector objectPos = object->GetPosition();
        float worldRadius = CalculateWorldRadius(objectPos);

        bool wasUsing = mIsUsing;

        DrawUniversalGizmo(object);
        mIsUsing = HandleUniversalInteraction(object, worldRadius);

        return wasUsing && !mIsUsing;
    }

    void Gizmo::DrawUniversalGizmo(CObject *object)
    {
        CVector objectPos = object->GetPosition();
        CMatrix objectMatrix = *object->GetMatrix();
        float worldRadius = CalculateWorldRadius(objectPos);

        // Draw rotation gizmo first (background)
        DrawRotationGizmo(objectPos, worldRadius);

        // Draw translation gizmo second (foreground)
        DrawTranslationGizmo(objectPos, objectMatrix, worldRadius);
    }

    void Gizmo::DrawTranslationGizmo(const CVector &position, const CMatrix &matrix, float worldRadius)
    {
        // Scale translation axis length with world radius
        float axisLength = worldRadius * 0.6f; // Make it proportional to rotation circles

        for (int axis = 0; axis < 3; axis++)
        {
            CVector axisDir = GetAxisDirection(axis, matrix);
            CVector axisEnd = position + axisDir * axisLength;

            float screenStartX, screenStartY, screenEndX, screenEndY;
            if (!Utils::WorldToScreen(position.x, position.y, position.z, screenStartX, screenStartY) ||
                !Utils::WorldToScreen(axisEnd.x, axisEnd.y, axisEnd.z, screenEndX, screenEndY))
            {
                continue;
            }

            ImVec2 screenStart(screenStartX, screenStartY);
            ImVec2 screenEnd(screenEndX, screenEndY);

            bool isHovered = (mHoveredAxis == axis && mIsTranslating);
            bool isActive = (mActiveAxis == axis && mIsTranslating);

            float thickness = isActive ? 4.0f : (isHovered ? 3.0f : 2.5f);
            ImU32 color = isActive ? mActiveColor : (isHovered ? mHoverColor : mAxisColors[axis]);

            ImDrawList *drawList = ImGui::GetBackgroundDrawList();

            // Draw axis line
            drawList->AddLine(screenStart, screenEnd, color, thickness);

            // Draw arrow head
            ImVec2 lineDir = ImVec2(screenEnd.x - screenStart.x, screenEnd.y - screenStart.y);
            float lineLength = sqrtf(lineDir.x * lineDir.x + lineDir.y * lineDir.y);
            if (lineLength > 0.0f)
            {
                lineDir = ImVec2(lineDir.x / lineLength, lineDir.y / lineLength);
            }

            float arrowSize = 15.0f;
            ImVec2 perpendicular = ImVec2(-lineDir.y, lineDir.x);

            ImVec2 arrowTip = screenEnd;
            ImVec2 arrowBase = ImVec2(screenEnd.x - lineDir.x * arrowSize, screenEnd.y - lineDir.y * arrowSize);
            ImVec2 arrowWing1 = ImVec2(arrowBase.x + perpendicular.x * arrowSize * 0.5f, arrowBase.y + perpendicular.y * arrowSize * 0.5f);
            ImVec2 arrowWing2 = ImVec2(arrowBase.x - perpendicular.x * arrowSize * 0.5f, arrowBase.y - perpendicular.y * arrowSize * 0.5f);

            // Draw filled arrow head
            drawList->AddTriangleFilled(arrowTip, arrowWing1, arrowWing2, color);

            // Draw axis label
            const char *axisLabels[] = {"X", "Y", "Z"};
            ImVec2 labelPos = ImVec2(screenEnd.x + lineDir.x * 20.0f, screenEnd.y + lineDir.y * 20.0f);

            // Text background for readability
            ImVec2 textSize = ImGui::CalcTextSize(axisLabels[axis]);
            ImVec2 textBgMin = ImVec2(labelPos.x - textSize.x * 0.5f - 3.0f, labelPos.y - textSize.y * 0.5f - 2.0f);
            ImVec2 textBgMax = ImVec2(labelPos.x + textSize.x * 0.5f + 3.0f, labelPos.y + textSize.y * 0.5f + 2.0f);
            drawList->AddRectFilled(textBgMin, textBgMax, IM_COL32(0, 0, 0, 180), 3.0f);
            drawList->AddText(labelPos, IM_COL32(255, 255, 255, 255), axisLabels[axis]);
        }

        // Draw center point
        float screenX, screenY;
        if (Utils::WorldToScreen(position.x, position.y, position.z, screenX, screenY))
        {
            ImDrawList *drawList = ImGui::GetBackgroundDrawList();
            drawList->AddCircleFilled(ImVec2(screenX, screenY), 6.0f, IM_COL32(255, 255, 255, 200));
            drawList->AddCircle(ImVec2(screenX, screenY), 6.0f, IM_COL32(255, 255, 255, 255), 0, 1.5f);
        }
    }

    void Gizmo::DrawRotationGizmo(const CVector &position, float worldRadius)
    {
        float screenX, screenY;
        if (!Utils::WorldToScreen(position.x, position.y, position.z, screenX, screenY))
            return;

        ImVec2 center(screenX, screenY);
        ImDrawList *drawList = ImGui::GetBackgroundDrawList();

        // Draw three circles in WORLD SPACE
        for (int axis = 0; axis < 3; axis++)
        {
            bool isHovered = (mHoveredAxis == axis && mIsRotating);
            bool isActive = (mActiveAxis == axis && mIsRotating);

            ImU32 color = isActive ? mActiveColor : (isHovered ? mHoverColor : mAxisColors[axis]);
            float thickness = isActive ? 3.0f : (isHovered ? 2.5f : 2.0f);

            // Draw circle in WORLD SPACE axes
            const int segments = 48; // Reduced segments for smaller circles
            for (int i = 0; i < segments; i++)
            {
                float angle1 = (float)i / (float)segments * 2.0f * 3.14159f;
                float angle2 = (float)(i + 1) / (float)segments * 2.0f * 3.14159f;

                CVector p1, p2;

                if (axis == 0)
                { // X-axis circle (YZ plane in world space)
                    p1 = position + CVector(0, cosf(angle1) * worldRadius, sinf(angle1) * worldRadius);
                    p2 = position + CVector(0, cosf(angle2) * worldRadius, sinf(angle2) * worldRadius);
                }
                else if (axis == 1)
                { // Y-axis circle (XZ plane in world space)
                    p1 = position + CVector(cosf(angle1) * worldRadius, 0, sinf(angle1) * worldRadius);
                    p2 = position + CVector(cosf(angle2) * worldRadius, 0, sinf(angle2) * worldRadius);
                }
                else
                { // Z-axis circle (XY plane in world space)
                    p1 = position + CVector(cosf(angle1) * worldRadius, sinf(angle1) * worldRadius, 0);
                    p2 = position + CVector(cosf(angle2) * worldRadius, sinf(angle2) * worldRadius, 0);
                }

                // Convert to screen space
                float p1X, p1Y, p2X, p2Y;
                if (Utils::WorldToScreen(p1.x, p1.y, p1.z, p1X, p1Y) &&
                    Utils::WorldToScreen(p2.x, p2.y, p2.z, p2X, p2Y))
                {
                    drawList->AddLine(ImVec2(p1X, p1Y), ImVec2(p2X, p2Y), color, thickness);
                }
            }
        }
    }

    bool Gizmo::HandleUniversalInteraction(CObject *object, float worldRadius)
    {
        CVector objectPos = object->GetPosition();
        CMatrix objectMatrix = *object->GetMatrix();

        float screenX, screenY;
        if (!Utils::WorldToScreen(objectPos.x, objectPos.y, objectPos.z, screenX, screenY))
        {
            return false;
        }

        ImVec2 center(screenX, screenY);
        ImVec2 mousePos = ImGui::GetMousePos();
        bool mouseDown = ImGui::IsMouseDown(0);

        bool wasUsing = mIsUsing;
        mIsUsing = false;
        mIsOver = false;

        // Reset hover state if not interacting
        if (mActiveAxis == -1)
        {
            mHoveredAxis = -1;
            mIsTranslating = false;
            mIsRotating = false;
        }

        // Hover detection - check both translation and rotation
        if (mActiveAxis == -1)
        {
            float bestDistance = 15.0f; // Pixel threshold

            // Check translation axes first (they have priority for interaction)
            for (int axis = 0; axis < 3; axis++)
            {
                CVector axisDir = GetAxisDirection(axis, objectMatrix);
                float axisLength = worldRadius * 0.6f; // Same as drawing
                CVector axisEnd = objectPos + axisDir * axisLength;

                float screenStartX, screenStartY, screenEndX, screenEndY;
                if (!Utils::WorldToScreen(objectPos.x, objectPos.y, objectPos.z, screenStartX, screenStartY) ||
                    !Utils::WorldToScreen(axisEnd.x, axisEnd.y, axisEnd.z, screenEndX, screenEndY))
                {
                    continue;
                }

                // Check distance to translation axis line
                ImVec2 lineDir = ImVec2(screenEndX - screenStartX, screenEndY - screenStartY);
                float lineLength = sqrtf(lineDir.x * lineDir.x + lineDir.y * lineDir.y);
                if (lineLength > 0.0f)
                {
                    lineDir = ImVec2(lineDir.x / lineLength, lineDir.y / lineLength);
                }

                ImVec2 toMouse = ImVec2(mousePos.x - screenStartX, mousePos.y - screenStartY);
                float projection = toMouse.x * lineDir.x + toMouse.y * lineDir.y;
                projection = std::max(0.0f, std::min(lineLength, projection));

                ImVec2 closestPoint = ImVec2(screenStartX + lineDir.x * projection, screenStartY + lineDir.y * projection);
                float distance = sqrtf(powf(mousePos.x - closestPoint.x, 2) + powf(mousePos.y - closestPoint.y, 2));

                // Also check arrow head
                float arrowDistance = sqrtf(powf(mousePos.x - screenEndX, 2) + powf(mousePos.y - screenEndY, 2));
                distance = std::min(distance, arrowDistance);

                if (distance < bestDistance)
                {
                    bestDistance = distance;
                    mHoveredAxis = axis;
                    mIsOver = true;
                    mIsTranslating = true;
                    mIsRotating = false;
                }
            }

            // If no translation axis hovered, check rotation circles
            if (mHoveredAxis == -1)
            {
                for (int axis = 0; axis < 3; axis++)
                {
                    // Test points around the world-space circle
                    const int testPoints = 20;
                    for (int i = 0; i < testPoints; i++)
                    {
                        float angle = (float)i / (float)testPoints * 2.0f * 3.14159f;
                        CVector testPoint;

                        if (axis == 0)
                        { // X-axis circle (YZ plane)
                            testPoint = objectPos + CVector(0, cosf(angle) * worldRadius, sinf(angle) * worldRadius);
                        }
                        else if (axis == 1)
                        { // Y-axis circle (XZ plane)
                            testPoint = objectPos + CVector(cosf(angle) * worldRadius, 0, sinf(angle) * worldRadius);
                        }
                        else
                        { // Z-axis circle (XY plane)
                            testPoint = objectPos + CVector(cosf(angle) * worldRadius, sinf(angle) * worldRadius, 0);
                        }

                        float testX, testY;
                        if (Utils::WorldToScreen(testPoint.x, testPoint.y, testPoint.z, testX, testY))
                        {
                            float distance = sqrtf(powf(mousePos.x - testX, 2) + powf(mousePos.y - testY, 2));
                            if (distance < bestDistance)
                            {
                                bestDistance = distance;
                                mHoveredAxis = axis;
                                mIsOver = true;
                                mIsTranslating = false;
                                mIsRotating = true;
                            }
                        }
                    }
                }
            }

            // Check center point
            float centerDist = sqrtf(powf(mousePos.x - center.x, 2) + powf(mousePos.y - center.y, 2));
            if (centerDist < 10.0f && centerDist < bestDistance)
            {
                mHoveredAxis = -1;
                mIsOver = true;
            }
        }

        // Start interaction
        static bool mouseWasDown = false;
        if (!mouseWasDown && mouseDown && mIsOver)
        {
            mActiveAxis = mHoveredAxis;
            mInteractionStartPos = object->GetPosition();
            mInteractionStartMatrix = *object->GetMatrix();
            mouseWasDown = true;
            ImGui::ResetMouseDragDelta(0);
        }

        // Handle active interaction
        if (mActiveAxis != -1 && mouseDown)
        {
            mIsUsing = true;

            if (mIsTranslating)
            {
                HandleTranslationInteraction(object, mInteractionStartMatrix);
            }
            else if (mIsRotating)
            {
                HandleRotationInteraction(object);
            }

            UpdateObjectInWorld(object);
        }

        // End interaction
        if (mouseWasDown && !mouseDown)
        {
            mActiveAxis = -1;
            mIsTranslating = false;
            mIsRotating = false;
            mouseWasDown = false;
            UpdateObjectInWorld(object);
        }

        mouseWasDown = mouseDown;
        return mIsUsing;
    }

    bool Gizmo::HandleTranslationInteraction(CObject *object, const CMatrix &matrix)
    {
        ImVec2 mouseDelta = ImGui::GetMouseDragDelta(0);

        CVector axisDir = GetAxisDirection(mActiveAxis, matrix);

        // Fixed: Use proper axis directions and consistent speed
        float movementX = mouseDelta.x * TRANSLATE_SPEED;
        float movementY = mouseDelta.y * TRANSLATE_SPEED;

        // Apply movement based on axis - fixed directions
        CVector movement;
        switch (mActiveAxis)
        {
        case 0: // X-axis - use X movement
            movement = axisDir * movementX;
            break;
        case 1: // Y-axis - use Y movement (fixed direction)
            movement = axisDir * movementY;
            break;
        case 2: // Z-axis - use combined movement for better control
            movement = axisDir * (movementX + movementY) * 0.7f;
            break;
        }

        // Apply movement relative to start position
        CVector newPos = mInteractionStartPos + movement;
        object->SetPosn(newPos);

        return true;
    }

    bool Gizmo::HandleRotationInteraction(CObject *object)
    {
        ImVec2 mouseDelta = ImGui::GetMouseDragDelta(0);
        float rotationAmount = mouseDelta.x * ROTATE_SPEED;

        CMatrix newMatrix = mInteractionStartMatrix;

        // Rotate around object's LOCAL axes
        switch (mActiveAxis)
        {
        case 0: // X-axis
            newMatrix.RotateX(rotationAmount);
            break;
        case 1: // Y-axis
            newMatrix.RotateY(rotationAmount);
            break;
        case 2: // Z-axis
            newMatrix.RotateZ(rotationAmount);
            break;
        }

        CVector currentPos = object->GetPosition();
        object->SetMatrix(newMatrix);
        object->SetPosn(currentPos);

        return true;
    }

    CVector Gizmo::GetAxisDirection(int axis, const CMatrix &matrix) const
    {
        switch (axis)
        {
        case 0:
            return mCurrentMode == SPACE_MODE::LOCAL ? matrix.GetRight() : CVector(1, 0, 0);
        case 1:
            return mCurrentMode == SPACE_MODE::LOCAL ? matrix.GetForward() : CVector(0, 1, 0);
        case 2:
            return mCurrentMode == SPACE_MODE::LOCAL ? matrix.GetUp() : CVector(0, 0, 1);
        default:
            return CVector(0, 0, 0);
        }
    }

    float Gizmo::CalculateWorldRadius(const CVector &objectPos) const
    {
        CVector cameraPos = TheCamera.GetPosition();
        float distance = (objectPos - cameraPos).Magnitude();

        // Make gizmo larger with distance using configurable factors
        float worldRadius = BASE_WORLD_RADIUS + (distance * DISTANCE_SCALE_FACTOR);

        // Clamp to reasonable limits
        return std::max(BASE_WORLD_RADIUS, std::min(MAX_WORLD_RADIUS, worldRadius));
    }

    void Gizmo::UpdateObjectInWorld(CObject *object)
    {
        if (object->m_nAreaCode != 0)
        {
            CWorld::Remove(object);
            CWorld::Add(object);
        }
    }
}
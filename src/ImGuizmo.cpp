#include "pch.h"
#include "utils/utils.h"
#include "ImGuizmo.h"
#include "CMatrix.h"
#include "CCamera.h"
#include "CWorld.h"
#include "CGame.h"
#include <cmath>

// Enhanced constants for better visibility
static constexpr float BASE_WORLD_RADIUS = 0.4f;     // Slightly larger base size
static constexpr float DISTANCE_SCALE_FACTOR = 0.1f; // More visible scaling
static constexpr float TRANSLATE_SPEED = 0.02f;
static constexpr float ROTATE_SPEED = 0.01f;
static constexpr float MAX_WORLD_RADIUS = 4.0f; // Larger maximum size

// New constants for improved visuals
static constexpr float TRANSLATE_AXIS_LENGTH_FACTOR = 1.6f; // Longer axes
static constexpr float ROTATION_CIRCLE_THICKNESS = 3.0f;    // Thicker circles
static constexpr float HOVERED_THICKNESS_MULTIPLIER = 1.5f; // More visible hover
static constexpr float ACTIVE_THICKNESS_MULTIPLIER = 2.0f;  // More visible active

static constexpr int GIZMO_VISIBLE_DISTANCE = 100; // More visible active

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

        // More vibrant Blender-like colors
        mAxisColors[0] = IM_COL32(255, 90, 90, 255);  // Brighter Red
        mAxisColors[1] = IM_COL32(90, 255, 90, 255);  // Brighter Green
        mAxisColors[2] = IM_COL32(90, 150, 255, 255); // Brighter Blue
        mHoverColor = IM_COL32(255, 255, 100, 255);   // Bright Yellow
        mActiveColor = IM_COL32(255, 255, 0, 255);    // Very Bright Yellow

        // Additional colors for better visibility
        mCenterColor = IM_COL32(255, 255, 255, 255);
        mCenterHoverColor = IM_COL32(255, 255, 200, 255);
    }

    bool Gizmo::Manipulate(CObject *object)
    {
        float dist = DistanceBetweenPoints(object->GetPosition(), TheCamera.GetPosition());
        if (!object || dist > GIZMO_VISIBLE_DISTANCE)
        {
            return false;
        }

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

        // Draw rotation gizmo first (background) with improved visibility
        DrawRotationGizmo(objectPos, worldRadius);

        // Draw translation gizmo second (foreground) with longer axes
        DrawTranslationGizmo(objectPos, objectMatrix, worldRadius);
    }

    void Gizmo::DrawTranslationGizmo(const CVector &position, const CMatrix &matrix, float worldRadius)
    {
        // Longer translation axis length
        float axisLength = worldRadius * TRANSLATE_AXIS_LENGTH_FACTOR;

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

            // Thicker lines with better visibility
            float baseThickness = 3.5f;
            if (isActive)
                baseThickness *= ACTIVE_THICKNESS_MULTIPLIER;
            else if (isHovered)
                baseThickness *= HOVERED_THICKNESS_MULTIPLIER;

            ImU32 color = isActive ? mActiveColor : (isHovered ? mHoverColor : mAxisColors[axis]);

            ImDrawList *drawList = ImGui::GetBackgroundDrawList();

            // Draw thicker axis line
            drawList->AddLine(screenStart, screenEnd, color, baseThickness);

            // Draw larger arrow head
            ImVec2 lineDir = ImVec2(screenEnd.x - screenStart.x, screenEnd.y - screenStart.y);
            float lineLength = sqrtf(lineDir.x * lineDir.x + lineDir.y * lineDir.y);
            if (lineLength > 0.0f)
            {
                lineDir = ImVec2(lineDir.x / lineLength, lineDir.y / lineLength);
            }

            float arrowSize = 23.0f; // Larger arrow head
            ImVec2 perpendicular = ImVec2(-lineDir.y, lineDir.x);

            ImVec2 arrowTip = screenEnd;
            ImVec2 arrowBase = ImVec2(screenEnd.x - lineDir.x * arrowSize, screenEnd.y - lineDir.y * arrowSize);
            ImVec2 arrowWing1 = ImVec2(arrowBase.x + perpendicular.x * arrowSize * 0.6f, arrowBase.y + perpendicular.y * arrowSize * 0.6f);
            ImVec2 arrowWing2 = ImVec2(arrowBase.x - perpendicular.x * arrowSize * 0.6f, arrowBase.y - perpendicular.y * arrowSize * 0.6f);

            // Draw filled arrow head with outline for better visibility
            drawList->AddTriangleFilled(arrowTip, arrowWing1, arrowWing2, color);
            drawList->AddTriangle(arrowTip, arrowWing1, arrowWing2, IM_COL32(0, 0, 0, 255), 1.5f);

            // Enhanced axis label with better visibility
            const char *axisLabels[] = {"X", "Y", "Z"};
            ImVec2 labelPos = ImVec2(screenEnd.x + lineDir.x * 25.0f, screenEnd.y + lineDir.y * 25.0f);

            // Better text background for readability
            ImVec2 textSize = ImGui::CalcTextSize(axisLabels[axis]);

            // Draw background with border
            drawList->AddText(labelPos, IM_COL32(255, 255, 255, 255), axisLabels[axis]);
        }

        // Enhanced center point
        float screenX, screenY;
        if (Utils::WorldToScreen(position.x, position.y, position.z, screenX, screenY))
        {
            ImDrawList *drawList = ImGui::GetBackgroundDrawList();
            bool centerHovered = (mHoveredAxis == -1 && mIsOver && !mIsTranslating && !mIsRotating);
            ImU32 centerColor = centerHovered ? mCenterHoverColor : mCenterColor;

            // Larger center point with outline
            drawList->AddCircleFilled(ImVec2(screenX, screenY), 8.0f, centerColor);
            drawList->AddCircle(ImVec2(screenX, screenY), 8.0f, IM_COL32(0, 0, 0, 255), 0, 2.0f);
        }
    }

    void Gizmo::DrawRotationGizmo(const CVector &position, float worldRadius)
    {
        float screenX, screenY;
        if (!Utils::WorldToScreen(position.x, position.y, position.z, screenX, screenY))
            return;

        ImVec2 center(screenX, screenY);
        ImDrawList *drawList = ImGui::GetBackgroundDrawList();

        // Draw three circles in WORLD SPACE with improved thickness
        for (int axis = 0; axis < 3; axis++)
        {
            bool isHovered = (mHoveredAxis == axis && mIsRotating);
            bool isActive = (mActiveAxis == axis && mIsRotating);

            ImU32 color = isActive ? mActiveColor : (isHovered ? mHoverColor : mAxisColors[axis]);

            // Much thicker circles with hover/active multipliers
            float thickness = ROTATION_CIRCLE_THICKNESS;
            if (isActive)
                thickness *= ACTIVE_THICKNESS_MULTIPLIER;
            else if (isHovered)
                thickness *= HOVERED_THICKNESS_MULTIPLIER;

            // More segments for smoother circles
            const int segments = 64;
            std::vector<ImVec2> screenPoints;
            screenPoints.reserve(segments);

            // Pre-calculate all screen points for the circle
            for (int i = 0; i <= segments; i++)
            {
                float angle = (float)i / (float)segments * 2.0f * 3.14159f;
                CVector worldPoint;

                if (axis == 0)
                { // X-axis circle (YZ plane in world space)
                    worldPoint = position + CVector(0, cosf(angle) * worldRadius, sinf(angle) * worldRadius);
                }
                else if (axis == 1)
                { // Y-axis circle (XZ plane in world space)
                    worldPoint = position + CVector(cosf(angle) * worldRadius, 0, sinf(angle) * worldRadius);
                }
                else
                { // Z-axis circle (XY plane in world space)
                    worldPoint = position + CVector(cosf(angle) * worldRadius, sinf(angle) * worldRadius, 0);
                }

                float pointX, pointY;
                if (Utils::WorldToScreen(worldPoint.x, worldPoint.y, worldPoint.z, pointX, pointY))
                {
                    screenPoints.push_back(ImVec2(pointX, pointY));
                }
            }

            // Draw the circle as connected lines
            if (screenPoints.size() > 1)
            {
                for (size_t i = 0; i < screenPoints.size() - 1; i++)
                {
                    drawList->AddLine(screenPoints[i], screenPoints[i + 1], color, thickness);
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

        // Increased hover thresholds for better interaction
        float translationThreshold = 20.0f; // Larger hit area for translation
        float rotationThreshold = 25.0f;    // Larger hit area for rotation

        // Hover detection - check both translation and rotation
        if (mActiveAxis == -1)
        {
            float bestDistance = translationThreshold;

            // Check translation axes first (they have priority for interaction)
            for (int axis = 0; axis < 3; axis++)
            {
                CVector axisDir = GetAxisDirection(axis, objectMatrix);
                float axisLength = worldRadius * TRANSLATE_AXIS_LENGTH_FACTOR;
                CVector axisEnd = objectPos + axisDir * axisLength;

                float screenStartX, screenStartY, screenEndX, screenEndY;
                if (!Utils::WorldToScreen(objectPos.x, objectPos.y, objectPos.z, screenStartX, screenStartY) ||
                    !Utils::WorldToScreen(axisEnd.x, axisEnd.y, axisEnd.z, screenEndX, screenEndY))
                {
                    continue;
                }

                // Check distance to translation axis line with larger threshold
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

                // Also check arrow head with larger threshold
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

            // If no translation axis hovered, check rotation circles with larger threshold
            if (mHoveredAxis == -1)
            {
                bestDistance = rotationThreshold;

                for (int axis = 0; axis < 3; axis++)
                {
                    // Test points around the world-space circle with more test points
                    const int testPoints = 30;
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

            // Check center point with larger threshold
            float centerDist = sqrtf(powf(mousePos.x - center.x, 2) + powf(mousePos.y - center.y, 2));
            if (centerDist < 15.0f && centerDist < bestDistance)
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
#pragma once
#include "plugin.h"
#include "CVector.h"
#include "CQuaternion.h"
#include "CObject.h"
#include "CMatrix.h"

namespace ImGuizmoSA
{
    enum class SPACE_MODE
    {
        LOCAL = 1,
        WORLD = 2
    };

    class Gizmo
    {
    private:
        SPACE_MODE mCurrentMode;
        bool mIsUsing;
        bool mIsOver;
        int mHoveredAxis;
        int mActiveAxis;
        bool mIsTranslating;
        bool mIsRotating;
        CVector mInteractionStartPos;
        CMatrix mInteractionStartMatrix;

        // Top-level constants for easy tuning
        static constexpr float BASE_WORLD_RADIUS = 0.3f;      // Base gizmo size
        static constexpr float DISTANCE_SCALE_FACTOR = 0.08f; // How much it grows with distance
        static constexpr float TRANSLATE_SPEED = 0.02f;       // Translation sensitivity
        static constexpr float ROTATE_SPEED = 0.01f;          // Rotation sensitivity
        static constexpr float MAX_WORLD_RADIUS = 3.0f;       // Maximum gizmo size

        // Colors
        ImU32 mAxisColors[3];
        ImU32 mHoverColor;
        ImU32 mActiveColor;

    public:
        Gizmo();

        bool Manipulate(CObject *object);
        void SetMode(SPACE_MODE mode) { mCurrentMode = mode; }
        bool IsUsing() const { return mIsUsing; }
        bool IsOver() const { return mIsOver; }

    private:
        void DrawUniversalGizmo(CObject *object);
        void DrawTranslationGizmo(const CVector &position, const CMatrix &matrix, float worldRadius);
        void DrawRotationGizmo(const CVector &position, float worldRadius);
        bool HandleUniversalInteraction(CObject *object, float worldRadius);
        bool HandleTranslationInteraction(CObject *object, const CMatrix &matrix);
        bool HandleRotationInteraction(CObject *object);
        CVector GetAxisDirection(int axis, const CMatrix &matrix) const;
        void UpdateObjectInWorld(CObject *object);
        float CalculateWorldRadius(const CVector &objectPos) const;
    };

    extern Gizmo g_Gizmo;
}
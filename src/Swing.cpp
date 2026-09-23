// ============================================================================
// Swing.cpp
// ============================================================================

#include "Swing.h"
#include "Primitives.h"
#include "Terrain.h"

using util::Vec3;

namespace
{
    const float POST_HEIGHT = 2.5f;
    const float POST_SPREAD = 0.85f;
    const float MAX_ANGLE   = 0.95f;

    // ------------------------------------------------------------------------
    // Draw a rope exactly between two points.
    // ------------------------------------------------------------------------
    void drawRopeBetween(
        const Vec3& from,
        const Vec3& to
    )
    {
        Vec3 delta = to - from;

        float length = delta.length();

        if (length <= 0.00001f)
            return;

        Vec3 direction = delta / length;

        Vec3 up(
            0.0f,
            1.0f,
            0.0f
        );

        float dotValue =
            util::clampf(
                util::dot(up, direction),
                -1.0f,
                1.0f
            );

        float angle =
            std::acos(dotValue) *
            180.0f /
            util::PI;

        Vec3 rotationAxis =
            util::cross(up, direction);

        glPushMatrix();

        glTranslatef(
            from.x,
            from.y,
            from.z
        );

        if (rotationAxis.lengthSquared() > 0.00000001f)
        {
            rotationAxis =
                rotationAxis.normalized();

            glRotatef(
                angle,
                rotationAxis.x,
                rotationAxis.y,
                rotationAxis.z
            );
        }
        else if (dotValue < 0.0f)
        {
            glRotatef(
                180.0f,
                1.0f,
                0.0f,
                0.0f
            );
        }

        prim::drawCylinder(
            0.022f,
            0.022f,
            length,
            5,
            false,
            false
        );

        glPopMatrix();
    }
}


// ============================================================================
// Constructor
// ============================================================================

Swing::Swing()
    : m_position(0.0f, 0.0f, 0.0f)
    , m_baseFacing(0.0f)
    , m_angle(0.0f)
    , m_angularVelocity(0.0f)
    , m_ropeLength(1.7f)
    , m_anchorHeight(POST_HEIGHT)
    , m_frameList(0)
    , m_treeMounted(false)
    , m_built(false)
{
}


// ============================================================================
// Normal swing initialization
// ============================================================================

void Swing::initialize(
    const Terrain& terrain,
    float x,
    float z,
    unsigned seed
)
{
    float ground =
        terrain.heightAt(
            x,
            z
        );

    m_position =
        Vec3(
            x,
            ground,
            z
        );

    util::Rng rng(
        seed ^ 0xA55EEDu
    );

    m_baseFacing =
        rng.range(
            0.0f,
            360.0f
        );

    m_angle = 0.0f;

    m_angularVelocity =
        0.0f;

    m_ropeLength =
        1.7f;

    m_anchorHeight =
        POST_HEIGHT;

    m_treeMounted =
        false;

    util::logInfo(
        "Swing placed."
    );
}


// ============================================================================
// Sakura tree-mounted swing
// ============================================================================

void Swing::initializeTreeMounted(
    const Terrain& terrain,
    float x,
    float z,
    float anchorHeight,
    float facingDegrees,
    unsigned seed
)
{
    (void)seed;

    float ground =
        terrain.heightAt(
            x,
            z
        );

    m_position =
        Vec3(
            x,
            ground,
            z
        );

    // Keep the value supplied by Sakura for the seat orientation.
    // The actual Sakura branch axes are fixed below:
    //
    // Branch / seat width = X
    // Swing movement       = Z
    m_baseFacing =
        facingDegrees;

    m_angle =
        0.0f;

    m_angularVelocity =
        0.0f;

    m_anchorHeight =
        anchorHeight;

    m_ropeLength =
        util::clampf(
            m_anchorHeight - 1.15f,
            3.45f,
            4.10f
        );

    m_treeMounted =
        true;

    util::logInfo(
        "Sakura swing attached to branch."
    );
}


// ============================================================================
// Build render data
// ============================================================================

void Swing::buildRenderData()
{
    if (m_built)
        return;

    m_frameList =
        glGenLists(1);

    if (!m_frameList)
    {
        util::logWarning(
            "Could not allocate the swing display list."
        );

        return;
    }

    glNewList(
        m_frameList,
        GL_COMPILE
    );

    glColor3f(
        0.34f,
        0.23f,
        0.15f
    );

    for (
        int side = -1;
        side <= 1;
        side += 2
    )
    {
        float sx =
            static_cast<float>(side)
            * POST_SPREAD;


        // Back post
        glPushMatrix();

        glTranslatef(
            sx,
            0.0f,
            -0.35f
        );

        glRotatef(
            static_cast<float>(-side) * 9.0f,
            0.0f,
            0.0f,
            1.0f
        );

        prim::drawCylinder(
            0.075f,
            0.055f,
            POST_HEIGHT,
            7,
            true,
            false
        );

        glPopMatrix();


        // Front post
        glPushMatrix();

        glTranslatef(
            sx,
            0.0f,
            0.35f
        );

        glRotatef(
            static_cast<float>(-side) * 9.0f,
            0.0f,
            0.0f,
            1.0f
        );

        prim::drawCylinder(
            0.075f,
            0.055f,
            POST_HEIGHT,
            7,
            true,
            false
        );

        glPopMatrix();
    }


    // Top beam
    glPushMatrix();

    glTranslatef(
        -POST_SPREAD,
        POST_HEIGHT,
        0.0f
    );

    glRotatef(
        90.0f,
        0.0f,
        0.0f,
        1.0f
    );

    prim::drawCylinder(
        0.065f,
        0.065f,
        POST_SPREAD * 2.0f,
        8,
        true,
        true
    );

    glPopMatrix();

    glEndList();

    m_built =
        true;
}


// ============================================================================
// Destroy render data
// ============================================================================

void Swing::destroyRenderData()
{
    if (m_frameList)
    {
        glDeleteLists(
            m_frameList,
            1
        );
    }

    m_frameList =
        0;

    m_built =
        false;
}


// ============================================================================
// Swing physics
// ============================================================================

void Swing::update(
    float deltaTime,
    bool occupied,
    float pumpInput
)
{
    const float gravityTerm =
        9.0f /
        m_ropeLength;

    float torque =
        -gravityTerm *
        std::sin(m_angle);


    if (occupied)
    {
        float nearBottom =
            std::cos(m_angle);

        torque +=
            util::clampf(
                pumpInput,
                -1.0f,
                1.0f
            )
            * 3.2f
            * nearBottom;
    }


    m_angularVelocity +=
        torque *
        deltaTime;


    m_angularVelocity *=
        (
            occupied
                ? 0.997f
                : 0.985f
        );


    m_angle +=
        m_angularVelocity *
        deltaTime;


    if (m_angle > MAX_ANGLE)
    {
        m_angle =
            MAX_ANGLE;

        m_angularVelocity *=
            0.3f;
    }


    if (m_angle < -MAX_ANGLE)
    {
        m_angle =
            -MAX_ANGLE;

        m_angularVelocity *=
            0.3f;
    }
}


// ============================================================================
// Seat position
// ============================================================================

Vec3 Swing::seatPosition() const
{
    // ========================================================================
    // SAKURA SWING AXES
    // ========================================================================
    //
    // The strong Sakura branch runs horizontally along X.
    //
    //       TRUNK
    //         |
    //         |
    //         |======================>
    //                    BRANCH
    //
    // The two ropes are separated along X.
    //
    // The swing itself moves perpendicular to the branch,
    // along Z (front/back).
    //
    // ========================================================================

    const Vec3 branchDirection(
        1.0f,
        0.0f,
        0.0f
    );

    const Vec3 swingDirection(
        0.0f,
        0.0f,
        1.0f
    );

    (void)branchDirection;

    float horizontal =
        m_ropeLength *
        std::sin(m_angle);

    float verticalDrop =
        m_ropeLength *
        std::cos(m_angle);

    Vec3 anchor =
        m_position +
        Vec3(
            0.0f,
            m_anchorHeight,
            0.0f
        );

    return
        anchor
        +
        swingDirection *
        horizontal
        +
        Vec3(
            0.0f,
            -verticalDrop,
            0.0f
        );
}


// ============================================================================
// Render
// ============================================================================

void Swing::renderSolid(
    const RenderContext& context
) const
{
    if (!m_built)
        return;


    if (!context.frustum.sphereVisible(
            m_position +
            Vec3(
                0.0f,
                m_anchorHeight * 0.55f,
                0.0f
            ),
            m_treeMounted
                ? 5.0f
                : 2.6f
        ))
    {
        return;
    }


    // ========================================================================
    // Freestanding swing
    // ========================================================================

    if (!m_treeMounted)
    {
        glPushMatrix();

        glTranslatef(
            m_position.x,
            m_position.y,
            m_position.z
        );

        glRotatef(
            m_baseFacing,
            0.0f,
            1.0f,
            0.0f
        );

        glCallList(
            m_frameList
        );

        glPopMatrix();
    }


    // ========================================================================
    // Tree-mounted Sakura swing
    // ========================================================================

    Vec3 anchor =
        m_position +
        Vec3(
            0.0f,
            m_anchorHeight,
            0.0f
        );


    // ------------------------------------------------------------------------
    // IMPORTANT:
    //
    // Sakura's strong branch is along X.
    //
    // Rope separation = X
    // Swing movement  = Z
    //
    // This is deliberately NOT calculated from m_baseFacing.
    // ------------------------------------------------------------------------

    const Vec3 branchDirection(
        1.0f,
        0.0f,
        0.0f
    );

    const Vec3 swingDirection(
        0.0f,
        0.0f,
        1.0f
    );


    Vec3 seat =
        seatPosition();


    // ========================================================================
    // ROPES
    // ========================================================================

    glColor3f(
        0.70f,
        0.60f,
        0.43f
    );

    const float ropeSpacing =
        0.30f;


    // ========================================================================
    //
    //                    SAKURA BRANCH
    //       ====================================
    //                  |              |
    //                  |              |
    //                  |              |
    //               +--------------------+
    //               |        SEAT        |
    //               +--------------------+
    //
    //                  <---> X
    //
    //              Swing movement
    //                    Z
    //                    ^
    //                    |
    //                    v
    //
    // ========================================================================

    for (
        int sideSign = -1;
        sideSign <= 1;
        sideSign += 2
    )
    {
        Vec3 ropeOffset =
            branchDirection *
            (
                ropeSpacing *
                static_cast<float>(sideSign)
            );


        Vec3 ropeAnchor =
            anchor +
            ropeOffset;


        Vec3 ropeSeat =
            seat +
            ropeOffset;


        drawRopeBetween(
            ropeAnchor,
            ropeSeat
        );
    }


    // ========================================================================
    // SEAT
    // ========================================================================

    glColor3f(
        0.43f,
        0.29f,
        0.16f
    );


    glPushMatrix();

    glTranslatef(
        seat.x,
        seat.y,
        seat.z
    );


    // The seat spans along the Sakura branch (X).
    // m_baseFacing is retained here because Sakura supplies 90 degrees,
    // which aligns the seat with the branch in the current scene.
glRotatef(
    0.0f,
    0.0f,
    1.0f,
    0.0f
);

    prim::drawBox(
        0.72f,
        0.09f,
        0.38f
    );


    glPopMatrix();
}
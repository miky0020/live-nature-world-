// ============================================================================
// src/Interaction.cpp
// ============================================================================
#include "Interaction.h"
#include "Terrain.h"

using util::Vec3;

InteractionSystem::InteractionSystem()
    : m_lakeEnabled(false)
    , m_lakeRadius(0.0f)
    , m_waterLevel(0.0f)
{
}

void InteractionSystem::addZone(const Vec3& position, float radius, const std::string& label) {
    Zone zone;
    zone.position = position;
    zone.radius   = radius;
    zone.label    = label;
    m_zones.push_back(zone);
}

void InteractionSystem::setLakeCheck(float radius, float waterLevel, const std::string& label) {
    m_lakeEnabled = true;
    m_lakeRadius  = radius;
    m_waterLevel  = waterLevel;
    m_lakeLabel   = label;
}

bool InteractionSystem::resolve(const Vec3& playerPosition, const Terrain& terrain,
                                 std::string& outLabel) const {
    bool hasBest = false;
    float bestDist = 0.0f;
    const std::string* bestLabel = 0;

    for (size_t i = 0; i < m_zones.size(); ++i) {
        float dist = util::distanceXZ(playerPosition, m_zones[i].position);
        if (dist <= m_zones[i].radius && (!hasBest || dist < bestDist)) {
            hasBest   = true;
            bestDist  = dist;
            bestLabel = &m_zones[i].label;
        }
    }

    if (m_lakeEnabled) {
        // "Near the lake": standing on ground close to the waterline - not
        // already swimming, not high and dry well inland.
        float ground = terrain.heightAt(playerPosition.x, playerPosition.z);
        float aboveWater = ground - m_waterLevel;
        if (aboveWater >= -0.4f && aboveWater <= m_lakeRadius) {
            float dist = aboveWater < 0.0f ? 0.0f : aboveWater;
            if (!hasBest || dist < bestDist) {
                hasBest   = true;
                bestDist  = dist;
                bestLabel = &m_lakeLabel;
            }
        }
    }

    if (bestLabel) {
        outLabel = *bestLabel;
        return true;
    }
    outLabel.clear();
    return false;
}

// =============================================================================
// PROJECT CHRONO - http://projectchrono.org
//
// Copyright (c) 2014 projectchrono.org
// All right reserved.
//
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file at the top level of the distribution and at
// http://projectchrono.org/license-chrono.txt.
//
// =============================================================================
// Authors: Radu Serban, Justin Madsen
// =============================================================================
//
// HMMWV 9-body vehicle model...
//
// =============================================================================

#include "chrono/assets/ChSphereShape.h"
#include "chrono/assets/ChTriangleMeshShape.h"
#include "chrono/utils/ChUtilsInputOutput.h"

#include "chrono_vehicle/ChVehicleModelData.h"

#include "chrono_models/vehicle/hmmwv/HMMWV_VehicleReduced.h"

namespace chrono {
namespace vehicle {
namespace hmmwv {

// -----------------------------------------------------------------------------
// Static variables
// -----------------------------------------------------------------------------

static const double in2m = 0.0254;
static const double lb2kg = 0.453592;
static const double lbf2N = 4.44822162;

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
HMMWV_VehicleReduced::HMMWV_VehicleReduced(const bool fixed,
                                           DrivelineType driveType,
                                           VisualizationType wheelVis,
                                           ChMaterialSurfaceBase::ContactMethod contactMethod)
    : ChWheeledVehicle(contactMethod), m_driveType(driveType) {
    Create(fixed, wheelVis);
}

HMMWV_VehicleReduced::HMMWV_VehicleReduced(ChSystem* system,
                                           const bool fixed,
                                           DrivelineType driveType,
                                           VisualizationType wheelVis)
    : ChWheeledVehicle(system), m_driveType(driveType) {
    Create(fixed, wheelVis);
}

void HMMWV_VehicleReduced::Create(bool fixed, VisualizationType wheelVis) {
    // -------------------------------------------
    // Create the chassis subsystem
    // -------------------------------------------
    m_chassis = std::make_shared<HMMWV_Chassis>("Chassis");

    // -------------------------------------------
    // Create the suspension subsystems
    // -------------------------------------------
    m_suspensions.resize(2);
    m_suspensions[0] = std::make_shared<HMMWV_DoubleWishboneReducedFront>("FrontSusp");
    m_suspensions[1] = std::make_shared<HMMWV_DoubleWishboneReducedRear>("RearSusp");

    // -----------------------------
    // Create the steering subsystem
    // -----------------------------
    m_steerings.resize(1);
    m_steerings[0] = std::make_shared<HMMWV_RackPinion>("Steering");

    // -----------------
    // Create the wheels
    // -----------------
    m_wheels.resize(4);
    m_wheels[0] = std::make_shared<HMMWV_WheelLeft>(wheelVis);
    m_wheels[1] = std::make_shared<HMMWV_WheelRight>(wheelVis);
    m_wheels[2] = std::make_shared<HMMWV_WheelLeft>(wheelVis);
    m_wheels[3] = std::make_shared<HMMWV_WheelRight>(wheelVis);

    // --------------------
    // Create the driveline
    // --------------------
    switch (m_driveType) {
        case DrivelineType::FWD:
        case DrivelineType::RWD:
            m_driveline = std::make_shared<HMMWV_Driveline2WD>();
            break;
        case DrivelineType::AWD:
            m_driveline = std::make_shared<HMMWV_Driveline4WD>();
            break;
    }

    // -----------------
    // Create the brakes
    // -----------------
    m_brakes.resize(4);
    m_brakes[0] = std::make_shared<HMMWV_BrakeSimple>();
    m_brakes[1] = std::make_shared<HMMWV_BrakeSimple>();
    m_brakes[2] = std::make_shared<HMMWV_BrakeSimple>();
    m_brakes[3] = std::make_shared<HMMWV_BrakeSimple>();
}

HMMWV_VehicleReduced::~HMMWV_VehicleReduced() {}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void HMMWV_VehicleReduced::Initialize(const ChCoordsys<>& chassisPos) {
    m_chassis->Initialize(m_system, chassisPos);

    // Initialize the steering subsystem (specify the steering subsystem's frame
    // relative to the chassis reference frame).
    ChVector<> offset = in2m * ChVector<>(56.735, 0, 3.174);
    m_steerings[0]->Initialize(m_chassis->GetBody(), offset, ChQuaternion<>(1, 0, 0, 0));

    // Initialize the suspension subsystems (specify the suspension subsystems'
    // frames relative to the chassis reference frame).
    m_suspensions[0]->Initialize(m_chassis->GetBody(), in2m * ChVector<>(66.59, 0, 1.039), m_steerings[0]->GetSteeringLink());
    m_suspensions[1]->Initialize(m_chassis->GetBody(), in2m * ChVector<>(-66.4, 0, 1.039), m_chassis->GetBody());

    // Initialize wheels
    m_wheels[0]->Initialize(m_suspensions[0]->GetSpindle(LEFT));
    m_wheels[1]->Initialize(m_suspensions[0]->GetSpindle(RIGHT));
    m_wheels[2]->Initialize(m_suspensions[1]->GetSpindle(LEFT));
    m_wheels[3]->Initialize(m_suspensions[1]->GetSpindle(RIGHT));

    // Initialize the driveline subsystem.
    std::vector<int> driven_susp_indexes(m_driveline->GetNumDrivenAxles());

    switch (m_driveType) {
        case DrivelineType::FWD:
            driven_susp_indexes[0] = 0;
            break;
        case DrivelineType::RWD:
            driven_susp_indexes[0] = 1;
            break;
        case DrivelineType::AWD:
            driven_susp_indexes[0] = 0;
            driven_susp_indexes[1] = 1;
            break;
    }

    m_driveline->Initialize(m_chassis->GetBody(), m_suspensions, driven_susp_indexes);

    // Initialize the four brakes
    m_brakes[0]->Initialize(m_suspensions[0]->GetRevolute(LEFT));
    m_brakes[1]->Initialize(m_suspensions[0]->GetRevolute(RIGHT));
    m_brakes[2]->Initialize(m_suspensions[1]->GetRevolute(LEFT));
    m_brakes[3]->Initialize(m_suspensions[1]->GetRevolute(RIGHT));
}

}  // end namespace hmmwv
}  // end namespace vehicle
}  // end namespace chrono

/* -------------------------------------------------------------------------- *
 * OpenSim Moco: exampleMocoTrack.cpp                                         *
 * -------------------------------------------------------------------------- *
 * Copyright (c) 2019 Stanford University and the Authors                     *
 *                                                                            *
 * Author(s): Nicholas Bianco                                                 *
 *                                                                            *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may    *
 * not use this file except in compliance with the License. You may obtain a  *
 * copy of the License at http://www.apache.org/licenses/LICENSE-2.0          *
 *                                                                            *
 * Unless required by applicable law or agreed to in writing, software        *
 * distributed under the License is distributed on an "AS IS" BASIS,          *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   *
 * See the License for the specific language governing permissions and        *
 * limitations under the License.                                             *
 * -------------------------------------------------------------------------- */

/// This example features two different tracking problems solved using the
/// MocoTrack tool. 
///  - The first problem demonstrates the basic usage of the tool interface
///    to solve a torque-driven marker tracking problem. 
///  - The second problem shows how to customize a muscle-driven state tracking 
///    problem using more advanced features of the tool interface.
/// 
/// Data and model source: https://simtk.org/projects/full_body
/// 
/// Model
/// -----
/// The model described in the file 'subject_walk_armless.osim' included in this
/// file is a modified version of the Rajagopoal et al. 2016 musculoskeletal 
/// model. The lumbar, subtalar, and mtp coordinates have been replaced with
/// WeldJoint%s and residual actuators have been added to the pelvis (1 N-m for
/// rotational coordinates and 10 N for translational coordinates). Finally, the
/// arms and all associated components have been removed for simplicity.
/// 
/// Data
/// ----
/// The coordinate and marker data included in the 'coordinates.sto' and 
/// 'marker_trajectories.trc' files also come from the Rajagopal et al. 2016
/// model distribution. The coordinates were computed using inverse kinematics
/// and modified via the Residual Reduction Algorithm (RRA). 

#include <Moco/osimMoco.h>
#include <Actuators/CoordinateActuator.h>

using namespace OpenSim;

void torqueDrivenMarkerTracking() {

    // Create and name an instance of the MocoTrack tool.
    MocoTrack track;
    track.setName("torque_driven_marker_tracking");

    // Construct a ModelProcessor and add it to the tool. ModelProcessors
    // accept a base model and allow you to easily modify the model by appending
    // ModelOperators. Operations are performed in the order that they are
    // appended to the model. In C++, you may use the pipe operator '|' to 
    // append ModelOperators.
    track.setModel(
            // Create the base Model by passing in the model file.
            ModelProcessor("subject_walk_armless.osim") |
            // Add ground reaction external loads in lieu of a ground-contact
            // model.
            ModOpAddExternalLoads("grf_walk.xml") |
            // Remove all the muscles in the model's ForceSet.
            ModOpRemoveMuscles() |
            // Add CoordinateActuators to the model degrees-of-freedom. This
            // ignores the pelvis coordinates which already have residual 
            // CoordinateActuators.
            ModOpAddReserves(250));

    // Use this convenience function to set the MocoTrack markers reference
    // directly from a TRC file. By default, the markers data is filtered at
    // 6 Hz and if in millimeters, converted to meters.
    track.setMarkersReferenceFromTRC("marker_trajectories.trc");

    // There is marker data in the 'marker_trajectories.trc' associated with
    // model markers that no longer exists (i.e. markers on the arms). Set this
    // flag to avoid an exception from being thrown.
    track.set_allow_unused_references(true);

    // Increase the global marker tracking weight, which is the weight
    // associated with the internal MocoMarkerTrackingGoal term.
    track.set_markers_global_tracking_weight(10);

    // Increase the tracking weights for individual markers in the data set 
    // placed on bony landmarks compared to markers located on soft tissue.
    MocoWeightSet markerWeights;
    markerWeights.cloneAndAppend({"R.ASIS", 20});
    markerWeights.cloneAndAppend({"L.ASIS", 20});
    markerWeights.cloneAndAppend({"R.PSIS", 20});
    markerWeights.cloneAndAppend({"L.PSIS", 20});
    markerWeights.cloneAndAppend({"R.Knee", 10});
    markerWeights.cloneAndAppend({"R.Ankle", 10});
    markerWeights.cloneAndAppend({"R.Heel", 10});
    markerWeights.cloneAndAppend({"R.MT5", 5});
    markerWeights.cloneAndAppend({"R.Toe", 2});
    markerWeights.cloneAndAppend({"L.Knee", 10});
    markerWeights.cloneAndAppend({"L.Ankle", 10});
    markerWeights.cloneAndAppend({"L.Heel", 10});
    markerWeights.cloneAndAppend({"L.MT5", 5});
    markerWeights.cloneAndAppend({"L.Toe", 2});
    track.set_markers_weight_set(markerWeights);

    // Initial time, final time, and mesh interval. The number of mesh points
    // used to discretize the problem is computed internally using these values.
    track.set_initial_time(0.81);
    track.set_final_time(1.65);
    track.set_mesh_interval(0.05);

    // Solve! The boolean argument indicates to visualize the solution.
    MocoSolution solution = track.solve(true);
}

void muscleDrivenStateTracking() {

    // Create and name an instance of the MocoTrack tool.
    MocoTrack track;
    track.setName("muscle_driven_state_tracking");

    // Construct a ModelProcessor and set it on the tool. The default
    // muscles in the model are replaced with optimization-friendly
    // DeGrooteFregly2016Muscles, and adjustments are made to the default muscle
    // parameters.
    ModelProcessor modelProcessor =
            ModelProcessor("subject_walk_armless.osim") |
            ModOpAddExternalLoads("grf_walk.xml") |
            ModOpReplaceMusclesWithDeGrooteFregly2016() |
            // Only valid for DeGrooteFregly2016Muscles.
            ModOpIgnorePassiveFiberForcesDGF() |
            // Only valid for DeGrooteFregly2016Muscles.
            ModOpScaleActiveFiberForceCurveWidthDGF(1.5);
    track.setModel(modelProcessor);

    // Construct a TableProcessor of the coordinate data and pass it to the 
    // tracking tool. TableProcessors can be used in the same way as
    // ModelProcessors by appending TableOperators to modify the base table.
    // A TableProcessor with no operators, as we have here, simply returns the
    // base table.
    track.setStatesReference(TableProcessor("coordinates.sto"));
    track.set_states_global_tracking_weight(10);

    // This setting allows extra data columns contained in the states
    // reference that don't correspond to model coordinates.
    track.set_allow_unused_references(true);

    // Since there is only coordinate position data the states references, this
    // setting is enabled to fill in the missing coordinate speed data using
    // the derivative of splined position data.
    track.set_track_reference_position_derivatives(true);

    // Initial time, final time, and mesh interval.
    track.set_initial_time(0.81);
    track.set_final_time(1.65);
    track.set_mesh_interval(0.08);

    // Instead of calling solve(), call initialize() to receive a pre-configured
    // MocoStudy object based on the settings above. Use this to customize the
    // problem beyond the MocoTrack interface.
    MocoStudy moco = track.initialize();

    // Get a reference to the MocoControlGoal that is added to every MocoTrack
    // problem by default.
    MocoProblem& problem = moco.updProblem();
    MocoControlGoal& effort =
        dynamic_cast<MocoControlGoal&>(problem.updGoal("control_effort"));

    // Put a large weight on the pelvis CoordinateActuators, which act as the
    // residual, or 'hand-of-god', forces which we would like to keep as small
    // as possible.
     Model model = modelProcessor.process();
     for (const auto& coordAct : model.getComponentList<CoordinateActuator>()) {
        auto coordPath = coordAct.getAbsolutePathString();
        if (coordPath.find("pelvis") != std::string::npos) {
            effort.setWeightForControl(coordPath, 10);
        }
    }
    
    // Solve and visualize.
    MocoSolution solution = moco.solve();
    moco.visualize(solution);
}

int main() {

    // Solve the torque-driven marker tracking problem.
    // This problem takes a few minutes to solve.
    torqueDrivenMarkerTracking();

    // Solve the muscle-driven state tracking problem.
    // This problem could take an hour or more to solve, depending on the 
    // number of processor cores available for parallelization. With 12 cores,
    // it takes around 25 minutes.
    muscleDrivenStateTracking();

    return EXIT_SUCCESS;
}

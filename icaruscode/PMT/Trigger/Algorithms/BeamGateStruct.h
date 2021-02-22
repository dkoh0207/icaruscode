/**
 * @file   icaruscode/PMT/Trigger/Algorithms/BeamGateStruct.h
 * @brief  An object representing a time gate, with a start and and end.
 * @author Gianluca Petrillo (petrillo@slac.stanford.edu)
 * @date   September 15, 2020
 */

#ifndef ICARUSCODE_PMT_TRIGGER_ALGORITHMS_BEAMGATESTRUCT_H
#define ICARUSCODE_PMT_TRIGGER_ALGORITHMS_BEAMGATESTRUCT_H


// ICARUS libraries
#include "icaruscode/PMT/Trigger/Algorithms/BeamGateMaker.h"
#include "sbnobj/ICARUS/PMT/Trigger/Data/OpticalTriggerGate.h"

// LArSoft libraries
#include "lardataalg/DetectorInfo/DetectorTimings.h"
#include "lardataalg/DetectorInfo/DetectorTimingTypes.h" // simulation_time, ...
#include "lardataalg/DetectorInfo/DetectorClocks.h"
#include "lardataalg/Utilities/quantities/spacetime.h" // microseconds

// C/C++ standard libraries
#include <utility> // std::pair<>
#include <ostream>


// -----------------------------------------------------------------------------
namespace icarus::trigger {
  struct BeamGateStruct;
  
  /**
   * @brief Creates a `BeamGateStruct` object of specified duration and start.
   * @param detTimings object used for time conversions
   * @param duration length of the gate [&micro;s]
   * @param delay (default: `0_us`) time the gate opens relative to beam gate
   *              opening [&micro;s]
   * @return a `BeamGateStruct`
   * 
   * This is for when I will regret to have coupled `BeamGateStruct` interface
   * to `detinfo::DetectorTimings`.
   */
  BeamGateStruct makeBeamGateStruct(
    detinfo::DetectorTimings const& detTimings,
    util::quantities::intervals::microseconds duration,
    util::quantities::intervals::microseconds delay
      = util::quantities::intervals::microseconds{ 0.0 }
    );
  
} // icarus::trigger


/**
 * @brief Object representing a time gate, with a start and and end.
 * 
 * The object caches the gate in a few formats.
 * 
 */
struct icarus::trigger::BeamGateStruct {
  
  // type aliases
  using microseconds = util::quantities::intervals::microseconds;
  using electronics_time = detinfo::timescales::electronics_time;
  using simulation_time = detinfo::timescales::simulation_time;
  using optical_tick = detinfo::timescales::optical_tick;
  
  /// Utility class expressing a time range.
  template <typename Time>
  struct TimeRange: std::pair<Time, Time> {
    using std::pair<Time, Time>::pair;
    auto start() const { return this->first; }
    auto end() const { return this->second; }
    auto duration() const { return end() - start(); }
  }; // TimeRange
  
  
  /**
   * @brief Constructor: gate of specified duration and start.
   * @param duration length of the gate [&micro;s]
   * @param delay (default: `0_us`) time the gate opens relative to beam gate
   *              opening [&micro;s]
   * @param detTimings object used for time conversions
   */
  BeamGateStruct(
    microseconds duration, microseconds delay,
    detinfo::DetectorTimings const& detTimings
    );
  
  
  // --- BEGIN -- Access -------------------------------------------------------
  /// @name Access
  /// @{
  
  /// Returns the gate as an OpticalTriggerGate (electronics time).
  icarus::trigger::OpticalTriggerGate const& asGate() const { return fGate; }
  
  /// Returns the gate as start/stop pair in simulation time scale.
  TimeRange<simulation_time> const& asSimulationRange() const
    { return fRangeSim; }
  
  /// Returns the gate as start/stop pair in optical ticks.
  TimeRange<optical_tick> const& asOptTickRange() const
    { return fRangeOpt; }
  
  /// Returns the time duration of the gate in the specified time unit.
  template <typename Time = microseconds>
  Time duration() const { return { asSimulationRange().duration() }; }
  
  /// @}
  // --- END -- Access ---------------------------------------------------------
  
  //@{
  /// Comparison operators.
  bool operator== (BeamGateStruct const& other) const
    { return asGate() == other.asGate(); }
  bool operator!= (BeamGateStruct const& other) const
    { return asGate() != other.asGate(); }
  //@}
  
    private:
  // so far, we cache everything (but not the timings helper)
  
  icarus::trigger::OpticalTriggerGate const fGate;
  
  TimeRange<simulation_time> const fRangeSim;
  
  TimeRange<optical_tick> const fRangeOpt;
  
}; // struct icarus::trigger::BeamGateStruct


// -----------------------------------------------------------------------------
// ---  inline implementation
// -----------------------------------------------------------------------------
icarus::trigger::BeamGateStruct::BeamGateStruct(
  microseconds duration, microseconds delay,
  detinfo::DetectorTimings const& detTimings
  )
  : fGate(icarus::trigger::BeamGateMaker{detTimings}(duration, delay))
  , fRangeSim
    {
      detTimings.toSimulationTime(detTimings.BeamGateTime() + delay),
      detTimings.toSimulationTime(detTimings.BeamGateTime() + delay + duration)
    }
  , fRangeOpt
    {
      detTimings.toOpticalTick(detTimings.BeamGateTime() + delay),
      detTimings.toOpticalTick(detTimings.BeamGateTime() + delay + duration)
    }
{} // icarus::trigger::BeamGateStruct::BeamGateStruct()


// -----------------------------------------------------------------------------
namespace icarus::trigger {
  
  template <typename Time>
  std::ostream& operator<< (
    std::ostream& out,
    icarus::trigger::BeamGateStruct::TimeRange<Time> const& range
    )
  {
    out << range.start() << " -- " << range.end()
      << " (duration: " << range.duration() << ")";
    return out;
  } // operator<< (icarus::trigger::BeamGateStruct::TimeRange<>)
  
} // namespace icarus::trigger


// -----------------------------------------------------------------------------
icarus::trigger::BeamGateStruct icarus::trigger::makeBeamGateStruct(
  detinfo::DetectorTimings const& detTimings,
  util::quantities::intervals::microseconds duration,
  util::quantities::intervals::microseconds delay
    /* = util::quantities::microsecond{ 0.0 } */
) {
  
  return { duration, delay, detTimings };
  
} // icarus::trigger::makeBeamGateStruct()


// -----------------------------------------------------------------------------

#endif // ICARUSCODE_PMT_TRIGGER_ALGORITHMS_BEAMGATESTRUCT_H

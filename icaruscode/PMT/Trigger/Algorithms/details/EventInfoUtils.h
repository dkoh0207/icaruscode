/**
 * @file   icaruscode/PMT/Trigger/Algorithms/details/EventInfoUtils.h
 * @brief  Functions dealing with `icarus::trigger::details::EventInfo_t`.
 * @author Gianluca Petrillo (petrillo@slac.stanford.edu)
 * @date   May 15, 2020
 * @see    icaruscode/PMT/Trigger/Algorithms/details/EventInfoUtils.cxx
 */

#ifndef ICARUSCODE_PMT_TRIGGER_ALGORITHM_DETAILS_EVENTINFOUTILS_H
#define ICARUSCODE_PMT_TRIGGER_ALGORITHM_DETAILS_EVENTINFOUTILS_H


// ICARUS libraries
#include "icaruscode/PMT/Trigger/Algorithms/details/EventInfo_t.h"
#include "icaruscode/IcarusObj/SimEnergyDepositSummary.h"

// LArSoft libraries
#include "lardataalg/DetectorInfo/DetectorTimingTypes.h" // simulation_time
#include "lardataalg/Utilities/quantities/energy.h" // gigaelectronvolt, ...
#include "lardataobj/Simulation/SimEnergyDeposit.h"
#include "lardataobj/Simulation/SimPhotons.h"
#include "larcoreobj/SimpleTypesAndConstants/geo_vectors.h" // geo::Point_t
#include "nusimdata/SimulationBase/MCTruth.h"

// framework information
#include "canvas/Utilities/InputTag.h"
#include "messagefacility/MessageLogger/MessageLogger.h" // mf namespace

// C/C++ standard libraries
#include <vector>
#include <string>
#include <variant>
#include <cassert>


//------------------------------------------------------------------------------
//---forward declarations
//---
namespace geo {
  class TPCGeo;
  class GeometryCore;
} // namespace geo

//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
namespace icarus::trigger::details {
  class EventInfoExtractor;
  class EventInfoExtractorMaker;
} // namespace icarus::trigger::details

/**
 * @brief Extracts from `event` the relevant information on the physics event.
 *
 * This returns a `EventInfo_t` object filled as completely as possible.
 * 
 * The `EventInfo_t` is managed so that it contains all the information about
 * the "main" neutrino interaction. All other neutrino interactions are
 * acknowledged but their information is not stored, with minor exceptions.
 * Currently:
 *   
 * * the main interaction is the interaction happening the earliest;
 * * the other interactions have only their location (`Vertices()`) stored in
 *   the list; the order of the vertices in the list stays the same as the order
 *   they are filled, with the exception of the main interaction that is always
 *   moved in `front()` of the list. This means that _interaction vertices are
 *   not ordered by time_ unless that is how they were fed to this object.
 *
 * This class can read any data product in the event, and is completely
 * configured at construction time via constructor arguments.
 * If information is needed from a data product that is not read yet,
 * the following actions are needed:
 *
 * 1. decide the input tag of the data product(s) to be read; this is usually
 *    delegated to the user via a configuration parameter;
 * 2. the required input tags need to be stored into data members;
 * 3. _art_ needs to be informed of the intention of reading the data products
 *    via `consumes()` or equivalent calls; this class can take care of that,
 *    if a `art::ConsumesCollector` object is provided;
 * 4. the data product can be read with the usual means.
 *
 */
class icarus::trigger::details::EventInfoExtractor {
  
    public:
  
  using simulation_time = detinfo::timescales::simulation_time;
  using TimeSpan_t = std::pair<simulation_time, simulation_time>;
  
  
  /// Utility tag to identify a parameter as for SimEnergyDepositSummary tag.
  struct SimEnergyDepositSummaryInputTag {
    
    explicit SimEnergyDepositSummaryInputTag(art::InputTag tag)
      : fTag(std::move(tag)) {}
    
    art::InputTag const& tag() const { return fTag; }
    explicit operator art::InputTag const& () const { return tag(); }
    
      private:
    art::InputTag fTag;
  }; // SimEnergyDepositSummaryInputTag
  
  /// Type of flexible energy deposition data specification.
  using EdepTags_t = std::variant<
    std::vector<art::InputTag>, // LArSoft energy deposition collections
    SimEnergyDepositSummaryInputTag // ICARUS energy deposition summary
    >;

  
  /**
    * @name Constructors
    * 
    * Constructors are available to take advantage of specific _art_ features,
    * and for the generic _art_/_gallery_ compatible interfaces.
    */
  /// @{
  
  /**
   * @brief Constructor: configures the object.
   * @param truthTags list of truth information data products to be read
   * @param particleTag particle list data products to be read (empty for none)
   * @param edepTags list of energy deposition data products to be read
   * @param inSpillTimes start and end of spill, in simulation time
   * @param inPreSpillTimes start and end of pre-spill, in simulation time
   * @param geom LArSoft geometry service provider
   * @param logCategory name of message facility stream to sent messages to
   * @see `EventInfoExtractor(std::vector<art::InputTag> const&, ConsumesColl&)`
   * 
   * In _art_ framework context, prefer the 
   * `EventInfoExtractor(std::vector<art::InputTag> const&, ConsumesColl&)`
   * constructor.
   * 
   * Two formats of energy depositions are supported:
   * * standard LArSoft (`std::vector<sim::SimEnergyDeposit>`): specify a vector
   *   of input tags in `edepTags` argument;
   * * ICARUS deposited energy summary: specify an input tag converted into a
   *   `SimEnergyDepositSummaryInputTag` parameter in `edepTags` argument.
   */
  EventInfoExtractor(
    std::vector<art::InputTag> truthTags,
    art::InputTag particleTag,
    EdepTags_t edepTags,
    TimeSpan_t inSpillTimes,
    TimeSpan_t inPreSpillTimes,
    geo::GeometryCore const& geom,
    std::string logCategory = "EventInfoExtractor"
    );
  
  /**
   * @brief Constructor: configures, and declares consuming data product.
   * @tparam ConsumesColl type with `art::ConsumesCollector` interface
   * @param truthTags list of truth information data products to be read
   * @param particleTag particle list data products to be read (empty for none)
   * @param edepTags list of energy deposition data products to be read
   * @param inSpillTimes start and end of spill, in simulation time
   * @param inPreSpillTimes start and end of pre-spill, in simulation time
   * @param geom LArSoft geometry service provider
   * @param logCategory name of message facility stream to sent messages to
   * @param consumesCollector object to declare the consumed products to
   * 
   * Typical usage is within a _art_ module constructor:
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
   * icarus::trigger::details::EventInfoExtractor const eventInfoFrom {
   *   { art:InputTag{ "generator" } },
   *   consumesCollector()
   *   };
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   * 
   */
  template <typename ConsumesColl>
  EventInfoExtractor(
    std::vector<art::InputTag> truthTags,
    art::InputTag particleTag,
    EdepTags_t edepTags,
    TimeSpan_t inSpillTimes,
    TimeSpan_t inPreSpillTimes,
    geo::GeometryCore const& geom,
    std::string logCategory,
    ConsumesColl& consumesCollector
    );
  
  /// @}
  
  
  // @{
  /**
    * @brief Returns an `EventInfo_t` object with information from `event`.
    * @tparam Event type of event to read data from (_art_ or _gallery_ event)
    * @param event event record to read the information from
    * @return a `EventInfo_t` with information extracted from `event`
    * 
    */
  template <typename Event>
  EventInfo_t extractInfo(Event const& event) const;
  
  template <typename Event>
  EventInfo_t operator() (Event const& event) const
    { return extractInfo(event); }
    
  // @}
  
  /// Returns whether we are extracting any generator information.
  bool hasGenerated() const { return !fGeneratorTags.empty(); }
  
  /// Returns whether we are extracting any information about particles in LAr.
  bool hasParticles() const { return !fParticleTag.empty(); }
  
  /// Returns whether we are extracting any energy deposition information.
  bool hasEDep() const { return isEDepSpecified(fEnergyDepositTags); }
  
  
  // --- BEGIN -- EdepTag protocol ---------------------------------------------
  
  /// Returns whether `edepTags` contains a energy deposition summary tag.
  static bool isEDepSummaryTag(EdepTags_t const& edepTags);
  
  /// Returns whether `edepTags` contains energy deposition list tags.
  static bool isEDepListTag(EdepTags_t const& edepTags);

  /// Returns whether `edepTags` contains any energy deposition tag.
  static bool isEDepSpecified(EdepTags_t const& edepTags)
    { return isEDepSummaryTag(edepTags) || isEDepListTag(edepTags); }
  
  // --- END -- EdepTag protocol -----------------------------------------------
  
    private:
  
  friend EventInfoExtractorMaker;
  
  // --- BEGIN -- Configuration variables --------------------------------------
  
  /// List of truth data product tags (`std::vector<simb::MCTruth>`).
  std::vector<art::InputTag> const fGeneratorTags;
  
  /// Tag of the particle data product (`std::vector<simb::MCParticle>`).
  art::InputTag const fParticleTag;
  
  /// Input tag for energy deposition product(s); may be a summary or a list
  /// of LArSoft data products.
  EdepTags_t fEnergyDepositTags;
  
  std::string const fLogCategory; ///< Stream name for message facility.

  // --- END -- Configuration variables ----------------------------------------
  
  
  // --- BEGIN -- Set up  ------------------------------------------------------
  
  geo::GeometryCore const& fGeom; ///< Geometry service provider.

  
  TimeSpan_t const fInSpillTimes; ///< Start and stop time for "in spill" label.
  
  /// Start and stop time for "pre-spill" label.
  TimeSpan_t const fInPreSpillTimes;
  
  // --- END -- Set up  --------------------------------------------------------
  
  /// Returns whether we use energy summary instead of full energy deposits.
  bool useEnergyDepositSummary() const
    { return isEDepSummaryTag(fEnergyDepositTags); }
  
  /// Fills `info` record with generation information from `truth`.
  void fillGeneratorInfo(
    EventInfo_t& info,
    simb::MCTruth const& truth,
    std::vector<simb::MCParticle> const* particles
    ) const;
  
  /**
   * @brief  Fills `info` record with generated neutrino information from
   *         `truth`.
   * 
   * If `info` record already contains an interaction (the "main" interaction),
   * the interaction in `truth` becomes the new main interaction of `info`
   * only if it is earlier than the old main interaction.
   * Otherwise the record is updated to take note that there was yet another
   * interaction in the event, with little-to-no information about it stored.
   */
  void fillGeneratorNeutrinoInfo
    (EventInfo_t& info, simb::MCTruth const& truth) const;
  void fillGeneratorCosmicInfo(
    EventInfo_t& info,
    simb::MCTruth const& truth,
    std::vector<simb::MCParticle> const* particles
    ) const;
  /// Extracts information from `truth` and sets it as the "main" interaction
  /// information. Previous information is typically overwritten.
  void setMainGeneratorNeutrinoInfo
    (EventInfo_t& info, simb::MCTruth const& truth) const;
  void setMainGeneratorCosmicInfo(
    EventInfo_t& info,
    simb::MCTruth const& truth,
    std::vector<simb::MCParticle> const* particles
    ) const;

  /// Adds selected information from the interaction in `truth` to `info`
  /// record.
  void addGeneratorNeutrinoInfo
    (EventInfo_t& info, simb::MCTruth const& truth) const;
  //void addGeneratorCosmicInfo
    //(EventInfo_t& info, simb::MCTruth const& truth) const;
 
  /// Adds the energy depositions from `energyDeposits` into `info` record.
  void addEnergyDepositionInfo(
    EventInfo_t& info, std::vector<sim::SimEnergyDeposit> const& energyDeposits
    ) const;
  bool interceptsTPCActiveVolume(const TLorentzVector& starPos, const TLorentzVector& endPos) const;
  /// Returns in which TPC volume `point` falls in (`nullptr` if none).
  geo::TPCGeo const* pointInTPC(geo::Point_t const& point) const;

  /// Returns in which active TPC volume `point` falls in (`nullptr` if none).
  geo::TPCGeo const* pointInActiveTPC(geo::Point_t const& point) const;
  
  /// Returns the time of the neutrino interaction in `truth`.
  ///
  /// Undefined behaviour if the truth information does not describe a neutrino
  /// interaction.
  simulation_time getInteractionTime(simb::MCTruth const& truth) const;
  
  
  /// Declares all the consumables to the collector.
  template <typename ConsumesColl>
  static void declareConsumables(
    ConsumesColl& consumesCollector,
    std::vector<art::InputTag> const& truthTags,
    art::InputTag const& particleTag,
    EdepTags_t const& edepTags
    );

}; // class icarus::trigger::details::EventInfoExtractor


// -----------------------------------------------------------------------------
/// A helper class creating a `EventInfoExtractor` with a specific setup.
class icarus::trigger::details::EventInfoExtractorMaker {
  
    public:
  
  using TimeSpan_t = EventInfoExtractor::TimeSpan_t;
  
  /// Constructor: stores parameters for construction of `EventInfoExtractor`.
  EventInfoExtractorMaker(
    std::vector<art::InputTag> truthTags,
    art::InputTag particleTag,
    EventInfoExtractor::EdepTags_t edepTags,
    geo::GeometryCore const& geom,
    std::string logCategory
    );
  
  /// Constructor: stores parameters for construction of `EventInfoExtractor`
  /// and declares consumables.
  template <typename ConsumesColl>
  EventInfoExtractorMaker(
    std::vector<art::InputTag> truthTags,
    art::InputTag particleTag,
    EventInfoExtractor::EdepTags_t edepTags,
    geo::GeometryCore const& geom,
    std::string logCategory,
    ConsumesColl& consumesCollector
    );
  
  
  //@{
  /// Creates and returns a new `EventInfoExtractor` object.
  EventInfoExtractor make
    (TimeSpan_t inSpillTimes, TimeSpan_t inPreSpillTimes) const;
  
  EventInfoExtractor operator()
    (TimeSpan_t inSpillTimes, TimeSpan_t inPreSpillTimes) const
    { return make(inSpillTimes, inPreSpillTimes); }
  
  //@}
  
  /// Returns whether we are extracting any generator information.
  bool hasGenerated() const { return !fGeneratorTags.empty(); }
  
  /// Returns whether we are extracting any information about particles in LAr.
  bool hasParticles() const { return !fParticleTag.empty(); }
  
  /// Returns whether we are extracting any energy deposition information.
  bool hasEDep() const
    { return EventInfoExtractor::isEDepSpecified(fEnergyDepositTags); }
  
  
    private:
  
  std::vector<art::InputTag> const fGeneratorTags;
  art::InputTag const fParticleTag;
  EventInfoExtractor::EdepTags_t const fEnergyDepositTags;
  std::string const fLogCategory;
  geo::GeometryCore const& fGeom;
  
}; // class icarus::trigger::details::EventInfoExtractorMaker


// -----------------------------------------------------------------------------
// ---  inline implementation
// -----------------------------------------------------------------------------
// --- icarus::trigger::details::EventInfoExtractor
// -----------------------------------------------------------------------------
bool icarus::trigger::details::EventInfoExtractor::isEDepSummaryTag
  (EdepTags_t const& edepTags)
  { return std::holds_alternative<SimEnergyDepositSummaryInputTag>(edepTags); }


// -----------------------------------------------------------------------------
bool icarus::trigger::details::EventInfoExtractor::isEDepListTag
  (EdepTags_t const& edepTags)
{
  auto* edeplist = std::get_if<std::vector<art::InputTag>>(&edepTags);
  return edeplist && !edeplist->empty();
} // icarus::trigger::details::EventInfoExtractor::isEDepListTag()


// -----------------------------------------------------------------------------
// ---  template implementation
// -----------------------------------------------------------------------------
// --- icarus::trigger::details::EventInfoExtractor
// -----------------------------------------------------------------------------
template <typename ConsumesColl>
icarus::trigger::details::EventInfoExtractor::EventInfoExtractor(
  std::vector<art::InputTag> truthTags,
  art::InputTag particleTag,
  EdepTags_t edepTags,
  TimeSpan_t inSpillTimes,
  TimeSpan_t inPreSpillTimes,
  geo::GeometryCore const& geom,
  std::string logCategory,
  ConsumesColl& consumesCollector
  )
  : EventInfoExtractor{
      std::move(truthTags), std::move(particleTag), std::move(edepTags),
      inSpillTimes, inPreSpillTimes, geom, std::move(logCategory)
    }
{
  declareConsumables
    (consumesCollector, fGeneratorTags, fParticleTag, fEnergyDepositTags);
} // icarus::trigger::details::EventInfoExtractor::EventInfoExtractor(coll)


// -----------------------------------------------------------------------------
template <typename Event>
auto icarus::trigger::details::EventInfoExtractor::extractInfo
  (Event const& event) const -> EventInfo_t
{
  
  EventInfo_t info;
  
  auto const* particles
    = fParticleTag.empty()
    ? nullptr
    : event.template getPointerByLabel<std::vector<simb::MCParticle>>(fParticleTag)
    ; 

  //
  // generator information
  //
  for (art::InputTag const& inputTag: fGeneratorTags) {
  
    auto const& truthRecords
      = event.template getByLabel<std::vector<simb::MCTruth>>(inputTag);
    
    for (simb::MCTruth const& truth: truthRecords) {
      
      fillGeneratorInfo(info, truth, particles);
      
    } // for truth records
    
  } // for generators
  
  //
  // propagation in the detector
  //
  
  if (auto* summaryTag
      = std::get_if<SimEnergyDepositSummaryInputTag>(&fEnergyDepositTags);
    summaryTag
  ) {
    
    using GeV = util::quantities::gigaelectronvolt;
    
    auto const& energyDeposits = event
      .template getByLabel<icarus::SimEnergyDepositSummary>(summaryTag->tag());
    
    info.SetDepositedEnergy                        (GeV(energyDeposits.Total));
    info.SetDepositedEnergyInSpill                 (GeV(energyDeposits.Spill));
    info.SetDepositedEnergyInPreSpill              (GeV(energyDeposits.PreSpill));
    info.SetDepositedEnergyInActiveVolume          (GeV(energyDeposits.Active));
    info.SetDepositedEnergyInSpillInActiveVolume   (GeV(energyDeposits.SpillActive));
    info.SetDepositedEnergyInPreSpillInActiveVolume(GeV(energyDeposits.PreSpillActive));
    
  }
  else if (
    auto* edepListTag
      = std::get_if<std::vector<art::InputTag>>(&fEnergyDepositTags);
    edepListTag
  ) {
    
    for (art::InputTag const& edepTag: *edepListTag) {
      
      auto const& energyDeposits
        = event.template getByLabel<std::vector<sim::SimEnergyDeposit>>(edepTag);
      mf::LogTrace(fLogCategory)
        << "Event " << event.id() << " has " << energyDeposits.size()
        << " energy deposits recorded in '" << edepTag.encode() << "'";
      
      addEnergyDepositionInfo(info, energyDeposits);
      
    } // for all energy deposit tags
    
  }
  else {
    throw std::logic_error(
      "icarus::trigger::details::EventInfoExtractor::EventInfoExtractor(): "
      "unexpected type from EdepTags_t"
      );
  }
  
  mf::LogTrace(fLogCategory) << "Event " << event.id() << ": " << info;
  
  return info;
} // icarus::trigger::details::EventInfoExtractor::extractInfo()


// -----------------------------------------------------------------------------
template <typename ConsumesColl>
void icarus::trigger::details::EventInfoExtractor::declareConsumables(
  ConsumesColl& consumesCollector,
  std::vector<art::InputTag> const& truthTags,
  art::InputTag const& particleTag,
  EdepTags_t const& edepTags
  )
{
  
  for (art::InputTag const& inputTag: truthTags)
    consumesCollector.template consumes<std::vector<simb::MCTruth>>(inputTag);
  
  if (
    auto* summaryTag = std::get_if<SimEnergyDepositSummaryInputTag>(&edepTags);
    summaryTag
  ) {
    
    consumesCollector.template consumes<icarus::SimEnergyDepositSummary>
      (static_cast<art::InputTag const&>(*summaryTag));
    
  }
  else if (
    auto* edepListTag = std::get_if<std::vector<art::InputTag>>(&edepTags);
    edepListTag
  ) {

    for (art::InputTag const& inputTag: *edepListTag) {
      consumesCollector.template consumes<std::vector<sim::SimEnergyDeposit>>
        (inputTag);
    }
  }
  
  if (!particleTag.empty()) {
    consumesCollector
      .template consumes<std::vector<simb::MCParticle>>(particleTag);
    consumesCollector
      .template consumes<std::vector<sim::SimPhotons>>(particleTag);
  }
  
} // icarus::trigger::details::EventInfoExtractor::declareConsumables()


//------------------------------------------------------------------------------
//--- icarus::trigger::details::EventInfoExtractorMaker
//------------------------------------------------------------------------------
template <typename ConsumesColl>
icarus::trigger::details::EventInfoExtractorMaker::EventInfoExtractorMaker(
  std::vector<art::InputTag> truthTags,
  art::InputTag particleTag,
  EventInfoExtractor::EdepTags_t edepTags,
  geo::GeometryCore const& geom,
  std::string logCategory,
  ConsumesColl& consumesCollector
  )
  : EventInfoExtractorMaker(
      std::move(truthTags), std::move(particleTag), std::move(edepTags),
      geom,
      std::move(logCategory)
    )
{
  EventInfoExtractor::declareConsumables
    (consumesCollector, fGeneratorTags, fParticleTag, fEnergyDepositTags);
} // icarus::trigger::details::EventInfoExtractorMaker::EventInfoExtractorMaker()


//------------------------------------------------------------------------------


#endif // ICARUSCODE_PMT_TRIGGER_ALGORITHM_DETAILS_EVENTINFOUTILS_H

////////////////////////////////////////////////////////////////////////
// Class:       photonfilter
// Module Type: filter
// File:        photonfilter_module.cc
//
// Generated at Fri Sep  6 04:41:23 2019 by Dominic Barker using artmod
// from cetpkgsupport v1_14_01.
////////////////////////////////////////////////////////////////////////

#include "art/Framework/Core/EDFilter.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Run.h"
#include "art/Framework/Principal/SubRun.h"
#include "fhiclcpp/ParameterSet.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

#include "larsim/MCCheater/ParticleInventoryService.h"
#include "nusimdata/SimulationBase/MCTruth.h"
#include "canvas/Persistency/Common/Ptr.h"
#include "art/Framework/Services/Registry/ServiceHandle.h"

#include "lardata/DetectorInfoServices/DetectorClocksService.h"

#include <memory>

namespace shower {
  class photonfilter;
}

class shower::photonfilter : public art::EDFilter {
public:
  explicit photonfilter(fhicl::ParameterSet const & p);
  // The destructor generated by the compiler is fine for classes
  // without bare pointers or other resource use.

  // Plugins should not be copied or assigned.
  photonfilter(photonfilter const &) = delete;
  photonfilter(photonfilter &&) = delete;
  photonfilter & operator = (photonfilter const &) = delete;
  photonfilter & operator = (photonfilter &&) = delete;

  // Required functions.
  bool filter(art::Event & e) override;


private:

  // Declare member data here.
  std::string fGenieGenModuleLabel;

  bool isFromNuVertex(const art::Ptr<simb::MCTruth>& mc, const simb::MCParticle* &particle, float distance=5) const;


};


shower::photonfilter::photonfilter(fhicl::ParameterSet const & pset): art::EDFilter{pset}
  // Initialize member data here.
{
  fGenieGenModuleLabel = pset.get<std::string>("GenieGenModuleLabel");

  // Call appropriate produces<>() functions here.
}

bool shower::photonfilter::filter(art::Event & evt)
{
  const art::ServiceHandle<cheat::ParticleInventoryService> particleInventory;
  
  // * MC truth information
  art::Handle<std::vector<simb::MCTruth> > mctruthListHandle;
  std::vector<art::Ptr<simb::MCTruth> > mclist;
  if (evt.getByLabel(fGenieGenModuleLabel,mctruthListHandle))
    art::fill_ptr_vector(mclist, mctruthListHandle);
  
  int photons = 0;

  //Loop over the neutrinos 
  for(auto const& mc: mclist){

    //List the particles in the event
    const sim::ParticleList& particles = particleInventory->ParticleList();
    
    for (sim::ParticleList::const_iterator particleIt = particles.begin(); particleIt != particles.end(); ++particleIt){
      const simb::MCParticle *particle = particleIt->second;
      
      //Only keep photons
      if(particle->PdgCode() != 22){continue;}

      //Do we have any photons from the vertex 
      if(!isFromNuVertex(mc,particle)){continue;}

      //Is it above 50 MeV. Remove any delta rays.
      if(particle->E()*1000 < 50){continue;}

      ++photons;

    }
  }
  
  //Only keep photon events
  if(photons > 0){
    std::cout << "filter photon" << std::endl;
    return true;}

  std::cout << "no photon to filter" << std::endl;
  return false;

}
		      

bool shower::photonfilter::isFromNuVertex(const art::Ptr<simb::MCTruth>& mc, const simb::MCParticle* &particle, float distance) const{
  
  const TLorentzVector nuVtx     = mc->GetNeutrino().Nu().Trajectory().Position(0);
  const TLorentzVector partstart = particle->Position();
  return TMath::Abs((partstart - nuVtx).Mag()) < distance;
}



DEFINE_ART_MODULE(shower::photonfilter)

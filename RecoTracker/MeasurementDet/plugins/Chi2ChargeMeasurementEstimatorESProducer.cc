/** \class Chi2ChargeMeasurementEstimatorESProducer
 *  ESProducer for Chi2ChargeMeasurementEstimator.
 *
 *  \author speer
 */



#include "DataFormats/SiStripCluster/interface/SiStripClusterTools.h"
#include "DataFormats/SiStripCluster/interface/SiStripCluster.h"
#include "TrackingTools/TrajectoryState/interface/TrajectoryStateOnSurface.h"

#include "RecoTracker/MeasurementDet/interface/ClusterFilterPayload.h"

#include<limits>

#include "TrackingTools/KalmanUpdators/interface/Chi2MeasurementEstimator.h"

#include "RecoLocalTracker/SiStripClusterizer/interface/ClusterChargeCut.h"


namespace {

class Chi2ChargeMeasurementEstimator final : public Chi2MeasurementEstimator {
public:

  /** Construct with cuts on chi2 and nSigma.
   *  The cut on Chi2 is used to define the acceptance of RecHits.
   *  The errors of the trajectory state are multiplied by nSigma 
   *  to define acceptance of Plane and maximalLocalDisplacement.
   */
  explicit Chi2ChargeMeasurementEstimator(double maxChi2, double nSigma,
        float maxSag, float minToll,
	float minGoodPixelCharge, float minGoodStripCharge,
	float pTChargeCutThreshold) : 
    Chi2MeasurementEstimator( maxChi2, nSigma, maxSag, minToll),
    minGoodPixelCharge_(minGoodPixelCharge),
    minGoodStripCharge_(minGoodStripCharge) {
      if (pTChargeCutThreshold>=0.) pTChargeCutThreshold2_=pTChargeCutThreshold*pTChargeCutThreshold;
      else pTChargeCutThreshold2_=std::numeric_limits<float>::max();
    }


  bool preFilter(const TrajectoryStateOnSurface& ts,
                 const MeasurementEstimator::OpaquePayload  & opay) const override;


  virtual Chi2ChargeMeasurementEstimator* clone() const {
    return new Chi2ChargeMeasurementEstimator(*this);
  }
private:

  float minGoodPixelCharge_; 
  float minGoodStripCharge_;
  float pTChargeCutThreshold2_;

  bool checkClusterCharge(DetId id, SiStripCluster const & cluster, const TrajectoryStateOnSurface& ts) const {
    return siStripClusterTools::chargePerCM(id, cluster, ts.localParameters() ) >  minGoodStripCharge_;

  }


};

bool Chi2ChargeMeasurementEstimator::preFilter(const TrajectoryStateOnSurface& ts,
                                               const MeasurementEstimator::OpaquePayload  & opay) const {

  // what we got?
  if (opay.tag != ClusterFilterPayload::myTag) return true;  // not mine...
  
  auto const & clf = reinterpret_cast<ClusterFilterPayload const &>(opay);

  if (ts.globalMomentum().perp2()>pTChargeCutThreshold2_) return true;

  DetId detid = clf.detId;
  uint32_t subdet = detid.subdetId();

  if (subdet>2) {
    return checkClusterCharge(detid, *clf.cluster[0],ts) && ( nullptr==clf.cluster[1] || checkClusterCharge(detid, *clf.cluster[1],ts) ) ; 

  }

  /*  pixel charge not implemented as not used...
     auto const & thit = static_cast<const SiPixelRecHit &>(hit);
     thit.cluster()->charge() ...

  */

  return true;
}

}



#include "FWCore/Framework/interface/ESProducer.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "TrackingTools/Records/interface/TrackingComponentsRecord.h"
#include <boost/shared_ptr.hpp>

namespace {


class  Chi2ChargeMeasurementEstimatorESProducer: public edm::ESProducer{
 public:
  // static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);
  Chi2ChargeMeasurementEstimatorESProducer(const edm::ParameterSet & p);
  virtual ~Chi2ChargeMeasurementEstimatorESProducer(); 
  boost::shared_ptr<Chi2MeasurementEstimatorBase> produce(const TrackingComponentsRecord &);
 private:
  boost::shared_ptr<Chi2MeasurementEstimatorBase> m_estimator;
  const edm::ParameterSet m_pset;
};

/*  need support for RefToPSet
void
Chi2ChargeMeasurementEstimatorESProducer::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
  edm::ParameterSetDescription desc;
  desc.add<std::string>("ComponentName","");
  desc.add<double>("MaxChi2",30);
  desc.add<double>("nSigma",3);
  desc.add<double>("pTChargeCutThreshold",-1.);
  edm::ParameterSetDescription descCCC;
  descCCC.add<what>("",...);
  desc.add<edm::ParameterSetDescription>("clusterChargeCut", descCCC);
  descriptions.add("Chi2ChargeMeasurementEstimator", desc);
}
*/


Chi2ChargeMeasurementEstimatorESProducer::Chi2ChargeMeasurementEstimatorESProducer(const edm::ParameterSet & pset) :
  m_pset(pset)
{
  std::string const & myname = pset.getParameter<std::string>("ComponentName");
  setWhatProduced(this,myname);
}

Chi2ChargeMeasurementEstimatorESProducer::~Chi2ChargeMeasurementEstimatorESProducer() {}

boost::shared_ptr<Chi2MeasurementEstimatorBase> 
Chi2ChargeMeasurementEstimatorESProducer::produce(const TrackingComponentsRecord & iRecord){ 

  auto maxChi2             =  m_pset.getParameter<double>("MaxChi2");
  auto nSigma              =  m_pset.getParameter<double>("nSigma");
  auto maxSag  = m_pset.existsAs<double>("MaxSagitta") ? m_pset.getParameter<double>("MaxSagitta") : -1.;
  auto minTol = m_pset.existsAs<double>("MinimalTolerance") ?  m_pset.getParameter<double>("MinimalTolerance") : 10;
  auto minGoodPixelCharge  = 0;
  auto minGoodStripCharge  =  clusterChargeCut(m_pset);
  auto pTChargeCutThreshold=   m_pset.getParameter<double>("pTChargeCutThreshold");

  m_estimator = boost::shared_ptr<Chi2MeasurementEstimatorBase>(
	new Chi2ChargeMeasurementEstimator(maxChi2,nSigma, maxSag, minTol, 
		minGoodPixelCharge, minGoodStripCharge, pTChargeCutThreshold));
  return m_estimator;
}

}


#include "FWCore/Framework/interface/ModuleFactory.h"
DEFINE_FWK_EVENTSETUP_MODULE(Chi2ChargeMeasurementEstimatorESProducer);


#ifndef MUSE_ARMCL_PREDICTION_MODEL_HPP
#define MUSE_ARMCL_PREDICTION_MODEL_HPP

#include <muse_smc/prediction/prediction_model.hpp>
#include <muse_armcl/state_space/state_space_description.hpp>

#include <cslibs_plugins/plugin.hpp>
#include <cslibs_plugins_data/data.hpp>
#include <ros/ros.h>
namespace muse_armcl {
class EIGEN_ALIGN16 PredictionModel : public muse_smc::PredictionModel<StateSpaceDescription, cslibs_plugins_data::Data>,
                        public cslibs_plugins::Plugin
{
public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
    using allocator_t = Eigen::aligned_allocator<PredictionModel>;
    using Ptr = std::shared_ptr<PredictionModel>;

    inline const static std::string Type()
    {
        return "muse_armcl::PredictionModel";
    }

    virtual void setup(ros::NodeHandle &nh) = 0;
};
}

#endif // MUSE_ARMCL_PREDICTION_MODEL_HPP

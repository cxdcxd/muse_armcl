#ifndef MUSE_ARMCL_UPDATE_MODEL_HPP
#define MUSE_ARMCL_UPDATE_MODEL_HPP

#include <muse_smc/update/update_model.hpp>
#include <muse_armcl/state_space/state_space_description.hpp>

#include <cslibs_plugins/plugin.hpp>
#include <cslibs_plugins_data/data.hpp>

namespace muse_armcl {
class UpdateModel : public muse_smc::UpdateModel<StateSpaceDescription, cslibs_plugins_data::Data>,
                    public cslibs_plugins::Plugin
{
public:
    using Ptr = std::shared_ptr<UpdateModel>;

    inline const static std::string Type()
    {
        return "muse_armcl::UpdateModel";
    }

    virtual inline std::size_t getId() const override
    {
        return cslibs_plugins::Plugin::getId();
    }

    virtual inline const std::string getName() const override
    {
        return cslibs_plugins::Plugin::getName();
    }

    virtual void setup(ros::NodeHandle &nh) = 0;
};
}

#endif // MUSE_ARMCL_UPDATE_MODEL_HPP

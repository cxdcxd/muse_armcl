#include <muse_armcl/sampling/uniform_sampling.hpp>

#include <cslibs_mesh_map/random_walk.hpp>
#include <cslibs_math/sampling/uniform.hpp>

namespace muse_armcl {
class EIGEN_ALIGN16 UniformAllLinks : public UniformSampling
{
public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
    using allocator_t = Eigen::aligned_allocator<UniformAllLinks>;
    using rng_t  = cslibs_math::random::Uniform<double,1>;

    virtual bool update(const std::string &frame) override
    {
        return true;
    }

    virtual void apply(sample_t &sample) override
    {
        /// get map
        const muse_smc::StateSpace<StateSpaceDescription>::ConstPtr ss = map_provider_->getStateSpace();
        if (!ss->isType<MeshMap>())
            return;

        using mesh_map_tree_t = cslibs_mesh_map::MeshMapTree;
        using mesh_map_tree_node_t = cslibs_mesh_map::MeshMapTreeNode;
        const mesh_map_tree_t *map = ss->as<MeshMap>().data();
        std::vector<std::string> frame_ids;
        map->getFrameIds(frame_ids);

        /// initialize random link generator
        if (!rng_link_) {
            if (random_seed_ >= 0)
                rng_link_.reset(new rng_t(0.0, frame_ids.size(), random_seed_));
            else
                rng_link_.reset(new rng_t(0.0, frame_ids.size(), random_seed_));
        }

        /// random link
        std::size_t link_i = std::min(frame_ids.size()-1, static_cast<std::size_t>(rng_link_->get()));
        const mesh_map_tree_node_t* link = map->getNode(frame_ids[link_i]);
        if (!link)
            throw std::runtime_error("[UniformSampler]: Link " + frame_ids[link_i] + " not found!");

        /// draw random element
        using state_t = StateSpaceDescription::state_t;
        std::vector<state_t> particles = random_walk_.createParticleSetForOneMap(1, *link);
        sample = sample_t(particles[0], 0.0);
    }

private:
    int                         random_seed_;
    MeshMapProvider::Ptr        map_provider_;
    cslibs_mesh_map::RandomWalk random_walk_;
    rng_t::Ptr                  rng_link_;

    virtual bool apply(sample_set_t &sample_set) override
    {
        sample_set_t::sample_insertion_t insertion = sample_set.getInsertion();
        const double weight = 1.0 / static_cast<double>(sample_size_);

        const muse_smc::StateSpace<StateSpaceDescription>::ConstPtr ss = map_provider_->getStateSpace();
        if (!ss->isType<MeshMap>())
            return false;

        using mesh_map_tree_t = cslibs_mesh_map::MeshMapTree;
        using mesh_map_tree_node_t = cslibs_mesh_map::MeshMapTreeNode;
        const mesh_map_tree_t *map = ss->as<MeshMap>().data();
        std::vector<std::string> frame_ids;
        map->getFrameIds(frame_ids);

        /// estimate total edges length
        double total_edges_length = 0.0;
        for(const mesh_map_tree_node_t::Ptr& link : *map){

            total_edges_length += link->map.sumEdgeLength();
        }

        /// uniform over all links
        for(const mesh_map_tree_node_t::Ptr& link : *map){

            /// estimate number of particles per link, depends on sum edge length on this link
            const double ratio = link->map.sumEdgeLength() / total_edges_length;
            const std::size_t particles_per_frame = static_cast<std::size_t>(
                        std::round(static_cast<double>(sample_size_) * ratio));

            /// draw random elements
            using state_t = StateSpaceDescription::state_t;
            std::vector<state_t> particles = random_walk_.createParticleSetForOneMap(particles_per_frame, *link);
            for (state_t &p : particles)
                if (insertion.canInsert())
                    insertion.insert(sample_t(p, weight));
        }
        return true;
    }

    using map_provider_map_t = std::map<std::string, MeshMapProvider::Ptr>;
    virtual void doSetup(const map_provider_map_t &map_providers,
                         ros::NodeHandle &nh) override
    {
        auto param_name = [this](const std::string &name){return name_ + "/" + name;};
        random_seed_ = nh.param(param_name("seed"), -1);

        const std::string map_provider_id = nh.param<std::string>("map", ""); /// toplevel parameter
        if (map_provider_id == "")
            throw std::runtime_error("[UniformSampling]: No map provider was found!");

        if (map_providers.find(map_provider_id) == map_providers.end())
            throw std::runtime_error("[UniformSampling]: Cannot find map provider '" + map_provider_id + "'!");

        map_provider_ = map_providers.at(map_provider_id);
    }
};
}

#include <class_loader/class_loader_register_macro.h>
CLASS_LOADER_REGISTER_CLASS(muse_armcl::UniformAllLinks, muse_armcl::UniformSampling)

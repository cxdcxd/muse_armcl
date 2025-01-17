#include <mutex>
#include <thread>
#include <atomic>
#include <condition_variable>

#include <muse_armcl/state_space/mesh_map_provider.hpp>
#include <cslibs_math_ros/tf/conversion_3d.hpp>
#include <cslibs_mesh_map/cslibs_mesh_map_visualization.h>
#include <ros/ros.h>

namespace muse_armcl {
class EIGEN_ALIGN16 MeshMapLoader : public MeshMapProvider
{
public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
    using allocator_t = Eigen::aligned_allocator<MeshMapLoader>;
    state_space_t::ConstPtr getStateSpace() const override
    {
        std::unique_lock<std::mutex> l(map_mutex_);
        return map_;
    }

    void waitForStateSpace() const override
    {
        std::unique_lock<std::mutex> l(map_mutex_);
        if (!map_)
            notify_.wait(l);
    }

    void doSetup(ros::NodeHandle &nh) override
    {
        auto param_name = [this](const std::string &name){return name_ + "/" + name;};

        path_        = nh.param<std::string>(param_name("path"), "");
        files_       = nh.param<std::vector<std::string>>(param_name("meshes"),     std::vector<std::string>());
        parent_ids_  = nh.param<std::vector<std::string>>(param_name("parent_ids"), std::vector<std::string>());
        frame_ids_   = nh.param<std::vector<std::string>>(param_name("frame_ids"),  std::vector<std::string>());

        pub_surface_ = nh.advertise<visualization_msgs::MarkerArray>("surface",1);
        last_update_ = ros::Time::now();
        auto load = [this]() {
            if (!map_) {
                ROS_INFO_STREAM("[" << name_ << "]: Loading mesh map [" << path_ << "]");

                if (frame_ids_.empty())
                    throw std::runtime_error("[" + name_ + "]: No frame id found!");

                /// load map
                tree_.loadFromFile(path_, parent_ids_, frame_ids_, files_);
//                mesh_map_tree_node_t* l1 = tree.getNode(frame_ids_.front());

                std::unique_lock<std::mutex> l(map_mutex_);
                map_.reset(new MeshMap(&tree_, frame_ids_.front()));

                /// update transformations
                first_load_ = true;
                updateTransformations();
                first_load_ = false;

                /// finish load by unlocking mutex
                l.unlock();
                ROS_INFO_STREAM("[" << name_ << "]: Loaded map.");

                notify_.notify_all();
            }
        };

        worker_ = std::thread(load);
    }

    bool initializeTF(const std::vector<tf::StampedTransform>& transforms)
    {
        return true;
    }

private:
    using mesh_map_tree_node_t = cslibs_mesh_map::MeshMapTreeNode;
    using mesh_map_tree_t = cslibs_mesh_map::MeshMapTree;
    using mesh_map_t      = cslibs_mesh_map::MeshMap;

    mutable std::mutex                      map_mutex_;
    std::thread                             worker_;
    mutable std::condition_variable         notify_;

    mesh_map_tree_t                         tree_;
    mutable MeshMap::Ptr                    map_;
    bool                                    first_load_;
    mutable ros::Time                       last_update_;

    std::string                             path_;
    std::vector<std::string>                files_;
    std::vector<std::string>                parent_ids_;
    std::vector<std::string>                frame_ids_;

    ros::Publisher                          pub_surface_;
    mutable visualization_msgs::MarkerArray markers_;
    mutable visualization_msgs::Marker      msg_;
    inline void updateTransformations() const
    {
        const ros::Time now = ros::Time(0);
        if (!map_ || (!first_load_ && (now == last_update_)))
            return;

        ROS_INFO_STREAM("update map tranformations: rate "  << 1.0/(now -last_update_).toSec()  );
        last_update_ = now;
        resetMarkers();

        /// set current transforms between links
//        for (const std::string& frame_id : frame_ids_) {
        for(mesh_map_tree_node_t::Ptr m : tree_){

//            mesh_map_tree_node_t* m = map_->data()->getNode(frame_id);
//            if (!m)
//                throw std::runtime_error("[" + name_ + "]: Mesh for frame " + frame_id + " does not exist!");

            std::string root;
            std::string frame_id = m->frameId();
            bool found = m->parentFrameId(root);
            if (!found) root = frame_id;

            cslibs_math_3d::Transform3d transform;
            if (!tf_->lookupTransform(root, frame_id , now, transform, tf_timeout_))
                std::cerr << "Can't lookup transform: " << root << " <- " << frame_id << std::endl;
            else
                m->transform = transform;

            addMarker(m->map);
        }

        publishMarkers();
    }

    inline void resetMarkers() const
    {
        markers_.markers.clear();
        msg_.action = visualization_msgs::Marker::MODIFY;
        msg_.lifetime = ros::Duration(0.2);
        msg_.id = 0;
    }

    inline void addMarker(mesh_map_t& link) const
    {
        cslibs_mesh_map::visualization::visualizeVertices(link, msg_);
        markers_.markers.push_back(msg_);
        cslibs_mesh_map::visualization::visualizeBoundry(link, markers_);
    }

    inline void publishMarkers() const
    {
        const ros::Time now = ros::Time::now();
        for (auto &m : markers_.markers)
            m.header.stamp = now;
        pub_surface_.publish(markers_);
    }
};
}

#include <class_loader/class_loader_register_macro.h>
CLASS_LOADER_REGISTER_CLASS(muse_armcl::MeshMapLoader, muse_armcl::MeshMapProvider)

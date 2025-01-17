#include <rosbag/bag.h>
#include <rosbag/view.h>
#include <ros/ros.h>
#include <muse_armcl/evaluation/contact_evaluation_data.hpp>
#include <muse_armcl/evaluation/data_set_loader.hpp>

int main(int argc, char *argv[])
{
    ros::init(argc, argv, "muse_armcl_test_bag_node");
    if(argc != 2) {
        std::cout << "Please run 'rosrun muse_armcl muse_armcl_test_bag_node <path-to-bag>'" << std::endl;
        return 0;
    }

    ros::NodeHandle nh("~");

    std::vector<std::string> topics = {{nh.param<std::string>("tf", "/first_tf"),
                                        nh.param<std::string>("joint", "/contact_data")}};
    rosbag::Bag     bag(argv[1]);
    rosbag::View    view(bag, rosbag::TopicQuery(topics));

    std::cout << view.getBeginTime() << "\n";

    muse_armcl::DataSet data_set;
    bool success = muse_armcl::DataSetLoader::loadFromBag(bag, topics[0], topics[1], data_set);
    std::cout << (success ? " loaded data successfully" : " loading data failed") << std::endl;
    std::size_t nd = 0;
    for(auto val : data_set){
        nd += val.data.size();
    }
    std::cout << "Data set contains: "
              << data_set.size() << " sequences and "
              << nd << " messages. " << std::endl;

    bag.close();

    auto it = data_set.begin();
    uint64_t mt = it->data.getMaxTime();
    ++mt;
    auto a = it->data.at(mt);

    for(const  muse_armcl::ContactEvaluationSample& di : data_set){
        for(const  muse_armcl::ContactSample& ci : di.data){
            if(ci.label < -1){
                std::cerr << "wrong label?" << std::endl;
            }
        }
    }

    return 0;
}

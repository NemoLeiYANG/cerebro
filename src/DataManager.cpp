#include "DataManager.h"

DataManager::DataManager(ros::NodeHandle &nh )
//: out_stream(ofstream("/dev/pts/0",ios::out) )
{
    this->nh = nh;
}



DataManager::DataManager(const DataManager &obj)
//: out_stream(ofstream("/dev/pts/0",ios::out) )
{
   cout << "Copy constructor allocating ptr." << endl;
}


// void DataManager::setCamera( const PinholeCamera& camera )
// {
//   this->camera = camera;
//
//   cout << "--- Camera Params from DataManager ---\n";
//   this->camera.printCameraInfo();
//   // cout << "K\n" << this->camera.e_K << endl;
//   // cout << "D\n" << this->camera.e_D << endl;
//   cout << "--- END\n";
// }




void DataManager::setAbstractCamera( camodocal::CameraPtr abs_camera, short cam_id )
{
    assert( cam_id >= 0 && "DataManager, you requested a camera with negative id which is an error. cam_id=0 for default camera and 1,2,.. for additional cameras.\n" );
    assert(abs_camera && "[DataManager::setCamera] in datamanager you are trying to set an invalid abstract camera. You need to loadFromYAML before you can set this camera\n");
    // this->abstract_camera = abs_camera;
    this->all_abstract_cameras[cam_id] = abs_camera;

    cout << "--- Abstract CameraParams(cam_id=" << cam_id << ") from DataManager ---\n";
    // cout << this->abstract_camera->parametersToString() << endl;
    cout << this->all_abstract_cameras[cam_id]->parametersToString() << endl;
    cout << "--- END\n";
}

camodocal::CameraPtr DataManager::getAbstractCameraRef(short cam_id) const
{
    assert( cam_id >= 0 && "DataManager, you requested a camera with negative id which is an error. cam_id=0 for default camera and 1,2,.. for additional cameras.\n" );
    // assert( abstract_camera && "[DataManager::getAbstractCameraRef] you are requesting a camera reference before setting.\n" );
    // return abstract_camera;
    assert( isAbstractCameraSet(cam_id) && "[DataManager::getAbstractCameraRef] you are requesting a camera reference before setting.\n");
    // return this->all_abstract_cameras[cam_id]; //old non-const access
    return this->all_abstract_cameras.at(cam_id);
}

bool DataManager::isAbstractCameraSet(short cam_id) const
{
    assert( cam_id >= 0 && "DataManager, you requested a camera with negative id which is an error. cam_id=0 for default camera and 1,2,.. for additional cameras.\n" );
    if( this->all_abstract_cameras.count( cam_id ) > 0 ) {
        return true;
        // if( this->all_abstract_cameras[cam_id] )
            // return true;
        // else
            // return false;
    }
    else {
        return false;
    }
}

vector<short> DataManager::getAbstractCameraKeys() const {
    vector<short> keys;
    for( auto it=this->all_abstract_cameras.begin(); it!=all_abstract_cameras.end() ; it++ ) {
        keys.push_back( it->first );
    }
    return keys;
}


void DataManager::setCameraRelPose( Matrix4d a_T_b, std::pair<int,int> pair_a_b )
{
    // assert a and b abstract cameras exisits
    assert( isAbstractCameraSet(pair_a_b.first) && isAbstractCameraSet(pair_a_b.second) && "in [DataManager::setCameraRelPose] one of the abstract-cameras were not set, even though you asked me to set their relative pose. You need to set the cameras first before you can set the relative poses.\n" );

    cout << "---DataManager::setCameraRelPose---\n";
    cout << "setting "<< pair_a_b.first << "_T_" << pair_a_b.second << " :::> " << PoseManipUtils::prettyprintMatrix4d( a_T_b ) << endl;

    cam_relative_poses[ pair_a_b ] = a_T_b;
    cout << "---DONE---\n";

}

bool DataManager::isCameraRelPoseSet( std::pair<int,int> pair_a_b ) const
{
    if( this->cam_relative_poses.count( pair_a_b ) > 0 )
        return true;
    else
        return false;
}

const Matrix4d& DataManager::getCameraRelPose( std::pair<int,int> pair_a_b ) const
{
    assert( isCameraRelPoseSet( pair_a_b ) && "[DataManager::getCameraRelPose] make sure the rel cam pose you are requesting is available\n" );
    if( !isCameraRelPoseSet( pair_a_b) ) {
        ROS_ERROR( "[DataManager::getCameraRelPose] make sure the rel cam pose you are requesting is available\nYou requested (%d,%d) which is not available", pair_a_b.first, pair_a_b.second );
        exit(2);
    }

    // return this->cam_relative_poses[ pair_a_b ]; //old non const
    return this->cam_relative_poses.at( pair_a_b );
}


vector< std::pair<int,int> > DataManager::getCameraRelPoseKeys()
{
    vector< pair<int,int> > keys;
    for( auto it=this->cam_relative_poses.begin(); it!=cam_relative_poses.end() ; it++ ) {
        keys.push_back( it->first );
    }
    return keys;
}

const Matrix4d& DataManager::getIMUCamExtrinsic() const
{
    std::lock_guard<std::mutex> lk(global_vars_mutex);
    assert( imu_T_cam_available && "[DataManager::getIMUCamExtrinsic] you request the value before setting it\n");
    return imu_T_cam;
}

bool DataManager::isIMUCamExtrinsicAvailable() const
{
    std::lock_guard<std::mutex> lk(global_vars_mutex);
    return imu_T_cam_available;
}

const ros::Time DataManager::getIMUCamExtrinsicLastUpdated() const
{
    std::lock_guard<std::mutex> lk(global_vars_mutex);
    return imu_T_cam_stamp;
}

//////////////////////////////////////////////////////////////////////////////
/////////////////// Kidnap Indicator Publisher ///////////////////////////////
//////////////////////////////////////////////////////////////////////////////
bool DataManager::isKidnapIndicatorPubSet() const
{
    return (bool)is_kidnapn_indicator_set ;
}

void DataManager::setKidnapIndicatorPublishers( ros::Publisher& pub_bool, ros::Publisher& pub_header )
{
    rcvd_flag_pub = pub_bool;
    kidnap_indicator_header_pub = pub_header;
    is_kidnapn_indicator_set = true;
}


void DataManager::PUBLISH__TRUE( const ros::Time _t ) const
{
    assert( is_kidnapn_indicator_set );
    cout << TermColor::RED() << "[Cerebro::PUBLISH__TRUE] PUBLISH TRUE (t= " << _t << " to indicate the vins_estimator to start again.\n" << TermColor::RESET();

    std_msgs::Bool bool_msg; bool_msg.data = true;
    rcvd_flag_pub.publish( bool_msg );

    // publish header msg
    std_msgs::Header header_msg;
    header_msg.stamp =  _t;
    header_msg.frame_id = "unkidnapped";
    kidnap_indicator_header_pub.publish( header_msg );
}

void DataManager::PUBLISH__FALSE( const ros::Time _t ) const
{
    assert( is_kidnapn_indicator_set );
    cout << TermColor::RED() << "[Cerebro::PUBLISH__FALSE] PUBLISH FALSE (t= " << _t << " to indicate the vins_estimator to stop.\n" << TermColor::RESET();

    // publish False (bool msg)
    std_msgs::Bool bool_msg; bool_msg.data = false;
    rcvd_flag_pub.publish( bool_msg );

    // publish header message
    std_msgs::Header header_msg;
    header_msg.stamp = _t;
    header_msg.frame_id = "kidnapped";
    kidnap_indicator_header_pub.publish( header_msg );
}


//////////////////////////////////////////////////////////////////////////////
//////////////////////////////// call backs //////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

// #define __DATAMANAGER_CALLBACK_PRINT( u ) u;
#define __DATAMANAGER_CALLBACK_PRINT( u ) ;

void DataManager::camera_pose_callback( const nav_msgs::Odometry::ConstPtr msg )
{
    if( pose_0_available == false ) { //record the 1st pose
        pose_0 = msg->header.stamp;
        pose_0_available = true;
    }

    // __DATAMANAGER_CALLBACK_PRINT( cout << TermColor::BLUE()  <<  "[cerebro/camera_pose_callback del]" <<  msg->header.stamp-pose_0 << TermColor::RESET() << endl; )
    __DATAMANAGER_CALLBACK_PRINT( cout << TermColor::BLUE()  <<  "[cerebro/camera_pose_callback]" <<  msg->header.stamp << "\t" << msg->header.stamp-pose_0 << TermColor::RESET() << endl; )
    // push this to queue. Another thread will associate the data
    pose_buf.push( msg );
    return;


}

// currently not in use.
void DataManager::keyframe_pose_callback( const nav_msgs::Odometry::ConstPtr msg )
{
    //__DATAMANAGER_CALLBACK_PRINT( cout << "[cerebro/camera_pose_callback del]" << msg->header.stamp - pose_0 << endl; )
    __DATAMANAGER_CALLBACK_PRINT( cout << TermColor::iBLUE() << "[cerebro/keyframe_pose_callback]" << msg->header.stamp << "\t" << msg->header.stamp-pose_0 << TermColor::RESET() << endl; )
    // push this to queue. Another thread will associate the data
    kf_pose_buf.push( msg );
}


void DataManager::raw_image_callback( const sensor_msgs::ImageConstPtr& msg )
{
    // __DATAMANAGER_CALLBACK_PRINT( cout << TermColor::GREEN() << "[cerebro/raw_image_callback del]" << msg->header.stamp-pose_0 << TermColor::RESET() << endl; )
    __DATAMANAGER_CALLBACK_PRINT( cout << TermColor::GREEN() << "[cerebro/raw_image_callback]" << msg->header.stamp << "\t" << msg->header.stamp-pose_0 << TermColor::RESET() << endl; )


    if( this->last_image_time != ros::Time() &&
            (msg->header.stamp.toSec() - this->last_image_time.toSec() > 1.0
                ||
            msg->header.stamp.toSec() < this->last_image_time.toSec())
    )
    {
        cout << TermColor::iBLUE()
        << "+++++++++++++[DataManager::raw_image_callback] "
        << " curr_image.stamp - prev_image.stamp > 1.0.\n"
        << "This means an unstable stream or more usually means a new bag has started with --skip-empty in rosbagplay.\n"
        << "I will publish a FALSE t=" << 10000 << " then wait for 500ms and publish TRUE t=" << 10000
        << " TODO Complete this implementation and verify correctness."
        << TermColor::RESET() << endl;



        cout << TermColor::BLUE() << "PUBLISH__FALSE\n" << TermColor::RESET();
        PUBLISH__FALSE( this->last_image_time );

        cout << TermColor::BLUE() << "Sleep 500ms\n" << TermColor::RESET();
        // sleep( 500 ms );
        std::this_thread::sleep_for (std::chrono::milliseconds(500));

        cout << TermColor::BLUE() << "PUBLISH__TRUE\n" << TermColor::RESET();
        PUBLISH__TRUE( msg->header.stamp );



    }


    img_buf.push( msg );
    this->last_image_time = msg->header.stamp;
    return;

}

void DataManager::raw_image_callback_1( const sensor_msgs::ImageConstPtr& msg )
{
    // __DATAMANAGER_CALLBACK_PRINT( cout << "[cerebro/raw_image_callback_1 del]" << msg->header.stamp-pose_0 << endl; )
    __DATAMANAGER_CALLBACK_PRINT( cout << "[cerebro/raw_image_callback_1]" << msg->header.stamp << "\t" << msg->header.stamp-pose_0 << endl; )
    img_1_buf.push( msg );
    return;
}

void DataManager::extrinsic_cam_imu_callback( const nav_msgs::Odometry::ConstPtr msg )
{
    // __DATAMANAGER_CALLBACK_PRINT( cout << "[cerebro/extrinsic_cam_imu_callback del]" << msg->header.stamp-pose_0 << endl; )
    __DATAMANAGER_CALLBACK_PRINT( cout << "[cerebro/extrinsic_cam_imu_callback]" << msg->header.stamp << "\t" << msg->header.stamp-pose_0 << endl; )
    extrinsic_cam_imu_buf.push( msg );
}



// there will be 5 channels. ch[0]: un, ch[1]: vn,  ch[2]: u, ch[3]: v.  ch[4]: globalid of the feature.
 // cout << "\tpoints.size() : "<< msg->points.size(); // this will be N (say 92)
 // cout << "\tchannels.size() : "<< msg->channels.size(); //this will be N (say 92)
 // cout << "\tchannels[0].size() : "<< msg->channels[0].values.size(); //this will be 5.
 // cout << "\n";
 // An Example Keypoint msg
     // ---
     // header:
     //   seq: 40
     //   stamp:
     //     secs: 1523613562
     //     nsecs: 530859947
     //   frame_id: world
     // points:
     //   -
     //     x: -7.59081602097
     //     y: 7.11367511749
     //     z: 2.85602664948
     //   .
     //   .
     //   .
     //   -
     //     x: -2.64935922623
     //     y: 0.853760659695
     //     z: 0.796766400337
     // channels:
     //   -
     //     name: ''
     //     values: [-0.06108921766281128, 0.02294199913740158, 310.8721618652344, 260.105712890625, 2.0]
     //     .
     //     .
     //     .
     //   -
     //     name: ''
     //     values: [-0.47983112931251526, 0.8081198334693909, 218.95481872558594, 435.47357177734375, 654.0]
     //   -
     //     name: ''
     //     values: [0.07728647440671921, 1.0073764324188232, 344.2176208496094, 473.7791442871094, 660.0]
     //   -
     //     name: ''
     //     values: [-0.6801641583442688, 0.10506453365087509, 159.75746154785156, 279.6077575683594, 663.0]
void DataManager::ptcld_callback( const sensor_msgs::PointCloud::ConstPtr msg )
{
    // __DATAMANAGER_CALLBACK_PRINT( cout << "[cerebro/ptcld_callback]" << msg->header.stamp << endl; )
    __DATAMANAGER_CALLBACK_PRINT( cout << TermColor::YELLOW() << "[cerebro/ptcld_callback]" << msg->header.stamp  << "\t" << msg->header.stamp-pose_0 << TermColor::RESET() << endl; )
    ptcld_buf.push( msg );
    return;

}


// currently not in use
void DataManager::tracked_feat_callback( const sensor_msgs::PointCloud::ConstPtr msg )
{
    __DATAMANAGER_CALLBACK_PRINT( cout << "[cerebro/tracked_feat_callback]" << msg->header.stamp << "\t" << msg->header.stamp-pose_0 << endl; )
    trackedfeat_buf.push( msg );
}



// #define __json_debuggin(msg) msg;
#define __json_debuggin(msg) ;
json DataManager::asJson()
{
    json obj;
    auto __data_map = this->getDataMapRef();
    IOFormat CSVFormat(StreamPrecision, DontAlignCols, ", ", ",\t\n");

    __json_debuggin( cout << "[DataManager::asJson] Start loop through data_map\n"; )

    #if 1
    // int max_dist = std::distance( __data_map.begin() , __data_map.end() ); //berks__old
    int max_dist = std::distance( __data_map->begin() , __data_map->end() );

    // for( auto it = __data_map.begin() ; it!= __data_map.end() ; it++ ) //berks__old
    for( auto it = __data_map->begin() ; it!= __data_map->end() ; it++ )
    {
        // int seq_id = std::distance( __data_map.begin() , it ); // berks__old
        int seq_id = std::distance( __data_map->begin() , it );
        __json_debuggin(
        cout << "[DataManager::asJson] " <<  seq_id << " max dist=" << max_dist << endl;)

        json curr_obj;
        curr_obj["stamp"] = it->first.toSec();
        curr_obj["stamp_relative"] =  ( it->first -  this->getPose0Stamp() ).toSec() ;
        curr_obj["seq"] = seq_id;
        __json_debuggin( cout << "A\n"; )

        DataNode * __u = it->second;
        curr_obj["getT"] = __u->getT().toSec();
        curr_obj["getT_image"] = __u->getT_image().toSec();
        // curr_obj["getT_image_1"] = __u->getT_image(1).toSec();
        curr_obj["getT_pose"] = __u->getT_pose().toSec();
        curr_obj["getT_uv"] = __u->getT_uv().toSec();
        __json_debuggin( cout << "B\n"; )

        curr_obj["isKeyFrame"] = __u->isKeyFrame();
        curr_obj["isImageAvailable"] = __u->isImageAvailable();
        curr_obj["isImageAvailable_1"] = __u->isImageAvailable(1);
        curr_obj["isPoseAvailable"] = __u->isPoseAvailable();
        curr_obj["isPtCldAvailable"] = __u->isPtCldAvailable();
        curr_obj["isUnVnAvailable"] = __u->isUnVnAvailable();
        curr_obj["isUVAvailable"] = __u->isUVAvailable();
        curr_obj["isFeatIdsAvailable"] = __u->isFeatIdsAvailable();
        curr_obj["getNumberOfSuccessfullyTrackedFeatures"] = __u->getNumberOfSuccessfullyTrackedFeatures();
        curr_obj["isWholeImageDescriptorAvailable"] = __u->isWholeImageDescriptorAvailable();
        __json_debuggin( cout << "C\n"; )

        if( __u->isPoseAvailable() )
        {
            const Matrix4d wTc = __u->getPose();
            json pose_ifo;
            pose_ifo["rows"] = wTc.rows();
            pose_ifo["cols"] = wTc.cols();
            pose_ifo["stamp"] = __u->getT_pose().toSec();
            std::stringstream ss;
            ss <<  wTc.format(CSVFormat);
            pose_ifo["data"] = ss.str();
            pose_ifo["data_pretty"] = PoseManipUtils::prettyprintMatrix4d(wTc);
            curr_obj["w_T_c"] = pose_ifo;
        }
        __json_debuggin( cout << "D\n"; )



        obj["DataNodes"].push_back( curr_obj );
    }
    #endif

    __json_debuggin( cout << "[DataManager::asJson] End loop through data_map\n"; )


    // Global Data
    __json_debuggin( cout << "[DataManager::asJson] Logging Global data\n"; )
    obj["global"]["isPose0Available"] = this->isPose0Available();
    obj["global"]["Pose0Stamp"] = this->getPose0Stamp().toSec();

    obj["global"]["isIMUCamExtrinsicAvailable"] = this->isIMUCamExtrinsicAvailable();
    if( this->isIMUCamExtrinsicAvailable() )
    {
        const Matrix4d __imu_T_cam = this->getIMUCamExtrinsic();
        json pose_ifo;
        pose_ifo["rows"] = __imu_T_cam.rows();
        pose_ifo["cols"] = __imu_T_cam.cols();
        pose_ifo["stamp"] = this->getIMUCamExtrinsicLastUpdated().toSec();
        std::stringstream ss;
        ss <<  __imu_T_cam.format(CSVFormat);
        pose_ifo["data"] = ss.str();
        pose_ifo["data_pretty"] = PoseManipUtils::prettyprintMatrix4d(__imu_T_cam);
        obj["global"]["Pose0Stamp"] = pose_ifo;
    }


    __json_debuggin( cout << "[DataManager::asJson] Done asJson()\n"; )
    return obj;

}



string DataManager::print_queue_size( int verbose=1 ) const
{
    std::stringstream buffer;

    if( verbose == 1)
    {
        buffer << "img_buf=" << img_buf.size() << "\t";
        buffer << "img_1_buf=" << img_1_buf.size() << "\t";
        buffer << "pose_buf=" << pose_buf.size() << "\t";
        buffer << "kf_pose_buf=" << kf_pose_buf.size() << "\t";
        buffer << "ptcld_buf=" << ptcld_buf.size() << "\t";
        buffer << "trackedfeat_buf=" << trackedfeat_buf.size() << "\t";
        buffer << "extrinsic_cam_imu_buf=" << extrinsic_cam_imu_buf.size() << "\t";
        buffer << endl;
    }

    if( verbose == 2 )
    {
        buffer << "img_buf=" << img_buf.size() << " (";
        if( img_buf.size()  > 0 ) {
            buffer << std::fixed << std::setprecision(4) << img_buf.front()->header.stamp-pose_0 << "-->";
            buffer << std::fixed << std::setprecision(4) << img_buf.back()->header.stamp-pose_0 << ";";
        }
        buffer << ")\t";

        buffer << "img_1_buf=" << img_1_buf.size() << " (";
        if( img_1_buf.size()  > 0 ) {
            buffer << std::fixed << std::setprecision(4) << img_1_buf.front()->header.stamp-pose_0 << "-->";
            buffer << std::fixed << std::setprecision(4) << img_1_buf.back()->header.stamp-pose_0 << ";";
        }
        buffer << ")\t";

        buffer << "pose_buf=" << pose_buf.size() << " (";
        if( pose_buf.size()  > 0 ) {
            buffer << std::fixed << std::setprecision(4) << pose_buf.front()->header.stamp-pose_0 << "-->";
            buffer << std::fixed << std::setprecision(4) << pose_buf.back()->header.stamp-pose_0 << ";";
        }
        buffer << ")\t";

        buffer << "kf_pose_buf=" << kf_pose_buf.size() << " (";
        if( kf_pose_buf.size()  > 0 ) {
            buffer << std::fixed << std::setprecision(4) << kf_pose_buf.front()->header.stamp-pose_0 << "-->";
            buffer << std::fixed << std::setprecision(4) << kf_pose_buf.back()->header.stamp-pose_0 << ";";
        }
        buffer << ")\t";

        buffer << "ptcld_buf=" << ptcld_buf.size() << " (";
        if( ptcld_buf.size()  > 0 ) {
            buffer << std::fixed << std::setprecision(4) << ptcld_buf.front()->header.stamp-pose_0 << "-->";
            buffer << std::fixed << std::setprecision(4) << ptcld_buf.back()->header.stamp-pose_0 << ";";
        }
        buffer << ")\t";

        buffer << "trackedfeat_buf=" << trackedfeat_buf.size() << " (";
        if( trackedfeat_buf.size()  > 0 ) {
            buffer << std::fixed << std::setprecision(4) << trackedfeat_buf.front()->header.stamp-pose_0 << "-->";
            buffer << std::fixed << std::setprecision(4) << trackedfeat_buf.back()->header.stamp-pose_0 << ";";
        }
        buffer << ")\t";

        buffer << "extrinsic_cam_imu_buf=" << extrinsic_cam_imu_buf.size() << " (";
        if( extrinsic_cam_imu_buf.size()  > 0 ) {
            buffer << std::fixed << std::setprecision(4) << extrinsic_cam_imu_buf.front()->header.stamp-pose_0 << "-->";
            buffer << std::fixed << std::setprecision(4) << extrinsic_cam_imu_buf.back()->header.stamp-pose_0 << ";";
        }
        buffer << ")\t";
        buffer << "\n";
    }

    if( verbose == 3 )
    {
        buffer << "img_buf.len=" << img_buf.size() << "\t";
        buffer << "pose_buf.len=" << pose_buf.size() << "\t";
        buffer << "kf_pose_buf.len=" << kf_pose_buf.size() << "\t";
        buffer << "ptcld_buf.len=" << ptcld_buf.size() << "\t";
        buffer << "trackedfeat_buf.len=" << trackedfeat_buf.size() << "\t";
        buffer << "extrinsic_cam_imu_buf.len=" << extrinsic_cam_imu_buf.size() << "\t";
        buffer << endl;

        if( img_buf.size() > 0 )
            buffer << "img_buf.back.t=" << img_buf.back()->header.stamp-pose_0 << "\t";
        if( pose_buf.size() > 0 )
            buffer << "pose_buf.back.t=" << pose_buf.back()->header.stamp-pose_0 << "\t";
        if( kf_pose_buf.size() > 0 )
            buffer << "kf_pose_buf.back.t=" << kf_pose_buf.back()->header.stamp-pose_0 << "\t";
        if( ptcld_buf.size() > 0 )
            buffer << "ptcld_buf.back.t=" << ptcld_buf.back()->header.stamp-pose_0 << "\t";
        if( trackedfeat_buf.size() > 0 )
            buffer << "trackedfeat_buf.back.t=" << trackedfeat_buf.back()->header.stamp-pose_0 << "\t";
        if( extrinsic_cam_imu_buf.size() > 0 )
            buffer << "extrinsic_cam_imu_buf.back.t=" << extrinsic_cam_imu_buf.back()->header.stamp-pose_0 << "\t";
        buffer << endl;

        if( img_buf.size() > 0 )
            buffer << "img_buf.front.t=" << img_buf.front()->header.stamp-pose_0 << "\t";
        if( pose_buf.size() > 0 )
            buffer << "pose_buf.front.t=" << pose_buf.front()->header.stamp-pose_0 << "\t";
        if( kf_pose_buf.size() > 0 )
            buffer << "kf_pose_buf.front.t=" << kf_pose_buf.front()->header.stamp-pose_0 << "\t";
        if( ptcld_buf.size() > 0 )
            buffer << "ptcld_buf.front.t=" << ptcld_buf.front()->header.stamp-pose_0 << "\t";
        if( trackedfeat_buf.size() > 0 )
            buffer << "trackedfeat_buf.front.t=" << trackedfeat_buf.front()->header.stamp-pose_0 << "\t";
        if( extrinsic_cam_imu_buf.size() > 0 )
            buffer << "extrinsic_cam_imu_buf.front.t=" << extrinsic_cam_imu_buf.front()->header.stamp-pose_0 << "\t";
        buffer << endl;
    }

    return buffer.str();
}


//////////////////////////// Callback ends /////////////////////////////////////




//////////////////////////////////////////////////////////////////////////////
/////////////////////////// Thread mains /////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

void DataManager::trial_thread()
{
    cout << TermColor::GREEN() << "Start DataManager::trial_thread "<< TermColor::RESET() << endl;
    cout << "HI\n ";
    ros::Rate looprate(10);
    while( b_trial_thread )
    {
        cout << "trial_thread\n";

        // berks__old
        // auto S = data_map.begin(); //.lower_bound( lb );
        // auto E = data_map.end();

        auto S = data_map->begin();
        auto E = data_map->end();
        cout << "S=" << S->first << "  E=" << E->first <<  endl;
        for( auto it = S ; it != E ; it++ )
        {
            #if 1
            if( it->second->isImageAvailable() && it->second->isImageAvailable(1) ) {
                cout << TermColor::GREEN();
            } else {
                if( it->second->isImageAvailable() )
                    cout << TermColor::CYAN() ;
                else
                    cout << TermColor::BLUE() ;
            }

            if( it->second->isPoseAvailable() && it->second->getNumberOfSuccessfullyTrackedFeatures() < 0 ) {
                cout << "A" ;
                // cout << it->second->getNumberOfSuccessfullyTrackedFeatures() << ",";
            }
            if( !it->second->isPoseAvailable() && it->second->getNumberOfSuccessfullyTrackedFeatures() < 0 ) {
                cout << "B" ;
                // cout << it->second->getNumberOfSuccessfullyTrackedFeatures() << ",";  // this means there thise node has no pose and no tracked features data
            }
            if( it->second->isPoseAvailable() && ! (it->second->getNumberOfSuccessfullyTrackedFeatures() < 0 ) ) {
                cout << "C" ;
                // cout << it->second->getNumberOfSuccessfullyTrackedFeatures() << ",";
            }
            if( !it->second->isPoseAvailable() && ! (it->second->getNumberOfSuccessfullyTrackedFeatures() < 0 )  ){
                cout << "D"; // has no pose data but has tracked features. Pose actually takes some time to arrive. tracked features will be avaiaable first,
            }
            cout << TermColor::RESET() ;
            #endif
        }
        cout << endl;



        looprate.sleep();
    }

    cout << TermColor::RED() << "END DataManager::trial_thread "<< TermColor::RESET() << endl;
}


// #define ___clean_up_cout(msg) msg;
#define ___clean_up_cout(msg) ;
void DataManager::clean_up_useless_images_thread()
{

    cout << TermColor::GREEN() << "Start DataManager::clean_up_useless_images_thread "<< TermColor::RESET() << endl;
    ros::Rate looprate(0.3);
    // data_association_thread_disable();
    while( b_clean_up_useless_images_thread )
    {
        looprate.sleep();

        // if( data_map.begin() == data_map.end() ) { //berks__old
        if( data_map->begin() == data_map->end() ) {
            cout << TermColor::CYAN() << "[clean_up_useless_images_thread] no nodes" << TermColor::RESET() << endl;
            continue;
        }

        // berks__old
        // auto S = data_map.begin();
        // auto E = data_map.upper_bound( data_map.rbegin()->first - ros::Duration( 3.0 ) );

        auto S = data_map->begin();
        auto E = data_map->upper_bound( data_map->rbegin()->first - ros::Duration( 10.0 ) );
        int q=0;
        ___clean_up_cout( cout << S->first << " to " << E->first << endl; )
        // for( auto it = data_map.begin() ; it->first < E->first ; it++ ) { //berks__old
        for( auto it = data_map->begin() ; it->first < E->first ; it++ ) {
            if( it->second->isImageAvailable() && !it->second->isKeyFrame() ) {
            // if( it->second->isImageAvailable()  ) {
                ___clean_up_cout(
                    cout << TermColor::CYAN() << "deallocate_all_images with t=" << it->first << " " << q++ << TermColor::RESET() << endl;
                    it->second->print_image_cvinfo();
                )
                it->second->deallocate_all_images();
            }
            else {
                ___clean_up_cout( cout << TermColor::YELLOW() << "not deallocate coz t=" << it->first << " is a keyframe (has pose info)" << q++ << TermColor::RESET() << endl; )
            }
        }
    }
    cout << TermColor::RED() << "END DataManager::clean_up_useless_images_thread "<< TermColor::RESET() << endl;

}

// #define _12335_print( str ) cout << "[data_association_thread]" << str;
#define _12335_print( str ) ;

// #define __DataManager__data_association_thread__( msg ) msg;
#define __DataManager__data_association_thread__( msg ) ;

void DataManager::data_association_thread( int max_loop_rate_in_hz )
{
    assert( max_loop_rate_in_hz > 0 && max_loop_rate_in_hz < 200 && "[DataManager::data_association_thread] I am expecting the loop rate to be between 1-50" );
    _12335_print( "Start thread");
    assert( b_data_association_thread && "You have not enabled thread execution. Call function DataManager::data_association_thread_enable() before you spawn this thread\n");
    float requested_loop_time_ms = 1000. / max_loop_rate_in_hz;

    while( b_data_association_thread )
    {
        auto loop_start_at = std::chrono::high_resolution_clock::now();

        //------------------------ Process here--------------------------------
        // std::this_thread::sleep_for( std::chrono::milliseconds(2000) );
        _12335_print( "---\n" );
        _12335_print( print_queue_size(2 ) ); //< sizes of buffer queues
        _12335_print( "\t\tSize_of_data_map = "+ to_string( data_map.size() ) + "\n" );


        // deqeue all raw images and make DataNodes of each of them, s
        while( img_buf.size() > 0 ) {
            #if ___USE_STD_QUEUES
            sensor_msgs::ImageConstPtr img_msg = img_buf.front();
            img_buf.pop();
            #else
            sensor_msgs::ImageConstPtr img_msg = img_buf.pop();
            #endif
            __DataManager__data_association_thread__(
                cout << TermColor::GREEN() << ">>>>>>>>> Added a new DataNode in data_map with poped() rawimage t=" << img_msg->header.stamp << " #####> ie." << img_msg->header.stamp - pose_0 << TermColor::RESET() << endl;
            )
            DataNode * n = new DataNode( img_msg->header.stamp );
            n->setImageFromMsg( img_msg );

            // data_map.insert( std::make_pair(img_msg->header.stamp, n) ); //berks__old
            data_map->insert( std::make_pair(img_msg->header.stamp, n) );
        }

        // dequeue additional raw images
        while( img_1_buf.size() > 0 ) {
            #if ___USE_STD_QUEUES
            sensor_msgs::ImageConstPtr img_1_msg = img_1_buf.front();
            img_1_buf.pop();
            #else
            sensor_msgs::ImageConstPtr img_1_msg = img_1_buf.pop();
            #endif
            ros::Time t = img_1_msg->header.stamp;

            __DataManager__data_association_thread__(
            cout << ">> Attempt adding poped() img_1_msg in data_map with t=" << img_1_msg->header.stamp << " ie. #####> " << img_1_msg->header.stamp-pose_0 << endl;
            )
            // if( data_map.count(t) > 0 ) { //berks__old
            //     data_map.at( t )->setImageFromMsg( img_1_msg, 1 );
            // }
            if( data_map->count(t) > 0 ) {
                data_map->at( t )->setImageFromMsg( img_1_msg, 1 );
            }
            else {
                // assert( false && "[DataManager::data_association_thread] attempting to set additional image into datanode. However that datanode is not found in the map. This cannot be happening\n");
                __DataManager__data_association_thread__(
                    cout << TermColor::RED() << "[DataManager::data_association_thread]";
                    cout << "attempting to set additional image_1 with t=" << t << " aka " << t-pose_0 <<  " into datanode. ";
                    cout << "However that datanode is not found in the map. This cannot be happening, but might happen when `image_1` preceeds (even a few milis) than `image`.";
                    cout << TermColor::RESET() << endl ;

                cout << TermColor::CYAN() << "pushing this image_1 again to the queue\n" << TermColor::RESET();
                )
                img_1_buf.push( img_1_msg );
                break;
            }
        }


        // dequeue all poses and set them to data_map
        while( pose_buf.size() > 0 ) {
            #if ___USE_STD_QUEUES
            nav_msgs::Odometry::ConstPtr pose_msg = pose_buf.front();
            pose_buf.pop();
            #else
            nav_msgs::Odometry::ConstPtr pose_msg = pose_buf.pop();
            #endif
             ros::Time t = pose_msg->header.stamp;
             __DataManager__data_association_thread__(
             cout << ">> Attempt adding poped() pose in data_map with t=" << pose_msg->header.stamp << " ie. #####> " << pose_msg->header.stamp -pose_0 << endl;
             )

             // find the DataNode with this timestamp
             //berks__old
            //  if( data_map.count( t ) > 0 ) {
            //      // a Node seem to exist with this t.
            //      data_map.at( t )->setPoseFromMsg( pose_msg );
            //  }
             if( data_map->count( t ) > 0 ) {
                 // a Node seem to exist with this t.
                 data_map->at( t )->setPoseFromMsg( pose_msg );
             }
             else {

                 // try range search
                 // t-delta <= x <= t+delta. x is the map key.

                 __DataManager__data_association_thread__( cout << "\tsince the key was not found in data_map do range_search\n"; )
                 //berks__old
                 auto __it = data_map->begin();
                 for( __it = data_map->begin() ; __it != data_map->end() ; __it++ ) {
                     ros::Duration diff =  __it->first-t;
                     if( (diff.sec == 0  &&  abs(diff.nsec) < 1000000) || (diff.sec == -1  &&  diff.nsec > (1000000000-1000000) )  )
                        break;
                 }

                 // berks__old
                 if( __it == data_map->end() ) {
                     __DataManager__data_association_thread__( cout << TermColor::RED() << "\trange search failed AAA\n");
                     assert( false && "\tnot fouind\n");
                     exit(2);
                 }

                __DataManager__data_association_thread__(
                 cout << "\tfind pose->t=" << t << " in data_map\n";
                 std::cout << "\tinsert at position " << (__it->first) << '\n';
                )

                if( true )
                 {
                    // berks__old
                     data_map->at( __it->first )->setPoseFromMsg( pose_msg );
                 }
                 else {

                 __DataManager__data_association_thread__(cout << TermColor::RED() << "[DataManager::data_association_thread] data_map does not seem to contain the t of pose_msg. This cannot be happening...fatal quit" << TermColor::RESET() << endl;)
                 assert( false && "[DataManager::data_association_thread] data_map does not seem to contain the t of pose_msg. This cannot be happening\n");
                 exit(2);
                }
             }
         }


        // dequeue all point clouds (these are at keyframes)
        while( ptcld_buf.size() > 0 ) {
            #if ___USE_STD_QUEUES
            sensor_msgs::PointCloudConstPtr ptcld_msg = ptcld_buf.front();
            ptcld_buf.pop();
            #else
            sensor_msgs::PointCloudConstPtr ptcld_msg = ptcld_buf.pop();
            #endif
            ros::Time t = ptcld_msg->header.stamp;
            __DataManager__data_association_thread__(
            cout << ">> Attempt adding poped() pointcloud in data_map at t=" << t << " ie. #####> " <<  t - pose_0 << endl;
            )

            // find the DataNode with this timestamp
            // berks__old
            if( data_map->count( t ) > 0 ) {
                // a Node seem to exist with this t.

                //NOte: the ``/vins_estimator/keyframe_point`` and ``/feature_tracker/feature``
                // are in different formats so be careful. vins_estimator/keyframe_point has n channels. while feature_tracker/feature has just 5 channels.
                // but they both convey the same info, so be careful what exact you want.
                #if 0
                data_map.at( t )->setPointCloudFromMsg( ptcld_msg );
                data_map.at( t )->setUnVnFromMsg( ptcld_msg );
                data_map.at( t )->setUVFromMsg( ptcld_msg );
                data_map.at( t )->setTrackedFeatIdsFromMsg( ptcld_msg );
                data_map.at( t )->setAsKeyFrame();
                #else
                // cout << "setNumberOfSuccessfullyTrackedFeatures " << t << " " << ptcld_msg->points.size() << endl;
                //berks__old
                data_map->at( t )->setNumberOfSuccessfullyTrackedFeatures( ptcld_msg->points.size() );
                data_map->at( t )->setAsKeyFrame();
                #endif
            }
            else {
                // try range search
                // t-delta <= x <= t+delta. x is the map key.
                __DataManager__data_association_thread__( cout << "\tsince the key was not found in data_map do range_search\n"; )

                // berks__old
                auto __it = data_map->begin();
                for( __it = data_map->begin() ; __it != data_map->end() ; __it++ ) {
                    ros::Duration diff =  __it->first-t;
                    if( (diff.sec == 0  &&  abs(diff.nsec) < 1000000) || (diff.sec == -1  &&  diff.nsec > (1000000000-1000000) )  )
                       break;
                }

                // berks__old
                if( __it == data_map->end() ) {
                    __DataManager__data_association_thread__( cout << TermColor::RED() << "\trange search failed BBB\n");
                    assert( false && "\tnot fouind\n");
                    exit(2);
                }

               __DataManager__data_association_thread__(
                cout << "\tfind ptcld->t=" << t << " in data_map\n";
                std::cout << "\tinsert ptcld at position " << (__it->first) << '\n';
               )


               if( true ) {
                   #if 0
                   data_map.at( t )->setPointCloudFromMsg( ptcld_msg );
                   data_map.at( t )->setUnVnFromMsg( ptcld_msg );
                   data_map.at( t )->setUVFromMsg( ptcld_msg );
                   data_map.at( t )->setTrackedFeatIdsFromMsg( ptcld_msg );
                   data_map.at( t )->setAsKeyFrame();
                   #else
                   //berks__old
                   data_map->at( __it->first )->setNumberOfSuccessfullyTrackedFeatures( ptcld_msg->points.size() );
                   data_map->at( __it->first )->setAsKeyFrame();
                   #endif
               }
               else {
                    __DataManager__data_association_thread__(cout << TermColor::RED() << "[DataManager::data_association_thread] data_map does not seem to contain the t of ptcld_msg. This cannot be happening." << TermColor::RESET() << endl ;)
                    assert( false && "[DataManager::data_association_thread] data_map does not seem to contain the t of ptcld_msg. This cannot be happening\n");
                    exit(2);
                }
            }
        }


        // Deal with cam_imu_extrinsic. Store the last in global variable of this class.
        bool flag = false;
        nav_msgs::OdometryConstPtr __msg;
        while( extrinsic_cam_imu_buf.size() > 0 ) {
            // dump all
            #if ___USE_STD_QUEUES
            __msg = extrinsic_cam_imu_buf.front();
            extrinsic_cam_imu_buf.pop();
            #else
            __msg = extrinsic_cam_imu_buf.pop();
            #endif
            flag = true;
        }
        if( flag ) {
            std::lock_guard<std::mutex> lk(global_vars_mutex);
            PoseManipUtils::geometry_msgs_Pose_to_eigenmat( __msg->pose.pose, imu_T_cam );
            imu_T_cam_available = true;
            imu_T_cam_stamp = __msg->header.stamp;
        }





        //--------------------------- Done processing--------------------------
        auto loop_end_at = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> ellapsed = loop_end_at-loop_start_at;

        // sleep_for( requested_loop_time - ellapsed )
        int sleep_for = int(requested_loop_time_ms - (float) ellapsed.count()) ;
        _12335_print( "Loop iteration done in "<< ellapsed.count() << " ms; sleep_for=" << sleep_for << " ms" << endl; )

        if( sleep_for > 0 )
            std::this_thread::sleep_for( std::chrono::milliseconds( sleep_for )  );
        else {
            _12335_print( "Queueing in thread `data_association_thread`" );
            ROS_WARN( "Queueing in thread `data_association_thread`" );
        }

    }

    _12335_print( "Finish thread");

}

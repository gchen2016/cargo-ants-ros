#include "at_drive_sim_node.h"

AtDriveNode::AtDriveNode():
    nh_(ros::this_node::getName()), 
    rate_(20), //default at 20Hz
    teleop_v_(0),
    teleop_w_(0), 
    current_idx_(0)
{
    //set up subscribers with callbacks
    trajectory_subscriber_ = nh_.subscribe("trajectory", 100, &AtDriveNode::trajectoryCallback, this);
    teleop_subscriber_ = nh_.subscribe("teleop", 100, &AtDriveNode::teleopCallback, this);
    
    //advertise publishers
    fl_steer_publisher_ = nh_.advertise<std_msgs::Float64>("fl_steer", 100);
    fr_steer_publisher_ = nh_.advertise<std_msgs::Float64>("fr_steer", 100);
    fl_wheel_rate_publisher_ = nh_.advertise<std_msgs::Float64>("fl_wheel_rate", 100);
    fr_wheel_rate_publisher_ = nh_.advertise<std_msgs::Float64>("fr_wheel_rate", 100);
    
    //set at kinematic constant parameters. TODO: get them parsing at_description/at.xacro file
    wheel_radius_ = 0.90; //wheel_radius (see at_description/at.xacro)
    axis_width_ = 2.95; //platform_width-wheel_width (see at_description/at.xacro)
    axes_separation_ = 7.8; //platform_length-2*wheel_radius-2*0.2 (see at_description/at.xacro)
}

AtDriveNode::~AtDriveNode()
{
    //nothing to do
}

double AtDriveNode::getRate() const
{
    return rate_;
}

void AtDriveNode::process()
{
    //std::cout << "AtDriveNode::process(): processing ..." << std::endl; 
    
    //if command is deprecated, jump to the next one or stop
    if (ros::Time::now() > end_command_time_)
    {
        if ( current_idx_ < trajectory_.size() ) //there are still trajectory points to be executed
        {
            //std::cout << "new point: " << trajectory_[current_idx_].transpose() << std::endl;            
            invKinematics( trajectory_[current_idx_] );
            end_command_time_ = ros::Time::now() + ros::Duration(dt_);
            current_idx_ ++;

        }
        else //no more commands to be executed in trajectory_, so stop.
        {
            stop();
        }
    }
}

void AtDriveNode::publish() const
{
    std_msgs::Float64 d_msg;
    //std::cout << "AtDriveNode::publish(): publishing ..." << std::endl; 

    //publish steering commands
    d_msg.data = sfl_; fl_steer_publisher_.publish(d_msg);
    d_msg.data = sfr_; fr_steer_publisher_.publish(d_msg);
      
    //publish wheel rate commands
    d_msg.data = wfl_; fl_wheel_rate_publisher_.publish(d_msg);
    d_msg.data = wfr_; fr_wheel_rate_publisher_.publish(d_msg);
  
}

void AtDriveNode::invKinematics(const Eigen::Vector3d & _pt)
{
    //see at_drive/doc/at_invKinematics.jpg for details on the model
    
    double rc, ri, ro; //curvature radius: center, inner wheel, outer wheel
    double sc, si, so; //steering of the virtual center wheel
    double wi, wo; //inner and outer wheel rotational rates
    double dd; //offset of the curvature point from the inner wheel intersection line
    double xdot, thdot; //auxiliar variables for linear and rotational rate commands

    //get absolute valued vehicle translational and rotational speeds
    xdot = fabs( _pt(0) );
    thdot = fabs( _pt(2) );
    
    //non-null displacement, null steering case
    if ( (xdot != 0.) && (thdot == 0.) )
    {
        //std::cout << "invKinematics(): " << __LINE__ << std::endl;

        //recover linear speed command with sign
        xdot = _pt(0);
        
        //steering
        sfl_ = 0; //sbl_ = 0;
        sfr_ = 0; //sbr_ = 0;
        
        //wheel rates
        wfl_ = xdot/wheel_radius_;// wbl_ = xdot/wheel_radius_;
        wfr_ = xdot/wheel_radius_;// wbr_ = xdot/wheel_radius_; 
    }
    
    //null displacement, non-null steering case. (just turn wheels proportional to rotational rate, but kinematically non-consistent)
    if ( (xdot == 0.) && (thdot != 0.) )
    {
        //std::cout << "invKinematics(): " << __LINE__ << std::endl;        
        
        //recover rotational rate command with sign
        thdot = _pt(2);
        
        //steering 
        sfl_ = thdot; sfr_ = thdot;
        //sbl_ = -thdot; sbr_ = -thdot;

        //wheel rates
        wfl_ = 0; //wbl_ = 0;
        wfr_ = 0; //wbr_ = 0;
    }
    
    //general case
    if ( (xdot != 0.) && (thdot != 0.) )
    {
        //std::cout << "invKinematics(): " << __LINE__ << std::endl;        
        
        //center radius, center steering and offset
        rc = xdot/thdot; //rc = xdot/thdot
        sc = asin( axes_separation_/(2*rc) );
        dd = axes_separation_/(2*tan(sc)) - axis_width_/2;
        
        //inner and outer steering
        si = atan( axes_separation_/(2*dd) );
        so = atan( axes_separation_/(2*(axis_width_+dd)) );
        
        //inner and outer curvature radius
        ri = axes_separation_/(2*sin(si));
        ro = axes_separation_/(2*sin(so));
        
        //inner and outer wheel rotational rates
        wi = thdot*ri/wheel_radius_;
        wo = thdot*ro/wheel_radius_;
        
        if ( _pt(2) > 0 ) //vehicle positive turn, inner wheels are left side ones
        {
            //steering
            sfl_ = si; //sbl_ = -si;
            sfr_ = so; //sbr_ = -so;
            
            //wheel rates
            wfl_ = wi; //wbl_ = wi;
            wfr_ = wo; //wbr_ = wo; 
        }
        else //vehicle negative turn, inner wheels are right side ones
        {
            //steering
            sfl_ = -so; //sbl_ = so;
            sfr_ = -si; //sbr_ = si;
            
            //wheel rates
            wfl_ = wo; //wbl_ = wo;
            wfr_ = wi; //wbr_ = wi;         
        }
        
        if ( _pt(0) < 0 ) //vehicle backwards translational velocity, invert wheel rates
        {
            wfl_ = -wfl_; wfr_ = -wfr_; //wbl_ = -wbl_; wbr_ = -wbr_;
        }
    }
    
}
        
void AtDriveNode::stop()
{
    //evreybody to zero
    sfl_=0; sfr_=0; //sbl_=0; sbr_=0; 
    wfl_=0; wfr_=0; //wbl_=0; wbr_=0; 
}

        
void AtDriveNode::trajectoryCallback(const cargo_ants_msgs::ReferenceTrajectory::ConstPtr& _msg)
{
    
    std::cout << "trajectoryCallback() " << std::endl; 
    
    //reset point index
    current_idx_ = 0; 
    
    //resize trajectory_
    trajectory_.resize(_msg->points.size()); 
    
    //set delta t for each trajectory point
    dt_ = _msg->dt/trajectory_.size();
    std::cout << "\tdt_: " << dt_ << std::endl; 
    
    //set trajectory with message content
    for (unsigned int ii=0; ii<trajectory_.size(); ii++)
    {
        trajectory_[ii] << _msg->points[ii].xd, _msg->points[ii].yd, _msg->points[ii].thd;  
        std::cout << "\t_msg->points[ii]: " << _msg->points[ii].xd << "," << _msg->points[ii].yd << "," << _msg->points[ii].thd << std::endl; 
    }
    
    //set end command time to now
    end_command_time_ = ros::Time::now();
}

void AtDriveNode::teleopCallback(const geometry_msgs::Twist::ConstPtr& _msg)
{
    //reset point index
    current_idx_ = 0; 
    
    //check stop case, otherwise use twist as an increment
    if ( (_msg->linear.x == 0.) && (_msg->angular.z == 0.) ) //STOP case.
    {
        //clearing trajectory forces stop in the process() function
        trajectory_.clear();
        teleop_v_ = 0; 
        teleop_w_ = 0; 
    }
    else // increment speeds
    {    
        //sets a trajectory with a single velocity point
        trajectory_.resize(1); 

        //set a long delta t for the point (1000 seconds)
        dt_ = 1000;
        
        //sets new speeds
        teleop_v_ += _msg->linear.x/10.; //steps of 0.5/10
        teleop_w_ += _msg->angular.z/100.; //steps of 0.5/100
        
        //set new trajectory point
        trajectory_[0] << teleop_v_,0,teleop_w_;
    }
    
    //set end command time to now
    end_command_time_ = ros::Time::now();
}



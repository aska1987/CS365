/*
	ar_harris.cpp
	Mike Zheng and Heidi He
	CS365 project 4
	4/8/19

    use harris corner as feature

    to compile:
        make ar_harris
    to run:
        ../bin/ar_harris ../data/params.txt
*/

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <fstream>
#include <math.h>

#include"shape.hpp"

// read camera parameters from file
std::pair<cv::Mat, cv::Mat> read_params(char* filename){
    std::string line;
    std::ifstream myfile(filename);

    cv::Mat cameraMat, distCoeffs;

    if (myfile.is_open()){

        // while ( getline (myfile,line) ){
        //       std::cout<< line << '\n';
        // }
        double cameraMatVals[9];
        double distCoeffsVals[5];

        // handle first line
        getline(myfile, line);
        // handle second through fourth line -> camera Matrix
        std::string buf;
        for (int i=0;i<3;i++) {
            getline(myfile, line);
            std::stringstream tmpstream(line);
            for (int j=0;j<3;j++) {
                getline(tmpstream, buf, ',');
                if (buf.front() == '['){
                    // std::cout<<stod(buf.substr(1))<<std::endl;
                    cameraMatVals[i*3+j]=stod(buf.substr(1));
                } else {
                    // std::cout<<stod(buf)<<std::endl;
                    cameraMatVals[i*3+j]=stod(buf);
                }
            }
        }
        cameraMat = cv::Mat(3,3,CV_64FC1, cameraMatVals);
        // std::cout<<"cameraMat is :"<<cameraMat<<std::endl;

        //handle fifth line -> skip
        getline(myfile, line);
        //handle distortion coefficients
        getline(myfile, line);
        std::stringstream tmpstream(line);
        for (int j=0;j<5;j++) {
            getline(tmpstream, buf, ',');
            // std::cout<<buf<<std::endl;
            if (buf.front() == '['){
                // std::cout<<stod(buf.substr(1))<<std::endl;
                distCoeffsVals[j]=stod(buf.substr(1));
            } else {
                // std::cout<<stod(buf)<<std::endl;
                distCoeffsVals[j]=stod(buf);
            }
        }
        distCoeffs = cv::Mat(1,5,CV_64FC1, distCoeffsVals);
        // std::cout<<"distCoeffs is :"<<distCoeffs<<std::endl;

        myfile.close();
    }
    else{
        std::cout<< "Unable to open file";
        exit(-1);
    }
    cameraMat = cameraMat.clone();
    distCoeffs = distCoeffs.clone();

    // std::cout<<cameraMat<<std::endl;
    // std::cout<<distCoeffs<<std::endl;

    std::pair<cv::Mat, cv::Mat> curCam = std::make_pair(cameraMat, distCoeffs);

    return curCam;

}

// find harris corner
std::vector<cv::Point2f> findHarrisCorners(cv::Mat grey) {
    int blockSize = 6;
    int apertureSize = 7;
    double k = 0.04;
    int thresh = 120;
    int max_thresh = 255;
    int dilation_size = 2;

    cv::Mat dilation_element = cv::getStructuringElement( cv::MORPH_RECT,
                                 cv::Size( 2*dilation_size + 1, 2*dilation_size+1 ),
                                 cv::Point( dilation_size, dilation_size ) );

    std::vector<cv::Point2f> corner_set;

    cv::Mat dst = cv::Mat::zeros( grey.size(), CV_32FC1 );
    cv::cornerHarris( grey, dst, blockSize, apertureSize, k );

    cv::Mat dst_norm, dst_norm_scaled, thresholded, dilation_dst;
    cv::Mat dst_cc_labels, dst_cc_stats, dst_cc_centroids;
    cv::normalize( dst, dst_norm, 0, 255, cv::NORM_MINMAX, CV_32FC1, cv::Mat() );
    cv::convertScaleAbs( dst_norm, dst_norm_scaled );
    cv::threshold(dst_norm_scaled, thresholded, thresh, max_thresh, 0);
    /// Apply the dilation operation
    cv::dilate( thresholded, dilation_dst, dilation_element );
    // std::cout<<dst_norm<<std::endl;

    cv::connectedComponentsWithStats(dilation_dst, dst_cc_labels, dst_cc_stats, dst_cc_centroids);

    for (int i=2; i<dst_cc_centroids.size().height; i++){
        // std::cout<<cv::Point2f(dst_cc_centroids.at<double>(0,i), dst_cc_centroids.at<double>(1,i))<<std::endl;
        corner_set.push_back(cv::Point2f(dst_cc_centroids.at<double>(i,0), dst_cc_centroids.at<double>(i,1)));
    }

    return corner_set;

}


bool compareCoords(cv::Point3f p1, cv::Point3f p2)
{
    return (p1.y<p2.y);
}


// this function attempts to rectify the image
// it works only when the image is not extremely tilted
std::vector<cv::Point2f> reorient(std::vector<cv::Point2f> corner_set_5){

    // std::cout<<corner_set_5<<std::endl;
    cv::Point2f vtx[4];
    cv::RotatedRect box = cv::minAreaRect(corner_set_5);
    box.points(vtx);
    // Draw the bounding box
    // for(int j = 0; j < 4; j++ ){
    //     cv::line(frame, vtx[j], vtx[(j+1)%4], cv::Scalar(0, 255, 0), 1, cv::LINE_AA);
    // }
    // cv::line(frame, corner_set_5[0], corner_set_5[1], cv::Scalar(0, 255, 0), 1, cv::LINE_AA);
    // cv::line(frame, corner_set_5[1], corner_set_5[3], cv::Scalar(0, 255, 0), 1, cv::LINE_AA);
    // cv::line(frame, corner_set_5[3], corner_set_5[4], cv::Scalar(0, 255, 0), 1, cv::LINE_AA);
    // cv::line(frame, corner_set_5[2], corner_set_5[4], cv::Scalar(0, 255, 0), 1, cv::LINE_AA);
    // cv::line(frame, corner_set_5[2], corner_set_5[0], cv::Scalar(0, 255, 0), 1, cv::LINE_AA);

    // for (int i=0; i<5; i++) {
    //     std::cout<<corner_set_5[i]<<std::endl;
    // }

    // cv::circle(frame, cv::Point((int)(vtx[0].x+vtx[2].x)/2, (int)(vtx[0].y+vtx[2].y)/2), 5, cv::Scalar(255, 255, 255), -1);

    // calculate angle
    float blob_angle_deg = box.angle;
    if (box.size.width >= box.size.height) {
      blob_angle_deg = blob_angle_deg-90;
    }

    double rot_cos = cos(-blob_angle_deg*3.14159265/180);
    double rot_sin = sin(-blob_angle_deg*3.14159265/180);

    // rectify the points
    double coords[15] = {corner_set_5[0].x,corner_set_5[1].x,corner_set_5[2].x,corner_set_5[3].x,corner_set_5[4].x,
        corner_set_5[0].y,corner_set_5[1].y,corner_set_5[2].y,corner_set_5[3].y,corner_set_5[4].y,
        1,1,1,1,1};
    cv::Mat corner_mat = cv::Mat(3,5,CV_64FC1,coords);

    double translation[9] = {1,0, -(vtx[0].x+vtx[2].x)/2, 0,1, -(vtx[0].y+vtx[2].y)/2, 0,0,1};
    cv::Mat translation_mat = cv::Mat(3,3,CV_64FC1,translation);
    double rotation[9] = {rot_cos,-rot_sin, 0, rot_sin, rot_cos, 0, 0,0,1};
    cv::Mat rotation_mat = cv::Mat(3,3,CV_64FC1,rotation);
    double translation2[9] = {1,0, (vtx[0].x+vtx[2].x)/2, 0,1, (vtx[0].y+vtx[2].y)/2, 0,0,1};
    cv::Mat translation2_mat = cv::Mat(3,3,CV_64FC1,translation2);

    // std::cout<<translation_mat<<std::endl;
    // std::cout<<corner_mat<<std::endl;

    cv::Mat corner_mat_new = translation2_mat*rotation_mat*translation_mat*corner_mat;
    // std::cout<<corner_mat_new<<std::endl;

    // get the indeces
    std::vector<cv::Point3f> corner_set_new;
    std::vector<cv::Point2f> corner_set_5_reorient;
    for (int i=0; i<5; i++) {
        corner_set_new.push_back(cv::Point3f(corner_mat_new.at<double>(0,i), corner_mat_new.at<double>(1,i), i));
    }

    std::sort(corner_set_new.begin(), corner_set_new.end(), compareCoords);
    for (int i=0; i<5; i++) {
        corner_set_5_reorient.push_back(cv::Point2f(corner_set_5[(int)corner_set_new[i].z].x,corner_set_5[(int)corner_set_new[i].z].y));
    }


    // manual correction
    if (abs(corner_set_5_reorient[0].x-(vtx[0].x+vtx[2].x)/2)>30){
        // std::cout<<"reverse"<<std::endl;
        std::reverse(corner_set_5_reorient.begin(), corner_set_5_reorient.end());
    }
    // for (int i=0; i<5; i++) {
    //     std::cout<<corner_set_5_reorient[i]<<std::endl;
    // }
    if (corner_set_5_reorient[1].x>corner_set_5_reorient[2].x) {
        cv::Point2f tmp = cv::Point2f(corner_set_5_reorient[1].x,corner_set_5_reorient[1].y);
        corner_set_5_reorient[1] = corner_set_5_reorient[2];
        corner_set_5_reorient[2] = tmp;
    }
    if (corner_set_5_reorient[3].x>corner_set_5_reorient[4].x) {
        cv::Point2f tmp = cv::Point2f(corner_set_5_reorient[3].x,corner_set_5_reorient[3].y);
        corner_set_5_reorient[3] = corner_set_5_reorient[4];
        corner_set_5_reorient[4] = tmp;
    }
    // for (int i=0; i<5; i++) {
    //     std::cout<<corner_set_5_reorient[i]<<std::endl;
    // }
    return corner_set_5_reorient;
}


int main(int argc, char *argv[]) {

    // usage
    if( argc < 2) {
        printf("Usage: %s <params.txt>\n", argv[0]);
        exit(-1);
    }

    char filename[256];
    strcpy(filename, argv[1]);


    std::pair<cv::Mat, cv::Mat> curCam;
    curCam = read_params(filename);
    // std::cout<<curCam.first<<std::endl;
    // std::cout<<curCam.second<<std::endl;

    cv::VideoCapture *capdev;
    int quit = 0;

    // open the video device
    capdev = new cv::VideoCapture(1); //default 0 for using webcam
    if( !capdev->isOpened() ) {
        printf("Unable to open video device\n");
        return(-1);
    }

    cv::Size refS( (int) capdev->get(cv::CAP_PROP_FRAME_WIDTH ),
               (int) capdev->get(cv::CAP_PROP_FRAME_HEIGHT));

    printf("Expected size: %d %d\n", refS.width, refS.height);

    cv::namedWindow("Video", 1);
    cv::Mat frame;

    bool patternfound = false;

    // set to handle iphone 6s screen
    std::vector<cv::Point3f> point_set;
    point_set.push_back(cv::Point3f(1.9, 0.8, 0));
    point_set.push_back(cv::Point3f(0, 0, 0));
    point_set.push_back(cv::Point3f(5.9, 0, 0));
    point_set.push_back(cv::Point3f(0, -10.5, 0));
    point_set.push_back(cv::Point3f(5.9, -10.5, 0));

    // start video capture
	for(;!quit;) {
		*capdev >> frame; // get a new frame from the camera, treat as a stream

		if( frame.empty() ) {
		  printf("frame is empty\n");
		  break;
		}

        // detect target and extract corners
        cv::Mat grey;

        // convert to gray scale
        cv::cvtColor(frame, grey, cv::COLOR_BGR2GRAY);

        std::vector<cv::Point2f> corner_set = findHarrisCorners(grey);

        if (corner_set.size()>0) {
            cv::cornerSubPix(grey, corner_set, cv::Size(11, 11), cv::Size(-1, -1),
                cv::TermCriteria(cv::TermCriteria::EPS+cv::TermCriteria::COUNT, 30, 0.1));

            if(corner_set.size()>=5){
                std::vector<cv::Point2f> corner_set_5;

                for (int i=0;i<5;i++) {
                    // std::cout<<corner_set[i]<<std::endl;
                    corner_set_5.push_back(corner_set[i]);
                    cv::circle( frame, cv::Point(corner_set[i].x,corner_set[i].y), 5,  cv::Scalar(0,0,255), 2, 8, 0 );
                }

                // this function call attemps to rectify the shape
                // does not work well yet
                corner_set_5 = reorient(corner_set_5);


                cv::Mat rvec, tvec;
                bool solveSuccess = cv::solvePnP(point_set, corner_set_5,
                    curCam.first, curCam.second, rvec, tvec);

                if (solveSuccess) {
                    // std::cout<<"tvec "<<tvec<<std::endl;
                    // std::cout<<"rvec "<<rvec<<std::endl;
                    drawAxes(frame, curCam, rvec, tvec);
                    drawCube(frame, curCam, rvec, tvec, cv::Point3f(2,-3,0), 2);
                    drawPyramid(frame, curCam, rvec, tvec, cv::Point3f(2,-3,2), 2);
                } else {
                    break;
                }

            }

        }

		cv::imshow("Video", frame);

		int key = cv::waitKey(10);

        switch(key) {
        case 'q':
            quit = 1;
            break;
        default:
            break;
        }

    }


    // terminate the video capture
    printf("Terminating\n");
    cv::destroyWindow("Video");
    delete capdev;



    return (0);
}

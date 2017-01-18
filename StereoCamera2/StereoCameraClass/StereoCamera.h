#include "stdafx.h"

typedef unsigned int uint;
class StereoCamera{

public:
	StereoCamera(){
		imgSize.height = 720;
		imgSize.width = 1280;
		chessboardSize.height = 6;
		chessboardSize.width = 9;
		chessboardLength = 19.1f;
		imgScale = 1;
		time_t = cvGetTickCount();
	}
	~StereoCamera(){};
	void saveCloudPoints(std::string cloud_point_filename, std::string distance_filename, const cv::Mat& mat);

	void initPictureFileList(std::string filename, int first, int last);

	void saveParallaxPic(std::string filename, const cv::Mat& mat);

	void fromGrayToColor(cv::Mat gray_pic, cv::Mat& color_pic);

	cv::Mat mergeImg(cv::Mat img1, cv::Mat img2);

	void stereoCalibrateCamera(std::string intrinsic_filename, std::string extrinsic_filename);

	void stereoMatch(int pic_num, bool no_display, std::string point_cloud_filename, std::string distance_filename);

	void tailImage(cv::Mat mat);

	void preBlurImage(cv::Mat left_img, cv::Mat right_img);

	void findchessboardCorners(cv::Mat left_img, cv::Mat right_img, uint i);

	//*****************************************************
    // Function���궨���ȼ��
	// ͨ�����ͼ���ϵ�����һ��ͼ��ļ��ߵľ��������۱궨�ľ��ȡ�Ϊ��ʵ�����Ŀ�ģ�ʹ�� undistortPoints ����ԭʼ����ȥ����Ĵ���
	// ���ʹ�� computeCorrespondEpilines �����㼫�ߣ��������ߵĵ�����ۼƵľ�������γ������
	void accuracyCheck();

	void stereoCameraRectify(cv::Mat camera_matrix[2], cv::Mat dist_coeffs[2], 
								 std::string intrinsic_filename, std::string extrinsic_filename);
	void BMstereoMatch(cv::Mat left_img, cv::Mat right_img, cv::Mat disp, cv::Mat disp8);

	void writeParameterMatrix(std::string intrinsic_filename, std::string extrinsic_filename, 
										cv::Mat R1, cv::Mat R2, cv::Mat P1, cv::Mat P2, cv::Mat Q);

	void readingParameterMatrix(std::string intrinsic_filename, std::string extrinsic_filename);
	cv::Mat *getCameraMatrix(){return cameraMatrix;}
	cv::Mat *getDistCoeffs(){return distCoeffs;}
	cv::Mat getLeftImage(){ return leftImage;}
	double getTimeSpended(){return time_t*1000/cv::getTickFrequency();}
	cv::Mat getRightImage(){return rightImage;}
	double getChessboardLength(){return chessboardLength;}

	void setChessboardLength(double length){ this->chessboardLength = length;}
	void setLeftImg(cv::Mat left_img){this->leftImage = left_img;}
	void setRightImg(cv::Mat right_img){this->rightImage = right_img;}

private:
	cv::Mat leftImage;
	cv::Mat rightImage;
	cv::Size imgSize;
	cv::Size chessboardSize;
	double chessboardLength;
	double imgScale;
	char* intrinsic_filename;
	char* extrinsic_filename;
	std::vector<std::string> picFileList;
	std::vector<std::vector<cv::Point2f>> imagePoints[2];
	std::vector<int> idx;
	cv::Mat cameraMatrix[2], distCoeffs[2];
	cv::Mat R, T, E, F;
	cv::Mat M1, D1, M2, D2,R1, R2, P1, P2, Q;
	int64 time_t;
	cv::Rect roi1, roi2;
	cv::Mat disp, disp8;
};
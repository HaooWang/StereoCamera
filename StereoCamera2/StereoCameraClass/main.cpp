#include "StereoCamera.h"
int main()
{
	std::string intrinsic_filename = "intrinsics.yml";
	std::string extrinsic_filename = "extrinsics.yml";
	std::string distance_filename = "���/�Ӳ�����.txt";
	std::string point_cloud_filename = "���/point_test3D.txt";
	std::string pic_filename = "calib_pic2";
	std::string test_pic_filename = "test_pic";

	StereoCamera *stereoCamera  = new StereoCamera();

	/* ����궨 ����һ�μ��� */	
	stereoCamera->initPictureFileList(pic_filename, 1, 13);
	stereoCamera->stereoCalibrateCamera(intrinsic_filename, extrinsic_filename); 

	stereoCamera->initPictureFileList(test_pic_filename, 1, 2);
	
	/*��ȡ����������*/
	stereoCamera->readingParameterMatrix(intrinsic_filename, extrinsic_filename);

	/* ����ƥ�� */
	stereoCamera->stereoMatch(0, true, point_cloud_filename, distance_filename);

	return 0;
}